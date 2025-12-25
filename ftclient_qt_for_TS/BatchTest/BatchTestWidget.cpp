#include "ui_BatchTestWidget.h"
#include "BatchTestWidget.h"
#include <QJsonObject>
#include <QMessageBox>
#include "ButtonDelegate.h"
#include "CSysSettings.h"
#include <memory>
#include <QJsonDocument>
#include <QNetworkInterface>

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

typedef enum
{
    TEST_ITEM_COLUMN_NAME = 0,        // "测试项目"
    TEST_ITEM_COLUMN_STATUS,          // = 1   // "状态"
    TEST_ITEM_COLUMN_RETURN_VALUE,   // = 2;  // "返回值"
    TEST_ITEM_COLUMN_PARA_TO_CHECK,  // = 3;  // "校验参数"
    TEST_ITEM_COLUMN_RESULT,          // = 4;  // "执行结果"
    TEST_ITEM_COLUMN_MAN_PASS,        // = 5;  // "手动pass"
    TEST_ITEM_COLUMN_ENABLE,          // = 6;  // "启用该项"
    TEST_ITEM_COLUMN_EXEC_TEST,       // = 7;  // "测试该项"
}TestColumn;


static QMqttClient *m_pMqtt =nullptr ;
static QString m_strTopic ;
void BatchTestWidget::setMQTTAgent(QMqttClient *pMqtt,const QString&strTopic)
{
    m_pMqtt = pMqtt ;
    m_strTopic = strTopic ;
}

BatchTestWidget::BatchTestWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::BatchTestWidget ), m_bAutoBatchTestOk( false ), m_bFinalTestOk( false )
{
    ui->setupUi( this );

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
            // testRow(0) ;
        }
    }) ;
    //m_TMHeart.start(55000) ;

    ui->checkBox_enable_all->click();

    ui->textEditLog->setMaximumBlockCount(200) ;/**/
}

BatchTestWidget::~BatchTestWidget()
{
    delete ui;
}

void BatchTestWidget::initTestItemTable()
{
    int column_num = 8;
    int row_num = 0;
    ui->tableView_test_items->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    m_pItemModelTestItems = std::make_shared<QStandardItemModel>(row_num, column_num, this);

    m_pItemModelTestItems->setHeaderData(TEST_ITEM_COLUMN_NAME, Qt::Horizontal, "项目");
    m_pItemModelTestItems->setHeaderData(TEST_ITEM_COLUMN_STATUS, Qt::Horizontal, "状态");
    m_pItemModelTestItems->setHeaderData(TEST_ITEM_COLUMN_RETURN_VALUE, Qt::Horizontal, "返回值");
    m_pItemModelTestItems->setHeaderData(TEST_ITEM_COLUMN_PARA_TO_CHECK, Qt::Horizontal, "核对值");
    m_pItemModelTestItems->setHeaderData(TEST_ITEM_COLUMN_RESULT, Qt::Horizontal, "结果");
    m_pItemModelTestItems->setHeaderData(TEST_ITEM_COLUMN_MAN_PASS, Qt::Horizontal, "确认");
    m_pItemModelTestItems->setHeaderData(TEST_ITEM_COLUMN_ENABLE, Qt::Horizontal, "启用");
    m_pItemModelTestItems->setHeaderData(TEST_ITEM_COLUMN_EXEC_TEST, Qt::Horizontal, "执行");

    ui->tableView_test_items->setModel(m_pItemModelTestItems.get());
    ui->tableView_test_items->horizontalHeader()->setStretchLastSection(true);
    updateTestItemTableSize() ;
}

void BatchTestWidget::updateTestItemTableSize()
{
    int w_name = 80;
    int w_status = 50;
    int w_ret_val = 140;
    int w_check_val = 60;
    int w_result = 48;
    int w_pass = 60;
    int w_en = 32;
    int w_exec = 45;

    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_NAME, w_name );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_STATUS, w_status );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_RETURN_VALUE, w_ret_val );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_PARA_TO_CHECK, w_check_val );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_RESULT, w_result );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_MAN_PASS, w_pass );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_ENABLE, w_en );
    ui->tableView_test_items->setColumnWidth( TEST_ITEM_COLUMN_EXEC_TEST, w_exec );
}

