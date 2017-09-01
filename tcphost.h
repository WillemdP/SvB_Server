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
    void newClient(QString MAC);
    void newEntryRequest(QStringList columns, QVariantList values); /// Create new entry
    void updateEntryRequest(QString column, QVariant value);        /// Incrementing laps
    void updateEntryRequest(QStringList columns, QVariant values);  /// Add user info to existing blank entry with laps.

public slots:
    void handleNewConnection();
    void handleDisconnect();
    void receiveMessage();
    void sendMessage(QString message);

private:
    QTcpServer *server;
    QList<QTcpSocket *> socketList;
};

#endif // TCPHOST_H
