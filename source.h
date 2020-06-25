#ifndef SOURCE_H
#define SOURCE_H

#include <QNetworkAccessManager>
#include <QUrl>
#include <QObject>
#include <QVariantMap>
#include <random>

namespace Sources {

//----------------------------------------------------------------------------

class FileSource : public QObject
{
    Q_OBJECT
public:
    explicit FileSource(QObject *parent = nullptr);
    virtual QString shortName();
    QString path();
    virtual QUrl source();
    virtual void processPath();
    virtual QVariant field();
    virtual void setField(const QVariant &field);

signals:
    void nextFile(QString fileName);
    void trayMessage(QString message);

public slots:
    void setPath(const QString &filePath);
    virtual void fetchFile();

protected:
    QString path_;
};

//----------------------------------------------------------------------------

class FileListSource : public FileSource
{
    Q_OBJECT
public:
    explicit FileListSource(QObject *parent = nullptr);
    QString shortName();
    QStringList files();
    void processPath();

public slots:
    void fetchFile();

protected:
    QStringList files_;
    std::random_device rseed;
    std::mt19937 rgen;
};

//----------------------------------------------------------------------------

class FolderSource : public FileListSource
{
    Q_OBJECT
public:
    explicit FolderSource(QObject *parent = nullptr);
    QString shortName();
    void processPath();
};

//----------------------------------------------------------------------------

class DropSource : public FileListSource
{
    Q_OBJECT
public:
    explicit DropSource(QObject *parent = nullptr);
    QString shortName();
    void processPath();
    void setFiles(const QStringList &files_);
    QVariant field();
    void setField(const QVariant &field);
};

//----------------------------------------------------------------------------

class WebSource : public FileSource
{
    Q_OBJECT
public:
    explicit WebSource(QObject *parent = nullptr);
    QString shortName();
    QUrl source();
    QString title();
    QStringList tags();
    QVariant field();
    void setField(const QVariant &field);

signals:
    void nextFile(QString fileName);

public slots:
    void setWorkFolder(const QString &folder);
    void setTitle(const QString &name);
    void setHost(const QString &hostname);
    void setApiPage(const QString &uri);
    void setTags(const QStringList &tags);
    void fetchFile();

private slots:
    void request_json(QNetworkReply *jsonReply);
    void request_file(QNetworkReply *fileReply, QUrl url);

protected:
    bool storeTempFile(const QByteArray &data, QString ext);

    QNetworkAccessManager qnam;
    QString workFolder;
    QString title_;
    QString host;
    QString apiPage;
    QUrl source_;
    QStringList tags_;
};

//----------------------------------------------------------------------------

}

#endif // SOURCE_H