void BatchTestWidget::addAllItemDataToTableOf( const std::map<int, QJsonObject> &mapCmdEx, bool bNeedManPass )
{
    int row_index = m_pItemModelTestItems->rowCount();
    for( const auto &pair : mapCmdEx )
    {
        QJsonObject cmdJson = pair.second;

        QString strCmd = cmdJson["item_cmd"].toString().trimmed();
        if( strCmd.isEmpty() )
            continue;

        bool bItemNeedManPass = ( bool )cmdJson["man_pass"].toInt();
        if( bItemNeedManPass != bNeedManPass )
            continue;

        m_pItemModelTestItems->setItem( row_index, TEST_ITEM_COLUMN_NAME, new QStandardItem(cmdJson["item_name"].toString() ) );
        m_pItemModelTestItems->setItem( row_index, TEST_ITEM_COLUMN_STATUS, new QStandardItem("待测试") );
        m_pItemModelTestItems->setItem( row_index, TEST_ITEM_COLUMN_RETURN_VALUE, new QStandardItem("") );

        m_pItemModelTestItems->setItem( row_index, TEST_ITEM_COLUMN_PARA_TO_CHECK, new QStandardItem(cmdJson["para_to_check"].toString()) );
        m_pItemModelTestItems->setItem( row_index, TEST_ITEM_COLUMN_RESULT, new QStandardItem("") );

        QStandardItem* item_pass = new QStandardItem( "----" );
        item_pass->setTextAlignment(Qt::AlignCenter);
        m_pItemModelTestItems->setItem( row_index, TEST_ITEM_COLUMN_MAN_PASS, item_pass );
        if( bItemNeedManPass )
            m_pButtonDelegateManPass->initButtonRow( row_index );
        else
            m_pButtonDelegateManPass->setHideButtonRow( row_index );

        QStandardItem *checkItem = new QStandardItem();
        checkItem->setCheckable( true );
        checkItem->setCheckState( Qt::Checked );
        checkItem->setTextAlignment(Qt::AlignCenter);
        m_pItemModelTestItems->setItem( row_index, TEST_ITEM_COLUMN_ENABLE, checkItem );

        m_pItemModelTestItems->setItem( row_index, TEST_ITEM_COLUMN_EXEC_TEST, new QStandardItem("") );

        ui->tableView_test_items->setRowHeight( row_index, 30 );

        row_index++;
    }
}

void BatchTestWidget::updateTestResult(const QString&strAction,const QString&strData,const QString&strStatus)
{
    m_TMHeart.stop();
    m_TMHeart.start(55000);

    int nCount = m_pItemModelTestItems->rowCount()  ;
    for(int i=0; i<nCount; i++)
    {
        QString strCmd = m_mapCmdEx[i]["item_cmd"].toString().trimmed();
        if(strAction == strCmd)
        {
            m_pItemModelTestItems->setItem( i, TEST_ITEM_COLUMN_RETURN_VALUE,new QStandardItem(strData)) ;
            m_pItemModelTestItems->setItem( i, TEST_ITEM_COLUMN_STATUS, new QStandardItem("完成") );
            QString strCheck = m_pItemModelTestItems->item( i, TEST_ITEM_COLUMN_PARA_TO_CHECK )->text().trimmed();

            bool bOK = (strStatus.toLower() == "ok") ;
            if(!strCheck.isEmpty() && strCheck != strData)
                bOK = false ;
            m_pItemModelTestItems->setItem( i, TEST_ITEM_COLUMN_RESULT, new QStandardItem(bOK?"成功":"失败") );

            break;
        }
    }

    if(strAction == "dev_ble_detection")
    {
        m_strDestBle = strData ;
        m_pBleDlg->scanBlueTooth(strData);
    }
}

void BatchTestWidget::updateTestItemData( const std::map<int, QJsonObject> &mapCmdEx )
{
    while( m_pItemModelTestItems->rowCount() > 0 )
        m_pItemModelTestItems->removeRow(0);
    m_mapCmdEx = mapCmdEx ;

    m_pButtonDelegateManPass = std::make_shared<ButtonDelegateManPass>( "通过", "待确认", this );
    ui->tableView_test_items->setItemDelegateForColumn( TEST_ITEM_COLUMN_MAN_PASS, m_pButtonDelegateManPass.get() );
    connect( m_pButtonDelegateManPass.get(), &ButtonDelegateManPass::buttonClicked, this, &BatchTestWidget::handleItemManPassButtonClicked );

    m_pButtonDelegateTest = std::make_shared<ButtonDelegate>( "测试", this );
    ui->tableView_test_items->setItemDelegateForColumn( TEST_ITEM_COLUMN_EXEC_TEST, m_pButtonDelegateTest.get() );
    connect( m_pButtonDelegateTest.get(), &ButtonDelegate::buttonClicked, this, &BatchTestWidget::handleItemTestButtonClicked );

    addAllItemDataToTableOf( mapCmdEx, false );
    addAllItemDataToTableOf( mapCmdEx, true );

    for( int row = 0; row < m_pItemModelTestItems->rowCount(); ++row )
    {
        QJsonObject o = mapCmdEx.at( row );
        QString btn_text = o["btn_text"].toString().trimmed();
        btn_text = btn_text.isEmpty() ? "测试" : btn_text;
        m_pItemModelTestItems->setData( m_pItemModelTestItems->index( row, TEST_ITEM_COLUMN_EXEC_TEST ), btn_text, Qt::UserRole + 1 );
    }

    QTimer::singleShot(1000,this,[=]
    {
       for(int j=0; j<m_pItemModelTestItems->rowCount(); j++)
       {
           for(int i=0; i<8; i++)
           {
               QStandardItem *item = m_pItemModelTestItems->item(j,i);
               if(i != TEST_ITEM_COLUMN_PARA_TO_CHECK && item)
                   item->setEditable(false) ;
           }
       }
    });
}

