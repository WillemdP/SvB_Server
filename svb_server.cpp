#include "svb_server.h"
#include "ui_svb_server.h"

SvB_Server::SvB_Server(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::SvB_Server)
{
    ui->setupUi(this);

    tables << ui->tableView_uL << ui->tableView_uR << ui->tableView_lL << ui->tableView_lR;

    // Display quad block class
    // Main thread: receive Tcp newConnection() signals and spawn requestHandler class/threads



    tcpServer = new TcpHost();
    tcpThread = new QThread();
    tcpServer->moveToThread(tcpThread);
    // connect tcpServer signals to local slots
    tcpThread->start();
    ///

    proxyOfKeyColumn = new QSortFilterProxyModel(this);
    proxyUpperLeft = new QSortFilterProxyModel(this);
    proxyUpperRight = new QSortFilterProxyModel(this);
    proxyLowerLeft = new QSortFilterProxyModel(this);
    proxyLowerRight = new QSortFilterProxyModel(this);
    proxies << proxyUpperLeft << proxyUpperRight << proxyLowerLeft << proxyLowerRight;
    for(int i=0; i<tables.count(); i++)
        tables.at(i)->setModel(proxies.at(i));

    QSqlDatabase db = QSqlDatabase::addDatabase("QSQLITE", tr("Database"));
    db.setDatabaseName(tr("%1SvB2017.db3").arg(QDir::currentPath() + QDir::separator()));

    if(!db.open()) {
        qDebug() << "Could not open database:" << db.lastError().text();
        exit(0);
    }

    model = new QSqlTableModel(this, db);

    model->setTable("Stappers");
    model->select();
    model->setEditStrategy(QSqlTableModel::OnFieldChange);

//    if(model->columnCount()==0)
    buildModel();

    // importSpreadsheetFromWeb();

    ui->label_uL->setText("Spanne");
    ui->label_uR->setText("Gemeente");
    ui->label_lL->setText("Mans");
    ui->label_lR->setText("Dames");

    QString svbBlueA("rgb(40,60,220)"); // 80,60,220
    QString svbBlueB("rgb(20,30,110)");

//    QString gradient = tr("qradialgradient(cx:0, cy:0, radius: 1, fx:0.5, fy:0.5, stop:0 white, stop:1 green)");
    QString gradient = tr("qradialgradient(cx:0.5, cy:0.5, radius: 0.8, fx:0.5, fy:0.5, stop:0 %1, stop:1 %2)").arg("white", svbBlueA);

    QString tableStyle = tr("background-color: %1; border: %2; color: %3;").arg("transparent", "none", "black");
    QString headerStyle = tr("QHeaderView::section { background-color: %1; opacity: %2; border: %3; color: %4; }").arg(
                                                                "transparent", "0", "none", "black");
    // margin-right: 2; <- for header spacing

    int paid, personalID, ID, cellphone;
    QString header;
    for(int i=0; i<model->columnCount(); i++) {
        header = model->headerData(i, Qt::Horizontal).toString();
        if(header=="Stapper ID")
            ID = i;
        else if(header=="ID nommer")
            personalID = i;
        else if(header=="Betaal")
            paid = i;
        else if(header=="Selfoon nommer")
            cellphone = i;
    }

    foreach(QTableView *table, tables) {
        table->horizontalHeader()->setStyleSheet(headerStyle);
        table->setStyleSheet(tableStyle);
        table->verticalHeader()->hide();
//        table->horizontalHeader()->setStretchLastSection(true);
        table->setShowGrid(false);
        table->hideColumn(paid);
        table->hideColumn(ID);
        table->hideColumn(personalID);
        table->hideColumn(cellphone);
        table->resizeColumnsToContents();
    }

    setStyleSheet(tr("QTableView::item { border-left: 1px solid grey; }"));
    setStyleSheet(tr("QMainWindow {background-color: %1}").arg(gradient));

    QString labelStyle = tr("color: black; font: bold 30px \"Times New Roman\";");
    ui->label_uL->setStyleSheet(labelStyle);
    ui->label_uR->setStyleSheet(labelStyle);
    ui->label_lL->setStyleSheet(labelStyle);
    ui->label_lR->setStyleSheet(labelStyle);

    QTimer::singleShot(0, this, SLOT(showFullScreen()));
    qDebug() << "Startup complete";
}

SvB_Server::~SvB_Server()
{
    delete ui;
}

void SvB_Server::addNewClient(QString address)
{
    // Add client to list or new QThread
}

void SvB_Server::addNewEntry(QStringList columns, QVariantList values)
{
    // Register User Request
//    QSqlDatabase db = QSqlDatabase::database("Database");
    QSqlRecord record;
    QSqlField field;
    for(int i=0; i<columns.length(); i++) {
        field.setName(columns.at(i));
        field.setValue(values.at(i));
        record.append(field);
    }
    if(!model->insertRecord(-1, record)) {
        qDebug() << "Failed to insert requested entry:" << model->lastError().text();
//        emit sendMessage(address, "Failed");
        return;
    }
    qDebug() << "Successfully inserted a new entry upon request.";
//    emit sendMessage(address, "Success");
    return;
}

void SvB_Server::updateEntry(QString column, QVariant value)
{
//    QSqlDatabase db = QSqlDatabase::database("Database");
    QSqlRecord record;
    QSqlField field;
    field.setName(column);
    field.setValue(value);
    record.append(field);
    if(!model->setRecord(-1, record)) {
        qDebug() << "Update entry request failed." << model->lastError().text();
//        emit sendMessage(address, "Failed");
    }
    qDebug() << "Updated an entry upon request.";
//    emit sendMessage(address, "Success");
    return;
}

