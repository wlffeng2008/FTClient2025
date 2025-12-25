
#include "ui_BatchTestWidget.h"
#include "BatchTestWidget.h"
#include "ButtonDelegate.h"
#include "CSysSettings.h"

#include <QJsonObject>
#include <QMessageBox>
#include <QJsonDocument>
#include <QNetworkInterface>
#include <QThread>

QString getMyIP()
{
    QString strIP ;
    QList<QNetworkInterface> interfaces = QNetworkInterface::allInterfaces();
    for (const QNetworkInterface &A : interfaces)
    {
        if ( A.flags().testFlag(QNetworkInterface::IsUp) &&
            !A.flags().testFlag(QNetworkInterface::IsLoopBack))
        {
            if(!A.flags().testFlag(QNetworkInterface::IsRunning))
                continue ;
            if(!(A.type() == QNetworkInterface::Wifi || A.type() == QNetworkInterface::Ethernet))
                continue;
            if(A.type()==(QNetworkInterface::Phonet))
                continue ;

            QList<QNetworkAddressEntry> entries = A.addressEntries();
            for (const QNetworkAddressEntry &entry : entries)
            {
                QHostAddress IP = entry.ip() ;
                if (IP.protocol() == QAbstractSocket::IPv4Protocol)
                {
                    QString strTmp = IP.toString() ;
                    if(!strTmp.endsWith(".1") && !strTmp.contains(".77."))
                    {
                        strIP = strTmp ;
                        break ;
                    }
                }
            }

            if(!strIP.isEmpty())
                break;
        }
    }
    return strIP ;
}


static int TEST_ITEM_COLUMN_NAME = 0;                // "测试项目"
static int TEST_ITEM_COLUMN_STATUS = 1;              // "测试状态"
static int TEST_ITEM_COLUMN_PARA_TO_WRITE = 2;      // "写入参数"
static int TEST_ITEM_COLUMN_RETURN_VALUE = 3;       // "返回值"
static int TEST_ITEM_COLUMN_PARA_TO_CHECK = 4;      // "校验参数"
static int TEST_ITEM_COLUMN_RESULT = 5;              // "执行结果"
static int TEST_ITEM_COLUMN_MAN_PASS = 6;            // "手动pass"
static int TEST_ITEM_COLUMN_CMD_ID = 7;              // "命令ID"
static int TEST_ITEM_COLUMN_ENABLE = 8;              // "启用该项"
static int TEST_ITEM_COLUMN_EXEC_TEST = 9;          // "测试该项"

static QMqttClient *m_pMqtt = nullptr ;
static QString m_strTopic ;
void BatchTestWidget::setMQTTAgent(QMqttClient *pMqtt,const QString&strTopic)
{
    m_pMqtt    = pMqtt ;
    m_strTopic = strTopic ;
}