void BatchTestWidget::addMqttLog(const QString&strLog)
{
    qDebug().noquote() << strLog;
    ui->textEditLog->appendPlainText(strLog) ;
}

void BatchTestWidget::handleItemManPassButtonClicked( const QModelIndex &index )
{
    int row = index.row();
    if( m_pButtonDelegateManPass != nullptr )
    {
        m_pButtonDelegateManPass->switchStatus( row );
        ui->tableView_test_items->viewport()->update();
    }

    checkFinalTestResult();
}

void BatchTestWidget::checkFinalTestResult()
{
    bool bAllManPassed = true;
    if( m_pButtonDelegateManPass != nullptr )
    {
        bAllManPassed = m_pButtonDelegateManPass->getAllPassed();
    }
    m_bFinalTestOk = m_bAutoBatchTestOk && bAllManPassed;

    ui->labelTestResult->setText( m_bFinalTestOk ? "全部通过" : "未通过" );
}

bool BatchTestWidget::testRow( int row )
{
    if(m_strDevId.isEmpty() || row < 0)
    {
        QMessageBox::warning(this,"提示","设备未绑定！");
        return false;
    }
    if(row >= m_pItemModelTestItems->rowCount())
        return 0 ;

    qDebug() << "testRow" << row;

    QString strAct = m_mapCmdEx[row]["item_cmd"].toString().trimmed() ;
    QString strData = m_mapCmdEx[row]["para_to_write"].toString().trimmed() ;
    m_strAct = strAct;

    QJsonObject jData ;
    if(strData.startsWith('{') && strData.endsWith('}'))
    {
        QJsonDocument jDoc = QJsonDocument::fromJson(strData.toUtf8()) ;
        jData = jDoc.object() ;
    }

    QJsonObject jIQ ;
    jIQ["type"] = "request";
    jIQ["action"] = strAct;
    jIQ["seq_number"] = 1500 + row;
    if(!strData.isEmpty())
        jIQ["data"] = strData ;

    if(strAct == "dev_wifi_throughput")
    {
        jData.empty() ;
        jData["server_ip"] = getMyIP() ;
        jData["server_port"] = 5002 ;
    }

    if(strAct == "ota_upgrade_start")
    {
        if(strData.isEmpty())
        {
            QMessageBox::critical(nullptr,"错误","OTA升级URL为空！");
            return false;
        }

        jData.empty() ;
        jData["url"] = strData ;
    }

    if(!jData.isEmpty())
        jIQ["data"] = jData ;

    m_pItemModelTestItems->item( row, TEST_ITEM_COLUMN_STATUS)->setText("测试中");

    QString strJson = QJsonDocument(jIQ).toJson() ;
    addMqttLog(strJson) ;
    m_strMqtt = strJson ;

    return true;
}

void BatchTestWidget::handleItemTestButtonClicked( const QModelIndex &index )
{
    qDebug() << "handleItemTestButtonClicked: index.row: " << index.row();
    testRow(index.row()) ;
    return ;
}


void BatchTestWidget::on_pbtn_batch_test_start_clicked()
{
    on_pbtn_reset_clicked();
    ui->labelTestResult->setText( "测试中" );

    if(m_strDevId.isEmpty())
    {
        QMessageBox::warning(this,"提示","设备未绑定！");
        return ;
    }

    if(!m_pBThread)
        m_pBThread = new BatTestThread(this) ;

    m_pBThread->start() ;
}

