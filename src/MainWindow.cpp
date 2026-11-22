#include "core/PipelinePlan.h"
#include "core/ResourcePolicy.h"
#include "core/OutputNaming.h"
#include <QProcess>
#include <QCoreApplication>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QHeaderView>
#include <QTableWidget>
#include <QSet>
#include <QCheckBox>
#include "MainWindow.h"
#include <QWidget>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QPushButton>
#include <QListWidget>
#include <QComboBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QLabel>
#include <QFileDialog>
#include <QFileInfo>
#include <QDir>
#include <QMessageBox>
#include <QStatusBar>
#include <QStandardPaths>
#include <QThread>
#include <algorithm>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), manager_(this) {
    setWindowTitle(QStringLiteral("LameXP Next — Portable Prototype 0.9"));
    resize(880, 560);
    auto* base = new QWidget(this);
    auto* layout = new QVBoxLayout(base);
    auto* headline = new QLabel(QStringLiteral("LameXP Next  ·  Konvertierungsprototyp"), base);
    QFont font = headline->font(); font.setPointSize(17); font.setBold(true); headline->setFont(font);
    layout->addWidget(headline);
    layout->addWidget(new QLabel(QStringLiteral("Portabler Codec-Betrieb: FFmpeg und FFprobe werden im codecs-Ordner automatisch erkannt."), base));
    auto* exeRow = new QHBoxLayout();
    const QString localCodec = QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("codecs/ffmpeg.exe"));
    const QString portableCodec = QFileInfo(localCodec).isExecutable() ? localCodec : QString{};
    const QString systemCodec = QStandardPaths::findExecutable(QStringLiteral("ffmpeg"));
    executable_ = new QLineEdit(!portableCodec.isEmpty() ? portableCodec : systemCodec, base);
    executable_->setPlaceholderText(QStringLiteral("Vollständiger Pfad zu ffmpeg.exe"));
    nativeLame_ = new QCheckBox(QStringLiteral("Direktes LAME für MP3 (FFmpeg → PCM-Pipe → LAME)"), base);
    nativeLame_->setToolTip(QStringLiteral("Benötigt lame.exe neben ffmpeg.exe. Überträgt PCM direkt per Pipe; Tags und erkanntes Cover werden anschließend übernommen."));
    auto* browseExe = new QPushButton(QStringLiteral("Encoder wählen"), base);
    connect(browseExe, &QPushButton::clicked, this, [this]() {
        const auto path = QFileDialog::getOpenFileName(this, QStringLiteral("ffmpeg.exe auswählen"), {}, QStringLiteral("Programme (*.exe);;Alle Dateien (*)"));
        if (!path.isEmpty()) executable_->setText(path);
    });
    exeRow->addWidget(executable_); exeRow->addWidget(browseExe); layout->addLayout(exeRow); layout->addWidget(nativeLame_);
    files_ = new QListWidget(base); files_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(files_);
    auto* row = new QHBoxLayout();
    auto* add = new QPushButton(QStringLiteral("Dateien hinzufügen"), base);
    auto* clear = new QPushButton(QStringLiteral("Liste leeren"), base);
    format_ = new QComboBox(base);
    format_->addItem(QStringLiteral("MP3 · LAME"), QStringLiteral("mp3"));
    format_->addItem(QStringLiteral("FLAC · verlustfrei"), QStringLiteral("flac"));
    format_->addItem(QStringLiteral("Opus"), QStringLiteral("opus"));
    workers_ = new QSpinBox(base); workers_->setRange(1, 32);
    workers_->setValue(lamexp::core::recommendedConcurrentJobs(QThread::idealThreadCount()));
    workers_->setToolTip(QStringLiteral("Anzahl gleichzeitig gestarteter Konvertierungen"));
    row->addWidget(add); row->addWidget(clear); row->addStretch(); row->addWidget(format_);
    row->addWidget(new QLabel(QStringLiteral("Jobs parallel:"), base)); row->addWidget(workers_);
    layout->addLayout(row);
    autoNames_ = new QCheckBox(QStringLiteral("Ausgabedateien automatisch anhand von Metadaten benennen"), base);
    namePattern_ = new QLineEdit(QStringLiteral("{artist} - {album} - {track:02} - {title}"), base);
    namePattern_->setPlaceholderText(QStringLiteral("z.B. {artist} - {album} - {track:02} - {title}"));
    namePattern_->setToolTip(QStringLiteral("Variablen: {artist}, {albumartist}, {album}, {track}, {track:02}, {disc}, {disc:02}, {title}, {year}, {filename}"));
    auto* nameRow = new QHBoxLayout();
    auto* previewButton = new QPushButton(QStringLiteral("Vorschau / Ausgabeordner"), base);
    nameRow->addWidget(namePattern_, 1); nameRow->addWidget(previewButton);
    layout->addWidget(autoNames_);
    layout->addLayout(nameRow);
    autoFolders_ = new QCheckBox(QStringLiteral("Unterordner nach Metadaten anlegen (optional)"), base);
    folderPattern_ = new QLineEdit(QStringLiteral("{artist}/{year} - {album}"), base);
    folderPattern_->setPlaceholderText(QStringLiteral("Beispiel: {artist}/{year} - {album}"));
    folderPattern_->setToolTip(QStringLiteral("Ordner mit / trennen. Erlaubt sind die gleichen Variablen wie beim Dateinamen."));
    layout->addWidget(autoFolders_);
    layout->addWidget(folderPattern_);
    preview_ = new QTableWidget(base);
    preview_->setColumnCount(2);
    preview_->setHorizontalHeaderLabels({QStringLiteral("Quelle"), QStringLiteral("Geplanter relativer Ausgabepfad")});
    preview_->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    preview_->setEditTriggers(QAbstractItemView::NoEditTriggers);
    preview_->setMaximumHeight(140);
    layout->addWidget(preview_);
    connect(previewButton, &QPushButton::clicked, this, [this] { prepare(false); });
    auto* action = new QHBoxLayout();
    auto* start = new QPushButton(QStringLiteral("Konvertieren"), base);
    auto* stop = new QPushButton(QStringLiteral("Alle abbrechen"), base);
    status_ = new QLabel(QStringLiteral("Bereit"), base);
    action->addWidget(start); action->addWidget(stop); action->addWidget(status_, 1); layout->addLayout(action);
    setCentralWidget(base);
    connect(add, &QPushButton::clicked, this, &MainWindow::addFiles);
    connect(clear, &QPushButton::clicked, files_, &QListWidget::clear);

    connect(start, &QPushButton::clicked, this, [this] { prepare(true); });
    connect(&scanner_, &MetadataScanner::progress, this, [this](int done, int total) {
        status_->setText(QStringLiteral("Analysiere Metadaten: %1 / %2").arg(done).arg(total));
    });
    connect(&scanner_, &MetadataScanner::completed, this, [this](const QStringList& errors) {
        if (!errors.isEmpty()) QMessageBox::warning(this, QStringLiteral("Metadatenanalyse"), errors.join(QStringLiteral("\n")));
        finishPreparation();
    });
    connect(stop, &QPushButton::clicked, &manager_, &JobManager::cancelAll);
    connect(&manager_, &JobManager::queueChanged, this, [this](int pending, int running) {
        status_->setText(QStringLiteral("Wartend: %1 | Aktiv: %2").arg(pending).arg(running));
    });
    connect(&manager_, &JobManager::jobFinished, this, [this](const QString& source, const QString&, bool ok, const QString& detail) {
        statusBar()->showMessage(QFileInfo(source).fileName() + (ok ? QStringLiteral(" abgeschlossen") : QStringLiteral(" fehlgeschlagen: ") + detail), 12000);
    });
}
void MainWindow::addFiles() {
    const auto paths = QFileDialog::getOpenFileNames(this, QStringLiteral("Audiodateien hinzufügen"));
    for (const auto& path : paths) {
        if (files_->findItems(path, Qt::MatchExactly).isEmpty()) files_->addItem(path);
    }
}
namespace {
QString foldPath(const QString& path) {return QDir::cleanPath(QFileInfo(path).absoluteFilePath()).toCaseFolded();}
}
QString MainWindow::ffprobePath() const {
    return QDir(QFileInfo(executable_->text().trimmed()).absolutePath()).filePath(QStringLiteral("ffprobe.exe"));
}
void MainWindow::prepare(bool start) {
    if (scanner_.busy()) { status_->setText(QStringLiteral("Analyse läuft bereits …")); return; }
    const bool needsTags = autoNames_->isChecked() || autoFolders_->isChecked() || format_->currentData().toString() == QStringLiteral("mp3");
    if (!needsTags) {
        if (start) startJobs(); else previewNames();
        return;
    }
    const auto probe = ffprobePath();
    if (!QFileInfo(probe).isExecutable()) {
        QMessageBox::warning(this, QStringLiteral("FFprobe fehlt"), QStringLiteral("ffprobe.exe muss neben ffmpeg.exe liegen."));
        return;
    }
    QStringList sources;
    for (int i = 0; i < files_->count(); ++i) sources << files_->item(i)->text();
    if (sources.isEmpty()) return;
    preparedStart_ = start;
    preparationPending_ = true;
    scanner_.scan(sources, probe);
}
void MainWindow::finishPreparation() {
    if (!preparationPending_) return;
    preparationPending_ = false;
    if (preparedStart_) startJobs(); else previewNames();
}
QString MainWindow::chooseOutputDir() {
    if(!outputDir_.isEmpty() && QDir(outputDir_).exists()) return outputDir_;
    outputDir_=QFileDialog::getExistingDirectory(this,QStringLiteral("Ausgabeordner wählen"));
    return outputDir_;
}
QList<ConversionJob> MainWindow::plannedJobs(const QString& dir, QStringList& errors) {
    QList<ConversionJob> jobs;
    const QString executable=executable_->text().trimmed();
    const QString codec=format_->currentData().toString();
    const bool useLame=nativeLame_->isChecked() && codec==QStringLiteral("mp3");
    const QString lamePath=QDir(QFileInfo(executable).absolutePath()).filePath(QStringLiteral("lame.exe"));
    const QString ffprobe=ffprobePath();
    if((autoNames_->isChecked() || autoFolders_->isChecked()) && !QFileInfo(ffprobe).isExecutable()) {
        errors << QStringLiteral("Automatische Benennung benötigt ffprobe.exe im gleichen Ordner wie ffmpeg.exe.");
        return jobs;
    }
    QSet<QString> reserved;
    for(int i=0;i<files_->count();++i) {
        const auto source=files_->item(i)->text();
        QString stem=QFileInfo(source).completeBaseName();
        const auto tags = (autoNames_->isChecked() || autoFolders_->isChecked())
            ? scanner_.result(source).tags : lamexp::core::Tags{};
        QString relativeDir;
        if(autoFolders_->isChecked()) {
            const auto pattern=folderPattern_->text().toUtf8();
            const auto base=stem.toUtf8();
            const auto result=lamexp::core::renderFolders(std::string(pattern.constData(),size_t(pattern.size())),tags,
                                                       std::string(base.constData(),size_t(base.size())));
            if(!result.error.empty()) {errors << QFileInfo(source).fileName()+QStringLiteral(": ")+QString::fromStdString(result.error);continue;}
            for(const auto& folder : result.components) relativeDir=QDir(relativeDir).filePath(QString::fromStdString(folder));
        }
        if(autoNames_->isChecked()) {
            const auto pattern=namePattern_->text().toUtf8();
            const auto base=stem.toUtf8();
            const auto result=lamexp::core::renderName(std::string(pattern.constData(),size_t(pattern.size())),tags,
                                                       std::string(base.constData(),size_t(base.size())));
            if(!result.error.empty()) { errors << QFileInfo(source).fileName()+QStringLiteral(": ")+QString::fromStdString(result.error); continue; }
            stem=QString::fromUtf8(result.stem.c_str(),qsizetype(result.stem.size()));
        }
        QString destination=QDir(QDir(dir).filePath(relativeDir)).filePath(stem+QStringLiteral(".")+codec);
        if(autoNames_->isChecked() || autoFolders_->isChecked()) {
            int number=2;
            while(QFileInfo::exists(destination) || reserved.contains(foldPath(destination)) || foldPath(destination)==foldPath(source)) {
                destination=QDir(QDir(dir).filePath(relativeDir)).filePath(stem+QStringLiteral(" (%1).").arg(number++)+codec);
            }
        }
        if(reserved.contains(foldPath(destination))) {
            errors << QFileInfo(source).fileName()+QStringLiteral(": Doppelter Ausgabename"); continue;
        }
        reserved.insert(foldPath(destination));
        lamexp::core::PipelineOptions options;
        options.useNativeLame=useLame;
        if(codec==QStringLiteral("mp3")) {
            // Determine whether an attached picture exists, not simply any video stream.
            options.hasAttachedCover=scanner_.result(source).attachedCover;
        }
        if(codec==QStringLiteral("flac")) options.format=lamexp::core::OutputFormat::FLAC;
        else if(codec==QStringLiteral("opus")) options.format=lamexp::core::OutputFormat::Opus;
        else options.format=lamexp::core::OutputFormat::MP3;
        const auto plan=lamexp::core::planPipeline(options);
        jobs.append({source,destination,plan,executable,lamePath});
    }
    return jobs;
}
void MainWindow::previewNames() {
    const auto dir=chooseOutputDir(); if(dir.isEmpty()) return;
    QStringList errors;
    auto jobs=plannedJobs(dir,errors);
    preview_->setRowCount(jobs.size());
    for(int i=0;i<jobs.size();++i) {
        preview_->setItem(i,0,new QTableWidgetItem(QFileInfo(jobs[i].source).fileName()));
        preview_->setItem(i,1,new QTableWidgetItem(QDir(dir).relativeFilePath(jobs[i].destination)));
    }
    if(!errors.isEmpty()) QMessageBox::warning(this,QStringLiteral("Vorschauhinweise"),errors.join(QStringLiteral("\n")));
}
void MainWindow::startJobs() {
    const QString executable=executable_->text().trimmed();
    if(!QFileInfo(executable).isFile() || !QFileInfo(executable).isExecutable()) {
        QMessageBox::warning(this,QStringLiteral("Encoder fehlt"),QStringLiteral("Keine ffmpeg.exe gefunden. Prüfe codecs/ffmpeg.exe im Programmordner oder wähle sie manuell.")); return;
    }
    const auto dir=chooseOutputDir(); if(dir.isEmpty()) return;
    const bool useLame=nativeLame_->isChecked() && format_->currentData().toString()==QStringLiteral("mp3");
    const QString lamePath=QDir(QFileInfo(executable).absolutePath()).filePath(QStringLiteral("lame.exe"));
    if(useLame && !QFileInfo(lamePath).isExecutable()) {
        QMessageBox::warning(this,QStringLiteral("LAME fehlt"),QStringLiteral("lame.exe muss neben ffmpeg.exe liegen.")); return;
    }
    manager_.setConcurrency(workers_->value());
    QStringList errors;
    auto jobs=plannedJobs(dir,errors);
    for(auto& job:jobs) {
        QString error;
        auto source=job.source;
        if(!manager_.enqueue(std::move(job),&error)) errors << QFileInfo(source).fileName()+QStringLiteral(": ")+error;
    }
    if(!errors.isEmpty()) QMessageBox::warning(this,QStringLiteral("Einige Jobs übersprungen"),errors.join(QStringLiteral("\n")));
}
