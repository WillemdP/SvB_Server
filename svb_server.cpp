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


    ///
    tcpServer = new TcpHost();
    tcpThread = new QThread();
    tcpServer->moveToThread(tcpThread);
    connect(tcpServer, SIGNAL(newClient(QTcpSocket*,QString)), this, SLOT(addNewClient(QTcpSocket*,QString)));
    connect(tcpServer, SIGNAL(newEntryRequest(QTcpSocket*,QStringList,QVariantList)), this, SLOT(addNewEntry(QTcpSocket*,QStringList,QVariantList)));
    connect(tcpServer, SIGNAL(updateEntryRequest(QTcpSocket*,QString,QVariant)), this, SLOT(updateEntry(QTcpSocket*,QString,QVariant)));
    connect(tcpServer, SIGNAL(updateEntryRequest(QTcpSocket*,QStringList,QVariantList)), this, SLOT(updateEntry(QTcpSocket*,QStringList,QVariantList)));
    connect(this, SIGNAL(sendMessage(QTcpSocket*,QString)), tcpServer, SLOT(sendMessage(QTcpSocket*,QString)));

    tcpThread->start();
    ///

    proxyOfKeyColumn = new QSortFilterProxyModel(this);
    proxyUpperLeft = new QSortFilterProxyModel(this);
    proxyUpperRight = new QSortFilterProxyModel(this);
    proxyLowerLeft = new QSortFilterProxyModel(this);
    proxyLowerRight = new QSortFilterProxyModel(this);
    proxies << proxyUpperLeft << proxyUpperRight << proxyLowerLeft << proxyLowerRight;
    for(int i=0; i<tables.count(); i++) {
        tables.at(i)->setModel(proxies.at(i));
        tables.at(i)->selectionModel()->clearSelection();
    }

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
        else if(header=="ID Nommer")
            personalID = i;
        else if(header=="Betaal")
            paid = i;
        else if(header=="Selfoon Nommer")
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

void SvB_Server::addNewClient(QTcpSocket *socket, QString address)
{
    // Add client to list or new QThread
}

void SvB_Server::addNewEntry(QTcpSocket *socket, QStringList columns, QVariantList values)
{
    // Register User Request
//    QSqlDatabase db = QSqlDatabase::database("Database");
    QSqlRecord record;
    QSqlField field;
    QString name;
    QVariant id = QVariant(proxyOfKeyColumn->data(proxyOfKeyColumn->index(model->rowCount()-1, 0)).toInt() + 1);
    for(int i=0; i<columns.length(); i++) {
        field.setName(columns.at(i));
        field.setValue(values.at(i));
        if(columns.at(i)=="Naam")
            name = values.at(i).toString();
        record.append(field);
    }
    field.setName("Stapper_9_ID");
    field.setValue(id);
    record.append(field);
    field.setName("Rondtes");
    field.setValue(QVariant(0));
    record.append(field);
    if(!model->insertRecord(-1, record)) {
        qDebug() << "Failed to insert requested entry:" << model->lastError().text();
        emit sendMessage(socket, tr("Failed|%1").arg(name));
        return;
    }
    qDebug() << "Successfully inserted a new entry upon request.";
    emit sendMessage(socket, tr("Success|%1").arg(name));
    foreach(QTableView *table, tables)
        table->resizeColumnsToContents();
    return;
}

