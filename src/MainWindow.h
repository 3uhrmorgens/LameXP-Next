#pragma once
#include <QMainWindow>
class QDragEnterEvent;
class QDropEvent;
#include "JobManager.h"
#include "MetadataScanner.h"
class QListWidget;
class QComboBox;
class QSpinBox;
class QLabel;
class QLineEdit;
class QCheckBox;
class QTableWidget;

class MainWindow final : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
protected:
    void dragEnterEvent(QDragEnterEvent* event) override;
    void dropEvent(QDropEvent* event) override;
    bool eventFilter(QObject* watched, QEvent* event) override;
private:
    void addFiles();
    void addInputPaths(const QStringList& paths);
    void updateParameterSummary();
    void startJobs();
    void previewNames();
    void prepare(bool start);
    void finishPreparation();
    QString ffprobePath() const;
    QString chooseOutputDir();
    QList<ConversionJob> plannedJobs(const QString& dir, QStringList& errors);
    QListWidget* files_ = nullptr;
    QComboBox* format_ = nullptr;
    QComboBox* mp3Quality_ = nullptr;
    QComboBox* mp3RateMode_ = nullptr;
    QComboBox* mp3Bitrate_ = nullptr;
    QComboBox* sampleRate_ = nullptr;
    QComboBox* profiles_ = nullptr;
    QComboBox* opusBitrate_ = nullptr;
    QComboBox* channelMode_ = nullptr;
    QLabel* parameters_ = nullptr;
    QSpinBox* workers_ = nullptr;
    QLineEdit* executable_ = nullptr;
    QCheckBox* nativeLame_ = nullptr;
    QLabel* status_ = nullptr;
    QLineEdit* namePattern_ = nullptr;
    QCheckBox* autoNames_ = nullptr;
    QCheckBox* autoFolders_ = nullptr;
    QLineEdit* folderPattern_ = nullptr;
    QTableWidget* preview_ = nullptr;
    QString outputDir_;
    JobManager manager_;
    MetadataScanner scanner_;
    bool preparedStart_ = false;
    bool preparationPending_ = false;
};