BatchTestWidget::BatchTestWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::BatchTestWidget )
{
    ui->setupUi( this );

    m_pBTDlg = new DialogBlueTooth(this) ;
    connect(m_pBTDlg,&DialogBlueTooth::reportBleDevice,this,[=](const QString &strDeviceMac,const QString &strDeviceName,bool bShow){
        QString strID = m_strDestBle.trimmed() ;
        QString strTmp = strDeviceMac.trimmed() ;

        if(strTmp.remove(':').toUpper() == strID.remove(':').toUpper() || strID == strTmp )
        {
            //m_TimeBle.stop() ;
            //updateTestResult("dev_ble_detection",strDeviceMac,"OK") ;
            if(bShow)
            {
                static bool bInshow = false ;
                if(bInshow)
                    return ;
                bInshow = true ;
                QString strInfo = QString("找到指蓝牙定设备：%1 -- %2").arg(strDeviceMac).arg(strDeviceName);
                QMessageBox::information(this,"提示",strInfo);
                bInshow = false ;
            }
        }
    });

    ui->tableView_test_items->setShowGrid(false);
    ui->tableView_test_items->setHorizontalScrollBarPolicy(Qt::ScrollBarAlwaysOff);
    ui->tableView_test_items->setSelectionBehavior(QAbstractItemView::SelectRows);
    ui->tableView_test_items->setSelectionMode(QAbstractItemView::SingleSelection);
    initTestItemTable();

    connect(&m_TMSendCmd,&QTimer::timeout,this,[this](){
        if(m_pMqtt && !m_strMqtt.isEmpty())
            m_pMqtt->publish(QMqttTopicName(m_strTopic),QByteArray(m_strMqtt.toUtf8()),0,false) ;
        m_strMqtt.clear() ;
    }) ;
    m_TMSendCmd.start(10) ;

    connect(&m_TMHeart,&QTimer::timeout,this,[this](){
        if(!m_strDevId.isEmpty())
        {
            m_bHeartbeat = true ;
            //testRow(0) ;
        }
    }) ;

    //m_TMHeart.start(55000) ;
    ui->checkBox_enable_all->click() ;

    ui->textEditLog->setMaximumBlockCount(200) ;
    QHeaderView *pHHDV = ui->tableView_test_items->horizontalHeader() ;
    connect(pHHDV,&QHeaderView::sectionDoubleClicked,this,[=](int logicalIndex){
        if(TEST_ITEM_COLUMN_ENABLE == logicalIndex)
        {
            int count = m_pModelItems->rowCount() ;
            bool bset = m_pModelItems->item(0,logicalIndex)->checkState() != Qt::Checked;
            for(int i=0; i<count; i++)
                m_pModelItems->item(i,logicalIndex)->setCheckState(bset?Qt::Checked:Qt::Unchecked) ;
        }
    });

    const QList<QPushButton*> btns = findChildren<QPushButton*>();
    for(QPushButton *btn : std::as_const(btns))
    {
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRepeat(false);
    }
}

BatchTestWidget::~BatchTestWidget()
{
    delete ui;
}


void BatchTestWidget::initTestItemTable()
{
    ui->tableView_test_items->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    ui->tableView_test_items->horizontalHeader()->setSectionResizeMode(QHeaderView::Fixed) ;
    ui->tableView_test_items->horizontalHeader()->setStretchLastSection(true) ;

    int column_num = 10;
    int row_num = 0;

    m_pModelItems = new QStandardItemModel(row_num, column_num, this);

    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_NAME, Qt::Horizontal, "项目");
    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_STATUS, Qt::Horizontal, "状态");
    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_PARA_TO_WRITE, Qt::Horizontal, "参数");
    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_RETURN_VALUE, Qt::Horizontal, "返回值");
    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_PARA_TO_CHECK, Qt::Horizontal, "核对值");
    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_RESULT, Qt::Horizontal, "结果");
    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_MAN_PASS, Qt::Horizontal, "确认");
    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_CMD_ID, Qt::Horizontal, "项目ID"); // 该列隐藏，用于保存测试ID
    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_ENABLE, Qt::Horizontal, "启用");
    m_pModelItems->setHeaderData(TEST_ITEM_COLUMN_EXEC_TEST, Qt::Horizontal, "执行");

    ui->tableView_test_items->setModel(m_pModelItems);
    ui->tableView_test_items->horizontalHeader()->setStretchLastSection(true);

    int w_name = 80;
    int w_status = 60;
    int w_para = 48;
    int w_ret_val = 100;
    int w_check_val = 100;
    int w_result = 48;
    int w_pass = 48;
    int w_en = 32;
    int w_exec = 45;

    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_NAME, w_name );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_STATUS, w_status );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_PARA_TO_WRITE, w_para );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_RETURN_VALUE, w_ret_val );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_PARA_TO_CHECK, w_check_val );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_RESULT, w_result );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_MAN_PASS, w_pass );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_ENABLE, w_en );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_EXEC_TEST, w_exec );
}

