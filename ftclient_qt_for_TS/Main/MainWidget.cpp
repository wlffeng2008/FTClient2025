
#include <Windows.h>
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
#include <QTimer>

bool isExeRunning(const QString &exeName)
{
    QProcess taskListProcess;
    taskListProcess.start("tasklist");
    taskListProcess.waitForFinished();
    QString output = taskListProcess.readAllStandardOutput();

    QStringList lines = output.split('\n');
    for (const QString &line : std::as_const(lines)) {
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
    {
        return;
    }

    HANDLE handle32Snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == handle32Snapshot)
        return;

    PROCESSENTRY32 pEntry;
    pEntry.dwSize = sizeof(PROCESSENTRY32);

    BOOL bProcess = Process32First(handle32Snapshot, &pEntry);

    while (bProcess)
    {
        QString thisProcessName = QString::fromWCharArray(pEntry.szExeFile);
        if (thisProcessName == processName) {
            //qInfo() << "找到目标进程";
            HANDLE handLe = OpenProcess(PROCESS_TERMINATE, FALSE, pEntry.th32ProcessID);
            if (handLe == NULL) {
                qCritical() << "没有打开目标进程";
                return;
            }
            BOOL ret = TerminateProcess(handLe, 0);
            Q_UNUSED(ret);
            //qInfo() << "关闭目标进程是否成功:" << (ret ? "成功" : "失败");
        }
        bProcess = Process32Next(handle32Snapshot, &pEntry);
    }
    CloseHandle(handle32Snapshot);
}

