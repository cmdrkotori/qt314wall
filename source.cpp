#include "source.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QNetworkReply>
#include <QUrlQuery>

using namespace Sources;

static const char msgNoJson[] = "JSON reply empty or null.\nWas there no images with that tag?";
static const char textNowhereUrl[] = "https://nowhere/page.json?a=b";

//----------------------------------------------------------------------------

FileSource::FileSource(QObject *parent) : QObject(parent)
{

}

QString FileSource::shortName()
{
    return "File";
}

QString FileSource::path()
{
    return path_;
}

QUrl FileSource::source()
{
    return QUrl::fromLocalFile(path_);
}

void FileSource::processPath()
{

}
QVariant FileSource::field()
{
    return path_;
}

void FileSource::setField(const QVariant &field)
{
    setPath(field.toString());
}

void FileSource::setPath(const QString &filePath)
{
    path_ = filePath;
    processPath();
}

void FileSource::fetchFile()
{
    emit nextFile(path_);
}

//----------------------------------------------------------------------------

FileListSource::FileListSource(QObject *parent)
    : FileSource(parent)
    , rgen(rseed())
{

}

QString FileListSource::shortName()
{
    return "FileList";
}

QStringList FileListSource::files()
{
    return files_;
}

void FileListSource::processPath()
{
    QFile f(path_);
    if (!f.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;
    }
    files_.clear();
    while (!f.atEnd()) {
        QString line = QString::fromUtf8(f.readLine()).trimmed();
        if (!line.isEmpty())
            files_.append(line);
    }
}

void FileListSource::fetchFile()
{
    if (files_.isEmpty()) {
        FileSource::fetchFile();
        return;
    }
    std::uniform_int_distribution<int> dist(0, files_.size()-1);
    int index = dist(rgen);
    emit nextFile(files_.value(index));
}

//----------------------------------------------------------------------------

FolderSource::FolderSource(QObject *parent) : FileListSource(parent)
{

}

QString FolderSource::shortName()
{
    return "Folder";
}

void FolderSource::processPath()
{
    QDir d(path_);
    files_.clear();
    for (auto &i : d.entryInfoList({"*.jpg", "*.png"}, QDir::Files))
        files_.append(i.absoluteFilePath());
}

//----------------------------------------------------------------------------


DropSource::DropSource(QObject *parent) : FileListSource(parent)
{

}

QString DropSource::shortName()
{
    return "Drop";
}

void DropSource::processPath()
{

}

void DropSource::setFiles(const QStringList &files)
{
    this->files_ = files;
}

QVariant DropSource::field()
{
    return files_;
}

void DropSource::setField(const QVariant &field)
{
    files_ = field.toStringList();
}

//----------------------------------------------------------------------------

static const char userAgent[] =
        "Mozilla/5.0 (X11; Linux x86_64)"
        " AppleWebKit/999.99 (KHTML, like Gecko)"
        " Qt314Wall/1.0";

WebSource::WebSource(QObject *parent)
    : FileSource(parent)
{
    title_ = "Unnamed web source";
}

QString WebSource::shortName()
{
    return "WebSource";
}

QUrl WebSource::source()
{
    return source_;
}

QString WebSource::title()
{
    return title_;
}

QStringList WebSource::tags()
{
    return tags_;
}

QVariant WebSource::field()
{
    return tags_;
}

void WebSource::setField(const QVariant &field)
{
    tags_ = field.toStringList();
}

void WebSource::setWorkFolder(const QString &folder)
{
    workFolder = folder;
}

void WebSource::setTitle(const QString &name)
{
    title_ = name;
}

void WebSource::setTags(const QStringList &tags)
{
    tags_ = tags;
}

void WebSource::fetchFile()
{
    if (workFolder.isEmpty())
        return;

    QUrl url = buildRequestUrl();
    if (url.isEmpty())
        return;

    QNetworkRequest request;
    request.setHeader(QNetworkRequest::UserAgentHeader, userAgent);

    QNetworkReply *reply = qnam.get(QNetworkRequest(url));
    connect(reply, &QNetworkReply::finished,
            this, [reply,this]() { request_json(reply); });
}