void BatchTestWidget::addAllItemDataToTableOf( const QMap<int, QJsonObject> &mapCmdEx, bool bNeedManPass )
{
    int row_index = m_pModelItems->rowCount();
    for( const auto &pair : mapCmdEx )
    {
        QJsonObject cmdJson = pair;

        QString strCmd = cmdJson["item_cmd"].toString().trimmed();
        if( strCmd.isEmpty() )
            continue;

        bool bItemNeedManPass = ( bool )cmdJson["man_pass"].toInt();
        if( bItemNeedManPass != bNeedManPass )
            continue;

        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_NAME, new QStandardItem(cmdJson["item_name"].toString() ) );
        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_STATUS, new QStandardItem("待测试") );
        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_PARA_TO_WRITE, new QStandardItem(cmdJson["para_to_write"].toString()) );
        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_RETURN_VALUE, new QStandardItem("") );

        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_PARA_TO_CHECK, new QStandardItem(cmdJson["para_to_check"].toString()) );
        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_RESULT, new QStandardItem("") );

        QStandardItem* item_pass = new QStandardItem( "----" );
        item_pass->setTextAlignment(Qt::AlignCenter);
        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_MAN_PASS, item_pass );
        if( bItemNeedManPass )
            m_pManPassDelegate->initButtonRow( row_index );
         else
            m_pManPassDelegate->setHideButtonRow( row_index );

        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_CMD_ID, new QStandardItem( QString("%1").arg( row_index )));

        QStandardItem *checkItem = new QStandardItem();
        checkItem->setCheckable( true );
        checkItem->setCheckState( Qt::Checked );
        checkItem->setTextAlignment(Qt::AlignCenter);
        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_ENABLE, checkItem );

        m_pModelItems->setItem( row_index, TEST_ITEM_COLUMN_EXEC_TEST, new QStandardItem("") );

        ui->tableView_test_items->setRowHeight( row_index, 30 );

        row_index++;
    }
}

void BatchTestWidget::updateTestItemData( const QMap<int, QJsonObject> &mapCmdEx )
{
    while( m_pModelItems->rowCount() > 0 )
        m_pModelItems->removeRow(0);
    m_mapCmdEx = mapCmdEx ;

    m_pManPassDelegate = new ButtonDelegateManPass( "通过", "待确认", this );
    ui->tableView_test_items->setItemDelegateForColumn( TEST_ITEM_COLUMN_MAN_PASS, m_pManPassDelegate );
    connect( m_pManPassDelegate, &ButtonDelegateManPass::buttonClicked, this, &BatchTestWidget::handleItemManPassButtonClicked );

    m_pTestDelegate = new ButtonDelegate( "测试", this );
    ui->tableView_test_items->setItemDelegateForColumn( TEST_ITEM_COLUMN_EXEC_TEST, m_pTestDelegate );
    connect( m_pTestDelegate, &ButtonDelegate::buttonClicked, this, &BatchTestWidget::handleItemTestButtonClicked );

    ui->tableView_test_items->hideColumn( TEST_ITEM_COLUMN_CMD_ID );
    ui->tableView_test_items->hideColumn( TEST_ITEM_COLUMN_PARA_TO_WRITE );

    addAllItemDataToTableOf( mapCmdEx, false );
    addAllItemDataToTableOf( mapCmdEx, true );

    for( int row = 0; row < m_pModelItems->rowCount(); ++row )
    {
        QJsonObject o = mapCmdEx[row] ;
        QString btn_text = o["btn_text"].toString().trimmed();
        btn_text = btn_text.isEmpty() ? "测试" : btn_text;
        m_pModelItems->setData( m_pModelItems->index( row, TEST_ITEM_COLUMN_EXEC_TEST ), btn_text, Qt::UserRole + 1 );
    }

    m_pManPassDelegate->markAllStatus(false) ;

    QTimer::singleShot(1000,this,[=]
    {
       for(int j=0; j<m_pModelItems->rowCount(); j++)
       {
           for(int i=0; i<10; i++)
           {
               QStandardItem *item = m_pModelItems->item(j,i);
               if(i != TEST_ITEM_COLUMN_PARA_TO_CHECK && item)
                   item->setEditable(false) ;
           }
       }
    });
}

