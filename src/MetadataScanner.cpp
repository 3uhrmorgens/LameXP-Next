#include "MetadataScanner.h"
#include <QFileInfo>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonParseError>
#include <QSet>
#include <QTimer>

namespace {
QString key(const QString& path) { return QFileInfo(path).absoluteFilePath(); }
lamexp::core::Tags parseTags(const QJsonObject& root) {
    lamexp::core::Tags tags;
    auto collect = [&](const QJsonObject& obj) {
        for (auto it = obj.begin(); it != obj.end(); ++it) {
            auto field = it.key().toLower();
            if (field == QStringLiteral("album_artist") || field == QStringLiteral("album artist")) field = QStringLiteral("albumartist");
            if (field == QStringLiteral("date") || field == QStringLiteral("creation_time")) field = QStringLiteral("year");
            if (field == QStringLiteral("tracknumber")) field = QStringLiteral("track");
            const auto f = field.toUtf8();
            const std::string k(f.constData(), size_t(f.size()));
            if (!tags.contains(k)) {
                const auto v = it.value().toVariant().toString().toUtf8();
                tags.emplace(k, std::string(v.constData(), size_t(v.size())));
            }
        }
    };
    collect(root.value(QStringLiteral("format")).toObject().value(QStringLiteral("tags")).toObject());
    for (const auto stream : root.value(QStringLiteral("streams")).toArray())
        collect(stream.toObject().value(QStringLiteral("tags")).toObject());
    return tags;
}
MetadataScanner::Result parse(const QByteArray& bytes, QString error) {
    MetadataScanner::Result data;
    if (!error.isEmpty()) { data.error = std::move(error); return data; }
    QJsonParseError parseError;
    const auto doc = QJsonDocument::fromJson(bytes, &parseError);
    if (parseError.error != QJsonParseError::NoError || !doc.isObject()) {
        data.error = QStringLiteral("Ungültige FFprobe-JSON-Ausgabe"); return data;
    }
    const auto obj = doc.object();
    data.tags = parseTags(obj);
    // Pipeline refers to 0:v:0. Only its disposition is relevant.
    for (const auto stream : obj.value(QStringLiteral("streams")).toArray()) {
        const auto s = stream.toObject();
        if (s.value(QStringLiteral("codec_type")).toString() != QStringLiteral("video")) continue;
        data.attachedCover = s.value(QStringLiteral("disposition")).toObject()
            .value(QStringLiteral("attached_pic")).toInt() == 1;
        break;
    }
    return data;
}
}
MetadataScanner::MetadataScanner(QObject* parent) : QObject(parent) {}
bool MetadataScanner::busy() const { return active_; }
void MetadataScanner::invalidate() { if (!active_) cache_.clear(); }
bool MetadataScanner::fresh(const QString& path, const QString& probe) const {
    const auto it = cache_.constFind(key(path));
    if (it == cache_.cend()) return false;
    const QFileInfo f(path);
    return f.exists() && it->size == f.size() && it->modified == f.lastModified() && it->probe == probe;
}
MetadataScanner::Result MetadataScanner::result(const QString& path) const {
    const auto it = cache_.constFind(key(path));
    return it == cache_.cend() ? Result{.error=QStringLiteral("Metadaten noch nicht analysiert")} : it->data;
}
void MetadataScanner::scan(const QStringList& paths, const QString& ffprobeExecutable) {
    if (active_) return;
    probe_ = ffprobeExecutable;
    errors_.clear(); pending_.clear(); total_ = 0; done_ = 0;
    QSet<QString> seen;
    for (const auto& path : paths) {
        const auto k = key(path);
        if (seen.contains(k)) continue;
        seen.insert(k);
        ++total_;
        if (fresh(path, probe_)) { ++done_; continue; }
        pending_.enqueue(path);
    }
    active_ = true;
    emit progress(done_, total_);
    QTimer::singleShot(0, this, &MetadataScanner::next);
}
void MetadataScanner::next() {
    if (pending_.isEmpty()) {
        active_ = false;
        emit completed(errors_);
        return;
    }
    const QString source = pending_.dequeue();
    auto* proc = new QProcess(this);
    running_ = proc;
    json_.clear();
    auto* timer = new QTimer(proc);
    timer->setSingleShot(true);
    auto finished = std::make_shared<bool>(false);
    auto timedOut = std::make_shared<bool>(false);
    auto finalize = [this, proc, timer, timedOut, finished, source](QString error) {
        if (*finished) return;
        *finished = true;
        timer->stop();
        auto data = parse(json_ + proc->readAllStandardOutput(), std::move(error));
        if (!data.error.isEmpty()) errors_ << QFileInfo(source).fileName() + QStringLiteral(": ") + data.error;
        const QFileInfo f(source);
        cache_.insert(key(source), Entry{f.size(), f.lastModified(), probe_, std::move(data)});
        ++done_;
        running_ = nullptr;
        emit progress(done_, total_);
        proc->deleteLater();
        QTimer::singleShot(0, this, &MetadataScanner::next);
    };
    connect(timer, &QTimer::timeout, proc, [proc, timedOut] {
        *timedOut = true;
        if (proc->state() != QProcess::NotRunning) proc->kill();
    });
    connect(proc, &QProcess::readyReadStandardOutput, proc, [this, proc] {
        // Protect against runaway / malformed tool output.
        if (json_.size() < 2 * 1024 * 1024) json_ += proc->readAllStandardOutput();
        else { proc->readAllStandardOutput(); proc->kill(); }
    });
    connect(proc, &QProcess::errorOccurred, this, [proc, finalize](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) finalize(QStringLiteral("FFprobe konnte nicht gestartet werden: ") + proc->errorString());
    });
    connect(proc, &QProcess::finished, this, [timedOut, finalize](int code, QProcess::ExitStatus exitStatus) {
        const bool success = !*timedOut && code == 0 && exitStatus == QProcess::NormalExit;
        finalize(success ? QString{} : (*timedOut ? QStringLiteral("FFprobe-Zeitlimit überschritten") : QStringLiteral("FFprobe fehlgeschlagen")));
    });
    proc->setProgram(probe_);
    proc->setArguments({QStringLiteral("-v"), QStringLiteral("error"), QStringLiteral("-show_entries"),
        QStringLiteral("format_tags:stream=index,codec_type:stream_tags:stream_disposition"),
        QStringLiteral("-of"), QStringLiteral("json"), source});
    proc->start();
    timer->start(10000);
}
