#ifndef SHSTTCPFILESERVICE_H
#define SHSTTCPFILESERVICE_H

#include <QObject>
#include <QTcpSocket>
#include <QFileInfo>
#include <QFile>
#include <QDir>

#include "ShstTcpFileIo.h"

class ShstTcpFileService : public ShstTcpFileIo
{
    Q_OBJECT
public:
    explicit ShstTcpFileService(QObject *parent = nullptr);
    ~ShstTcpFileService() override;

    bool init(qintptr socketDescriptor);
    bool isFinished() const;

signals:
    void finished();

protected:
    void readyFile(const QString fileName, const QByteArray &file) override;

private slots:
    void socketError(QTcpSocket::SocketError socketError);
    void socketStateChanged(QAbstractSocket::SocketState socketState);

private:
    QTcpSocket *m_pTcpSocket = nullptr;
    bool m_isFinished = false;
};

#endif // SHSTTCPFILESERVICE_H
