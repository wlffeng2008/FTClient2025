#include "CTcpClientAgent.h"
#include "CommonLib.h"

#include <QDebug>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <sstream>


CTcpClientAgent* CTcpClientAgent::m_pTcpClientAgent = nullptr;

CTcpClientAgent::CTcpClientAgent( QObject *parent )
    : QObject( parent ), m_tcp_conn_status( TCP_CONN_STATUS::UNCONNECTED ), m_device_bound( false )
{
    m_pTcpClient = std::make_shared<CTcpClient>();
    m_sHostIP = "127.0.0.1";
    m_iPort = 3736;
}

CTcpClientAgent::~CTcpClientAgent()
{
    getTcpClient()->closeConnection();
}

bool CTcpClientAgent::connectServer()
{
    if( !getTcpClient()->connectToServer( m_sHostIP, m_iPort ) ) {
        updateTcpConnStatus( TCP_CONN_STATUS::UNCONNECTED );
        m_sError = "Connect server error";
        qDebug() << m_sError.data();
        return false;
    }
    updateTcpConnStatus( TCP_CONN_STATUS::CONNECT_OK );

    //getTcpClient()->startMonitoring();  // 启动数据监测
    return true;
}

// { "action": "register", "client_type": "debug_tool", "factory_id": "factory_id_01",
//   "factory_name": "factory_name_01", "work_pos_id": "work_pos_01", "user_name": "user_name_01" }
bool CTcpClientAgent::makeCmdData_register( const std::string& factory_id, const std::string& factory_name,
                                            const std::string& work_pos_id, const std::string& user_name, std::string& sCmdDataOut )
{
    // 创建 JSON 字符串
    sCmdDataOut = "{ \"action\": \"register\", "
                  "\"client_type\": \"debug_tool\", "
                  "\"factory_id\": \"" + factory_id + "\", "
                  "\"factory_name\": \"" + factory_name + "\", "
                  "\"work_pos_id\": \"" + work_pos_id + "\", "
                  "\"user_name\": \"" + user_name + "\" }";
    sCmdDataOut += "\n";
    return true;
}

bool CTcpClientAgent::execCmd_register( const std::string& factory_id, const std::string& factory_name,
                                       const std::string& work_pos_id, const std::string& user_name, std::string &sCmdResponse )
{
    if( m_tcp_conn_status != TCP_CONN_STATUS::CONNECT_OK )
    {
        m_sError = "tcp server not connected";
        qDebug() << m_sError.data();
        return false;
    }

    std::string sCmd;
    if( !makeCmdData_register( factory_id, factory_name, work_pos_id, user_name, sCmd ) )
    {
        m_sError = "makeCmdData_register error";
        qDebug() << m_sError.data();
        return false;
    }

    if( !execCmd_base( sCmd, sCmdResponse ) )
    {
        m_sError = "execCmd_register error - execCmd_base error";
        qDebug() << m_sError.data();
        return false;
    }
    updateTcpConnStatus( TCP_CONN_STATUS::REGISTER_OK );

    return true;
}


bool CTcpClientAgent::makeCmdData_bind( const std::string& device_id, std::string& sCmdDataOut )
{
    QJsonObject jBind ;
    jBind["action"] = "bind";
    jBind["target_device_id"] = device_id.c_str();

    QJsonDocument doc(jBind);
    sCmdDataOut = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString() + "\n";
    return true;
}

bool CTcpClientAgent::execCmd_bind( const std::string& device_id, std::string &sCmdResponse )
{
    std::string sCmd;
    if( !makeCmdData_bind( device_id, sCmd ) )
    {
        m_sError = "makeCmdData_bind error";
        qDebug() << m_sError.data();
        return false;
    }

    if( !execCmd_base( sCmd, sCmdResponse ) )
    {
        m_sError = "execCmd_bind error - execCmd_base error";
        qDebug() << m_sError.data();
        return false;
    }

    QJsonObject jsonObjOut;
    if( cb::strToJsonObject( sCmdResponse, jsonObjOut ) )
    {
        if( jsonObjOut["status"].toString() == "bind success" )
        {
            m_sError = "bound ok";
            qDebug() << m_sError.data();
            m_device_bound = true;
            return true;
        }
        else
        {
            m_sError = jsonObjOut["status"].toString().toStdString();
            qDebug() << m_sError.data();
            return false;
        }
    }
    else
    {
        m_sError = "json data parse error: " + sCmdResponse;
        qDebug() << m_sError.data();
        return false;
    }
}

bool CTcpClientAgent::makeCmdData_inquire_devices( std::string& sCmdDataOut )
{
    QJsonObject jIQ ;
    jIQ["action"] = "inquire_dev";

    QJsonDocument doc(jIQ);
    sCmdDataOut = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString() + "\n";

    return true;
}

