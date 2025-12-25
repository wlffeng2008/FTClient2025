#include "MainWidget.h"
#include "ui_MainWidget.h"
#include "CHttpClientAgent.h"
#include "CLabelEdit.h"
#include <QMessageBox>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <QNetworkInterface>
#include <QDir>
#include <QProcess>
#include <Windows.h>
#include <QTimer>
#include <QFile>
#include <windows.h>

#include <QSettings>

QSettings *g_pSet = nullptr;

bool isExeRunning(const QString &exeName)
{
    QProcess taskListProcess;
    taskListProcess.start("tasklist");
    taskListProcess.waitForFinished();
    QString output = taskListProcess.readAllStandardOutput();

    QStringList lines = output.split('\n');
    for (const QString &line : lines) {
        if (line.contains(exeName, Qt::CaseInsensitive)) {
            return true;
        }
    }
    return false;
}

#include<tlhelp32.h>
void killProcess(QString processName)
{
    if (processName.isEmpty())
        return;

    HANDLE handle32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == handle32Snapshot)
        return;

    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof(PROCESSENTRY32);

    BOOL bProcess = Process32First(handle32Snapshot, &pEntry);

    while (bProcess)
    {
        QString thisProcessName = QString::fromWCharArray(pEntry.szExeFile);
        if (thisProcessName == processName)
        {
            //qInfo() << "找到目标进程";
            HANDLE handLe = OpenProcess(PROCESS_TERMINATE, FALSE, pEntry.th32ProcessID);
            if (handLe == NULL)
            {
                qCritical() << "没有打开目标进程";
                return;
            }
            BOOL ret = TerminateProcess(handLe, 0);

            Q_UNUSED(ret)
            //qInfo() << "关闭目标进程是否成功:" << (ret ? "成功" : "失败");
        }
        bProcess = Process32Next(handle32Snapshot, &pEntry);
    }
    CloseHandle(handle32Snapshot);
}

