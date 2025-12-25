#ifndef CTCPCLIENT_H
#define CTCPCLIENT_H

#include <winsock2.h>
#include <string>
#include <thread>
#include <atomic>
#include <QObject>


class CTcpClient : public QObject
{
    Q_OBJECT

public:
    explicit CTcpClient( QObject *parent = nullptr );
    virtual ~CTcpClient();

    bool connectToServer( const std::string& ipAddress, int port );
    bool sendData( const std::string& data );
    bool receiveData( std::string& dataReceived );
    void closeConnection();
    bool isConnected();
    bool clearReceiveBuffer();

    void startRcvMonitoring();  // 启动监测线程
    void stopRcvMonitoring();   // 停止监测线程

    std::string getErrorStr()
    {
        return m_sError;
    }

    bool setRcvTimeout_tmp_ms( int iRcvTimeout_ms );
    int getRcvTimeout_tmp_ms(){
        return m_iRcvTimeout_tmp_ms;
    }
    bool setRcvTimeout_default_ms( int iRcvTimeout_ms );
    int getRcvTimeout_default_ms(){
        return m_iRcvTimeout_default_ms;
    }

signals:
    void sigConnBroken();

private:
    bool applyReceiveTimeout( int timeout_ms );
    void monitorIncomingData();  // 监测线程的功能

    void startConnMonitoring();
    void monitorConnRun();
    bool monitorConnection();

private:
    SOCKET m_clientSocket;
    sockaddr_in m_serverAddr;

    int m_iRcvTimeout_default_ms;
    int m_iRcvTimeout_tmp_ms;

    int m_iConnectTimeout_ms;
    bool m_b_initialized;
    std::string m_sError;

    std::thread m_conn_monitorThread;
    std::atomic<bool> m_b_conn_monitoringActive;  // 标识监测是否激活
    std::atomic<bool> m_bConnected;

    std::thread m_rcv_monitorThread;
    std::atomic<bool> m_b_rcv_monitoringActive;  // 标识监测是否激活

    bool initializeWinsock();
};

#endif // CTCPCLIENT_H