bool CTcpClientAgent::execCmd_inquire_devices( QJsonArray& jsonArray )
{
    std::string sCmd;
    if( !makeCmdData_inquire_devices( sCmd ) )
    {
        m_sError = "makeCmdData_inquire_devices error";
        qDebug() << m_sError.data();
        return false;
    }

    std::string sCmdResponse;
    if( !execCmd_base( sCmd, sCmdResponse ) )
    {
        m_sError = "execCmd_forward error - execCmd_base error";
        qDebug() << m_sError.data();
        return false;
    }

    QJsonObject jsonResponse;
    if( !cb::strToJsonObject( sCmdResponse, jsonResponse ) )
    {
        m_sError = "execCmd_forward error - strToJsonObject error";
        qDebug() << m_sError.data();
        return false;
    }

    if( !jsonResponse["data"].isArray() )
    {
        return false;
    }

    jsonArray = jsonResponse["data"].toArray();
    return true;
}

bool CTcpClientAgent::makeCmdData_forward( const std::string& cmd, std::string& sCmdDataOut )
{
    QJsonObject jCmd ;
    jCmd["action"] = "forward";
    jCmd["forward_data"] = cmd.c_str();

    QJsonDocument doc(jCmd);
    sCmdDataOut = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString() + "\n";

    return true;
}


bool CTcpClientAgent::execCmd_factory_test(int cmd_id, const std::string &para, const std::string &check_val, int iTimeout_s, std::string &sCmdResponse, std::string *pReturnValue )
{
    m_sError = "";
    // 检查有没有绑定设备
    if( !m_device_bound )
    {
        //m_sError = "请先绑定设备";
        //qDebug() << m_sError.data();
        //return false;
    }

    // 获取指令字符串
    std::string sCmdOrig = m_mapCmdEx[cmd_id]["item_cmd"].toString().toStdString();
    if( QString( sCmdOrig.data() ).trimmed().isEmpty() )
    {
        m_sError = "Command not found for cmd_id: " + std::to_string( cmd_id );
        qDebug() << m_sError.data();
        return false;
    }

    // 设置指令执行超时时间
    if( !getTcpClient()->setRcvTimeout_tmp_ms( iTimeout_s*1000 ) )
    {
        qDebug() << m_sError.data();
        return false;
    }

    std::string sCmdEx = sCmdOrig;
    if( !para.empty() )
    {
        sCmdEx += " " + para;
    }

    std::string sCmdFrame;
    if( !makeCmdData_forward( sCmdEx, sCmdFrame ) )
    {
        m_sError = "makeCmdData_forward error - " + m_sError;
        qDebug() << m_sError.data();
        return false;
    }
    std::string sCmdResRaw;
    if( !execCmd_base( sCmdFrame, sCmdResRaw ) )
    {
        m_sError = "execCmd_forward error - execCmd_base error - " + m_sError;
        qDebug() << m_sError.data();
        return false;
    }

    // 解析响应数据
    bool bCmdExecSuccess = false;
    std::string sReturnVal;
    bool bParsOk = parseResponse( sCmdOrig, sCmdResRaw, bCmdExecSuccess, sCmdResponse, &sReturnVal );
    if( pReturnValue != nullptr )
    {
        *pReturnValue = sReturnVal;
    }
    if( !bParsOk )
    {
        m_sError = "execCmd_forward error - parseResponse error - " + sCmdResRaw;
        qDebug() << m_sError.data();
        return false;
    }

    // 判断返回值是否与预期值相同
    bool bTestedOk = bCmdExecSuccess;
    if( !check_val.empty() )
    {
        bTestedOk = ( sReturnVal == check_val );
    }

    return bTestedOk;
}

bool CTcpClientAgent::execCmd_write_sn( const std::string &dev_sn, std::string &sCmdResponse )
{
    std::string sCmd = "write_sn";
    return execCmd_entrance( sCmd, dev_sn, sCmdResponse );
}

bool CTcpClientAgent::execCmd_write_license( const std::string &dev_license, std::string &sCmdResponse )
{
    std::string sCmd = "write_license";
    return execCmd_entrance( sCmd, dev_license, sCmdResponse );
}

