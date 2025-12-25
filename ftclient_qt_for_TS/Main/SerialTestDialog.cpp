#include "SerialTestDialog.h"
#include "ui_SerialTestDialog.h"

#include <QSerialPortInfo>
#include <QFileDialog>
#include <QFile>
#include <QTextStream>
#include <QMessageBox>
#include <QSettings>

static QSettings *g_set;

SerialTestDialog::SerialTestDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::SerialTestDialog)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

    g_set = new QSettings(QCoreApplication::applicationDirPath() + "FTClientCOM.ini",QSettings::IniFormat) ;

    m_pCOM = new GenComport(this) ;

    m_model = new QStandardItemModel();
    QStringList headers;
    headers << "串口命令" <<  "测试结果";
    m_model->setHorizontalHeaderLabels(headers);
    ui->tableView->setModel(m_model);
    ui->tableView->setColumnWidth(0,400) ;
    ui->tableView->setColumnWidth(1,158) ;

    ui->tableView->horizontalHeader()->setStyleSheet("QHeaderView::section{background:skyblue;border:none;}");
    ui->tableView->setStyleSheet(
        R"(
            QTableView{border: 1px solid skyblue; color:#000000; font: 12px 75 Fixedsys; background: transparent;}
            QTableView::Item{ border: none; color:#000000; padding-left:5px; font: 20px 75 微软雅黑; border-bottom: 1px solid skyblue;}
            QTableView::Item::selected{ background-color: #A0bb9e; color:white; outline: none;}
            QTableView::Item::focus { outline: none; }

            QTableView::indicator {
                width: 18px;
                height: 18px;
            }

            QTableView::indicator:unchecked {
                border: 2px solid #888;
                background-color: #fff;
            }

            QTableView::indicator:checked {
                image: url(:/UI/check.png);
            }

            QTableView::indicator:indeterminate {
                border: 2px solid #888;
                background-color: #ccc;
            }
        )"
    );


//image: url(data:image/svg+xml;base64,PHN2ZyB4bWxucz0iaHR0cDovL3d3dy53My5vcmcvMjAwMC9zdmciIHdpZHRoPSIyNCIgaGVpZ2h0PSIyNCIgdmlld0JveD0iMCAwIDI0IDI0Ij48cGF0aCBkPSJNMjAgMy44MjhsLTEwLjgxNyAxMS4zOTctNS4zODMtNC44OTctMS4yMTIgMS4yOTkgNi42MDEgNS45ODQgMTIuMDE4LTEzLjQ4NnoiIGZpbGw9IiNmZmZmZmYiLz48L3N2Zz4=);

    ui->tableView->setShowGrid(false);
    ui->tableView->setSelectionBehavior(QAbstractItemView::SelectRows);

    on_pushButton_clicked() ;

    LoadCmdFile(g_set->value("lastFile").toString().trimmed()) ;

    connect(ui->tableView,&QAbstractItemView::clicked,this,[=](const QModelIndex&index){

        m_nCurRow = index.row() ;
        QStandardItem *item1 =  m_model->item(index.row());
        ui->lineEditCommand->setText(item1->text().trimmed()) ;
        //qDebug() << item1->text().trimmed() ;

    });

    QItemSelectionModel *selectionModel = ui->tableView->selectionModel();

    connect(selectionModel, &QItemSelectionModel::selectionChanged, this,[=](const QItemSelection &selected, const QItemSelection &deselected){
        Q_UNUSED(deselected)
        QModelIndexList selectedIndexes = selected.indexes();
        QModelIndex&index = selectedIndexes[0] ;
        QStandardItem *item1 =  m_model->item(index.row());
        ui->lineEditCommand->setText(item1->text().trimmed()) ;

        m_nCurRow = index.row() ;
    });

    ui->tableView->setSelectionMode(QAbstractItemView::SingleSelection);

    connect(m_pCOM,&GenComport::onReceive,this,[=](const QByteArray &data){
        if(m_nCurRow != -1)
        {
            QString strRet(data) ;
            QStandardItem *item1 = m_model->item(m_nCurRow,1);
            item1->setText(strRet) ;
        }
    });

    updateCOMStatus() ;

    QString strCOM = g_set->value("lastCOM","COM1").toString().trimmed() ;
    QString strBaud = g_set->value("lastBaud","9600").toString().trimmed() ;
    QString strData = g_set->value("lastData","8").toString().trimmed() ;
    int nStop = g_set->value("lastStop",0).toInt() ;
    int nParity = g_set->value("lastParity",0).toInt()  ;
    int nFlow = g_set->value("lastFlow",0).toInt()  ;

    int nCount = ui->comboBox->count() ;
    for(int i=0; i<nCount; i++)
    {
        if(ui->comboBox->itemText(i).startsWith(strCOM))
        {
            ui->comboBox->setCurrentIndex(i);
            break;
        }
    }

    int nFind = -1 ;
    nFind = ui->comboBoxBaud->findText(strBaud);
    if(nFind != -1)
        ui->comboBoxBaud->setCurrentIndex(nFind) ;

    nFind = ui->comboBoxData->findText(strData);
    if(nFind != -1)
        ui->comboBoxData->setCurrentIndex(nFind) ;

    ui->comboBoxStop->setCurrentIndex(nStop);
    ui->comboBoxParity->setCurrentIndex(nParity);
    ui->comboBoxFlow->setCurrentIndex(nFlow);
}

SerialTestDialog::~SerialTestDialog()
{
    delete ui;
}

void SerialTestDialog::LoadCmdFile(const QString&strFile)
{
    QStringList cmdList ;

    QFile file(strFile);
    if (file.open(QIODevice::ReadOnly | QIODevice::Text))
    {
        m_strCurFile = strFile ;
        setWindowTitle("串口测试 [命令文件: "  + strFile + "]");
        g_set->setValue("lastFile",strFile) ;
        QTextStream in(&file);

        while (!in.atEnd()) {
            QString line = in.readLine();
            cmdList << line.trimmed();
        }
        file.close();
    }

    if(cmdList.isEmpty())
    {
        cmdList << "start_set_factory_info";
        cmdList << "factory_set_wifi Redmi_7325 qqqqqqqq";
        cmdList << "factory_set_server 47.106.117.102 3736";
        cmdList << "factory_set_mqtt tanshi tanshi987123";
        cmdList << "factory_set_work_pos_id 6688";
        cmdList << "stop_set_factory_info";
    }

    while(m_model->rowCount())
        m_model->removeRow(0);
    foreach (const QString &strCmd, cmdList)
    {
        QList<QStandardItem *> items;
        QStandardItem *item1 = new QStandardItem(strCmd);
        item1->setCheckable(true) ;
        item1->setCheckState(Qt::Checked) ;
        items.append(item1);

        QStandardItem *item2 = new QStandardItem("");
        items.append(item2);

        m_model->appendRow(items) ;
    }
}

void SerialTestDialog::on_pushButton_clicked()
{
    ui->comboBox->clear() ;
    QList<QSerialPortInfo> serialPortInfos = QSerialPortInfo::availablePorts();

    foreach (const QSerialPortInfo &SPI, serialPortInfos)
    {
        ui->comboBox->addItem(SPI.portName() + QString("  - ") + SPI.description()) ;
    }
    ui->comboBox->setCurrentIndex(0) ;
}


void SerialTestDialog::on_pushButtonLoad_clicked()
{
    QString strFile = QFileDialog::getOpenFileName(
        nullptr,
        "请选择串口命令文件",
        "",
        "文本文件 (*.txt);;All Files (*.*)"
        );
    if( strFile.isEmpty() )
        return ;

    m_strCurFile = strFile;
}


void SerialTestDialog::on_pushButtonSave_clicked()
{
    if(m_strCurFile.isEmpty())
    {
        on_pushButtonSaveAs_clicked() ;
        return  ;
    }

    QFile file( m_strCurFile );
    if( file.open( QIODevice::WriteOnly ) )
    {
        int nCount = m_model->rowCount();
        for(int i=0; i<nCount; i++)
        {
            QStandardItem *item1 =  m_model->item(i);
            QStandardItem *item2 =  m_model->item(i,1);

            item2->setText("OK") ;
            QString strLine = item1->text().trimmed() + "\n" ;
            file.write(strLine.toStdString().c_str()) ;
        }
        file.close() ;

        QMessageBox::information(this,"提示","保存 成功！") ;
    }
}


void SerialTestDialog::on_pushButtonSaveAs_clicked()
{
    QString strFile = QFileDialog::getSaveFileName(
        nullptr,
        "请选择串口命令文件",
        "",
        "文本文件 (*.txt);;All Files (*.*)"
        );
    if( strFile.isEmpty() )
        return ;

    m_strCurFile = strFile;    
    setWindowTitle("串口测试 [命令文件: "  + strFile + "]");

    g_set->setValue("lastFile",strFile) ;
    on_pushButtonSave_clicked() ;
}


void SerialTestDialog::on_pushButtonHide_clicked()
{
    this->hide() ;
}


void SerialTestDialog::on_checkBox_clicked()
{
    bool bToOpen = ui->checkBox->isChecked() ;

    m_pCOM->closePort() ;

    if(bToOpen)
    {
        QString strCOM = ui->comboBox->currentText().trimmed() ;
        int nFind = strCOM.indexOf('-') ;
        strCOM = strCOM.left(nFind).trimmed() ;

        int nBaud = ui->comboBoxBaud->currentText().toInt() ;
        int nData = ui->comboBoxData->currentText().toInt() ;
        int nStop = ui->comboBoxStop->currentIndex();
        int nParity = ui->comboBoxParity->currentIndex() ;
        int nFlow = ui->comboBoxFlow->currentIndex();

        g_set->setValue("lastCOM",strCOM) ;
        g_set->setValue("lastBaud",nBaud) ;
        g_set->setValue("lastData",nData) ;
        g_set->setValue("lastStop",nStop) ;
        g_set->setValue("lastParity",nParity) ;
        g_set->setValue("lastFlow",nFlow) ;

        m_pCOM->setPortParam(nBaud,nData,nStop,nParity,nFlow) ;
        m_pCOM->setPortName(strCOM) ;
    }
    updateCOMStatus() ;
}

void SerialTestDialog::updateCOMStatus()
{
    if(m_pCOM && m_pCOM->isOpen())
    {
        QString strInfo = m_pCOM->getPortName() + " 已打开";
        ui->labelStatus->setText(strInfo) ;
        ui->labelStatus->setStyleSheet("QLabel{color: blue;}") ;
    }
    else
    {
        ui->labelStatus->setText("串口未打开") ;
        ui->labelStatus->setStyleSheet("QLabel{color: red;}") ;
    }
}

void SerialTestDialog::on_comboBox_currentIndexChanged(int index)
{
    Q_UNUSED(index)
    if(ui->checkBox->isChecked())
        on_checkBox_clicked() ;
}

bool SerialTestDialog::sendCommand(const QString&strCmd)
{
    if(m_pCOM && m_pCOM->isOpen())
    {
        m_pCOM->send(strCmd,true) ;
        return true ;
    }
    return false ;
}


void SerialTestDialog::on_pushButtonSend_clicked()
{
    QString strCmd = ui->lineEditCommand->text().trimmed() ;
    sendCommand(strCmd);
}


void SerialTestDialog::on_pushButtonNew_clicked()
{
    QList<QStandardItem *> items;
    QString strCmd = ui->lineEditCommand->text().trimmed() ;
    QStandardItem *item1 = new QStandardItem(strCmd);
    item1->setCheckable(true) ;
    item1->setCheckState(Qt::Checked) ;
    items.append(item1);

    QStandardItem *item2 = new QStandardItem("");
    items.append(item2);

    m_model->appendRow(items) ;
}


void SerialTestDialog::on_pushButtonDel_clicked()
{
    if(m_nCurRow != -1)
    {
        m_model->removeRow(m_nCurRow);
        m_nCurRow = -1 ;
    }

}

