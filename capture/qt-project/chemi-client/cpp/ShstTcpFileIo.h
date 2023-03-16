#ifndef SHSTTCPFILEIO_H
#define SHSTTCPFILEIO_H

#include <QObject>
#include <QTcpSocket>
#include <QJsonObject>
#include <QJsonArray>
#include <QFile>
#include <QElapsedTimer>

#include "ShstTcpIo.h"

class ShstTcpFileIo : public ShstTcpIo
{
    Q_OBJECT
public:
    explicit ShstTcpFileIo(QObject *parent = nullptr);
    ~ShstTcpFileIo() override;

    void sendFiles(const QStringList &fileNames, const QStringList &remoteFileNames);

protected:
    QElapsedTimer m_elapsedTimer;
    void readyJson() override;
    void readyStream() override;
    virtual void readyFile(const QString fileName, const QByteArray &file) = 0;

private:
    QStringList m_fileNames;
};

#endif // SHSTTCPFILEIO_H