bool CTcpClientAgent::execCmd_ptz_move( EPTZ_MOVE_TYPE move_type, std::string &sCmdResponse )
{
    std::string sCmd = "ptz_start_move_";
    switch( move_type )
    {
    case EPTZ_MOVE_TYPE::STOP:
        sCmd += "stop";
        break;
    case EPTZ_MOVE_TYPE::UP:
        sCmd += "up";
        break;
    case EPTZ_MOVE_TYPE::DOWN:
        sCmd += "down";
        break;
    case EPTZ_MOVE_TYPE::LEFT:
        sCmd += "left";
        break;
    case EPTZ_MOVE_TYPE::RIGHT:
        sCmd += "right";
        break;
    }

    std::string para = "";
    return execCmd_entrance( sCmd, para, sCmdResponse );
}

// 指令：
//      video_start
//      video_stop
bool CTcpClientAgent::execCmd_video_control( bool video_on, const std::string& video_type, std::string &sCmdResponse )
{
    std::string sCmd = "video_" + ( video_on ? std::string( "start" ) : std::string( "stop" ) );
    qDebug() << sCmd << video_type;
    std::string para = video_type;
    return execCmd_entrance( sCmd, para, sCmdResponse );
}

// 输入参数 var_info 格式：
// {
//      "var_name": "name",
//      "var_type": "int",
//      "var_value": "val"
// }
// 指令格式：set_var_default var_name var_value var_data_type
// 其中 var_data_type 取值范围: int,float,string
// 举例：set_var_default status_led_switch 1 int
bool CTcpClientAgent::execCmd_write_var( const QJsonObject &var_info, std::string &sCmdResponse )
{
    std::string var_name = var_info["var_name"].toString().toStdString();
    std::string var_type = var_info["var_type"].toString().toStdString();
    std::string var_value = var_info["var_value"].toString().toStdString();
    std::string sCmdOrig = "set_var_default";
    std::string sPara = var_name + " " + var_value + " " + var_type;

    bool bRet = execCmd_entrance( sCmdOrig, sPara, sCmdResponse, false );
    qDebug() << m_sError.data();
    return bRet ;
}

void CTcpClientAgent::slConnBroken()
{
    updateTcpConnStatus( TCP_CONN_STATUS::UNCONNECTED );
}

bool CTcpClientAgent::execCmd_entrance( const std::string& cmd, const std::string& para, std::string &sCmdResponse, bool bResponseSimple )
{
    std::string cmd_new = QString( cmd.data() ).trimmed().toStdString();
    if( cmd_new.empty() )
    {
        m_sError = "cmd is empty";
        return false;
    }
    std::string sCmdPure = cmd_new;
    std::string sCmdOrig = sCmdPure;

    std::string para_new = QString( para.data() ).trimmed().toStdString();
    if( !para_new.empty() )
    {
        sCmdOrig += " " + para_new;
    }

    std::string sCmdFrame;
    if( !makeCmdData_forward( sCmdOrig, sCmdFrame ) )
    {
        m_sError = "makeCmdData_forward error";
        return false;
    }

    std::string sCmdResRaw;
    if( !execCmd_base( sCmdFrame, sCmdResRaw ) )
    {
        m_sError = "execCmd_forward error：" + sCmdResRaw;
        return false;
    }

    // 解析响应数据
    bool bCmdExecSuccess = false;
    std::string sCmdHead = bResponseSimple ? sCmdPure : sCmdOrig;
    if( !parseResponse( sCmdHead, sCmdResRaw, bCmdExecSuccess, sCmdResponse, nullptr ) )
    {
        m_sError = "execCmd_forward error - parseResponse error";
        return false;
    }

    return bCmdExecSuccess;
}

void CTcpClientAgent::updateTcpConnStatus( TCP_CONN_STATUS new_status )
{
    qDebug() << "updateTcpConnStatus ing - new_status: " << (int)new_status;
    m_tcp_conn_status = new_status;
    emit sigConnStatusChanged( m_tcp_conn_status );
}