MainWidget::MainWidget( QWidget *parent ) : QWidget( parent )
    , ui( new Ui::MainWidget )
{
    ui->setupUi( this );

    QString strTitle = QString("产测工具客户端 - V4.7 (Build: %1) - by QT%2").arg(__TIMESTAMP__, QT_VERSION_STR) ;
    setWindowTitle( strTitle );
    qDebug() << strTitle;

    const QList<QPushButton*> btns = findChildren<QPushButton*>();
    for(QPushButton *btn : std::as_const(btns))
    {
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRepeat(false);
    }

    g_pSet = new QSettings(QCoreApplication::applicationDirPath() + "/commonConfig.ini",QSettings::IniFormat) ;
    ui->lineEdit_order_no->setText(g_pSet->value("lastOrder","Z01Y").toString());
    ui->widget_video->setProperty( "PanelStyle", true );

    m_pStsWgt = new DialogSysSetting(this);
    connect( m_pStsWgt, &DialogSysSetting::sigWindowHidden,this, [=]{ updateDevType(); });

    CLabelEdit *pLabel = CLabelEdit::getInstance();
    pLabel->setParent(this);
    pLabel->setWindowFlags(Qt::Popup|Qt::Dialog|pLabel->windowFlags()) ;

    m_pLabel3 = new DialogLabelPrint(this) ;
    m_pLabel2 = new DialogBoxLabelPrint(this) ;

    QTimer*pTimerRepaint  = new QTimer(this);
    connect(pTimerRepaint,&QTimer::timeout,this,[this](){ update() ;repaint() ;}) ;
    pTimerRepaint->start(2000) ;
    QString currentPath = QDir::currentPath();

    if(!isExeRunning("iperf-2.2.n-win64.exe"))
    {
        QString strExe = currentPath + "/run.bat";
        QProcess *process = new QProcess();
        process->start(strExe);
    }

    if(!isExeRunning("MediaServer.exe"))
    {
        QString strExe = currentPath + "/zlmediakit/MediaServer.exe";
        QProcess *process = new QProcess();
        process->start(strExe);
        if(!isExeRunning("MediaServer.exe"))
            QMessageBox::warning(this,"提示","请手动运行本地RTMP服务器程序：./zlmediakit/MediaServer.exe");
    }
    connect(ui->pushButtonSystemCfg,&QPushButton::clicked,this,[=](){ m_pStsWgt->show() ; }) ;

    ui->checkBoxAutoBind->setChecked(g_pSet->value("AutoBind",true).toBool());
    ui->checkBoxAutoLogin->setChecked(g_pSet->value("AutoLogin",true).toBool());

    connect(&m_TMCheckVideo,&QTimer::timeout,this,[this]{
        if(m_lastVideoIn>0 &&  time(nullptr) - m_lastVideoIn>8)
        {
            m_lastVideoIn = 0 ;
            ui->widget_video->stopVideo() ;
            if(m_bCanBindWhenVideoLost && ui->checkBoxAutoBind->isChecked())
            {
                on_pushButtonBind_clicked() ;
                m_Client.disconnectFromHost() ;
            }
        }
    });
    m_TMCheckVideo.start(500) ;

    connect(ui->widget_video,&VideoAndPtzWidget::onVideoMannual,this,[=](){ m_bCanBindWhenVideoLost = false ; }) ;
    connect(ui->widget_video,&VideoAndPtzWidget::onVideoOnline,this,[=](){ m_lastVideoIn = time(nullptr) ; }) ;
    connect(ui->widget_video,&VideoAndPtzWidget::onVideoLog,this,[=](const QString&strLog){ ui->widget_batch_test->addMqttLog(strLog) ; }) ;

    connect(ui->pushButtonReadSN,&QPushButton::clicked,this,[=](){
        ui->lineEditEchoSN->setText("");
        QTimer::singleShot(300,[=]{
            QJsonObject jIQ;
            jIQ["type"] = "request";
            jIQ["action"] = "read_sn";
            jIQ["seq_number"] = 601;
            m_Client.publish(QMqttTopicName(m_strSendTopic),QByteArray(QJsonDocument(jIQ).toJson()),0,false) ;
        });
    }) ;

    connect(&m_TMAutoBind,&QTimer::timeout,this,[=](){
        m_TMAutoBind.stop() ;
        doDeviceBinding() ;
    });

    // MQTT
    {
        connect(&m_Client,&QMqttClient::connected,this,[=](){
            qDebug()<<"Mqtt Login OK!" ;
            ui->labelMqttFlag->setText("已登录");
            QString strWorkPosId  = m_pStsWgt->getCurWorkPosId().c_str() ;
            QString strDeviceType = m_pStsWgt->getCurDeviceType().c_str() ;
            QString strRootRTopic = QString("/%1/%2/device").arg(strDeviceType.toLower(), strWorkPosId);
            QString strRootSTopic = QString("/%1/%2/tool").arg(strDeviceType.toLower(), strWorkPosId) ;

            m_Client.subscribe(QMqttTopicFilter(strRootRTopic),0) ;
            ui->widget_batch_test->addMqttLog("接收通道:" + strRootRTopic) ;
            ui->widget_batch_test->addMqttLog("发送通道:" + strRootSTopic) ;
            m_strRootTopic = strRootSTopic ;
            on_pbtn_inquire_devices_clicked() ;
        }) ;

        connect(&m_Client,&QMqttClient::disconnected,this,[=](){
            ui->labelMqttFlag->setText("未登录");
            ui->labelBindFlag->setText("未绑定");

            QTimer::singleShot(500,this,[=](){ connectMqtt() ;});
        }) ;

        connect(&m_Client,&QMqttClient::messageReceived,this,[=](const QByteArray &message){

            ui->widget_batch_test->addMqttLog(QString(message.data()).trimmed()) ;
            QJsonParseError parseError;
            QJsonDocument jDoc = QJsonDocument::fromJson(message.data(),&parseError) ;
            if(jDoc.isNull())
                return ;
            QJsonObject jRoot = jDoc.object();
            if(jRoot.isEmpty())
                return ;

            QString strType   = jRoot["type"].toString() ;
            QString strAction = jRoot["action"].toString() ;
            QString strStatus = jRoot["status"].toString().toLower() ;

            if( strAction == "get_device_id"  ||
                strAction == "get_device_mac" ||
                strAction == "device_search"  ||
                strAction == "device_online"  )
            {
                QJsonObject jData = jRoot["data"].toObject();
                QString strData = jData["device_mac"].toString();

                bool bExist = false ;
                int nCount = ui->comboBoxDeviceList->count();
                for (int i=0; i< nCount; i++)
                {
                    if(ui->comboBoxDeviceList->itemText(i) == strData)
                    {
                        bExist = true ;
                        break;
                    }
                }
                if(!bExist)
                {
                    ui->comboBoxDeviceList->addItem(strData) ;
                    m_TMAutoBind.stop() ;
                    if(ui->checkBoxAutoBind->isChecked() && m_strBindId.isEmpty())
                        m_TMAutoBind.start(500);
                }
            }

            if(strAction == "video_start" && !m_pStsWgt->isRtspEnable())
            {
                int rtmp_port = 1953;
                std::string rtmp_server_ip;
                CSysSettings::getInstance()->getRtmpPara( rtmp_server_ip, rtmp_port ) ;
                QString strUrl = QString("rtmp://%1:%2/live/%3").arg( rtmp_server_ip.data() ).arg( rtmp_port ).arg(m_strBindId);
                qDebug() << "rtmp url : "<< strUrl;

                m_bCanBindWhenVideoLost = true ;
                ui->widget_video->startVideo(strUrl);
            }

            if(strAction == "get_rtsp_url")
            {
                QJsonObject jData = jRoot["data"].toObject();
                QString strUrl = jData["main_stream_url"].toString();

                m_bCanBindWhenVideoLost = true ;
                ui->widget_video->startVideo(strUrl);
            }

            if(strAction == "write_sn")
            {
                QJsonObject jIQ;
                jIQ["type"] = "request";
                jIQ["action"] = "read_sn";
                jIQ["seq_number"] = 600;
                m_Client.publish(QMqttTopicName(m_strSendTopic),QByteArray(QJsonDocument(jIQ).toJson()),0,false) ;

                if(strStatus != "ok")
                {
                    QMessageBox::warning( this, "错误", "设备写号失败！" );
                    return ;
                }

                {
                    QString orderNo = ui->lineEdit_order_no->text().trimmed();
                    QString strDevSN = ui->lineEditDeviceSN->text().trimmed();

                    QString strName =  m_jBak["device_name"].toString();
                    QString strKey  =  m_jBak["product_key"].toString();
                    QString strSecret1 =  m_jBak["product_secret"].toString();
                    QString strSecret2 =  m_jBak["device_secret"].toString();

                    QJsonObject jObj;
                    QString orderType = ui->lineEdit_device_type->text().trimmed();
                    bool bOK = m_pHttpAli->reportDeviceSN(orderNo, orderType,m_strBindId,strName,strKey,strSecret1,strSecret2,jObj) ;

                    QString  strRes = QJsonDocument(jObj).toJson() ;
                    ui->widget_batch_test->addMqttLog(strRes) ;

                    if(!bOK)
                    {
                        QMessageBox::warning( this, "错误", "上报设备信息失败！\n" + strRes );
                    }
                }
            }

            if(strAction == "read_sn")
            {
                QJsonObject jData = jRoot["data"].toObject();
                ui->lineEditEchoSN->setText(jData["uuid"].toString());
                ui->lineEditDeviceSN->setText(jData["uuid"].toString());
                ui->lineEditDeviceKey->setText(jData["key"].toString());
                if(CLabelEdit::getInstance()->isVisible())
                    on_pbtn_print_dev_label_clicked() ;
            }

             QString strData ;
             if(jRoot.contains("data") && !jRoot["data"].isObject())
                 strData = jRoot["data"].toString() ;

             if(strAction == "wifi_throughput")
             {
                 if(jRoot["data"].isObject())
                 {
                     QJsonObject jData = jRoot["data"].toObject();
                     strData = jData["bandwidth"].toString() ;
                 }
             }

            ui->widget_batch_test->updateTestResult(strAction.trimmed(),strData.trimmed(),strStatus.trimmed()) ;
        }) ;

        connect(ui->widget_video,&VideoAndPtzWidget::onPZTCtrol,this,[=](int nDire){
            if(m_strBindId.isEmpty())
            {
                QMessageBox::warning(nullptr,"提示","设备未绑定！");
                return ;
            }
            static QStringList PZTDires={"up","down","left","right","stop","reset"};
            QJsonObject jVideo ;
            jVideo["type"]="request" ;
            jVideo["action"] = "ptz_start_move_" + PZTDires[nDire];
            jVideo["seq_number"] = 308 + nDire;
            m_Client.publish(QMqttTopicName(m_strSendTopic),QByteArray(QJsonDocument(jVideo).toJson()),0,false) ;
        });

        connectMqtt();
    }

    connect(ui->pushButtonMqtt,&QPushButton::clicked,this,[=](){ connectMqtt(); }) ;

    connect(m_pLabel3,&DialogLabelPrint::reportPachInfo,this,[=](const QString&strBoxNo,const QStringList&strDeviceNos)
    {
        QJsonObject jObj ;
        bool  bOK = m_pHttpAli->reportPackInfo(strBoxNo,strDeviceNos,jObj) ;
        if(!bOK)
        {
            QJsonDocument doc(jObj);
            ui->widget_batch_test->addMqttLog(doc.toJson(QJsonDocument::JsonFormat::Compact)) ;
            CLabelEdit::getInstance()->hide();
            QMessageBox::warning( this, "错误", "上报包装箱信息失败！" );
            return ;
        }
    }) ;

    ui->lineEditDeviceSN->setPlaceholderText("") ;
    QList<QPushButton*> allButtons = findChildren<QPushButton*>();
    foreach (QPushButton *button, allButtons) {
        button->setCursor(Qt::PointingHandCursor);
        button->setFocusPolicy(Qt::ClickFocus);
    }

    initHttpPara();
    updateDevType();
}

