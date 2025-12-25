#include "CSysSettings.h"
#include "BatchTestWidget.h"
#include <fstream>
#include <map>
#include <QDebug>
#include <QString>
#include <QStringList>
#include <QJsonObject>
#include <QJsonDocument>
#include <QFile>
#include <QDir>


CSysSettings* CSysSettings::m_pThis = nullptr;

CSysSettings::CSysSettings():
    m_sSysCfgPath( "config" ),
    m_sSysCfgFilePath( "./config/sysconfig" ),
    m_sCurDeviceType( "Z03Y" ),
    m_sCurDeviceComType( "4G" )
{
    QString curPath = QDir::currentPath();
    m_sSysCfgPath = curPath.toStdString() + "/config/";
    m_sSysCfgFilePath = m_sSysCfgPath + "sysconfig";
    loadSysConfig();
}

CSysSettings::~CSysSettings()
{

}

bool CSysSettings::setCurDeviceComType( const QString &curDeviceComType )
{
    std::string newDevComType = curDeviceComType.trimmed().toStdString();
    if( newDevComType.empty() )
    {
        qDebug() << "setCurDeviceType error: curDeviceType is empty";
        return false;
    }
    m_sCurDeviceComType = newDevComType;

    qDebug() << "saving sys config..";
    if( !saveSysConfig() )
    {
        qDebug() << "setCurDeviceType error: saveSysConfig error";
        return false;
    }

    return true;
}

const std::vector<std::string> CSysSettings::getAllDeviceTyps()
{
    std::vector<std::string> allDevTypes;
    for( auto pair : m_mapDevTypeSettings )
    {
        allDevTypes.push_back( pair.first );
    }
    return allDevTypes;
}

bool CSysSettings::setNetCfg( const QJsonObject &newNetCfg )
{
    m_jsonNetCfg = newNetCfg;

    return saveSysConfig();
}

bool CSysSettings::getLoginPara(QString&strUser,QString&strPWord)
{
    strPWord ="ts123456" ;
    strUser = "tanshi" ;
    if(m_jsonNetCfg.contains( "LoginUser" ) )
    {
        strUser = m_jsonNetCfg["LoginUser"].toString().trimmed() ;
        strPWord = m_jsonNetCfg["LoginPass"].toString().trimmed()  ;
        return true ;
    }
    return false ;
}

bool CSysSettings::setLoginPara(const QString&strUser,const QString&strPWord)
{
    m_jsonNetCfg["LoginUser"] = strUser ;
    m_jsonNetCfg["LoginPass"] = strPWord ;
    return true ;
}


bool CSysSettings::getHttpPara( std::string &strIP, int &nPort )
{
    strIP = "47.106.117.102";
    nPort =  2375;
    if(m_jsonNetCfg.contains( "http_server_ip" ) )
    {
        strIP = m_jsonNetCfg["http_server_ip"].toString().trimmed().toStdString();
        nPort = m_jsonNetCfg["http_server_port"].toInt();
        return true;
    }
    return false;
}

bool CSysSettings::getMqttPara( std::string& strIP, int&nPort,std::string&strUser,std::string& strPassword)
{
    strIP = "47.106.117.102";
    nPort = 1883 ;
    strUser="tanshi" ;
    strPassword="tanshi987123";

    if( m_jsonNetCfg.contains( "mqtt_server_ip" ))
    {
        strIP = m_jsonNetCfg["mqtt_server_ip"].toString().toStdString();
        nPort = m_jsonNetCfg["mqtt_server_port"].toInt() ;
        strUser= m_jsonNetCfg["mqtt_user"].toString().toStdString() ;
        strPassword=m_jsonNetCfg["mqtt_password"].toString().toStdString();
        return true ;
    }

    return false ;
}

bool CSysSettings::getRtmpPara( std::string& sIP, int& iPort )
{
    sIP = m_jsonNetCfg["rtmp_server_ip"].toString().toStdString();
    iPort = m_jsonNetCfg["rtmp_server_port"].toInt();

    if(m_jsonNetCfg["rtmp_local_enable"].toBool())
    {
        sIP = getMyIP().toStdString() ;
        iPort = m_jsonNetCfg["rtmp_local_port"].toInt();
        if(iPort <= 0)
            iPort = 1935 ;
    }

    return true;
}

bool CSysSettings::getRtmpDelay_ms( int& iDelay_ms )
{
    iDelay_ms = m_jsonNetCfg["rtmp_delay_ms"].toInt();
    return true;
}