void BatchTestWidget::batTest()
{
    for (int row = 0; row < m_pItemModelTestItems->rowCount(); row++)
    {
        QStandardItem *checkItem = m_pItemModelTestItems->item( row, TEST_ITEM_COLUMN_ENABLE );
        bool checked = ( checkItem->checkState() == Qt::Checked );
        if(!checked) continue ;

        testRow(row) ;

        int nSleep = 700 ;
        if( m_strAct == "led_on"  ||
            m_strAct == "dev_led_detection" ||
            m_strAct == "dev_key_detection" ||
            m_strAct == "infrared_led_switch_on" ||
            m_strAct == "key_detect")
            nSleep = 5000;
        QThread::msleep(nSleep);
    }
}

void BatchTestWidget::on_pbtn_browse_default_values_clicked()
{
    std::vector<QJsonObject> vecData;
    if( !CSysSettings::getInstance()->loadCurDevTypeSetDefaultCmds( vecData ) )
    {
        QMessageBox::warning( this, "错误", "载入默认值配置错误!" );
    }

    getDefaultValuesWidget()->updateItemData( vecData );
    getDefaultValuesWidget()->show();
    getDefaultValuesWidget()->raise();
}

std::shared_ptr<DefaultValuesWidget>& BatchTestWidget::getDefaultValuesWidget()
{
    if( nullptr == m_pDefaultValuesWidget )
    {
        m_pDefaultValuesWidget = std::make_shared<DefaultValuesWidget>();

        connect(m_pDefaultValuesWidget.get(),&DefaultValuesWidget::writeDefaultValue,[=](const QJsonArray&jValue){
            if(m_strDevId.isEmpty())
            {
                QMessageBox::warning(this,"提示","设备未绑定！");
                return ;
            }

            QJsonObject jIQ ;
            jIQ["type"] = "request";
            jIQ["action"] = "set_default_param";
            jIQ["seq_number"] = 520;
            jIQ["data"] = jValue ;
            m_pMqtt->publish(QMqttTopicName(m_strTopic),QByteArray(QJsonDocument(jIQ).toJson()),0,false) ;
        });
    }
    return m_pDefaultValuesWidget;
}


void BatchTestWidget::reset()
{
    m_strDevId.clear();
    m_strTopic.clear();
    on_pbtn_reset_clicked();
    //ui->textEditLog->clear();
}

void BatchTestWidget::on_pbtn_reset_clicked()
{
    for (int row = 0; row < m_pItemModelTestItems->rowCount(); ++row)
    {
        m_pItemModelTestItems->setItem(row, TEST_ITEM_COLUMN_STATUS, new QStandardItem("待测试"));
        m_pItemModelTestItems->setItem(row, TEST_ITEM_COLUMN_RETURN_VALUE, new QStandardItem(""));
        m_pItemModelTestItems->setItem(row, TEST_ITEM_COLUMN_RESULT, new QStandardItem(""));
    }

    if( m_pButtonDelegateManPass != nullptr )
        m_pButtonDelegateManPass->markAllStatus( false );

    ui->labelTestResult->setText( "待测试" );
}

void BatchTestWidget::resetResult()
{
    on_pbtn_reset_clicked() ;
}

void BatchTestWidget::on_checkBox_enable_all_checkStateChanged(const Qt::CheckState &state )
{
    qDebug() << "on_checkBox_enable_all_checkStateChanged..state: " << state;

    bool bEnableAll = (state == Qt::Checked);
    for (int iRow = 0; iRow < m_pItemModelTestItems->rowCount(); iRow++) {
        QStandardItem *checkItem = m_pItemModelTestItems->item(iRow, TEST_ITEM_COLUMN_ENABLE);
        checkItem->setCheckState(bEnableAll ? Qt::Checked : Qt::Unchecked);
    }
}

void BatchTestWidget::on_pbtn_batch_success_clicked()
{
    m_bAutoBatchTestOk = true ;
    m_pButtonDelegateManPass->markAllStatus(true) ;
    checkFinalTestResult();
}

void BatchTestWidget::setBlueToothDlg(DialogBlueTooth *pBleDlg)
{
    m_pBleDlg = pBleDlg ;
    connect(pBleDlg,&DialogBlueTooth::reportBleDevice,this,&BatchTestWidget::onBlueTooth);

    connect(&m_TimeBle,&QTimer::timeout,[=]()
    {
        updateTestResult("dev_ble_detection","","Error") ;
    });
}

void BatchTestWidget::onBlueTooth(const QString &strDeviceMac,const QString &strDeviceName,bool bShow)
{
    QString strID = m_strDestBle.trimmed() ;
    QString strTmp = strDeviceMac.trimmed() ;

    if(strTmp.remove(':').toUpper() == strID.remove(':').toUpper())
    {
        m_TimeBle.stop() ;
        updateTestResult("dev_ble_detection",strDeviceMac,"OK") ;
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
}

