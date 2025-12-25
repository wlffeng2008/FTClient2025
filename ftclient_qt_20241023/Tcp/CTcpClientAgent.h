#ifndef CTCPCLIENTAGENT_H
#define CTCPCLIENTAGENT_H

#include "CTcpClient.h"
#include <QObject>
#include <QJsonArray>
#include <QJsonObject>
#include <QDebug>
#include <QMutex>
#include <memory>
#include <string>
#include <map>

// 服务器连接状态
enum class TCP_CONN_STATUS{
    UNCONNECTED = 0,
    CONNECT_OK,
    REGISTER_OK
};

// 定义产测指令id
enum class EPTZ_MOVE_TYPE{
    STOP = 0,
    UP,
    DOWN,
    LEFT,
    RIGHT,
};


class CTcpClientAgent : public QObject
{
    Q_OBJECT

public:
    explicit CTcpClientAgent( QObject *parent = nullptr );
    virtual ~CTcpClientAgent();

    static CTcpClientAgent* getInstance()
    {
        if( nullptr == m_pTcpClientAgent )
        {
            m_pTcpClientAgent = new CTcpClientAgent();
        }
        return m_pTcpClientAgent;
    }

    void setPara( const std::string& sHostIP, int port ){
        m_sHostIP = sHostIP;
        m_iPort = port;
    }

    bool connectServer();
    bool execCmd_register( const std::string& factory_id, const std::string& factory_name,
                           const std::string& work_pos_id, const std::string& user_name, std::string &sCmdResponse );
    bool execCmd_bind( const std::string& device_id, std::string &sCmdResponse );
    bool execCmd_inquire_devices( QJsonArray& jsonArray );
    bool execCmd_factory_test(int cmd_id, const std::string& para, const std::string &check_val, int iTimeout_s, std::string &sCmdResponse, std::string* pReturnValue );
    bool execCmd_write_sn( const std::string& dev_sn, std::string &sCmdResponse );
    bool execCmd_write_license( const std::string& dev_license, std::string &sCmdResponse );
    bool execCmd_ptz_move( EPTZ_MOVE_TYPE move_type, std::string &sCmdResponse );
    bool execCmd_video_control( bool video_on, const std::string &video_type, std::string &sCmdResponse );
    bool execCmd_write_var( const QJsonObject& var_info, std::string &sCmdResponse );
    const std::map<int,QJsonObject>& getCmdMap() { return m_mapCmdEx; }
    void setCmdMap( const std::map<int,QJsonObject>& mapCmdIn ) { m_mapCmdEx = mapCmdIn; }

    std::string getErrorStr()
    {
        return m_sError;
    }

    bool isServerConnOk()
    {
        return ( TCP_CONN_STATUS::UNCONNECTED != m_tcp_conn_status );
    }

    bool isServerRegistered()
    {
        qDebug() << "m_tcp_conn_status: " << (int)m_tcp_conn_status.load();
        return ( TCP_CONN_STATUS::REGISTER_OK == m_tcp_conn_status );
    }

    bool isDeviceBound()
    {
        return m_device_bound;
    }

signals:
    void sigConnStatusChanged( TCP_CONN_STATUS newStatus );
private slots:
    void slConnBroken();

private:
    std::shared_ptr<CTcpClient> getTcpClient()
    {
        if( nullptr == m_pTcpClient )
        {
            m_pTcpClient = std::make_shared<CTcpClient>();
            connect( m_pTcpClient.get(), SIGNAL( sigConnBroken() ), this, SLOT( slConnBroken() ) );
        }
        return m_pTcpClient;
    }

    bool makeSureConnected();
    bool parseResponse( const std::string& sCmdOrig, const std::string& sResponseToBeParsed, bool& bCmdExecSuccess, std::string& sResult, std::string* pReturnValue );
    bool execCmd_base( const std::string& sCmd, std::string& sCmdResponse );
    bool makeCmdData_register( const std::string& factory_id, const std::string& factory_name,
                               const std::string& work_pos_id, const std::string& user_name, std::string& sCmdDataOut );
    bool makeCmdData_bind( const std::string& device_id, std::string& sCmdDataOut );
    bool makeCmdData_inquire_devices( std::string& sCmdDataOut );
    bool makeCmdData_forward( const std::string& cmd, std::string& sCmdDataOut );
    bool execCmd_entrance( const std::string& cmd, const std::string& para, std::string &sCmdResponse, bool bResponseSimple = true );

    void updateTcpConnStatus( TCP_CONN_STATUS new_status );

private:
    static CTcpClientAgent* m_pTcpClientAgent;

    std::atomic<TCP_CONN_STATUS> m_tcp_conn_status;
    std::shared_ptr<CTcpClient> m_pTcpClient;
    std::string m_sHostIP;
    int m_iPort;
    std::atomic<bool> m_device_bound;
    std::string m_sError;
    std::map<int,QJsonObject> m_mapCmdEx;
    QMutex m_mtx_exec_cmd;

};

#endif // CTCPCLIENTAGENT_H
