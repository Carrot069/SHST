#include <QCoreApplication>
#include <QNetworkInterface>
#include <QNetworkAddressEntry>
#include <QUdpSocket>
#include <QList>
#include <QHostAddress>
#include <QDebug>

QList<QHostAddress> getAllBroadcastAddresses() {
    QList<QHostAddress> addresses;
    auto allInterfaces = QNetworkInterface::allInterfaces();
    foreach (auto interface, allInterfaces) {
        qDebug() << interface.isValid();
        if (interface.flags().testFlag(QNetworkInterface::CanBroadcast) &&
                interface.flags().testFlag(QNetworkInterface::IsRunning)) {
            foreach (auto ae, interface.addressEntries()) {
                if (!ae.broadcast().isNull()) {
                    addresses << ae.broadcast();
                }
            }
        }
    }
    return addresses;
}

int main(int argc, char *argv[])
{
    QCoreApplication a(argc, argv);

    QUdpSocket udpSocket;

    auto allBroadcastAddresses = getAllBroadcastAddresses();
    foreach (auto address, allBroadcastAddresses) {
        qDebug() << address;
        udpSocket.writeDatagram("test", address, 30030);
    }

    return a.exec();
}