void WebSource::request_json(QNetworkReply *jsonReply)
{
    QUrl imageUrl;
    QNetworkReply *fileReply;

    QByteArray response = jsonReply->readAll();
    QJsonDocument json = QJsonDocument::fromJson(response);
    bool emptyJson = json.isNull() || json.isEmpty() || response == "[]" || response == "{}";
    if (!emptyJson)
        imageUrl = jsonToImageUrl(json, jsonReply->request().url());
    if (emptyJson || imageUrl.isEmpty()) {
        emit trayMessage(msgNoJson);
        FileSource::fetchFile();
        jsonReply->deleteLater();
        return;
    }


    fileReply = qnam.get(QNetworkRequest(imageUrl));
    connect(fileReply, &QNetworkReply::finished,
            this, [this,fileReply,imageUrl]() { request_file(fileReply, imageUrl); });

    jsonReply->deleteLater();
}

void WebSource::request_file(QNetworkReply *fileReply, QUrl url)
{
    source_ = url;
    QString ext = QFileInfo(url.path()).suffix();
    storeTempFile(fileReply->readAll(), ext);
    FileSource::fetchFile();
    fileReply->deleteLater();
}

bool WebSource::storeTempFile(const QByteArray &data, QString ext)
{
    path_ = workFolder + "/dl/image." + ext;
    QDir(workFolder).mkpath("dl");
    QFile f(path_);
    if (!f.open(QIODevice::WriteOnly | QIODevice::Truncate))
        return false;
    if (f.write(data) < 0)
        return false;
    return true;
}

//----------------------------------------------------------------------------

BooruSource::BooruSource(QObject *parent) : WebSource(parent)
{
    host = "nowhere";
    setTitle("Unnamed booru source");
}

QString BooruSource::shortName()
{
    return host;
}

void BooruSource::setHost(const QString &hostname)
{
    host = hostname;
}

void BooruSource::setApiPage(const QString &uri)
{
    apiPage = uri;
}

QUrl BooruSource::buildRequestUrl()
{
    QUrl url(textNowhereUrl);
    url.setScheme("https");
    url.setHost(host);
    url.setPath(apiPage);

    QUrlQuery query;
    query.addQueryItem("limit", "1");
    query.addQueryItem("random", "true");
    query.addQueryItem("tags", tags_.join("+"));

    url.setQuery(query);
    return url;
}

QUrl BooruSource::jsonToImageUrl(const QJsonDocument &document, const QUrl &documentUrl)
{
    QVariantMap map = document.toVariant().toList().first().toMap();

    QString fileUrlString = map.value("file_url").toString();
    QUrl imageUrl = documentUrl;
    if (fileUrlString.startsWith("//")) {
        imageUrl = imageUrl.scheme() + ":" + fileUrlString;
    } else {
        imageUrl = fileUrlString;
    }
    return imageUrl;
}

//----------------------------------------------------------------------------

WallhavenSource::WallhavenSource(QObject *parent)
    : WebSource(parent)
{
    setTitle("Wallhaven");
}

QString WallhavenSource::shortName()
{
    return "WallhavenSource";
}

QUrl WallhavenSource::buildRequestUrl()
{
    QUrl url(textNowhereUrl);
    url.setScheme("https");
    url.setHost("wallhaven.cc");
    url.setPath("/api/v1/search");

    QUrlQuery query;
    query.addQueryItem("sorting", "random");
    query.addQueryItem("q", tags_.join("+"));

    url.setQuery(query);
    return url;
}

QUrl WallhavenSource::jsonToImageUrl(const QJsonDocument &document, const QUrl &documentUrl)
{
    Q_UNUSED(documentUrl);
    QVariantMap map = document.toVariant().toMap();
    QVariantList data = map.value("data", QVariantList()).toList();
    if (data.isEmpty())
        return QUrl();

    QVariantMap firstImage = data.first().toMap();
    QString fileUrlString = firstImage.value("path").toString();
    return QUrl::fromUserInput(fileUrlString);

}
