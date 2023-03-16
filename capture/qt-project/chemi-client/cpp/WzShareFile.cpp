#include "WzShareFile.h"

WzShareFile *WzShareFile::m_instance = nullptr;

#ifdef Q_OS_ANDROID
static void shareFinished(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    emit WzShareFile::instance()->finished();
}
static void shareZipFinished(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    WzShareFile::instance()->zipFinishedFromJava();
}
static void shareZipProgress(JNIEnv *env, jobject thiz, jint done, jint total)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    emit WzShareFile::instance()->zipProgress(done, total);
}
static void shareZipStart(JNIEnv *env, jobject thiz)
{
    Q_UNUSED(env)
    Q_UNUSED(thiz)
    emit WzShareFile::instance()->zipStart();
}
#endif

QJsonArray WzShareFile::makeSharedImages(const QJsonObject &imageInfo,
                                  const QJsonObject &options,
                                  const QString &path)
{
    qInfo() << "makeSharedImages";
    qInfo() << "\timageInfo:" << imageInfo;
    qInfo() << "\toptions:" << options;

    QJsonArray result;
    makeImageService();

    QFileInfo imageFileInfo = QFileInfo(imageInfo["imageFile"].toString());
    QFileInfo imageWhiteFileInfo = QFileInfo(imageInfo["imageWhiteFile"].toString());
    bool isPseudoColor = imageInfo["palette"].toString() != "Gray";
    bool isShowColorBar = options["isPseudoColorBarInImage"].toBool();

    QString fileBaseName;
    QString dir;
    if (options.contains("pcSavedFileName")) {
        QFileInfo fi = QFileInfo(options["pcSavedFileName"].toString());
        fileBaseName = fi.completeBaseName();
        QTemporaryDir tempDir;
        tempDir.setAutoRemove(false);
        dir = tempDir.path();
    } else {
        fileBaseName = imageFileInfo.completeBaseName();
        dir = path;
    }
    qInfo() << "\tfileBaseName:" << fileBaseName;
    qInfo() << "\tdir:" << dir;

    // 16位TIFF - 信号图/化学发光图
    if (options.contains("darkImage")) {
        if (options["darkImage"]["tiff16bit"].toBool()) {
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_dark_16bit.tif");
            QFile::copy(imageFileInfo.absoluteFilePath(), newFileInfo.absoluteFilePath());
            result.append(newFileInfo.absoluteFilePath());
        }
    }

    // 16位TIFF - 白光图/Marker图
    if (options.contains("lightImage")) {
        if (options["lightImage"]["tiff16bit"].toBool()) {
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_marker_16bit.tif");
            QFile::copy(imageWhiteFileInfo.absoluteFilePath(), newFileInfo.absoluteFilePath());
            result.append(newFileInfo.absoluteFilePath());
        }
    }

    bool imagesMade = false;
    auto makeImageInMemory = [&] {
        if (imagesMade)
            return;
        m_imageService->openImage(imageInfo["imageFile"].toString());
        m_imageService->updateLowHigh(imageInfo["grayLow"].toInt(), imageInfo["grayHigh"].toInt());
        m_imageService->setMarkerImageName(imageWhiteFileInfo.absoluteFilePath());
        m_imageService->updateLowHighMarker(imageInfo["grayLowMarker"].toInt(), imageInfo["grayHighMarker"].toInt());
        m_imageService->setShowMarker(true);
        m_imageService->setShowChemi(true);
        m_imageService->setInvert(false);
        m_imageService->updateColorTable(imageInfo["palette"].toString());

        if (isPseudoColor) {
            m_imageView->setColorTableInvert(true);
            m_imageService->setIsPseudoColor(true);
        } else {
            m_imageView->setColorTableInvert(false);
            m_imageService->setIsPseudoColor(false);
        }

        m_imageService->updateView();
        QPainter painter;
        m_imageView->paint(&painter);
        if (isShowColorBar)
            m_imageService->drawColorBar(*m_imageView->paintImage());
        imagesMade = true;
    };

    // 叠加图
    if (options.contains("overlapImage")) {
        // 叠加图 - 8位tiff
        if (options["overlapImage"]["tiff8bit"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_overlap_8bit.tif");
            WzImageBuffer imageBuffer8bit;
            imageBuffer8bit.width = m_imageService->getImageBuffer()->width;
            imageBuffer8bit.height = m_imageService->getImageBuffer()->height;
            imageBuffer8bit.bitDepth = 8;
            imageBuffer8bit.buf = m_imageService->getImage8bit();
            m_imageService->saveImageAsTiff(imageBuffer8bit, newFileInfo.absoluteFilePath());
            result.append(newFileInfo.absoluteFilePath());
        }

        // 叠加图 - 24位tiff
        if (options["overlapImage"]["tiff24bit"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_overlap_24bit.tif");
            m_imageService->saveImageAsTiff24(*m_imageView->paintImage(),
                                              static_cast<int>(m_imageService->getImageBuffer()->width),
                                              static_cast<int>(m_imageService->getImageBuffer()->height),
                                              newFileInfo.absoluteFilePath());
            result.append(newFileInfo.absoluteFilePath());
        }
        // 叠加图 - jpeg
        if (options["overlapImage"]["jpeg"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_overlap.jpg");
            m_imageView->paintImage()->save(newFileInfo.absoluteFilePath(), "jpg", 100);
            result.append(newFileInfo.absoluteFilePath());
        }
        // 叠加图 - png
        if (options["overlapImage"]["png"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_overlap.png");
            m_imageView->paintImage()->save(newFileInfo.absoluteFilePath(), "png", 100);
            result.append(newFileInfo.absoluteFilePath());
        }
    }

    // 信号图/化学发光图
    if (options.contains("darkImage")) {
        m_imageService->setShowMarker(false);
        m_imageService->setInvert(true);
        m_imageService->updateView();
        // 信号图/化学发光图 - 8位tiff
        if (options["darkImage"]["tiff8bit"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_dark_8bit.tif");
            WzImageBuffer imageBuffer8bit;
            imageBuffer8bit.width = m_imageService->getImageBuffer()->width;
            imageBuffer8bit.height = m_imageService->getImageBuffer()->height;
            imageBuffer8bit.bitDepth = 8;
            imageBuffer8bit.buf = m_imageService->getImage8bit();
            m_imageService->saveImageAsTiff(imageBuffer8bit, newFileInfo.absoluteFilePath());
            result.append(newFileInfo.absoluteFilePath());
        }
        if(isPseudoColor) {
            m_imageView->setColorTableInvert(true);
        } else {
            m_imageService->setInvert(false);
        }
        m_imageService->updateView();
        QPainter painter;
        m_imageView->paint(&painter);
        // 信号图/化学发光图 - 24位tiff
        if (options["darkImage"]["tiff24bit"].toBool()) {
            makeImageInMemory();
            m_imageService->updateView();
            m_imageView->paint(&painter);
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_dark_24bit.tif");
            m_imageService->saveImageAsTiff24(*m_imageView->paintImage(),
                                              static_cast<int>(m_imageService->getImageBuffer()->width),
                                              static_cast<int>(m_imageService->getImageBuffer()->height),
                                              newFileInfo.absoluteFilePath());
            result.append(newFileInfo.absoluteFilePath());
        }
        // 信号图/化学发光图 - jpeg
        if (options["darkImage"]["jpeg"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_dark.jpg");
            m_imageView->paintImage()->save(newFileInfo.absoluteFilePath(), "jpg", 100);
            result.append(newFileInfo.absoluteFilePath());
        }
        // 信号图/化学发光图 - png
        if (options["darkImage"]["png"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_dark.png");
            m_imageView->paintImage()->save(newFileInfo.absoluteFilePath(), "png", 100);
            result.append(newFileInfo.absoluteFilePath());
        }
    }

    // 白光图
    if (options.contains("lightImage")) {
        m_imageService->setShowMarker(true);
        m_imageService->setShowChemi(false);
        m_imageService->setInvert(true);
        m_imageService->updateView();
        QPainter painter;
        m_imageView->paint(&painter);
        // 白光图 - 8位tiff
        if (options["lightImage"]["tiff8bit"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_marker_8bit.tif");
            WzImageBuffer imageBuffer8bit;
            imageBuffer8bit.width = m_imageService->getImageBuffer()->width;
            imageBuffer8bit.height = m_imageService->getImageBuffer()->height;
            imageBuffer8bit.bitDepth = 8;
            imageBuffer8bit.buf = m_imageService->getImage8bit();
            m_imageService->saveImageAsTiff(imageBuffer8bit, newFileInfo.absoluteFilePath());
            result.append(newFileInfo.absoluteFilePath());
        }
        // 白光图 - 24位tiff
        if (options["lightImage"]["tiff24bit"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_marker_24bit.tif");
            m_imageService->saveImageAsTiff24(*m_imageView->paintImage(),
                                              static_cast<int>(m_imageService->getImageBuffer()->width),
                                              static_cast<int>(m_imageService->getImageBuffer()->height),
                                              newFileInfo.absoluteFilePath());
            result.append(newFileInfo.absoluteFilePath());
        }
        // 白光图 - jpeg
        if (options["lightImage"]["jpeg"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_marker.jpg");
            m_imageView->paintImage()->save(newFileInfo.absoluteFilePath(), "jpg", 100);
            result.append(newFileInfo.absoluteFilePath());
        }
        // 白光图 - png
        if (options["lightImage"]["png"].toBool()) {
            makeImageInMemory();
            QFileInfo newFileInfo(QDir(dir), fileBaseName + "_marker.png");
            m_imageView->paintImage()->save(newFileInfo.absoluteFilePath(), "png", 100);
            result.append(newFileInfo.absoluteFilePath());
        }
    }

    return result;
}

void WzShareFile::zipFinishedFromJava()
{
    emit zipFinished();
    if (m_options.contains("shareToUsbDisk")) {        
        filesToUsbDisk();
    }
}

void WzShareFile::fileToUsbDiskFinished()
{
    //emit finished();
    emit toUsbDiskFinished();
}

void WzShareFile::makeImageService()
{
    if (!m_imageService)
        m_imageService = new WzImageService();
    if (!m_imageView)
        m_imageView = new WzImageView();

    m_imageService->setImageView(m_imageView);
}

QString WzShareFile::getUsbDiskBasePath()
{
    qInfo() << Q_FUNC_INFO;
    QJsonArray usbDisks = m_options["usbDisks"].toArray();
    if (usbDisks.count() == 0)
        return QString();

    auto dateStr = QDateTime::currentDateTime().toString("yyyy-MM-dd");
    return usbDisks[0].toString() + "/SHST/images/" + dateStr + "/";
}

void WzShareFile::filesToUsbDisk()
{
    qInfo() << Q_FUNC_INFO;

    emit toUsbDisk();

    m_fileToUsbDiskThread->setOptions(
      {{"serverAddress", m_options["usbDiskServerHost"].toString()},
       {"serverPort", m_options["usbDiskServerPort"].toInt()}}
    );

    QStringList remoteFiles;
    QStringList localFiles;

    if (m_options.contains("singleFile") && m_options["singleFile"].toBool()) {
        QString fileName = m_options["fileName"].toString();
        fileName = QFileInfo(fileName).fileName();

        remoteFiles << getUsbDiskBasePath() + fileName;
        localFiles << m_options["fileName"].toString();

    } if (m_options.contains("isZip") && m_options["isZip"].toBool()) {
        QString zipFileName = m_options["zipFileName"].toString();
        zipFileName = QFileInfo(zipFileName).fileName();

        remoteFiles << getUsbDiskBasePath() + zipFileName;
        localFiles << m_options["zipFileName"].toString();
    } else {
        auto basePath = getUsbDiskBasePath();
        foreach(auto v, m_options["fileNames"].toArray()) {
            QString fileName = v.toString();
            localFiles << fileName;
            remoteFiles << (basePath + QFileInfo(fileName).fileName());
        }
    }
    qInfo() << "\tremoteFiles:" << remoteFiles;
    qInfo() << "\tlocalFiles:" << localFiles;

    m_fileToUsbDiskThread->sendFiles(remoteFiles, localFiles);
    if (!m_fileToUsbDiskThread->isRunning())
        m_fileToUsbDiskThread->start();
}

WzShareFile::WzShareFile(QObject *parent) : QObject(parent),
    m_fileToUsbDiskThread(new FileToUsbDiskThread())
{
    m_instance = this;
#ifdef Q_OS_ANDROID
    JNINativeMethod methods[] {
        {"shareFinished", "()V", reinterpret_cast<void *>(shareFinished)},
        {"shareZipFinished", "()V", reinterpret_cast<void *>(shareZipFinished)},
        {"shareZipProgress", "(II)V", reinterpret_cast<void *>(shareZipProgress)},
        {"shareZipStart", "()V", reinterpret_cast<void *>(shareZipStart)}
    };
    QAndroidJniObject javaClass("bio/shenhua/ShareFileService");

    QAndroidJniEnvironment env;
    jclass objectClass = env->GetObjectClass(javaClass.object<jobject>());
    env->RegisterNatives(objectClass,
                         methods,
                         sizeof(methods) / sizeof(methods[0]));
    env->DeleteLocalRef(objectClass);

    connect(m_fileToUsbDiskThread, &FileToUsbDiskThread::sendFilesFinished, this,
            &WzShareFile::fileToUsbDiskFinished);
#endif    
}

WzShareFile::~WzShareFile()
{
    m_fileToUsbDiskThread->stop();
    m_fileToUsbDiskThread->wait();
    delete m_fileToUsbDiskThread;
    if (m_imageView) {
        delete m_imageView;
        m_imageView = nullptr;
    }
    if (m_imageService) {
        delete m_imageService;
        m_imageService = nullptr;
    }
}

WzShareFile *WzShareFile::instance()
{
    return m_instance;
}

void WzShareFile::shareFile(const QString &fileName, const QJsonObject &options)
{
    qInfo() << Q_FUNC_INFO;
#ifdef Q_OS_ANDROID
    m_options = options;
    if (m_options.contains("shareToUsbDisk") && m_options["shareToUsbDisk"].toBool()) {
        m_options["singleFile"] = true;
        m_options["fileName"] = fileName;
        filesToUsbDisk();
        return;
    }
    QAndroidIntent serviceIntent(QtAndroid::androidActivity().object(),
                                        "bio/shenhua/ShareFileService");
    serviceIntent.putExtra("fileName", fileName.toUtf8());
    QAndroidJniObject result = QtAndroid::androidActivity().callObjectMethod(
                "startService",
                "(Landroid/content/Intent;)Landroid/content/ComponentName;",
                serviceIntent.handle().object());
#else
    Q_UNUSED(fileName)
    Q_UNUSED(options)
#endif
}

void WzShareFile::shareFiles(const QJsonArray &fileNames, const QJsonObject &options)
{
    qInfo() << Q_FUNC_INFO;
    qInfo() << "\t" << QString(QJsonDocument(options).toJson());
    m_options = options;
    clearHistoryFiles();
    bool isZip = m_options.contains("isZip") && m_options["isZip"].toBool();

    QStringList sl;
    foreach(QVariant v, fileNames)
        if (v.toString() != "") {
            sl << v.toString();
            qInfo() << "\t" << v.toString();
        }

#ifdef Q_OS_ANDROID
    bool shareToUsbDisk = m_options.contains("shareToUsbDisk") && m_options["shareToUsbDisk"].toBool();

    // 分享到U盘且不使用zip压缩则直接上传,无需调用Java代码
    if (shareToUsbDisk && !isZip) {
        m_options["fileNames"] = fileNames;
        filesToUsbDisk();
        return;
    }

    QAndroidIntent serviceIntent(QtAndroid::androidActivity().object(),
                                        "bio/shenhua/ShareFileService");

    serviceIntent.putExtra("fileName", sl.join('\n').toUtf8());
    if (isZip) {
        serviceIntent.putExtra("isZip", QString("1").toUtf8());
        serviceIntent.putExtra("zipFileName", options["zipFileName"].toString().toUtf8());
    } else {
        serviceIntent.putExtra("isZip", QString("0").toUtf8());
    }
    if (shareToUsbDisk) {
        serviceIntent.putExtra("isNoJavaShare", QString("1").toUtf8());
    } else {
        serviceIntent.putExtra("isNoJavaShare", QString("0").toUtf8());
    }

    QAndroidJniObject result = QtAndroid::androidActivity().callObjectMethod(
                "startService",
                "(Landroid/content/Intent;)Landroid/content/ComponentName;",
                serviceIntent.handle().object());

#endif
#ifdef Q_OS_MAC
#endif
#ifdef Q_OS_WIN
    if (isZip) {
        if (!m_zipUtils) {
            m_zipUtils = new ZipUtils(this);
            connect(m_zipUtils, &ZipUtils::finished, this, [this]() {
                emit this->finished();
            });
        }
        m_zipUtils->createZipFile(sl, options["zipFileName"].toString());
    } else {
        emit finished();
    }
#endif
}

QString WzShareFile::getShareDir()
{
    QDateTime now = QDateTime::currentDateTime();
    QString nowStr = now.toString("yyyyMMddhhmmss");

    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.mkdir("share");
    dir.cd("share");
    dir.mkdir(nowStr);
    dir.cd(nowStr);

    qInfo() << "Share dir:" << dir.absolutePath();

    return dir.absolutePath();
}

QString WzShareFile::getShareFileName(const QString &fileName, const QString &imageFormat)
{    
    QFileInfo src(fileName);
    QString suffix = "";
    if ("tiff16" == imageFormat)
        suffix = "_16bit.tif";
    else if ("tiff8" == imageFormat)
        suffix = "_8bit.tif";
    else if ("tiff24" == imageFormat)
        suffix = "_24bit.tif";
    else if ("jpeg" == imageFormat)
        suffix = ".jpg";
    else if ("png" == imageFormat)
        suffix = ".png";

    QDateTime now = QDateTime::currentDateTime();
    QString nowStr = now.toString("yyyyMMdd");

    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.mkdir("share");
    dir.cd("share");
    dir.mkdir(nowStr);
    dir.cd(nowStr);

    QFileInfo dest(dir, src.completeBaseName() + suffix);
    return dest.absoluteFilePath();
}

QString WzShareFile::getZipFileName(const QJsonObject &images)
{
    qDebug() << "getZipFileName";
    QDateTime now = QDateTime::currentDateTime();
    QString nowStr = now.toString("yyyyMMddhhmmss");

    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.mkdir("share");
    dir.cd("share");
    dir.mkdir(nowStr);
    dir.cd(nowStr);

    QString fileName;
    //qDebug() << "\t" << images;
    if (images.count() == 1) {
        QFileInfo fi(images["0"]["imageFile"].toString());
        fileName = fi.completeBaseName();
        if (fileName == "") {
            fileName = nowStr;
        }
    } else {
        fileName = "shst-" + nowStr;
    }

    QFileInfo dest(dir, fileName + ".zip");
    qInfo() << "zip filename:" << dest.absoluteFilePath();
    return dest.absoluteFilePath();
}

void WzShareFile::clearHistoryFiles()
{
    qInfo() << "ShareFile::clearHistoryFiles";
    QDir dir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation));
    dir.cd("share");

    QDateTime now = QDateTime::currentDateTime();

    foreach (QString dirName, dir.entryList()) {
        if (dirName == ".") continue;
        if (dirName == "..") continue;
        auto dirDateTime = QDateTime::fromString(dirName, "yyyyMMddhhmmss");
        // 文件夹的日期是一小时之前的
        if (dirDateTime.addSecs(60 * 60) < now) {
            QDir removeDir(dir);
            removeDir.cd(dirName);
            if (removeDir.removeRecursively())
                qDebug() << "auto remove: " << removeDir.absolutePath();
        }
    }
}

FileToUsbDiskThread::FileToUsbDiskThread(QObject *parent): QThread{parent}
{
    qInfo() << "FileToUsbDiskThread";
}

FileToUsbDiskThread::~FileToUsbDiskThread()
{
    qInfo() << "~FileToUsbDiskThread";
}

void FileToUsbDiskThread::setOptions(const QJsonObject &options)
{
    QMutexLocker locker(&m_sendFilesMutex);
    m_options = options;
}

void FileToUsbDiskThread::sendFiles(const QStringList &remoteFiles, const QStringList &localFiles)
{
    qInfo() << "FileToUsbDiskThread::sendFiles";
    QMutexLocker locker(&m_sendFilesMutex);
    m_remoteFiles = remoteFiles;
    m_localFiles = localFiles;
    m_sendFilesWaitCondition.wakeAll();
}

void FileToUsbDiskThread::stop()
{
    m_isStop = true;
}

void FileToUsbDiskThread::run()
{
    qInfo() << "FileToUsbDiskThread::run";
    qInfo() << "\tThread Id:" << QThread::currentThreadId();
    ShstFileToUsbDiskClient *c = new ShstFileToUsbDiskClient();
    QObject::connect(c, &ShstFileToUsbDiskClient::finished, this, &FileToUsbDiskThread::sendFilesFinished);
    forever {
        QCoreApplication::processEvents();
        m_sendFilesMutex.lock();
        m_sendFilesWaitCondition.wait(&m_sendFilesMutex, 1000);
        m_sendFilesMutex.unlock();
        // 有文件需要发送
        if (m_localFiles.count() > 0) {
            m_sendFilesMutex.lock();
            c->setServerAddress(m_options["serverAddress"].toString());
            c->setServerPort(m_options["serverPort"].toInt());
            QStringList localFiles = m_localFiles;
            QStringList remoteFiles = m_remoteFiles;
            m_localFiles.clear();
            m_remoteFiles.clear();
            m_sendFilesMutex.unlock();
            c->sendFiles(remoteFiles, localFiles);
        }
        if (m_isStop) {
            break;
        }
    }
    delete c;
}