bool CSysSettings::setCurDeviceType( const QString &curDeviceType )
{
    if( curDeviceType.trimmed().isEmpty() )
    {
        qDebug() << "setCurDeviceType error: curDeviceType is empty";
        return false;
    }

    std::string newDevType = curDeviceType.trimmed().toStdString();
    if( m_mapDevTypeSettings.find( newDevType ) == m_mapDevTypeSettings.end() )
    {
        qDebug() << "setCurDeviceType error: deviceType not found: " << curDeviceType;
        return false;
    }

    m_sCurDeviceType = newDevType;

    qDebug() << "saving sys config..";
    if( !saveSysConfig() )
    {
        qDebug() << "setCurDeviceType error: saveSysConfig error";
        return false;
    }

    return true;
}

bool CSysSettings::loadSysConfig()
{
    bool bExecOk = true;

    // 清空当前的设备类型设置
    m_mapDevTypeSettings.clear();

    QFile file( m_sSysCfgFilePath.data() );
    if( !file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
        qWarning() << "Could not open file for reading:" << m_sSysCfgFilePath.data() << file.errorString();
        return false;
    }

    QTextStream in( &file );
    QString jsonString = in.readAll();
    file.close();

    QJsonParseError parseError;
    QJsonDocument jsonDoc = QJsonDocument::fromJson( jsonString.toUtf8(), &parseError );
    if( jsonDoc.isNull() )
    {
        qWarning() << "Failed to parse JSON:" << parseError.errorString();
        return false;
    }

    if( !jsonDoc.isObject() )
    {
        qWarning() << "The file does not contain a JSON object";
        return false;
    }

    QJsonObject jsonAllData = jsonDoc.object();

    // 读取网络配置
    if( !jsonAllData["net_config"].isObject() )
    {
        qWarning() << "net_config item is not a valid json object.";
        bExecOk = false;
    }
    else
    {
        m_jsonNetCfg = jsonAllData["net_config"].toObject();
    }

    // 读取标签模板配置
    if( !jsonAllData["label_tem_config"].isObject() )
    {
        qWarning() << "label_tem_config item is not a valid json object.";
        bExecOk = false;
    }
    else
    {
        m_jsonLabelTemCfg = jsonAllData["label_tem_config"].toObject();
    }

    // 读取工位id
    if( !jsonAllData["work_pos_id"].isString() )
    {
        qWarning() << "work_pos_id item is not a valid json object.";
        bExecOk = false;
    }
    else
    {
        m_sWorkPosId = jsonAllData["work_pos_id"].toString().toStdString();
        m_sWorkPosId = QString( m_sWorkPosId.data() ).trimmed().toStdString();
    }

    // 读取当前设备通讯类型
    if( !jsonAllData["current_device_com_type"].isString() )
    {
        qWarning() << "current_device_com_type item is not a valid json string.";
        bExecOk = false;
    }
    else
    {
        m_sCurDeviceComType = jsonAllData["current_device_com_type"].toString().toStdString();
    }

    // 读取当前设备类型
    if( !jsonAllData["current_device_type"].isString() )
    {
        qWarning() << "current_device_type item is not a valid json string.";
        bExecOk = false;
    }
    else
    {
        m_sCurDeviceType = jsonAllData["current_device_type"].toString().toStdString();
    }

    // 读取所有设备类型配置
    if( !jsonAllData["all_types"].isArray() )
    {
        qWarning() << "The device type array data is not a json array";
        bExecOk = false;
    }
    else
    {
        QJsonArray jsonAllTypes = jsonAllData["all_types"].toArray();
        for( const QJsonValue& value : jsonAllTypes )
        {
            if( value.isObject() )
            {
                QJsonObject jsonObject = value.toObject();
                if( jsonObject.contains( "device_type" ) )
                {
                    std::string dev_type = jsonObject["device_type"].toString().toStdString();
                    if( jsonObject.contains( "config" ) )
                    {
                        m_mapDevTypeSettings[ dev_type ] = jsonObject["config"].toObject();
                    }
                }
                else
                {
                    qWarning() << "JSON object does not contain a 'key' field.";
                }
            }
        }
    }

    return bExecOk;
}

