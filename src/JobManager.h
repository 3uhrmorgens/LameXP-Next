#pragma once
#include "core/PipelinePlan.h"
#include <QObject>
#include <QProcess>
#include <QQueue>
#include <QHash>
#include <QString>
#include <QStringList>
#include <QTemporaryDir>
#include <memory>

struct ConversionJob {
    QString source;
    QString destination;
    lamexp::core::PipelinePlan plan;
    QString ffmpegExecutable;
    QString lameExecutable;
};

class JobManager final : public QObject {
    Q_OBJECT
public:
    explicit JobManager(QObject* parent = nullptr);
    void setConcurrency(int concurrentJobs);
    bool enqueue(ConversionJob job, QString* error = nullptr);
    void cancelAll();
    [[nodiscard]] int pendingCount() const;
    [[nodiscard]] int runningCount() const;
signals:
    void jobStarted(QString source);
    void jobFinished(QString source, QString destination, bool success, QString detail);
    void queueChanged(int pending, int running);
private:
    struct ActiveJob {
        ConversionJob job;
        std::unique_ptr<QTemporaryDir> temporary;
        QString stagedOutput;
        QString encodedPath;
        int step = 0;
        bool cancelled = false;
        QProcess* decoder = nullptr;
        bool decoderDone = false;
        bool encoderDone = false;
        bool decoderOk = false;
        bool encoderOk = false;
        QByteArray decoderErrorTail;
        QByteArray errorTail;
    };
    void pump();
    void startStep(QProcess* process);
    void completeStreamPair(QProcess* encoder);
    void finish(QProcess* process, bool success, const QString& detail);
    int concurrency_ = 1;
    QQueue<ConversionJob> waiting_;
    QHash<QProcess*, std::shared_ptr<ActiveJob>> running_;
    bool cancelling_ = false;
};