void MainWidget::doDeviceBinding()
{
    //if(!m_strRecvTopic.isEmpty())
    //    m_Client.unsubscribe(QMqttTopicFilter(m_strRecvTopic));

    QString strWorkPosId  = m_pStsWgt->getCurWorkPosId().c_str() ;
    QString strDeviceType = m_pStsWgt->getCurDeviceType().c_str() ;
    QString strDeviceId = ui->comboBoxDeviceList->currentText();

    if(strDeviceId.isEmpty())
        return;

    m_strRecvTopic = QString("/%1/%2/%3/device").arg(strDeviceType.toLower(),strWorkPosId,strDeviceId);
    m_strSendTopic = QString("/%1/%2/%3/tool").arg(strDeviceType.toLower(),strWorkPosId,strDeviceId) ;
    QMqttSubscription *pSubsri = m_Client.subscribe(QMqttTopicFilter(m_strRecvTopic),0) ;
    qDebug() << pSubsri->reason();

    ui->widget_batch_test->addMqttLog("设备接收通道:" + m_strRecvTopic) ;
    ui->widget_batch_test->addMqttLog("设备发送通道:" + m_strSendTopic) ;
    ui->widget_batch_test->setMQTTAgent(&m_Client,m_strSendTopic);
    ui->widget_batch_test->resetResult() ;
    ui->widget_video->stopVideo() ;
    m_strBindId = strDeviceId ;

    ui->widget_batch_test->setDevId(strDeviceId) ;

    ui->labelBindFlag->setText("已绑定");

    if(ui->checkBoxAutoLogin->isChecked())
        on_pbtn_login_order_clicked();

    if(ui->checkBoxAutoTest->isChecked())
        ui->widget_batch_test->autoTestAll();


    if(!m_pStsWgt->isRtspEnable())
    {
        QJsonObject jVideo ;
        QJsonObject jParam ;
        jVideo["type"] = "request" ;
        jVideo["action"] = "video_start";
        jVideo["seq_number"] = 108;
        jParam["type"] = "rtmp";

        std::string strIP;
        int nPort = 1953 ;
        CSysSettings::getInstance()->getRtmpPara(strIP,nPort );
        jParam["url"] = QString("rtmp://%1:%2/live/%3").arg(strIP.c_str()).arg(nPort).arg(strDeviceId);
        jVideo["data"] = jParam;
        m_Client.publish(QMqttTopicName(m_strSendTopic),QByteArray(QJsonDocument(jVideo).toJson()),0,false) ;
    }
    else
    {
        QJsonObject jVideo ;
        jVideo["type"] = "request" ;
        jVideo["seq_number"] = 119;
        jVideo["action"] = "get_rtsp_url";
        m_Client.publish(QMqttTopicName(m_strSendTopic),QByteArray(QJsonDocument(jVideo).toJson()),0,false) ;
    }
}

