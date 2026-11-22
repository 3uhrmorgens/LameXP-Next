#include "JobManager.h"
#include "core/ResourcePolicy.h"
#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QThread>
#include <QUuid>
#include <algorithm>

namespace {
QString norm(const QString& path) {
    return QDir::cleanPath(QFileInfo(path).absoluteFilePath()).toCaseFolded();
}
QStringList substitute(const std::vector<std::string>& tokens, const QString& source,
                       const QString& encoded, const QString& output) {
    QStringList args;
    for (const auto& token : tokens) {
        if (token == lamexp::core::kSource) args << source;
        else if (token == lamexp::core::kEncoded) args << encoded;
        else if (token == lamexp::core::kOutput) args << output;
        else args << QString::fromStdString(token);
    }
    return args;
}
}
JobManager::JobManager(QObject* parent) : QObject(parent) {
    concurrency_ = lamexp::core::recommendedConcurrentJobs(QThread::idealThreadCount());
}
void JobManager::setConcurrency(int n) { concurrency_ = std::clamp(n, 1, 32); pump(); }
int JobManager::pendingCount() const { return waiting_.size(); }
int JobManager::runningCount() const { return running_.size(); }
bool JobManager::enqueue(ConversionJob job, QString* error) {
    auto fail = [&](const QString& msg) { if (error) *error = msg; return false; };
    const QFileInfo in(job.source), out(job.destination);
    if (!in.isFile() || !in.isReadable()) return fail(QStringLiteral("Quelldatei nicht lesbar."));
    if (job.destination.isEmpty() || job.plan.steps.empty()) return fail(QStringLiteral("Ausgabepfad oder Pipeline fehlt."));
    if (norm(job.source) == norm(job.destination)) return fail(QStringLiteral("Quelle und Ziel sind identisch."));
    if (out.exists()) return fail(QStringLiteral("Zieldatei existiert bereits."));
    if (!out.dir().exists() && !QDir().mkpath(out.dir().absolutePath()))
        return fail(QStringLiteral("Ausgabeordner konnte nicht angelegt werden."));
    if (!QFileInfo(job.ffmpegExecutable).isExecutable()) return fail(QStringLiteral("FFmpeg fehlt."));
    for (const auto& step : job.plan.steps) {
        if (step.executable == lamexp::core::Backend::NativeLame &&
            !QFileInfo(job.lameExecutable).isExecutable()) return fail(QStringLiteral("LAME fehlt."));
    }
    for (const auto& pending : waiting_) if (norm(pending.destination) == norm(job.destination))
        return fail(QStringLiteral("Ziel bereits in Warteschlange."));
    for (const auto& active : running_) if (norm(active->job.destination) == norm(job.destination))
        return fail(QStringLiteral("Ziel bereits in Bearbeitung."));
    waiting_.enqueue(std::move(job)); pump(); return true;
}
void JobManager::cancelAll() {
    cancelling_ = true;
    while (!waiting_.isEmpty()) {
        auto job = waiting_.dequeue();
        emit jobFinished(job.source, job.destination, false, QStringLiteral("Vor Start abgebrochen."));
    }
    // Mark BEFORE killing: finished() may arrive asynchronously.
    for (auto it = running_.begin(); it != running_.end(); ++it) {
        it.value()->cancelled = true;
        it.key()->kill();
        if (it.value()->decoder) it.value()->decoder->kill();
    }
    emit queueChanged(waiting_.size(), running_.size());
    cancelling_ = false;
}
void JobManager::pump() {
    while (!cancelling_ && running_.size() < concurrency_ && !waiting_.isEmpty()) {
        auto job = waiting_.dequeue();
        auto active = std::make_shared<ActiveJob>();
        active->job = std::move(job);
        active->temporary = std::make_unique<QTemporaryDir>();
        if (!active->temporary->isValid()) {
            emit jobFinished(active->job.source, active->job.destination, false, QStringLiteral("Temporärordner konnte nicht erstellt werden."));
            continue;
        }
        active->encodedPath = QDir(active->temporary->path()).filePath(QStringLiteral("encoded.mp3"));
        const QFileInfo dest(active->job.destination);
        const QString token = QUuid::createUuid().toString(QUuid::WithoutBraces);
        // Keep final extension for FFmpeg container detection, hidden random file in same output directory.
        active->stagedOutput = dest.dir().filePath(QStringLiteral(".lamexp-%1.%2").arg(token, dest.suffix()));
        auto* proc = new QProcess(this);
        proc->setProcessChannelMode(QProcess::SeparateChannels);
        running_.insert(proc, active);
        connect(proc, &QProcess::readyReadStandardError, this, [this, proc] {
            const auto active = running_.value(proc);
            if (!active) return;
            active->errorTail += proc->readAllStandardError();
            if (active->errorTail.size() > 4096) active->errorTail = active->errorTail.right(4096);
        });
        connect(proc, &QProcess::readyReadStandardOutput, this, [proc] { (void)proc->readAllStandardOutput(); });
        connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
            if (error == QProcess::FailedToStart) finish(proc, false, proc->errorString());
        });
        connect(proc, &QProcess::finished, this, [this, proc](int exitCode, QProcess::ExitStatus status) {
            auto active = running_.value(proc);
            if (!active) return;
            if (active->cancelled) {
                active->encoderDone = true;
                if (active->decoder) active->decoder->kill();
                if (!active->decoder || active->decoderDone) finish(proc, false, QStringLiteral("Abgebrochen."));
                return;
            }
            if (active->decoder) {
                active->encoderDone = true;
                active->encoderOk = status == QProcess::NormalExit && exitCode == 0;
                if (!active->encoderOk && !active->decoderDone) active->decoder->kill();
                completeStreamPair(proc);
                return;
            }
            if (status != QProcess::NormalExit || exitCode != 0) {
                finish(proc, false, active->errorTail.isEmpty() ? QStringLiteral("Codec-Prozess fehlgeschlagen.") : QString::fromUtf8(active->errorTail));
                return;
            }
            ++active->step;
            if (active->step < static_cast<int>(active->job.plan.steps.size())) {
                startStep(proc);
                return;
            }
            const QFileInfo staged(active->stagedOutput);
            if (!staged.isFile() || staged.size() == 0) {
                finish(proc, false, QStringLiteral("Keine gültige Ausgabedatei erzeugt.")); return;
            }
            // QFile::rename fails when destination already exists; no deliberate replacement.
            if (QFileInfo::exists(active->job.destination) || !QFile::rename(active->stagedOutput, active->job.destination)) {
                finish(proc, false, QStringLiteral("Ausgabe konnte nicht sicher übernommen werden (Zielkonflikt oder I/O-Fehler)."));
                return;
            }
            finish(proc, true, QStringLiteral("Konvertiert."));
        });
        emit jobStarted(active->job.source);
        startStep(proc);
    }
    emit queueChanged(waiting_.size(), running_.size());
}
void JobManager::startStep(QProcess* proc) {
    const auto active = running_.value(proc);
    if (!active) return;
    active->errorTail.clear();
    const auto& step = active->job.plan.steps.at(static_cast<size_t>(active->step));
    proc->setProgram(step.executable == lamexp::core::Backend::NativeLame ? active->job.lameExecutable : active->job.ffmpegExecutable);
    proc->setArguments(substitute(step.args, active->job.source, active->encodedPath, active->stagedOutput));
    // The first two stages are connected by a kernel-managed pipe, not a WAV file.
    if (active->step == 0 && active->job.plan.steps.size() >= 2 &&
        step.executable == lamexp::core::Backend::FFmpeg &&
        active->job.plan.steps[1].executable == lamexp::core::Backend::NativeLame) {
        auto* decoder = new QProcess(this);
        active->decoder = decoder;
        active->decoderDone = active->encoderDone = false;
        active->decoderOk = active->encoderOk = false;
        const auto& encode = active->job.plan.steps[1];
        proc->setProgram(active->job.lameExecutable);
        proc->setArguments(substitute(encode.args, active->job.source, active->encodedPath, active->stagedOutput));
        decoder->setProgram(active->job.ffmpegExecutable);
        decoder->setArguments(substitute(step.args, active->job.source, active->encodedPath, active->stagedOutput));
        decoder->setProcessChannelMode(QProcess::SeparateChannels);
        decoder->setStandardOutputProcess(proc);
        connect(decoder, &QProcess::readyReadStandardError, this, [this, proc, decoder] {
            const auto active = running_.value(proc);
            if (!active || active->decoder != decoder) return;
            active->decoderErrorTail += decoder->readAllStandardError();
            if (active->decoderErrorTail.size() > 4096)
                active->decoderErrorTail = active->decoderErrorTail.right(4096);
        });
        connect(decoder, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
            auto active = running_.value(proc);
            if (!active || error != QProcess::FailedToStart) return;
            active->decoderDone = true;
            active->decoderOk = false;
            proc->kill();
            completeStreamPair(proc);
        });
        connect(decoder, &QProcess::finished, this, [this, proc](int code, QProcess::ExitStatus status) {
            auto active = running_.value(proc);
            if (!active) return;
            active->decoderDone = true;
            active->decoderOk = code == 0 && status == QProcess::NormalExit;
            if (!active->decoderOk && !active->encoderDone) proc->kill();
            completeStreamPair(proc);
        });
        // Start receiver first, then producer. No shell process or manual pipe buffer.
        proc->start();
        if (running_.contains(proc)) decoder->start();
        return;
    }
    proc->start();
}
void JobManager::completeStreamPair(QProcess* encoder) {
    const auto active = running_.value(encoder);
    if (!active || !active->decoderDone || !active->encoderDone) return;
    if (active->cancelled) { finish(encoder, false, QStringLiteral("Abgebrochen.")); return; }
    if (!active->decoderOk || !active->encoderOk) {
        const auto details = !active->decoderOk ? active->decoderErrorTail : active->errorTail;
        finish(encoder, false, details.isEmpty() ? QStringLiteral("PCM-Streaming fehlgeschlagen.") : QString::fromUtf8(details));
        return;
    }
    // Skip the two coordinated processes and continue with the MP3 metadata remux.
    active->decoder->deleteLater();
    active->decoder = nullptr;
    active->step = 2;
    if (active->step < static_cast<int>(active->job.plan.steps.size())) startStep(encoder);
    else finish(encoder, false, QStringLiteral("Fehlender MP3-Remux-Schritt."));
}
void JobManager::finish(QProcess* proc, bool success, const QString& detail) {
    if (!running_.contains(proc)) return;
    const auto active = running_.take(proc);
    // On success stagedOutput has been renamed; on failure always delete remnants.
    QFile::remove(active->stagedOutput);
    if (active->decoder) {
        if (active->decoder->state() != QProcess::NotRunning) active->decoder->kill();
        active->decoder->deleteLater();
    }
    // QTemporaryDir cleans intermediate MP3 on destruction after this scope.
    emit jobFinished(active->job.source, active->job.destination, success, detail);
    proc->deleteLater();
    emit queueChanged(waiting_.size(), running_.size());
    pump();
}