MainWidget::MainWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::MainWidget )
{
    ui->setupUi( this );

    // 设置 style 属性，用于应用定制风格
    //ui->widget_video->setProperty( "PanelStyle", true );

    m_pLogin = new DialogUserLogin(this);
    m_pStsWgt = new SysSettingsWidget(this);

    m_pLogin->m_pSet = m_pStsWgt ;
    if(m_pLogin->exec() != QDialog::Accepted)  { exit(0) ; };

    QString strTitle = QString("TS产测工具客户端 - V3.8 (Build: %1 by QT%2)  [ 当前登录用户：%3(%4) ]  每天至少重启一次本软件，重新登录服务器！！！")
                           .arg(__TIMESTAMP__,QT_VERSION_STR,m_pLogin->m_strUser,m_pLogin->m_strName) ;
    setWindowTitle( strTitle );
    qDebug() << strTitle ;

    initHttpPara();

    m_pLabel3 = new DialogLabelPrint(this) ;
    m_pLabel2 = new DialogBoxLabelPrint(this) ;

    m_pBTDlg = new DialogBlueTooth(this) ;
    ui->widget_batch_test->setBlueToothDlg(m_pBTDlg) ;

    m_pCOMDlg = new SerialTestDialog(this) ;

    m_pStsWgt->setParent(this);
    m_pStsWgt->setWindowFlags(Qt::Popup|Qt::Dialog|m_pStsWgt->windowFlags()) ;
    connect( m_pStsWgt, &SysSettingsWidget::sigWindowHidden, [=]{ updateDevType(); });

    CLabelEdit *pLabel = CLabelEdit::getInstance();
    pLabel->setParent(this);
    pLabel->setWindowFlags(Qt::Popup|Qt::Dialog|pLabel->windowFlags()) ;

    ui->pushButtonWriteSN->setVisible(false) ;
    ui->checkBoxForceWrite->setVisible(false) ;
    ui->label_5->setVisible(false)  ;
    ui->lineEditDevicePid->setVisible(false)  ;
    ui->horizontalSpacer_3->changeSize(0,0)  ;
    // ui->checkBoxAutoBind->setChecked(true);

    QString currentPath = QDir::currentPath();

    if(!isExeRunning("iperf-2.2.n-win64.exe"))
    {
        QString strExe = currentPath + "/run.bat";
        QProcess *process = new QProcess();
        process->start(strExe);
    }

    if(!isExeRunning("nginx.exe"))
    {
        //QString strExe = currentPath + "/nginx-rtmp-win32/start.bat";
        //QProcess *process = new QProcess();
        //process->start(strExe);
    }

    if(!isExeRunning("MediaServer.exe"))
    {
        QString strExe = currentPath + "/zlmediakit/MediaServer.exe";
        QProcess *process = new QProcess();
        process->start(strExe);

        //if(!isExeRunning("MediaServer.exe"))
        //    QMessageBox::warning(this,"提示","请手动运行本地RTMP服务器程序：./zlmediakit/MediaServer.exe");
    }
    connect(ui->pushButtonSystemCfg,&QPushButton::clicked,[=](){ m_pStsWgt->show() ; }) ;


    connect(&m_TMCheckVideo,&QTimer::timeout,this,[this]{
        m_TMCheckVideo.stop();
        ui->widget_video->stopVideo() ;
        if(m_bCanBindWhenVideoLost && ui->checkBoxAutoBind->isChecked())
        {
            on_pushButtonBind_clicked() ;
            m_Client.disconnectFromHost() ;
        }
    });

    connect(ui->widget_video,&VideoAndPtzWidget::onVideoOnline,this,[=](){
        m_TMCheckVideo.stop();
        m_TMCheckVideo.start(20000) ;
    }) ;

    connect(ui->widget_video,&VideoAndPtzWidget::onVideoMannual,this,[=](){
        m_bCanBindWhenVideoLost = false ;
    }) ;

    connect(ui->widget_video,&VideoAndPtzWidget::onVideoLog,this,[=](const QString&strLog){
        ui->widget_batch_test->addMqttLog(strLog) ;
    }) ;

    // MQTT
    {
        connect(&m_Client,&QMqttClient::connected,[=](){
            qDebug()<<"Mqtt Login OK!" ;
            m_bWriteSN_OK = false ;

            QTimer::singleShot(300,this,[=](){
                ui->widget_video->stopVideo();
                ui->labelMqttFlag->setText("已登录");
                QString strWorkPosId  = m_pStsWgt->getCurWorkPosId().c_str() ;
                QString strDeviceType = ui->comboBox_device_type->currentText();//getSysStsWgt()->getCurDeviceType().c_str() ;

                QString strRootRTopic = QString("/%1/%2/device").arg(strDeviceType).arg(strWorkPosId);//.toLower()
                QString strRootSTopic = QString("/%1/%2/tool").arg(strDeviceType).arg(strWorkPosId) ;//.toLower()
                m_Client.subscribe(QMqttTopicFilter(strRootRTopic),0) ;
                ui->widget_batch_test->addMqttLog("接收通道:" + strRootRTopic) ;
                ui->widget_batch_test->addMqttLog("发送通道:" + strRootSTopic) ;
                m_strRootTopic = strRootSTopic ;
                on_pbtn_inquire_devices_clicked() ;
            });
        }) ;

        connect(&m_Client,&QMqttClient::disconnected,[=](){
            ui->labelMqttFlag->setText("未登录");
            ui->labelBindFlag->setText("未绑定");
            m_bWriteSN_OK = false ;
            ui->widget_video->stopVideo();
            QTimer::singleShot(2000,[=](){ connectMqtt() ;});

            m_strBindId.clear() ;
            ui->widget_batch_test->m_strDevId.clear() ;
        }) ;

        connect(&m_Client,&QMqttClient::messageReceived,[=](const QByteArray &message){

            ui->widget_batch_test->addMqttLog(QString(message.data()).trimmed()) ;
            qDebug().noquote() <<"messageReceived: "  << QString(message.data()).trimmed() ;

            QJsonParseError parseError;
            QJsonDocument jDoc = QJsonDocument::fromJson( message.data(),&parseError) ;
            if(parseError.error != QJsonParseError::NoError || jDoc.isNull() || !jDoc.isObject())
                return ;
            QJsonObject jRoot = jDoc.object();
            if(jRoot.isEmpty())
                return ;

            QString strType   = jRoot["type"].toString().trimmed() ;
            QString strAction = jRoot["action"].toString().trimmed() ;
            QString strStatus = jRoot["status"].toString().trimmed().toLower() ;
            BOOL bSuccess     = (strStatus == "ok") ;
            if(strAction.isEmpty())
                return;

            QString strData ;

            if( strAction == "get_device_id"  ||
                strAction == "get_device_mac" ||
                strAction == "device_search"  ||
                strAction == "device_online")
            {
                QJsonObject jD = jRoot["data"].toObject();
                strData = jD["device_mac"].toString() ;
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

            if(strAction == "video_start")
            {
                int rtmp_port = 1953;
                std::string rtmp_server_ip;
                CSysSettings::getInstance()->getRtmpPara( rtmp_server_ip, rtmp_port ) ;
                QString strUrl = QString("rtmp://%1:%2/live/%3").arg( rtmp_server_ip.data() ).arg( rtmp_port ).arg(m_strBindId);
                qDebug() << "rtmp url : "<< strUrl;

                m_bCanBindWhenVideoLost = true ;
                ui->widget_video->startVideo(strUrl);
            }

            if(strAction == "dev_write_sn")
            {
                QString orderNo = ui->lineEdit_order_no->text().trimmed();
                QString orderType = ui->lineEdit_device_type->text().trimmed();
                QString strProduct = ui->lineEditProduct->text().trimmed();
                QString strDevUuid = ui->lineEditDeviceUuid->text().trimmed();
                QString strAuth = ui->lineEditDeviceKey->text().trimmed();

                ui->lineEditDeviceUuid->clear() ;
                ui->lineEditDeviceKey->clear() ;
                ui->lineEditProduct->clear() ;

                if(!bSuccess)
                {
                    QMessageBox::critical( this, "错误", "设备写号失败！" );
                    return ;
                }

                on_pushButtonReadSN_clicked() ;
                QThread::msleep(100) ;

                QJsonObject jRet ;
                CHttpClientAgent::getInstance()->reportDeviceSN(orderNo, orderType,m_strBindId,strDevUuid,strProduct,strAuth,jRet) ;
                if(jRet.isEmpty() || jRet["code"].toInt() != 200)
                {
                    QMessageBox::critical( this, "错误", "上报设备信息失败！\n" + jRet["msg"].toString());
                    return ;
                }
                m_bWriteSN_OK = true ;
            }

            if(strAction == "dev_read_sn")
            {
                QJsonObject jD = jRoot["data"].toObject();
                strData = jD["device_mac"].toString() ;

                if(strData == m_strBindId)
                {
                    strData = jD["uuid"].toString() ;
                    ui->lineEditEchoUuid->setText(strData);
                    QString strPID = jD["pid"].toString() ;
                    ui->lineEditDevicePid->setText(strPID) ;
                }
                else
                {
                    QMessageBox::warning(nullptr,"提示","MAC ERROR！");
                    return ;
                }
            }

            if(jRoot.contains("data"))
            {
                if(jRoot["data"].isString())
                {
                    strData = jRoot["data"].toString() ;
                }
                else if(jRoot["data"].isObject())
                {
                    QJsonObject jD = jRoot["data"].toObject();

                    if(strAction == "dev_get_mac")
                        strData = jD["device_mac"].toString() ;

                    if(strAction == "dev_get_version")
                        strData = jD["version"].toString() ;

                    if(strAction == "dev_get_battery")
                        strData = jD["battery"].toString() ;

                    if(strAction == "dev_tf_detection")
                        strData = jD["memory"].toString() ;

                    if(strAction == "module4g_signal")
                        strData = jD["signal"].toString() ;

                    if(strAction == "dev_wifi_throughput")
                        strData = jD["value"].toString() ;

                    if(strAction == "dev_ble_detection")
                        strData = jD["ble_mac"].toString() ;

                    if(strAction == "dev_get_sn")
                    {
                        strData = jD["device_sn"].toString() ;
                        m_strDeviceSN = strData ;
                        ui->lineEditDeviceSN->setText(strData);
                    }
                }
            }

            ui->widget_batch_test->updateTestResult(strAction,strData,strStatus) ;
        }) ;

        connect(ui->widget_video,&VideoAndPtzWidget::onPZTCtrol,[=](int nDire){
            if(m_strBindId.isEmpty())
            {
                QMessageBox::warning(this,"提示","设备未绑定！");
                return ;
            }
            static QStringList PZTDires = {"up","down","left","right","stop","reset"};
            QJsonObject jVideo ;
            jVideo["type"]="request" ;
            jVideo["action"] = "ptz_start_move_" + PZTDires[nDire];
            jVideo["seq_number"] = 308 + nDire;
            m_Client.publish(QMqttTopicName(m_strSendTopic),QByteArray(QJsonDocument(jVideo).toJson()),0,false) ;
        });

        connectMqtt();
    }

    connect(ui->pushButtonMqtt,&QPushButton::clicked,[=]{ connectMqtt(); }) ;

    connect(m_pLabel3,&DialogLabelPrint::reportPachInfo,[=](const QString&strBoxNo,const QStringList&strDeviceNos)
    {
        QJsonObject jRet ;
        CHttpClientAgent::getInstance()->reportPackInfo(strBoxNo,strDeviceNos,jRet) ;
        if(jRet.isEmpty() || jRet["code"].toInt() != 200)
        {
            QJsonDocument doc(jRet);
            ui->widget_batch_test->addMqttLog(doc.toJson(QJsonDocument::JsonFormat::Compact)) ;
            CLabelEdit::getInstance()->hide();
            QMessageBox::warning( this, "错误", "上报包装箱信息失败！\n" + jRet["msg"].toString() );
            return ;
        }
    }) ;

    connect(&m_TTrimText,&QTimer::timeout,this,[this](){
        m_TTrimText.stop() ;
        QString strText = ui->lineEditDeviceUuid->text().trimmed() ;
        if(strText.startsWith("schema://"))
        {
            int nFind = strText.indexOf("&u=",10)  ;
            if(nFind>=0)
            {
                int nLen = strText.length() ;
                strText = strText.right(nLen-nFind-3) ;
                ui->lineEditDeviceUuid->setText(strText) ;
            }
        }
    }) ;

    on_pushButton_clicked();
    updateDevType();
}

void MainWidget::doDeviceBinding()
{
    //if(!m_strRecvTopic.isEmpty())
    //    m_Client.unsubscribe(QMqttTopicFilter(m_strRecvTopic));

    QString strWorkPosId  = m_pStsWgt->getCurWorkPosId().c_str() ;
    QString strDeviceType = ui->comboBox_device_type->currentText();//getSysStsWgt()->getCurDeviceType().c_str() ;
    QString strDeviceId = ui->comboBoxDeviceList->currentText();
    m_strBindId.clear() ;
    ui->widget_batch_test->m_strDevId.clear() ;    
    ui->lineEditDevicePid->clear() ;

    if(strDeviceId.isEmpty())
        return;
    on_pushButtonClear_clicked() ;

    ui->labelBindFlag->setText("已绑定");
    m_strRecvTopic = QString("/%1/%2/%3/device").arg(strDeviceType).arg(strWorkPosId).arg(strDeviceId);//.toLower()
    m_strSendTopic = QString("/%1/%2/%3/tool").arg(strDeviceType).arg(strWorkPosId).arg(strDeviceId) ;//.toLower()
    m_Client.subscribe(QMqttTopicFilter(m_strRecvTopic),0) ;
    ui->widget_batch_test->addMqttLog("设备接收通道:" + m_strRecvTopic) ;
    ui->widget_batch_test->addMqttLog("设备发送通道:" + m_strSendTopic) ;
    ui->widget_batch_test->setMQTTAgent(&m_Client,m_strSendTopic);
    ui->widget_batch_test->resetResult() ;
    ui->widget_video->stopVideo() ;
    m_strBindId = strDeviceId ;
    ui->widget_batch_test->m_strDevId = strDeviceId ;
    m_bWriteSN_OK = false ;

    QJsonObject jVideo ;
    jVideo["type"] = "request" ;
    jVideo["action"] = "video_start";
    jVideo["seq_number"] = 10086;

    std::string strIP;
    int nPort = 1953 ;
    CSysSettings::getInstance()->getRtmpPara(strIP,nPort );

    QJsonObject jParam ;
    jParam["type"] = "rtmp";
    jParam["url"] = QString("rtmp://%1:%2/live/%3").arg(strIP.c_str()).arg(nPort).arg(strDeviceId);

    jVideo["data"] = jParam;

    m_Client.publish(QMqttTopicName(m_strSendTopic),QByteArray(QJsonDocument(jVideo).toJson()),0,false) ;
    ui->widget_batch_test->addMqttLog(QJsonDocument(jVideo).toJson());
}

void MainWidget::connectMqtt()
{
    m_strBindId.clear() ;
    ui->lineEditDevicePid->clear() ;
    ui->labelMqttFlag->setText("未连接");
    ui->labelBindFlag->setText("未绑定");

    if(m_Client.state() == QMqttClient::Connected)
    {
        m_Client.disconnectFromHost();
    }
    else
    {
        int nPort = 1883 ;
        std::string strIP = "47.106.117.102";
        std::string strUser ="hy";
        std::string strPassword="hy302302" ;

        CSysSettings::getInstance()->getMqttPara(strIP,nPort,strUser,strPassword) ;

        m_Client.setHostname(strIP.c_str()) ;
        m_Client.setPort(nPort) ;
        m_Client.setUsername(strUser.c_str()) ;
        m_Client.setPassword(strPassword.c_str()) ;
        m_Client.connectToHost() ;
        qDebug() <<  "connectMqtt: " << strIP.c_str() << nPort << strUser.c_str() << strPassword.c_str() ;
    }
}

MainWidget::~MainWidget()
{
    killProcess("iperf-2.2.n-win64.exe") ;
    delete ui;
    exit(0) ;
}

void MainWidget::initTestCmdListData()
{
    std::map<int,QJsonObject> mapCmdEx;
    CSysSettings::getInstance()->loadCurDevTypeTestCmds( mapCmdEx );
    ui->widget_batch_test->updateTestItemData( mapCmdEx );
}

void MainWidget::initHttpPara()
{
    std::string sHttpIp;
    int iHttpPort=0;
    CSysSettings::getInstance()->getHttpPara( sHttpIp, iHttpPort );
    CHttpClientAgent::getInstance()->setPara( sHttpIp.c_str(), iHttpPort );
}

void MainWidget::on_pbtn_inquire_devices_clicked()
{
    qDebug() << "on_pbtn_inquire_devices_clicked..";
    ui->widget_video->stopVideo();
    ui->comboBoxDeviceList->clear() ;
    m_strBindId.clear() ;
    m_strSendTopic.clear() ;
    ui->lineEditDevicePid->clear() ;
    ui->labelBindFlag->setText("未绑定");
    ui->widget_batch_test->reset() ;

    if(!m_strRecvTopic.isEmpty())
        m_Client.unsubscribe(QMqttTopicFilter(m_strRecvTopic));

    QJsonObject jIQ ;
    jIQ["type"] = "request";
    jIQ["action"] = "device_search";
    jIQ["seq_number"] = 1000;
    m_Client.publish(QMqttTopicName(m_strRootTopic),QByteArray(QJsonDocument(jIQ).toJson()),0,false) ;
}

void MainWidget::on_pbtn_login_order_clicked()
{
    on_pushButton_clicked() ;
    qDebug() << "pbtn_login_order_clicked ...";
    if(m_strBindId.isEmpty())
    {
        QMessageBox::warning( this, "错误", "未绑定设备！" );
        return;
    }

    QString strOrderId = ui->lineEdit_order_no->text().trimmed();
    QString strOrderType = ui->lineEdit_device_type->text().trimmed();
    QString strDevUuid = ui->lineEditDeviceUuid->text().trimmed();
    ui->lineEditDeviceKey->setText("") ;
    ui->lineEditProduct->setText("") ;

    QString strTemp = m_strBindId;

    QJsonObject jObj ;
    CHttpClientAgent::getInstance()->getDeviceSN(strOrderId, strOrderType, strTemp, strDevUuid,jObj) ;
    if(jObj.isEmpty() || jObj["code"].toInt() != 200)
    {
        QJsonDocument doc(jObj);
        ui->widget_batch_test->addMqttLog(doc.toJson(QJsonDocument::JsonFormat::Compact)) ;
        QMessageBox::warning( this, "错误", "获取设备信息失败！\n" + jObj["msg"].toString() );
        return ;
    }
    QJsonObject jData = jObj["data"].toObject();

    m_jData = jData ;

    QString strProduct  = jData["product"].toString();
    QString strUuid = jData["uuid"].toString();
    QString strAuth = jData["auth_key"].toString();

    ui->lineEditDeviceKey->setText(strAuth) ;
    ui->lineEditDeviceUuid->setText(strUuid) ;
    ui->lineEditProduct->setText(strProduct) ;

    on_pushButtonWriteSN_clicked();
}

void MainWidget::updateDevType()
{
    ui->comboBox_device_type->clear();
    std::vector<std::string> devTypes = m_pStsWgt->getAllDeviceTyps();
    for( auto type : devTypes )
    {
        ui->comboBox_device_type->addItem( type.data() );
    }

    ui->comboBox_device_type->setCurrentText( m_pStsWgt->getCurDeviceType().data() );
}

void MainWidget::on_pbtn_print_dev_label_clicked()
{
    //if(!m_bWriteSN_OK)
    //    QMessageBox::warning( this, "温馨提示", "写号尚未完成！" );

    CLabelEdit *pLabel = CLabelEdit::getInstance() ;

    pLabel->setMode( 0 );
    QString curDevLabTem = CSysSettings::getInstance()->getDevLabelTemplateFile();
    pLabel->loadTemplate( curDevLabTem );
    pLabel->show();

    QString strUuid = ui->lineEditEchoUuid->text() ;
    QString strSN = ui->lineEditDeviceSN->text() ;

    pLabel->setLabelInfo( strUuid,strSN ) ;
}

void MainWidget::on_pbtn_print_box_label_clicked()
{
    m_pLabel2->show() ;
}

void MainWidget::on_pbtn_print_pack_label_clicked()
{
    m_pLabel3->show() ;
}

void MainWidget::on_comboBox_device_type_currentTextChanged( const QString &text )
{
    ui->lineEdit_device_type->setText(text) ;

    if( !text.trimmed().isEmpty() )
    {
        CSysSettings::getInstance()->setCurDeviceType( text );

        initTestCmdListData() ;
    }
}

void MainWidget::on_pushButtonClear_clicked()
{
    ui->lineEditDeviceUuid->setText("") ;
    ui->lineEditDeviceKey->setText("") ;
    ui->lineEditEchoUuid->setText("");
    ui->lineEditProduct->setText("");
    ui->lineEditDeviceUuid->setFocus();
}

void MainWidget::on_pushButtonReadSN_clicked()
{
    if(m_strBindId.isEmpty())
    {
        QMessageBox::warning(this,"提示","设备未绑定！");
        return ;
    }
    ui->lineEditEchoUuid->setText("");
    ui->lineEditDeviceSN->setText("");

    QJsonObject jIQ;
    jIQ["type"] = "request";
    jIQ["action"] = "dev_read_sn";
    jIQ["seq_number"] = rand()%2000;

    QJsonObject jData = m_jData;

    jData["device_mac"] = m_strBindId ;
    jIQ["data"] = jData;

    m_Client.publish(QMqttTopicName(m_strSendTopic),QByteArray(QJsonDocument(jIQ).toJson()),0,false) ;

    m_jData = {} ;

    jIQ["action"] = "dev_get_sn";
    jIQ["seq_number"] = rand()%2000;
    jIQ["data"] = jData;
    m_Client.publish(QMqttTopicName(m_strSendTopic),QByteArray(QJsonDocument(jIQ).toJson()),0,false) ;


}


void MainWidget::on_pbtn_bluetooth_scan_clicked()
{
    m_pBTDlg->show() ;
}


void MainWidget::on_pushButtonWriteSN_clicked()
{
    QString strTopic = m_strSendTopic ;

    QJsonObject jIQ,jData ;
    jIQ["type"] = "request";
    jIQ["action"] = "dev_write_sn";
    jIQ["seq_number"] = 500;

    jData["device_mac"] = m_strBindId ;
    jData["uuid"] = ui->lineEditDeviceUuid->text().trimmed() ;
    jData["product"] =  ui->lineEditProduct->text().trimmed() ;
    jData["auth_key"] = ui->lineEditDeviceKey->text().trimmed() ;    
    jData["force"] = ui->checkBoxForceWrite->isChecked() ? "yes":"no";
    jIQ["data"] = jData;

    m_Client.publish(QMqttTopicName(strTopic),QByteArray(QJsonDocument(jIQ).toJson()),0,false) ;
}

void MainWidget::on_lineEdit_order_no_editingFinished()
{
    //m_pStsWgt->setOrderId(ui->lineEdit_order_no->text().trimmed()) ;
}

void MainWidget::on_lineEditDeviceUuid_textChanged(const QString &arg1)
{
    m_TTrimText.stop() ;
    m_TTrimText.start(300) ;
}

void MainWidget::on_comboBoxDeviceList_activated(int index)
{
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


void MainWidget::on_pushButton_clicked()
{
    QJsonObject jRet ;
    int nTry= 5;
    while(nTry>0)
    {
        QString strOrderId = ui->lineEdit_order_no->text().trimmed();
        int nCount = CHttpClientAgent::getInstance()->queryNumber(strOrderId,jRet) ;
        ui->labelOrderCount->setNum(nCount) ;
        if(nCount>=0)
            break;
        ui->widget_batch_test->addMqttLog(CHttpClientAgent::getInstance()->m_strReturn);
        QThread::usleep(100) ;
        nTry-- ;
    }
}


void MainWidget::on_lineEdit_order_no_textEdited(const QString &arg1)
{
    Q_UNUSED(arg1)
    on_pushButton_clicked() ;
}


void MainWidget::on_pbtn_serialsetting_clicked()
{
    m_pCOMDlg->show() ;
}