void MainWidget::connectMqtt()
{
    if(m_Client.state() == QMqttClient::Connected)
    {
        m_Client.disconnectFromHost();
    }
    else
    {
        std::string strIP = "120.79.120.31";
        int nPort = 1883 ;
        std::string strUser ="hy";
        std::string strPassword="hy302302" ;

        CSysSettings::getInstance()->getMqttPara(strIP,nPort,strUser,strPassword) ;

        m_Client.setHostname(strIP.c_str()) ;
        m_Client.setPort(nPort) ;
        m_Client.setUsername(strUser.c_str()) ;
        m_Client.setPassword(strPassword.c_str()) ;
        m_Client.connectToHost() ;
    }
}

MainWidget::~MainWidget()
{
    killProcess("iperf-2.2.n-win64.exe") ;
    ui->widget_video->stopVideo() ;
    delete ui;
}

void MainWidget::initTestCmdListData()
{
    QMap<int,QJsonObject> mapCmdEx;
    std::map<int,QJsonObject> mapTemp;
    CSysSettings::getInstance()->loadCurDevTypeTestCmds( mapTemp );
    int i = 0 ;
    for(const auto &pair:mapTemp)
        mapCmdEx[i++] = pair.second ;
    ui->widget_batch_test->updateTestItemData( mapCmdEx );
}

