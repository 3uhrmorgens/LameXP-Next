#pragma once
#include "core/OutputNaming.h"
#include <QObject>
#include <QDateTime>
#include <QHash>
#include <QProcess>
#include <QQueue>
#include <QStringList>
#include <memory>

// Event-driven FFprobe queue. No waitForFinished() on the GUI thread.
class MetadataScanner final : public QObject {
    Q_OBJECT
public:
    struct Result {
        lamexp::core::Tags tags;
        bool attachedCover = false;
        QString error;
    };
    explicit MetadataScanner(QObject* parent = nullptr);
    void scan(const QStringList& paths, const QString& ffprobeExecutable);
    void invalidate();
    [[nodiscard]] Result result(const QString& path) const;
    [[nodiscard]] bool busy() const;
signals:
    void progress(int done, int total);
    void completed(QStringList errors);
private:
    struct Entry { qint64 size = -1; QDateTime modified; QString probe; Result data; };
    bool fresh(const QString& path, const QString& probe) const;
    void next();
    QHash<QString, Entry> cache_;
    QQueue<QString> pending_;
    QString probe_;
    QProcess* running_ = nullptr;
    QByteArray json_;
    QStringList errors_;
    int total_ = 0;
    int done_ = 0;
    bool active_ = false;
};
