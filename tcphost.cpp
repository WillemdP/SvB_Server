#include "tcphost.h"

TcpHost::TcpHost()
{
    server = new QTcpServer(this);

    connect(server, SIGNAL(newConnection()), this, SLOT(handleNewConnection()), Qt::DirectConnection);
    server->setMaxPendingConnections(20);

    if(!server->listen(QHostAddress::Any, 8000)) {
        qDebug() << "Error listening for clients:" << server->errorString();
        while(1);
    }
    qDebug() << "Server started.";
    return;
}

void TcpHost::handleNewConnection()
{
    QTcpSocket *socket = new QTcpSocket(this);
    socket = server->nextPendingConnection();
    socketList.append(socket);

    connect(socket, SIGNAL(readyRead()), this, SLOT(receiveMessage()), Qt::DirectConnection);
    connect(socket, SIGNAL(disconnected()), this, SLOT(handleDisconnect()), Qt::DirectConnection);

    qDebug() << "Client connected" << socket->peerName() << socket->peerAddress().toString();

    emit newClient(socket->peerAddress().toString());

    socket->write("Hello Client");
    socket->flush();

    socket->waitForBytesWritten(300);
    return;
}

void TcpHost::handleDisconnect()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    // emit disconnected(socket->peerAddress.toString());
    socketList.removeAll(socket);
    socket->close();
    return;
}

void TcpHost::receiveMessage()
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if(!socket->isOpen())
        return;
    QString message = socket->readAll();
    qDebug() << "Message received from" << socket->peerAddress() << ":" << message;
    // parse
    QStringList separated = message.split("|");
    separated.pop_front();
    message = separated.join("|");
    if(separated.at(0)=="reqUpdateEntry") {
        // update entry
        QStringList columns;
        QVariantList values;
        QStringList columnvalue;
        foreach(QString field, separated) {
            columnvalue = field.split(";");
            if(columnvalue.size()<2)
                continue;
            if(columnvalue.at(1)=="")
                continue;
            columns.append(columnvalue.at(0));
            values.append(QVariant(columnvalue.at(1)));
            qDebug() << "Conversion result:" << values.last().toString();
        }
        if(columns.size()<1)
            return;
        qDebug() << "Relaying message";
        emit updateEntryRequest(columns, values);
    }
    else if(separated.at(0)=="reqAddEntry") {
        // add entry
    }
    else if(separated.at(0)=="reqUpdateField") {
        // update field
    }
    return;
}

void TcpHost::sendMessage(QString message)
{
    QTcpSocket *socket = qobject_cast<QTcpSocket *>(sender());
    if(!socket->isOpen())
        return;
    socket->write(message.toStdString().c_str());
    socket->waitForBytesWritten(300);
    return;
}