void MainWidget::initHttpPara()
{
    std::string sHttpIp;
    int iHttpPort;
    CSysSettings::getInstance()->getHttpPara( sHttpIp, iHttpPort );
    if(!m_pHttpAli)
        m_pHttpAli = new CHttpClientAgent() ;
    m_pHttpAli->setPara( sHttpIp, iHttpPort );
}

void MainWidget::on_pbtn_inquire_devices_clicked()
{
    qDebug() << "on_pbtn_inquire_devices_clicked..";

    ui->widget_video->stopVideo() ;
    ui->widget_batch_test->setDevId("") ;

    m_strBindId.clear();
    ui->comboBoxDeviceList->clear() ;
    ui->labelBindFlag->setText("未绑定");

    QJsonObject jIQ ;
    jIQ["type"] = "request";
    jIQ["action"] = "device_search";
    jIQ["seq_number"] = 1000;
    m_Client.publish(QMqttTopicName(m_strRootTopic),QByteArray(QJsonDocument(jIQ).toJson()),0,false) ;
}

void MainWidget::on_pbtn_login_order_clicked()
{
    if(!m_bCanLogin)
        return ;
    m_bCanLogin = false ;
    QTimer::singleShot(1500,this,[=]{ m_bCanLogin = true; }) ;
    QString orderNo = ui->lineEdit_order_no->text().trimmed();
    QString orderType = ui->lineEdit_device_type->text().trimmed();
    QString strDevSN = ui->lineEditDeviceSN->text().trimmed();

    g_pSet->setValue("lastOrder",orderNo);
    g_pSet->setValue("AutoBind",ui->checkBoxAutoBind->isChecked());
    g_pSet->setValue("AutoLogin",ui->checkBoxAutoLogin->isChecked());

    // m_strBindId = "11:22:33:44:55:66" ;
    // orderNo = "ss" ;
    // orderType = "ss" ;

    if(m_strBindId.isEmpty())
    {
        QMessageBox::warning( this, "错误", "未绑定设备！" );
        return;
    }
    //ui->label_dev_sn->setText("") ;
    ui->lineEditDeviceKey->setText("") ;

    QJsonObject jObj ;
    bool bOK = m_pHttpAli->getDeviceSN(orderNo, orderType,m_strBindId,strDevSN,jObj) ;
    if(!bOK)
    {
        QJsonDocument doc(jObj);
        ui->widget_batch_test->addMqttLog(doc.toJson(QJsonDocument::JsonFormat::Compact)) ;
        QMessageBox::warning( this, "获取设备信息失败！", jObj["msg"].toString());
        return ;
    }

    m_jBak = jObj ;

    QJsonObject jData = jObj["data"].toObject();

    m_jData = jData;

    QString strUuid = jData["uuid"].toString();
    QString strKey = jData["key"].toString();

    ui->lineEditDeviceKey->setText(strKey) ;
    ui->lineEditDeviceSN->setText(strUuid) ;

    {
        QString strTopic = m_strSendTopic ;

        QJsonObject jIQ ;
        jIQ["type"] = "request";
        jIQ["action"] = "write_sn";
        jIQ["seq_number"] = 500;

        QJsonObject jWD = jData ;
        jWD["device_mac"] = m_strBindId ;
        jWD["uuid"] = strUuid ;
        jWD["key"] = strKey ;

        jIQ["data"] = jWD;

        m_Client.publish(QMqttTopicName(strTopic),QByteArray(QJsonDocument(jIQ).toJson()),0,false) ;
    }
}


