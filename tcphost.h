#ifndef TCPHOST_H
#define TCPHOST_H

#include <QThread>
#include <QTcpServer>
#include <QTcpSocket>

class TcpHost : public QObject
{
    Q_OBJECT
public:
    TcpHost();

signals:
    void newClient(QTcpSocket *socket, QString MAC);
    void newEntryRequest(QTcpSocket *socket, QStringList columns, QVariantList values); /// Create new entry
    void updateEntryRequest(QTcpSocket *socket, QString column, QVariant value);        /// Incrementing laps
    void updateEntryRequest(QTcpSocket *socket, QStringList columns, QVariantList values);  /// Add user info to existing blank entry with laps.

public slots:
    void handleNewConnection();
    void handleDisconnect();
    void receiveMessage();
    void sendMessage(QTcpSocket *socket, QString message);

private:
    QTcpServer *server;
    QList<QTcpSocket *> socketList;
};

#endif // TCPHOST_H