bool CSysSettings::saveSysConfig()
{
    QJsonObject jsonAll;
    jsonAll["work_pos_id"] = m_sWorkPosId.data();
    jsonAll["current_device_com_type"] = m_sCurDeviceComType.data();
    jsonAll["current_device_type"] = m_sCurDeviceType.data();
    QJsonArray jsonArray_all_types;
    for( const auto& pair : m_mapDevTypeSettings )
    {
        QJsonObject jsonItem;
        jsonItem["device_type"] = QString::fromStdString( pair.first );
        jsonItem["config"] = pair.second;
        jsonArray_all_types.append( jsonItem );
    }
    jsonAll["all_types"] = jsonArray_all_types;
    jsonAll["net_config"] = m_jsonNetCfg;
    jsonAll["label_tem_config"] = m_jsonLabelTemCfg;

    QJsonDocument jsonDoc( jsonAll );
    QString jsonString = jsonDoc.toJson( QJsonDocument::Indented );

    QFile file( m_sSysCfgFilePath.data() );
    if( !file.open( QIODevice::WriteOnly | QIODevice::Text ) )
    {
        qWarning() << "Could not open file for writing:" << m_sSysCfgFilePath.data() << file.errorString();
        return false;
    }

    QTextStream out( &file );
    out << jsonString;
    file.close();

    if( file.error() != QFile::NoError )
    {
        qWarning() << "Error occurred while writing to file:" << m_sSysCfgFilePath.data() << file.errorString();
        return false;
    }

    return true;
}

bool CSysSettings::addDevType(const std::string &devType, const QJsonObject &objConfig )
{
    if( QString( devType.data() ).isEmpty() )
    {
        qDebug() << "addDevType error: devType is empty.";
        return false;
    }

    m_mapDevTypeSettings[ devType ] = objConfig;

    if( !saveSysConfig() )
    {
        qDebug() << "saveSysConfig() error";
        return false;
    }

    return true;
}

bool CSysSettings::removeDevType( const std::string &devType )
{
    size_t erased_num = m_mapDevTypeSettings.erase( devType );
    if( erased_num <= 0 )
    {
        qDebug() << "0 device type erased";
        return false;
    }

    if( m_mapDevTypeSettings.empty() )
    {
        m_sCurDeviceType = "";
    }
    else if( devType == m_sCurDeviceType )
    {
        m_sCurDeviceType = m_mapDevTypeSettings.begin()->first;
    }

    if( !saveSysConfig() )
    {
        qDebug() << "saveSysConfig error";
        return false;
    }

    return true;
}

bool CSysSettings::setLabelTemCfg( const QJsonObject &newLabelTemCfg )
{
    m_jsonLabelTemCfg = newLabelTemCfg;

    if( !saveSysConfig() )
    {
        qDebug() << "saveSysConfig error";
        return false;
    }
    return true;
}

bool CSysSettings::setDevLabelTemplateFile(const QString &devLabelTemplateFile)
{
    m_jsonLabelTemCfg["devLabelTem"] = devLabelTemplateFile;

    if( !saveSysConfig() )
    {
        qDebug() << "saveSysConfig() error";
        return false;
    }
    return true;
}

QString CSysSettings::getDevLabelTemplateFile()
{
    return m_jsonLabelTemCfg["devLabelTem"].toString();
}

bool CSysSettings::setBoxLabelTemplateFile( const QString& boxLabelTemplateFile )
{
    m_jsonLabelTemCfg["boxLabelTem"] = boxLabelTemplateFile;

    if( !saveSysConfig() )
    {
        qDebug() << "saveSysConfig() error";
        return false;
    }
    return true;
}

QString CSysSettings::getBoxLabelTemplateFile()
{
    return m_jsonLabelTemCfg["boxLabelTem"].toString();
}

bool CSysSettings::setPackLabelTemplateFile( const QString& packLabelTemplateFile )
{
    m_jsonLabelTemCfg["packLabelTem"] = packLabelTemplateFile;
    if( !saveSysConfig() )
    {
        qDebug() << "saveSysConfig() error";
        return false;
    }
    return true;
}

QString CSysSettings::getPackLabelTemplateFile()
{
    return m_jsonLabelTemCfg["packLabelTem"].toString();
}

