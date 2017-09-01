#ifndef SVB_SERVER_H
#define SVB_SERVER_H

#include <QMainWindow>

#include <QTableView>

#include <QtSql/QtSql>
#include <QSqlTableModel>
#include <QSortFilterProxyModel>

#include <QThread>
#include <QTimer>

#include <QDateTime>

#include <QKeyEvent>

#include "tcphost.h"

namespace Ui {
class SvB_Server;
}

class SvB_Server : public QMainWindow
{
    Q_OBJECT

public:
    explicit SvB_Server(QWidget *parent = 0);
    ~SvB_Server();

public slots:
    void addNewClient(QTcpSocket *socket, QString address);
    void addNewEntry(QTcpSocket *socket, QStringList columns, QVariantList values);
    void updateEntry(QTcpSocket *socket, QString column, QVariant value);
    void updateEntry(QTcpSocket *socket, QStringList columns, QVariantList values);

//    void newClient(QString MAC);
//    void newEntryRequest(QStringList columns, QVariantList values); /// Create new entry
//    void updateEntryRequest(QString column, QVariant value);        /// Incrementing laps
//    void updateEntryRequest(QStringList columns, QVariant values);  /// Add user info to existing blank entry with laps.

protected:
    void keyPressEvent(QKeyEvent *event);

signals:
    void sendMessage(QTcpSocket *socket, QString message);

private:
    void buildModel();
    QString toTableFormat(QString refString);
    QString toViewFormat(QString refString);

    Ui::SvB_Server *ui;
    TcpHost *tcpServer;
    QThread *tcpThread;
    QList<QTableView *> tables;

    QSqlTableModel *model;
    QList<QSortFilterProxyModel *> proxies;
    QSortFilterProxyModel *proxyOfKeyColumn;
    QSortFilterProxyModel *proxyUpperLeft;
    QSortFilterProxyModel *proxyUpperRight;
    QSortFilterProxyModel *proxyLowerLeft;
    QSortFilterProxyModel *proxyLowerRight;
};

#endif // SVB_SERVER_H