void BatchTestWidget::updateTestResult(const QString&strAction,const QString&strData,const QString&strStatus)
{
    m_TMHeart.stop() ;
    m_TMHeart.start(55000) ;

    int nCount = m_pModelItems->rowCount()  ;
    for(int i=0; i<nCount; i++)
    {
        QString strCmd = m_mapCmdEx[i]["item_cmd"].toString().trimmed();
        if(strAction == strCmd)
        {
            m_pModelItems->item(i, TEST_ITEM_COLUMN_RETURN_VALUE)->setText(strData) ;
            m_pModelItems->item(i, TEST_ITEM_COLUMN_STATUS)->setText("完成");
            QString strCheck = m_pModelItems->item(i, TEST_ITEM_COLUMN_PARA_TO_CHECK)->text().trimmed();

            bool bOK = (strStatus.toLower().trimmed() == "ok") ;
            if(!strCheck.isEmpty() && strCheck != strData)
                bOK = false ;

            m_pModelItems->item(i, TEST_ITEM_COLUMN_RESULT)->setText(bOK?"成功":"失败");
            m_pManPassDelegate->setPassStatus(i,bOK) ;
            break;
        }
    }
}

void BatchTestWidget::addMqttLog(const QString&strLog)
{
    ui->textEditLog->appendPlainText(strLog) ;
    qDebug().noquote() << strLog;
}

void BatchTestWidget::handleItemManPassButtonClicked( const QModelIndex &index )
{
    int row = index.row();
    if( row < 0 )
        return;

    if( m_pManPassDelegate != nullptr )
    {
        m_pManPassDelegate->switchStatus( row );
        ui->tableView_test_items->viewport()->update();  // 强制刷新视图

        bool bOK = m_pManPassDelegate->getPassStatus( row );

        QStandardItem *pItem = m_pModelItems->item(row, TEST_ITEM_COLUMN_RESULT);
        pItem->setText(bOK?"成功":"失败") ;
    }

    checkFinalTestResult();
}

void BatchTestWidget::checkFinalTestResult()
{
    m_bFinalTestOk = m_pManPassDelegate->getAllPassed() && m_bAutoBatchTestOk;
    ui->labelTestResult->setText( m_bFinalTestOk ? "全部通过" : "部分通过" );
}

bool BatchTestWidget::testRow( int row )
{
    if(m_strDevId.isEmpty() || row < 0)
    {
        QMessageBox::warning(nullptr,"提示","设备未绑定！");
        return false;
    }

    if(row >= m_pModelItems->rowCount())
        return 0 ;

    QString strAct = m_mapCmdEx[row]["item_cmd"].toString() ;
    QString strData = m_mapCmdEx[row]["para_to_write"].toString().trimmed() ;
    m_strAct = strAct ;

    QJsonObject jData = {};
    if(strData.startsWith('{') && strData.endsWith('}'))
    {
        QJsonDocument jDoc = QJsonDocument::fromJson(strData.toUtf8()) ;
        jData = jDoc.object() ;
    }

    QJsonObject jIQ ;
    jIQ["type"] = "request";
    jIQ["action"] = strAct;
    jIQ["seq_number"] = 500 + row;
    if(!strData.isEmpty())
         jIQ["data"] = strData ;

    if(strAct == "wifi_throughput")
    {
        jData = {} ;
        jData["server_ip"] = getMyIP() ;
        jData["server_Port"] = 5002 ;
    }

    if(strAct == "dev_ble_detection")
    {
        m_strDestBle = strData ;
        m_pBTDlg->scanBlueTooth(strData);
    }

    if(!jData.isEmpty())
        jIQ["data"] = jData ;

    m_pModelItems->item( row, TEST_ITEM_COLUMN_STATUS)->setText("测试中");

    QString strJson = QJsonDocument(jIQ).toJson() ;
    addMqttLog(strJson) ;
    m_strMqtt = strJson ;

    return true;
}

void BatchTestWidget::handleItemTestButtonClicked( const QModelIndex &index )
{
    m_bHeartbeat = false ;
    testRow(index.row()) ;
}