bool CSysSettings::loadDevTypeTestCmds( const std::string &devType, std::map<int,QJsonObject>& mapTestItems )
{
    mapTestItems.clear();

    if( QString( devType.data() ).isEmpty() )
    {
        qDebug() << "addDevType error: devType is empty.";
        return false;
    }

    if( m_mapDevTypeSettings.end() == m_mapDevTypeSettings.find( devType ) )
    {
        qDebug() << "Device type " << devType.data() << " does not exist.";
        return false;
    }

    std::string cfgFilePath = m_mapDevTypeSettings[devType]["test_cmd_file"].toString().toStdString();
    std::ifstream file( cfgFilePath );
    if( !file.is_open() )
    {
        qDebug() << "Failed to open file: " << cfgFilePath.data() ;
        return false;
    }

    int index = 0;
    int colum_num = 7;
    std::string strLine;
    std::getline( file, strLine ) ;
    while( std::getline( file, strLine ) )
    {
        QString strTmp = QString("%1").arg(strLine.c_str()).trimmed() + "\n" ;
        strLine = strTmp.toStdString() ;

        int nGet = 0 ;
        int nLen = strLine.length();
        QStringList lineList ;
        for(int i=0; i<nLen; i++)
        {
            char C = strLine.at(i) ;
            if(C == ',' || C == '\n')
            {
                std::string strStage = strLine.substr(i - nGet,nGet);
                lineList.append(strStage.c_str()) ;
                nGet = 0 ;
                continue;
            }

            nGet ++ ;

            if(C == '{')
            {
                int pair = 1 ;
                while(pair && i<nLen)
                {
                    i++ ;
                    C = strLine.at(i) ;
                    nGet ++ ;

                    if(C == '{') pair++ ;
                    if(C == '}') pair-- ;
                }
            }
        }

        if( lineList.size() < colum_num )
            continue;

        QString item_name = lineList[0].trimmed();
        QString item_cmd = lineList[1].trimmed();
        int man_pass = lineList[2].trimmed().toInt();
        QString para_to_write = lineList[3].trimmed();
        QString para_to_check = lineList[4].trimmed();
        int time_out_s = lineList[5].trimmed().toInt();
        QString btn_text = lineList[6].trimmed().isEmpty() ? "测试" : lineList[6].trimmed();
        if( item_cmd.isEmpty() || item_cmd.toLower() == "null" )
            continue;

        QJsonObject cmdJson;
        cmdJson["item_name"] = item_name;
        cmdJson["item_cmd"] = item_cmd;
        cmdJson["man_pass"] = ( 0==man_pass ? 0 : 1 );
        cmdJson["para_to_write"] = para_to_write.toLower() == "null" ? "" : para_to_write;
        cmdJson["para_to_check"] = para_to_check.toLower() == "null" ? "" : para_to_check;
        cmdJson["time_out_s"] = time_out_s;
        cmdJson["btn_text"] = btn_text.toLower() == "null" ? "测试" : btn_text;

        mapTestItems[index] = cmdJson;
        index++;
    }

    file.close();
    return true;
}

bool CSysSettings::loadCurDevTypeTestCmds( std::map<int, QJsonObject> &mapTestItems )
{
    return loadDevTypeTestCmds( m_sCurDeviceType, mapTestItems );
}

bool CSysSettings::loadDevTypeSetDefaultCmds( const std::string &devType, std::vector<QJsonObject> &vecItems )
{
    vecItems.clear();

    if( QString( devType.data() ).isEmpty() )
    {
        qDebug() << "addDevType error: devType is empty.";
        return false;
    }

    if( m_mapDevTypeSettings.end() == m_mapDevTypeSettings.find( devType ) )
    {
        qDebug() << "Device type " << devType.data() << " does not exist.";
        return false;
    }

    std::string cfgFilePath = m_mapDevTypeSettings[devType]["default_var_value_file"].toString().toStdString();
    std::ifstream file( cfgFilePath );
    if( !file.is_open() )
    {
        qDebug() << "Failed to open file: " << cfgFilePath.data() ;
        return false;
    }
    qDebug() << cfgFilePath;

    std::string line;
    int column_num = 4;
    bool first_line_read = false;
    while( std::getline( file, line ) ) {
        if( !first_line_read )
        {
            first_line_read = true;
            continue;
        }

        QStringList strList = QString( line.data() ).split( "," );
        if( strList.count() < column_num )
        {
            continue;
        }

        // 将数据保存到QJsonObject中
        // status_led_switch, 1, int, 状态灯开关
        QJsonObject cmdJson;
        cmdJson["var_name"] = strList[0].trimmed();
        cmdJson["var_value"] = strList[1].trimmed();
        cmdJson["var_group"] = strList[2].trimmed();
        cmdJson["var_note"] = strList[3].trimmed();

        // 保存到vector中
        vecItems.push_back( cmdJson );
    }

    file.close();
    return true;
}

bool CSysSettings::loadCurDevTypeSetDefaultCmds( std::vector<QJsonObject> &vecItems )
{
    return loadDevTypeSetDefaultCmds( m_sCurDeviceType, vecItems );
}