// sCmdOring = "read_reset key"
// sResponseToBeParsed = "read_reset key ok 1" 说明执行成功, 读到的结果值为 1
// sResponseToBeParsed = "read_reset key error read failed" 说明执行失败, 失败原因为：read failed
bool CTcpClientAgent::parseResponse( const std::string& sCmdOrig, const std::string& sResponseToBeParsed, bool& bCmdExecSuccess, std::string& sResult, std::string *pReturnValue )
{
    qDebug() << "sCmdOrig: " << sCmdOrig.data() ;

    bCmdExecSuccess = false;
    sResult.clear();

    if( sResponseToBeParsed.find( sCmdOrig ) == 0 )
    {
        // Extract the part of the response that comes after the command
        std::string remainingResponse = sResponseToBeParsed.substr( sCmdOrig.length() );
        qDebug() <<"remainingResponse:" << remainingResponse ;

        size_t pos = remainingResponse.find_first_not_of(' ');
        if( pos != std::string::npos ) {
            remainingResponse = remainingResponse.substr( pos );
        }

        std::istringstream ss( remainingResponse);
        std::string word;

        // Check for the status word (e.g., "ok" or "error")
        ss >> word;

        if( word == "ok" )
        {
            bCmdExecSuccess = true;
            sResult = "Cmd exec ok";

            // Read the result if the command was successful
            std::string sValReturn;
            if( ss >> sValReturn )
            {
                if( pReturnValue != nullptr )
                    *pReturnValue = sValReturn;
            }

            return true;

        }else if( word == "error" )
        {
            bCmdExecSuccess = false;

            // Capture the error message
            if( std::getline( ss, sResult ) ) {
                // Remove leading spaces from the error message
                pos = sResult.find_first_not_of(' ');
                if( pos != std::string::npos ) {
                    sResult = sResult.substr( pos );
                }
                return true;
            } else {
                sResult = "Unknown error";
                return true;
            }
        } else {
            qDebug() << "word: " << word.data();
        }
    }

    // If we reach here, the response didn't match the expected format
    sResult = "Invalid response format";
    return false;
}

bool CTcpClientAgent::execCmd_base( const std::string &sCmd, std::string &sCmdResponse )
{
    m_mtx_exec_cmd.lock();
    if( !makeSureConnected() )
    {
        m_sError = getTcpClient()->getErrorStr();
        qDebug() << m_sError.data();
        m_mtx_exec_cmd.unlock();
        return false;
    }

    if( !getTcpClient()->sendData( sCmd ) )
    {
        m_sError = getTcpClient()->getErrorStr();
        qDebug() << m_sError.data();
        m_mtx_exec_cmd.unlock();
        return false;
    }

    qDebug() << "TCP Socket Send Data OK: " << sCmd.data();

    if( !getTcpClient()->receiveData( sCmdResponse ) )
    {
        m_sError = getTcpClient()->getErrorStr();
        sCmdResponse = m_sError;
        m_mtx_exec_cmd.unlock();
        return false;
    }

    QJsonObject jsobj_o;
    if( !cb::strToJsonObject( sCmdResponse, jsobj_o ) )
    {
        m_sError = "Invalid response data: " + sCmdResponse;
        sCmdResponse = m_sError;
        m_mtx_exec_cmd.unlock();
        return false;
    }

    if( !jsobj_o["error"].toString().trimmed().isEmpty() )
    {
        m_sError = jsobj_o["error"].toString().toStdString();
        sCmdResponse = m_sError;
        m_mtx_exec_cmd.unlock();
        return false;
    }
    else if( jsobj_o["status"].toString() != "forward success" )
    {
        m_sError = jsobj_o["status"].toString().toStdString();
        m_mtx_exec_cmd.unlock();
        return true;
    }

    // 对于转发指令，如果发送成功，设备端响应也成功，则：
    // 第一次收到的应该是服务器发来的转发成功确认信息： {"status":"forward success"}
    // 第二次是设备端发来的响应信息
    int timeout_ms = getTcpClient()->getRcvTimeout_tmp_ms();
    auto start_time = std::chrono::steady_clock::now();
    while( true )
    {
        // 检查是否超时
        auto current_time = std::chrono::steady_clock::now();
        auto elapsed_time = std::chrono::duration_cast<std::chrono::milliseconds>(current_time - start_time);
        if( elapsed_time.count() >= timeout_ms )
        {
            m_sError = "设备端响应超时";
            qDebug() << m_sError.data();
            sCmdResponse = m_sError;
            m_mtx_exec_cmd.unlock();
            return false;
        }

        // 再次尝试接收数据
        if( !getTcpClient()->receiveData( sCmdResponse ) )
        {
            m_mtx_exec_cmd.unlock();
            return false;
        }

        QJsonObject jsobj_w;
        if( !cb::strToJsonObject( sCmdResponse, jsobj_w ) )
        {
            m_mtx_exec_cmd.unlock();
            return true;
        }
        if( !jsobj_w["error"].toString().trimmed().isEmpty() )
        {
            m_sError = jsobj_w["error"].toString().toStdString();
            sCmdResponse = m_sError;
            m_mtx_exec_cmd.unlock();
            return false;
        }

        // 每次检查间隔 100 毫秒
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    m_mtx_exec_cmd.unlock();
    return true;
}

bool CTcpClientAgent::makeSureConnected()
{
    if( getTcpClient()->isConnected() )
        return true;

    if( !getTcpClient()->connectToServer( m_sHostIP, m_iPort ) ) {
        m_sError = "Connect server error";
        qDebug() << m_sError.data();
        return false;
    }

    return true;
}