void BatchTestWidget::on_pbtn_browse_default_values_clicked()
{
    std::vector<QJsonObject> vecData;
    if( !CSysSettings::getInstance()->loadCurDevTypeSetDefaultCmds( vecData ) )
    {
        QMessageBox::warning( this, "错误", "载入默认值配置错误!" );
        return ;
    }

    if( !m_pDefaultValuesWidget )
    {
        m_pDefaultValuesWidget = new DialogDefaultValues(this);

        connect(m_pDefaultValuesWidget,&DialogDefaultValues::writeDefaultValue,this,[=](const QJsonArray&jValue)
        {
            if(m_strDevId.isEmpty() )
            {
                QMessageBox::warning(nullptr,"提示","设备未绑定！");
                return ;
            }

            QJsonObject jIQ ;
            jIQ["type"]       = "request";
            jIQ["action"]     = "set_var_default";
            jIQ["seq_number"] = 520;
            jIQ["data"]       = jValue ;

            QString strJson = QJsonDocument(jIQ).toJson() ;
            addMqttLog(strJson) ;

            if(m_pMqtt) m_pMqtt->publish(QMqttTopicName(m_strTopic),QByteArray(strJson.toUtf8()),0,false) ;
        });
    }

    m_pDefaultValuesWidget->updateItemData( vecData );
    m_pDefaultValuesWidget->show();
}


void BatchTestWidget::on_pbtn_batch_test_start_clicked()
{
    on_pbtn_reset_clicked();
    if(m_strDevId.isEmpty())
    {
        QMessageBox::warning(nullptr,"提示","设备未绑定！");
        return ;
    }

    ui->labelTestResult->setText( "测试中" );

    if(!m_pBThread)
        m_pBThread = new BatTestThread(this) ;

    m_pBThread->start() ;
}

void BatchTestWidget::batTest()
{
    m_bHeartbeat = false ;
    for (int row = 0; row < m_pModelItems->rowCount(); ++row)
    {
        QStandardItem *checkItem = m_pModelItems->item( row, TEST_ITEM_COLUMN_ENABLE );
        bool checked = ( checkItem->checkState() == Qt::Checked );
        if(!checked) continue ;

        testRow(row) ;

        int nSleep = 700 ;
        if( m_strAct == "led_on"  ||
            m_strAct == "infrared_led_switch_on" ||
            m_strAct == "key_detect")
            nSleep = 3000;
        QThread::msleep(nSleep);
    }
}

void BatchTestWidget::autoTestAll()
{
    on_pbtn_batch_test_start_clicked();
}

void BatchTestWidget::on_pbtn_reset_clicked()
{
    for (int row = 0; row < m_pModelItems->rowCount(); ++row)
    {
        m_pModelItems->item(row, TEST_ITEM_COLUMN_STATUS)->setText("待测试");
        m_pModelItems->item(row, TEST_ITEM_COLUMN_RETURN_VALUE)->setText("");
        m_pModelItems->item(row, TEST_ITEM_COLUMN_RESULT)->setText("");
    }

    m_pManPassDelegate->markAllStatus( false );

    ui->labelTestResult->setText( "待测试" );
    ui->textEditLog->clear() ;
}

void BatchTestWidget::resetResult()
{
    on_pbtn_reset_clicked() ;
}

void BatchTestWidget::on_checkBox_enable_all_checkStateChanged(const Qt::CheckState &state )
{
    for (int row = 0; row < m_pModelItems->rowCount(); row++)
    {
        QStandardItem *checkItem = m_pModelItems->item(row, TEST_ITEM_COLUMN_ENABLE);
        checkItem->setCheckState(state);
    }
}

void BatchTestWidget::on_pbtn_batch_success_clicked()
{
    m_bAutoBatchTestOk = true ;
    checkFinalTestResult();
    if(!m_bFinalTestOk)
        QMessageBox::warning(nullptr,"提示","注意：有一个或多个测试项没通过！");
}

void BatchTestWidget::setDevId(const QString&strDevId)
{
    m_strDevId = strDevId;
    if(!strDevId.isEmpty())
        m_pBTDlg->scanBlueTooth(strDevId);
}

void BatchTestWidget::on_pushButtonBle_clicked()
{
    m_pBTDlg->show() ;
}