void SvB_Server::updateEntry(QTcpSocket *socket, QString column, QVariant value)
{
    if(!column.contains("QR_9_Code"))
        return;
    int qr = value.toInt();

    QString camera = column.split("|").at(0);
    qDebug() << column;

    QSqlDatabase db = QSqlDatabase::database("Database");
    QSqlQuery query(db);
    QString queryText = tr("select * from Stappers where QR_9_Code = %1").arg(qr); // Stapper_9_ID,Naam,Van,Rondtes,Laaste_9_Rondte,
    if(!query.exec(queryText)) {
        qDebug() << "Select query failed:" << query.lastError().text();
        emit sendMessage(socket, tr("Failed|%1").arg(value.toString()));
        return;
    }

    QString name;
    QString surname;
    QString sex;
    QString denom;
    QString congreg;
    QString id;
    int laps;
    QString lastLap;

    while(query.next()) {
        name = query.value(tr("Naam")).toString();
        surname = query.value(tr("Van")).toString();
        sex = query.value(tr("Geslag")).toString();
        denom = query.value(tr("Kerkverband")).toString();
        congreg = query.value(tr("Gemeente")).toString();
        id = query.value(tr("Stapper_9_ID")).toString();
        laps = query.value(tr("Rondtes")).toInt();
        lastLap = query.value(tr("Laaste_9_Rondte")).toString();
    }

    QTime last = QTime::fromString(lastLap, tr("HH:mm:ss"));
    QTime now = QDateTime::currentDateTime().time();
    if(lastLap!="")
        if(last.secsTo(now)<60) {
            qDebug() << "Too soon";
            emit sendMessage(socket, tr("Failed|%1").arg(value.toString()));
            return;
        }

    // The setRecord approach:
    // Using the proxyOfKeyColumn model, filter by QR Code, get modelIndex of result
    // baseIndex = proxyOfKeyColumn->mapToSource(resultIndex);
    // model->setRecord(baseIndex.row(),QVariant(TheValue));
    // Asynchronous?

    laps++;

    queryText = tr("update Stappers set %1 = %2 , %3 = '%4' where Stapper_9_ID = %5").arg("Rondtes", QString::number(laps), "Laaste_9_Rondte", now.toString("HH:mm:ss"), id);

    if(!query.exec(queryText)) {
        qDebug() << "Update entry request failed." << query.lastError().text();
        qDebug() << "Query:" << queryText.toStdString().c_str();
        emit sendMessage(socket, tr("Failed|%1").arg(value.toString()));
        return;
    }
    qDebug() << "Updated an entry upon request.";
    QStringList response;
    response << tr("Naam;%1").arg(name);
    response << tr("Van;%1").arg(surname);
    response << tr("Rondtes;%1").arg(QString::number(laps));
    response << tr("Geslag;%1").arg(sex);
    response << tr("Kerkverband;%1").arg(denom);
    response << tr("Gemeente;%1").arg(congreg);
//    response << tr("")
    emit sendMessage(socket, tr("Incremented|%1|%2").arg(camera, response.join("|")));
    foreach(QTableView *table, tables)
        table->resizeColumnsToContents();
    model->select();
    return;
}

void SvB_Server::updateEntry(QTcpSocket *socket, QStringList columns, QVariantList values)
{
//    QSqlDatabase db = QSqlDatabase::database("Database");
    QSqlRecord record;
    QSqlField field;
    QString name;
    for(int i=0; i<columns.length(); i++) {
        field.setName(columns.at(i));
        field.setValue(values.at(i));
        if(columns.at(i)=="Naam")
            name = values.at(i).toString();
        record.append(field);
    }
    if(!model->setRecord(-1, record)) {
        qDebug() << "Failed to insert requested entry:" << model->lastError().text();
        emit sendMessage(socket, tr("Failed|%1").arg(name));
        return;
    }
    qDebug() << "Successfully inserted a new entry upon request.";
    emit sendMessage(socket, tr("Success|%1").arg(name));
    foreach(QTableView *table, tables)
        table->resizeColumnsToContents();
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
                      "ID_9_Nommer varchar(13),"
                      "Rondtes int,"
                      "Laaste_9_Rondte varchar(8),"
                      "Betaal int,"
                      "Selfoon_9_Nommer varchar(10),"
                      "Span varchar(30),"
                      "Kerkverband varchar(30),"
                      "Gemeente varchar(30),"
                      "QR_9_Code varchar(8)"
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
    field.setName(tr("%1").arg(model->headerData(1, Qt::Horizontal).toString()));
    field.setValue(QVariant("Naam1"));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(2, Qt::Horizontal).toString()));
    field.setValue(QVariant(tr("a")));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(3, Qt::Horizontal).toString()));
    field.setValue(QVariant(tr("b")));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(4, Qt::Horizontal).toString()));
    field.setValue(QVariant(tr("c")));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(5, Qt::Horizontal).toString()));
    field.setValue(QVariant(0));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(12, Qt::Horizontal).toString()));
    field.setValue(QVariant(915));
    newEntry.append(field);

    if(!model->insertRecord(0, newEntry)) {
        qDebug() << "Insert record failed:" << model->lastError().text();
    }
    newEntry.clear();


    field.setName(tr("%1").arg(model->headerData(0, Qt::Horizontal).toString()));
    field.setValue(QVariant(30));

    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(1, Qt::Horizontal).toString()));
    field.setValue(QVariant("Naam2"));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(2, Qt::Horizontal).toString()));
    field.setValue(QVariant(tr("dsoifsdofijsdof")));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(3, Qt::Horizontal).toString()));
    field.setValue(QVariant(tr("f")));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(4, Qt::Horizontal).toString()));
    field.setValue(QVariant(tr("j")));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(5, Qt::Horizontal).toString()));
    field.setValue(QVariant(0));
    newEntry.append(field);
    field.setName(tr("%1").arg(model->headerData(12, Qt::Horizontal).toString()));
    field.setValue(QVariant(930));
    newEntry.append(field);

    if(!model->insertRecord(0, newEntry)) {
        qDebug() << "Insert record failed:" << model->lastError().text();
    }
    newEntry.clear();



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
    foreach(QTableView *table, tables)
        table->resizeColumnsToContents();
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