void MainWidget::on_comboBox_device_type_currentTextChanged( const QString &text )
{
    ui->lineEdit_device_type->setText(text) ;
    if(m_bLoading)
        return ;
    if( !text.trimmed().isEmpty() )
    {
        CSysSettings::getInstance()->setCurDeviceType( text );
        initTestCmdListData();
    }
}

void MainWidget::updateDevType()
{
    m_bLoading = true ;
    QString strDev = m_pStsWgt->getCurDeviceType().data() ;
    qDebug() << "updateDevType:" << strDev ;
    ui->comboBox_device_type->clear();
    std::vector<std::string> devTypes = m_pStsWgt->getAllDeviceTyps();
    for( auto type : devTypes )
    {
        ui->comboBox_device_type->addItem( type.data() );
    }

    ui->comboBox_device_type->setCurrentText( strDev );
    m_bLoading = false ;
    initTestCmdListData();
}


void MainWidget::on_pbtn_print_dev_label_clicked()
{
    qDebug() << "on_pbtn_print_dev_label_clicked ...";
    CLabelEdit::getInstance()->setMode( 0 );
    QString curDevLabTem = CSysSettings::getInstance()->getDevLabelTemplateFile();
    CLabelEdit::getInstance()->loadTemplate( curDevLabTem );
    CLabelEdit::getInstance()->show();

    QString strName = ui->lineEditDeviceSN->text() ;
    QString strKey  = ui->lineEditDeviceKey->text() ;

    CLabelEdit::getInstance()->setLabelInfo( strName, strKey ) ;
}

void MainWidget::on_pbtn_print_box_label_clicked()
{
    m_pLabel2->show() ;
}

void MainWidget::on_pbtn_print_pack_label_clicked()
{
    m_pLabel3->show() ;
}



void MainWidget::on_pushButtonClear_clicked()
{
    ui->lineEditDeviceSN->clear() ;
    ui->lineEditDeviceKey->clear() ;
    ui->lineEditEchoSN->clear();
    ui->lineEditDeviceSN->setFocus();
}

void MainWidget::on_comboBoxDeviceList_activated(int index)
{
    Q_UNUSED(index)
    if(ui->checkBoxAutoBind->isChecked())
    {
        int nTimeout = 200 ;
        if(!m_strBindId.isEmpty())
        {
            on_pushButtonBind_clicked();
            nTimeout = 3000 ;
        }
        m_bCanBindWhenVideoLost = false ;
        m_TMAutoBind.stop() ;
        m_TMAutoBind.start(nTimeout) ;
    }
}

void MainWidget::on_pushButtonBind_clicked()
{
    if(!m_strBindId.isEmpty())
    {
        m_bCanBindWhenVideoLost = false ;
        ui->widget_video->stopVideo() ;
        on_pushButtonClear_clicked() ;
        ui->labelBindFlag->setText("未绑定");
        m_strBindId.clear() ;
        ui->widget_batch_test->resetResult() ;
    }
    else
    {
        doDeviceBinding();
    }
}

void MainWidget::on_pushButtonGetOrderCount_clicked()
{
    QJsonObject jRet ;
    int nTry  = 5 ;
    while(true)
    {
        QString strOrderId = ui->lineEdit_order_no->text().trimmed();
        int nCount = m_pHttpAli->queryNumber(strOrderId,jRet) ;
        ui->labelOrderCount->setNum(nCount) ;
        if(nCount>=0)
            break;
        ui->widget_batch_test->addMqttLog(m_pHttpAli->m_strReturn);

        if(nTry-- == 0)
            break;
        QThread::usleep(200) ;
    }
}