void SvB_Server::updateEntry(QStringList columns, QVariantList values)
{
//    QSqlDatabase db = QSqlDatabase::database("Database");
    QSqlRecord record;
    QSqlField field;
    for(int i=0; i<columns.length(); i++) {
        field.setName(columns.at(i));
        field.setValue(values.at(i));
        record.append(field);
    }
    if(!model->setRecord(-1, record)) {
        qDebug() << "Failed to insert requested entry:" << model->lastError().text();
//        emit sendMessage(address, "Failed");
        return;
    }
    qDebug() << "Successfully inserted a new entry upon request.";
//    emit sendMessage(address, "Success");
    return;
}

void SvB_Server::keyPressEvent(QKeyEvent *event)
{
    QMainWindow::keyPressEvent(event);

    switch (event->key()) {
    case Qt::Key_F:
        if(!event->modifiers().testFlag(Qt::ControlModifier))
            return;
        if(!isFullScreen())
            showFullScreen();
        break;
    case Qt::Key_X:
        if(event->modifiers().testFlag(Qt::ControlModifier))
            exit(0);
        break;
    case Qt::Key_Escape:
        if(isFullScreen())
            showNormal();
        break;
    default:
        break;
    }
    return;
}

void SvB_Server::buildModel()
{
    QSqlDatabase db = QSqlDatabase::database("Database");

    QSqlQuery query(db);
    QString queryText("drop table Stappers");
    if(!query.exec(queryText)) {
        qDebug() << "Table removed" << query.lastError().text();
    }

    queryText = QString("create table if not exists Stappers("
                      "Stapper_9_ID int primary key not null,"
                      "Naam varchar(20),"
                      "Van varchar(20),"
                      "Geslag varchar(1),"
                      "ID_9_nommer varchar(13),"
                      "Rondtes int,"
                      "Laaste_9_Rondte varchar(8),"
                      "Betaal int,"
                      "Selfoon_9_nommer varchar(10),"
                      "Span varchar(30),"
                      "Kerkverband varchar(30),"
                      "Gemeente varchar(30)"
                      ")"
                      );

    if(!query.exec(queryText)) {
        qDebug() << "Create table failed:" << query.lastError().text();
    }


    QSqlRecord newEntry;
    QSqlField field;

    field.setName(tr("%1").arg(model->headerData(0, Qt::Horizontal).toString()));
    field.setValue(QVariant(50));

    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(6, Qt::Horizontal).toString()));
    field.setValue(QVariant(QDateTime::currentDateTime().toString(tr("HH:MM:ss"))));
    newEntry.append(field);

    if(!model->insertRecord(0, newEntry)) {
        qDebug() << "Insert record failed:" << model->lastError().text();
    }
    newEntry.clear();
    ///
    field.setName(tr("%1").arg(model->headerData(0, Qt::Horizontal).toString()));
    field.setValue(QVariant(30));

    newEntry.append(field);

    if(!model->insertRecord(0, newEntry)) {
        qDebug() << "Insert record failed:" << model->lastError().text();
    }
    newEntry.clear();

    field.setName(tr("%1").arg(model->headerData(0, Qt::Horizontal).toString()));
    field.setValue(QVariant(44));

    newEntry.append(field);

    if(!model->insertRecord(0, newEntry)) {
        qDebug() << "Insert record failed:" << model->lastError().text();
    }
    newEntry.clear();

    field.setName(tr("%1").arg(model->headerData(0, Qt::Horizontal).toString()));
    field.setValue(QVariant(12));

    newEntry.append(field);

    if(!model->insertRecord(0, newEntry)) {
        qDebug() << "Insert record failed:" << model->lastError().text();
    }
    newEntry.clear();
    ///

    model->setTable("Stappers"); // Because the table was "dropped" and created anew.
    model->select();
    model->setEditStrategy(QSqlTableModel::OnFieldChange);
    proxyOfKeyColumn->setSourceModel(model);
    proxyOfKeyColumn->setSortCaseSensitivity(Qt::CaseInsensitive);
    proxyOfKeyColumn->sort(0, Qt::AscendingOrder);

    foreach(QSortFilterProxyModel *proxy, proxies) {
        proxy->setSourceModel(model);
        proxy->setSortCaseSensitivity(Qt::CaseInsensitive);
    }
    proxies.at(0)->sort(1);
    proxies.at(1)->sort(2);
    proxies.at(2)->sort(3);
    proxies.at(3)->sort(4);

    for(int i=0; i<model->columnCount(); i++) {
        model->setHeaderData(i, Qt::Horizontal, QVariant(toViewFormat(model->headerData(i, Qt::Horizontal).toString())));
    }


}

QString SvB_Server::toTableFormat(QString refString)
{
    return refString.simplified().replace(" ", "_9_").replace("/", "_0_").replace("(", "_1_").replace(")", "_");
}

QString SvB_Server::toViewFormat(QString refString)
{
    return refString.replace("_9_", " ").replace("_0_", "/").replace("_1_", "(").replace("_", ")");
}



//    Assuming you created your original table like so:

//    CREATE TABLE my_table (rowid INTEGER PRIMARY KEY, name TEXT, somedata TEXT) ;

//    You can create another sorted table like so:

//    CREATE TABLE my_ordered_table (rowid INTEGER PRIMARY KEY, name TEXT, somedata TEXT) ;

//    INSERT INTO my_ordered_table (name, somedata) SELECT name,somedata FROM my_table ORDER BY name ;

//    And if you then want to replace the orignal table:

//    DROP TABLE my_table ;

    //    ALTER TABLE my_ordered_table RENAME TO my_table;


















//
