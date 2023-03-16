#include "WzHaierConf.h"

WzHaierConf::WzHaierConf(QObject *parent) : QObject(parent)
{

}

bool WzHaierConf::isExists() const
{
    QFileInfo fi(getConfFileName());
    return fi.exists();
}

bool WzHaierConf::readConfig()
{
    if (!isExists())
        return false;

    QFile keyFile(":/haier.key");
    if (!keyFile.open(QIODevice::ReadOnly))
        return false;

    aes256_key key;
    keyFile.read(keyFile.read(reinterpret_cast<char*>(key), AES256_KEY_LEN));

    char* buf = nullptr;
    int bufSize = 0;

    if (!WzUtils::aesDecryptFileToBuf(key, getConfFileName(), &buf, &bufSize))
        return false;

    QTextStream haierConf(buf);
    QString line;
    QMap<QString, QString> keyValues;
    while (haierConf.readLineInto(&line)) {
        qDebug() << line;
        int p = line.indexOf('=');
        if (p > -1) {
            keyValues[line.left(p)] = line.right(line.count() - p - 1);
        }
    }
    delete []buf;

    m_clientId = keyValues["clientId"];
    m_clientSecret = keyValues["clientSecret"];
    m_username = keyValues["username"];
    m_password = keyValues["password"];
    m_devCode = keyValues["devCode"];
    m_devName = keyValues["devName"];
    m_devModel = keyValues["devModel"];
    m_office =  keyValues["office"];
    m_serverUrl = keyValues["serverUrl"];
    m_tokenUrl = keyValues["tokenUrl"];
    m_heartUrl = keyValues["heartUrl"];
    m_saveDeviceUrl = keyValues["saveDeviceUrl"];
    m_updateDeviceUrl = keyValues["updateDeviceUrl"];
    m_saveDeviceDataUrl = keyValues["saveDeviceDataUrl"];

    return true;
}

QString WzHaierConf::getConfFileName() const
{
    return QStandardPaths::writableLocation(QStandardPaths::AppDataLocation) +
            "/" + kHaierConfFileName;
}

void WzHaierConf::aesDecrypt(aes256_key key, QString inFileName, QByteArray &content) const
{
    using namespace std;

    ifstream inFile(inFileName.toLocal8Bit(), ios::binary | ios::in | ios::ate);
    if (inFile.is_open()) {
        int fileSize = inFile.tellg();
        int bufSize = fileSize - IV_LEN;

        char* fileBuf = new char[bufSize];
        memset(fileBuf, 0, bufSize);

        inFile.seekg(0, ios::beg);
        inFile.read(fileBuf, bufSize);

        aes_iv iv;
        inFile.read(reinterpret_cast<char*>(&iv[0]), IV_LEN);

        struct AES_ctx ctx;
        AES_init_ctx_iv(&ctx, key, iv);
        AES_CBC_decrypt_buffer(&ctx, reinterpret_cast<uint8_t*>(fileBuf), bufSize);

        unsigned char padSize = fileBuf[bufSize-1];

        content.append(fileBuf, bufSize - padSize);

        delete []fileBuf;
        inFile.close();
    } else {
        qInfo() << "Unable open the file: " << inFileName;
    }

}
