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
#include <QDragEnterEvent>
#include <QDragMoveEvent>
#include <QDropEvent>
#include <QMimeData>
#include <QEvent>
#include <QSettings>
#include <QInputDialog>
#include <QRegularExpression>
#include <QUrl>
#include <algorithm>

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent), manager_(this) {
    setWindowTitle(QStringLiteral("LameXP Next — Portable Prototype 0.10"));
    resize(1010, 720);
    setAcceptDrops(true);
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
    files_->setAcceptDrops(true);
    files_->viewport()->setAcceptDrops(true);
    files_->installEventFilter(this);
    files_->viewport()->installEventFilter(this);
    files_->setToolTip(QStringLiteral("Audiodateien hierher ziehen oder über die Schaltfläche hinzufügen."));
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
    auto* paramsRow = new QHBoxLayout();
    paramsRow->addWidget(new QLabel(QStringLiteral("MP3-Modus:"), base));
    mp3RateMode_ = new QComboBox(base);
    mp3RateMode_->addItem(QStringLiteral("VBR"), 0);
    mp3RateMode_->addItem(QStringLiteral("ABR"), 1);
    mp3RateMode_->addItem(QStringLiteral("CBR"), 2);
    paramsRow->addWidget(mp3RateMode_);
    paramsRow->addWidget(new QLabel(QStringLiteral("MP3-Bitrate:"), base));
    mp3Bitrate_ = new QComboBox(base);
    for (const int rate : {32,40,48,56,64,80,96,112,128,160,192,224,256,320})
        mp3Bitrate_->addItem(QStringLiteral("%1 kbit/s").arg(rate), rate);
    mp3Bitrate_->setCurrentIndex(10);
    paramsRow->addWidget(mp3Bitrate_);
    paramsRow->addWidget(new QLabel(QStringLiteral("MP3-VBR:"), base));
    mp3Quality_ = new QComboBox(base);
    for (int q = 0; q <= 9; ++q) mp3Quality_->addItem(QStringLiteral("Qualität %1").arg(q), q);
    mp3Quality_->setCurrentIndex(2);
    mp3Quality_->setToolTip(QStringLiteral("FFmpeg libmp3lame -q:a; VBR 0 = höchste Qualität, 9 = kleinste Datei. Keine feste Bitrate."));
    paramsRow->addWidget(mp3Quality_);
    paramsRow->addWidget(new QLabel(QStringLiteral("Opus:"), base));
    opusBitrate_ = new QComboBox(base);
    for (const int kbps : {64, 96, 128, 160, 192, 256, 320})
        opusBitrate_->addItem(QStringLiteral("%1 kbit/s").arg(kbps), kbps);
    opusBitrate_->setCurrentIndex(2);
    paramsRow->addWidget(opusBitrate_);
    paramsRow->addWidget(new QLabel(QStringLiteral("Kanäle:"), base));
    channelMode_ = new QComboBox(base);
    channelMode_->addItem(QStringLiteral("Original"), 0);
    channelMode_->addItem(QStringLiteral("Mono"), 1);
    channelMode_->addItem(QStringLiteral("Stereo"), 2);
    paramsRow->addWidget(channelMode_);
    layout->addLayout(paramsRow);
    auto* sampleRow = new QHBoxLayout();
    sampleRow->addWidget(new QLabel(QStringLiteral("Samplerate:"), base));
    sampleRate_ = new QComboBox(base);
    sampleRate_->addItem(QStringLiteral("Quelle (automatisch)"), 0);
    sampleRate_->addItem(QStringLiteral("32.000 Hz"), 32000);
    sampleRate_->addItem(QStringLiteral("44.100 Hz"), 44100);
    sampleRate_->addItem(QStringLiteral("48.000 Hz"), 48000);
    sampleRow->addWidget(sampleRate_);
    sampleRow->addStretch();
    layout->addLayout(sampleRow);
    auto* profileRow = new QHBoxLayout();
    profileRow->addWidget(new QLabel(QStringLiteral("Profil:"), base));
    profiles_ = new QComboBox(base);
    profiles_->addItem(QStringLiteral("Benutzerdefiniert"), QStringLiteral("custom"));
    profiles_->addItem(QStringLiteral("MP3 VBR V2"), QStringLiteral("builtin-vbr"));
    profiles_->addItem(QStringLiteral("MP3 CBR 320"), QStringLiteral("builtin-cbr"));
    profiles_->addItem(QStringLiteral("MP3 ABR 192"), QStringLiteral("builtin-abr"));
    profiles_->addItem(QStringLiteral("Opus 128"), QStringLiteral("builtin-opus"));
    profiles_->addItem(QStringLiteral("FLAC verlustfrei"), QStringLiteral("builtin-flac"));
    QSettings portableSettings(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("LameXPNext.ini")), QSettings::IniFormat);
    portableSettings.beginGroup(QStringLiteral("profiles"));
    for (const QString& name : portableSettings.childKeys())
        profiles_->addItem(name, QStringLiteral("user:") + name);
    portableSettings.endGroup();
    auto* saveProfile = new QPushButton(QStringLiteral("Profil speichern"), base);
    auto* deleteProfile = new QPushButton(QStringLiteral("Profil löschen"), base);
    profileRow->addWidget(profiles_, 1);
    profileRow->addWidget(saveProfile);
    profileRow->addWidget(deleteProfile);
    layout->addLayout(profileRow);
    auto apply = [this](const QVariantMap& profile) {
        applyingProfile_ = true;
        auto select = [](QComboBox* box, const QVariant& value) {
            const int index = box->findData(value);
            if (index >= 0) box->setCurrentIndex(index);
        };
        select(format_, profile.value(QStringLiteral("format")));
        select(mp3RateMode_, profile.value(QStringLiteral("rateMode")));
        select(mp3Quality_, profile.value(QStringLiteral("vbr")));
        select(mp3Bitrate_, profile.value(QStringLiteral("mp3Bitrate")));
        select(opusBitrate_, profile.value(QStringLiteral("opusBitrate")));
        select(channelMode_, profile.value(QStringLiteral("channels")));
        select(sampleRate_, profile.value(QStringLiteral("sampleRate")));
        applyingProfile_ = false;
        updateParameterSummary();
    };
    connect(profiles_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this, apply](int) {
        const QString id = profiles_->currentData().toString();
        if (id == QStringLiteral("custom")) return;
        QVariantMap profile{{QStringLiteral("rateMode"),0}, {QStringLiteral("vbr"),2},
                            {QStringLiteral("mp3Bitrate"),192}, {QStringLiteral("opusBitrate"),128},
                            {QStringLiteral("channels"),0}, {QStringLiteral("sampleRate"),0}};
        if (id == QStringLiteral("builtin-vbr")) profile[QStringLiteral("format")] = QStringLiteral("mp3");
        else if (id == QStringLiteral("builtin-cbr")) {
            profile[QStringLiteral("format")] = QStringLiteral("mp3");
            profile[QStringLiteral("rateMode")] = 2;
            profile[QStringLiteral("mp3Bitrate")] = 320;
        } else if (id == QStringLiteral("builtin-abr")) {
            profile[QStringLiteral("format")] = QStringLiteral("mp3");
            profile[QStringLiteral("rateMode")] = 1;
        } else if (id == QStringLiteral("builtin-opus")) profile[QStringLiteral("format")] = QStringLiteral("opus");
        else if (id == QStringLiteral("builtin-flac")) profile[QStringLiteral("format")] = QStringLiteral("flac");
        else if (id.startsWith(QStringLiteral("user:"))) {
            QSettings settings(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("LameXPNext.ini")), QSettings::IniFormat);
            profile = settings.value(QStringLiteral("profiles/") + id.mid(5)).toMap();
        }
        apply(profile);
    });
    connect(saveProfile, &QPushButton::clicked, this, [this] {
        bool ok = false;
        const QString name = QInputDialog::getText(this, QStringLiteral("Konvertierungsprofil"),
            QStringLiteral("Profilname:"), QLineEdit::Normal, {}, &ok).trimmed();
        if (!ok || name.isEmpty()) return;
        if (!QRegularExpression(QStringLiteral("^[\\p{L}\\p{N} _-]{1,50}$")).match(name).hasMatch()) {
            QMessageBox::warning(this, QStringLiteral("Ungültiger Profilname"),
                QStringLiteral("Bitte höchstens 50 Buchstaben, Zahlen, Leerzeichen, Bindestriche oder Unterstriche eingeben."));
            return;
        }
        QSettings settings(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("LameXPNext.ini")), QSettings::IniFormat);
        const QVariantMap profile{
            {QStringLiteral("format"), format_->currentData()},
            {QStringLiteral("rateMode"), mp3RateMode_->currentData()},
            {QStringLiteral("vbr"), mp3Quality_->currentData()},
            {QStringLiteral("mp3Bitrate"), mp3Bitrate_->currentData()},
            {QStringLiteral("opusBitrate"), opusBitrate_->currentData()},
            {QStringLiteral("channels"), channelMode_->currentData()},
            {QStringLiteral("sampleRate"), sampleRate_->currentData()}};
        settings.setValue(QStringLiteral("profiles/") + name, profile);
        settings.sync();
        if (settings.status() != QSettings::NoError) {
            QMessageBox::warning(this, QStringLiteral("Profil konnte nicht gespeichert werden"),
                QStringLiteral("Der Programmordner ist möglicherweise nicht beschreibbar."));
            return;
        }
        const QString id = QStringLiteral("user:") + name;
        int index = profiles_->findData(id);
        if (index < 0) { profiles_->addItem(name, id); index = profiles_->count()-1; }
        profiles_->setCurrentIndex(index);
    });
    connect(deleteProfile, &QPushButton::clicked, this, [this] {
        const QString id = profiles_->currentData().toString();
        if (!id.startsWith(QStringLiteral("user:"))) {
            QMessageBox::information(this, QStringLiteral("Profil löschen"),
                QStringLiteral("Vordefinierte Profile können nicht gelöscht werden."));
            return;
        }
        QSettings settings(QDir(QCoreApplication::applicationDirPath()).filePath(QStringLiteral("LameXPNext.ini")), QSettings::IniFormat);
        settings.remove(QStringLiteral("profiles/") + id.mid(5));
        profiles_->removeItem(profiles_->currentIndex());
        profiles_->setCurrentIndex(0);
    });
    parameters_ = new QLabel(base);
    parameters_->setWordWrap(true);
    layout->addWidget(parameters_);
    connect(format_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateParameterSummary(); });
    connect(mp3Quality_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateParameterSummary(); });
    connect(mp3RateMode_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateParameterSummary(); });
    connect(mp3Bitrate_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateParameterSummary(); });
    connect(sampleRate_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateParameterSummary(); });
    connect(opusBitrate_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateParameterSummary(); });
    connect(channelMode_, QOverload<int>::of(&QComboBox::currentIndexChanged), this, [this] { updateParameterSummary(); });
    connect(nativeLame_, &QCheckBox::toggled, this, [this] { updateParameterSummary(); });
    updateParameterSummary();
    // A manual edit must not leave a saved preset displayed as though it still matched.
    const auto markCustom = [this](int) {
        if (!applyingProfile_ && profiles_->currentData().toString() != QStringLiteral("custom"))
            profiles_->setCurrentIndex(0);
    };
    for (QComboBox* control : {format_, mp3RateMode_, mp3Quality_, mp3Bitrate_,
                               opusBitrate_, channelMode_, sampleRate_})
        connect(control, QOverload<int>::of(&QComboBox::currentIndexChanged), this, markCustom);
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
        if (!errors.isEmpty()) QMessageBox::warning(this, QStringLiteral("Dateiprüfung"), errors.join(QStringLiteral("\n")));
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
void MainWindow::updateParameterSummary() {
    const QString format = format_->currentData().toString();
    const bool mp3 = format == QStringLiteral("mp3");
    const int mp3Mode = mp3RateMode_->currentData().toInt();
    mp3RateMode_->setEnabled(mp3);
    mp3Quality_->setEnabled(mp3 && mp3Mode == 0);
    mp3Bitrate_->setEnabled(mp3 && mp3Mode != 0);
    sampleRate_->setEnabled(format != QStringLiteral("opus"));
    opusBitrate_->setEnabled(format == QStringLiteral("opus"));
    nativeLame_->setEnabled(mp3);
    const QString channels = channelMode_->currentData().toInt() == 0 ? QStringLiteral("Kanäle beibehalten")
        : channelMode_->currentData().toInt() == 1 ? QStringLiteral("Mono (1 Kanal)")
        : QStringLiteral("Stereo (2 Kanäle)");
    QString details;
    if (mp3) {
        const QString mode = mp3Mode == 0
            ? QStringLiteral("VBR Qualität %1 (variable Bitrate)").arg(mp3Quality_->currentData().toInt())
            : QStringLiteral("%1 %2 kbit/s").arg(mp3Mode == 1 ? QStringLiteral("ABR") : QStringLiteral("CBR"))
                .arg(mp3Bitrate_->currentData().toInt());
        details = QStringLiteral("MP3 · %1 · %2")
            .arg(nativeLame_->isChecked() ? QStringLiteral("direkte LAME-EXE") : QStringLiteral("libmp3lame"), mode);
    } else if (format == QStringLiteral("opus")) {
        details = QStringLiteral("Opus · libopus · %1 kbit/s Zielbitrate").arg(opusBitrate_->currentData().toInt());
    } else {
        details = QStringLiteral("FLAC · verlustfrei · keine feste Zielbitrate");
    }
    const QString sample = format == QStringLiteral("opus")
        ? QStringLiteral("Opus (48 kHz)")
        : sampleRate_->currentData().toInt() == 0 ? QStringLiteral("Quelle")
        : QStringLiteral("%1 Hz").arg(sampleRate_->currentData().toInt());
    parameters_->setText(QStringLiteral("Konvertierungsparameter: %1 · %2 · Samplerate: %3")
        .arg(details, channels, sample));
}
void MainWindow::addInputPaths(const QStringList& paths) {
    int rejected = 0;
    for (const auto& path : paths) {
        const QFileInfo file(path);
        if (!file.isFile() || !file.isReadable()) { ++rejected; continue; }
        if (files_->findItems(file.absoluteFilePath(), Qt::MatchExactly).isEmpty())
            files_->addItem(file.absoluteFilePath());
    }
    if (rejected) QMessageBox::information(this, QStringLiteral("Dateien übersprungen"),
        QStringLiteral("%1 Einträge sind keine lesbaren Dateien.").arg(rejected));
}
void MainWindow::addFiles() {
    addInputPaths(QFileDialog::getOpenFileNames(this, QStringLiteral("Audiodateien hinzufügen")));
}
bool MainWindow::eventFilter(QObject* watched, QEvent* event) {
    if (watched == files_ || watched == files_->viewport()) {
        if (event->type() == QEvent::DragEnter || event->type() == QEvent::DragMove) {
            auto* drag = static_cast<QDragMoveEvent*>(event);
            if (drag->mimeData()->hasUrls()) {
                for (const auto& url : drag->mimeData()->urls()) {
                    if (url.isLocalFile()) { drag->acceptProposedAction(); return true; }
                }
            }
        } else if (event->type() == QEvent::Drop) {
            auto* drop = static_cast<QDropEvent*>(event);
            QStringList paths;
            for (const auto& url : drop->mimeData()->urls())
                if (url.isLocalFile()) paths << url.toLocalFile();
            if (!paths.isEmpty()) { addInputPaths(paths); drop->acceptProposedAction(); return true; }
        }
    }
    return QMainWindow::eventFilter(watched, event);
}
void MainWindow::dragEnterEvent(QDragEnterEvent* event) {
    if (event->mimeData()->hasUrls()) {
        for (const auto& url : event->mimeData()->urls()) {
            if (url.isLocalFile()) { event->acceptProposedAction(); return; }
        }
    }
    event->ignore();
}
void MainWindow::dropEvent(QDropEvent* event) {
    QStringList paths;
    for (const auto& url : event->mimeData()->urls())
        if (url.isLocalFile()) paths << url.toLocalFile();
    addInputPaths(paths);
    event->acceptProposedAction();
}
namespace {
QString foldPath(const QString& path) {return QDir::cleanPath(QFileInfo(path).absoluteFilePath()).toCaseFolded();}
}
QString MainWindow::ffprobePath() const {
    return QDir(QFileInfo(executable_->text().trimmed()).absolutePath()).filePath(QStringLiteral("ffprobe.exe"));
}
void MainWindow::prepare(bool start) {
    if (scanner_.busy()) { status_->setText(QStringLiteral("Analyse läuft bereits …")); return; }
    const bool needsTags = true; // Always validate an actual audio stream, regardless of extension.
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
        const auto scan = scanner_.result(source);
        if (!scan.error.isEmpty() || !scan.hasAudio) {
            errors << QFileInfo(source).fileName() + QStringLiteral(": Keine konvertierbare Audiospur gefunden.");
            continue;
        }
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
        options.mp3VbrQuality=mp3Quality_->currentData().toInt();
        const int mode = mp3RateMode_->currentData().toInt();
        options.mp3RateMode = mode == 1 ? lamexp::core::RateMode::ABR :
            mode == 2 ? lamexp::core::RateMode::CBR : lamexp::core::RateMode::VBR;
        options.mp3BitrateKbps=mp3Bitrate_->currentData().toInt();
        options.sampleRateHz=codec==QStringLiteral("opus") ? 0 : sampleRate_->currentData().toInt();
        options.opusBitrateKbps=opusBitrate_->currentData().toInt();
        options.outputChannels=channelMode_->currentData().toInt();
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
