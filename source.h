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
    void setTags(const QStringList &tags);
    void fetchFile();

private slots:
    void request_json(QNetworkReply *jsonReply);
    void request_file(QNetworkReply *fileReply, QUrl url);

protected:
    bool storeTempFile(const QByteArray &data, QString ext);
    virtual QUrl buildRequestUrl() = 0;
    virtual QUrl jsonToImageUrl(const QJsonDocument &document, const QUrl &documentUrl) = 0;

    QNetworkAccessManager qnam;
    QString workFolder;
    QUrl source_;
    QString title_;
    QStringList tags_;
};

//----------------------------------------------------------------------------

class BooruSource : public WebSource
{
    Q_OBJECT
public:
    explicit BooruSource(QObject *parent = nullptr);
    QString shortName();

public slots:
    void setHost(const QString &hostname);
    void setApiPage(const QString &uri);

protected:
    QUrl buildRequestUrl();
    QUrl jsonToImageUrl(const QJsonDocument &document, const QUrl &documentUrl);

    QString host;
    QString apiPage;
};

//----------------------------------------------------------------------------

class WallhavenSource : public WebSource
{
    Q_OBJECT
public:
    explicit WallhavenSource(QObject *parent = nullptr);
    QString shortName();

protected:
    QUrl buildRequestUrl();
    QUrl jsonToImageUrl(const QJsonDocument &document, const QUrl &documentUrl);
};

//----------------------------------------------------------------------------

}

#endif // SOURCE_H
