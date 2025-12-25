#include "CTcpClient.h"
#include <QDebug>


CTcpClient::CTcpClient( QObject *parent ) : QObject( parent ),
    m_clientSocket( INVALID_SOCKET ),
    m_iRcvTimeout_default_ms( 1000 ),
    m_iRcvTimeout_tmp_ms( 1000 ),
    m_iConnectTimeout_ms( 1000 ),
    m_b_initialized( false ),
    m_b_conn_monitoringActive( false ),
    m_bConnected( false ),
    m_b_rcv_monitoringActive( false )
{
    m_b_initialized = initializeWinsock();
    startConnMonitoring();
}

CTcpClient::~CTcpClient() {
    stopRcvMonitoring();  // 停止监测线程
    closeConnection();
    if( m_b_initialized ) {
        WSACleanup();
    }
}

bool CTcpClient::initializeWinsock() {
    WSADATA wsaData;
    int result = WSAStartup( MAKEWORD(2, 2), &wsaData );
    if ( result != 0 ) {
        m_sError = "WSAStartup failed: " + std::to_string( result );
        qDebug() << m_sError.data();
        return false;
    }
    return true;
}

bool CTcpClient::connectToServer( const std::string& ipAddress, int port ) {
    if( !m_b_initialized )
    {
        m_bConnected = false;
        return false;
    }

    m_clientSocket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if( m_clientSocket == INVALID_SOCKET ) {
        m_bConnected = false;
        m_sError = "Socket creation failed: " + std::to_string( WSAGetLastError() );
        qDebug() << m_sError.data();
        return false;
    }

    // 设置 socket 为非阻塞模式
    u_long mode = 1;
    ioctlsocket(m_clientSocket, FIONBIO, &mode);

    m_serverAddr.sin_family = AF_INET;
    m_serverAddr.sin_port = htons(port);
    m_serverAddr.sin_addr.s_addr = inet_addr(ipAddress.c_str());

    if( ::connect( m_clientSocket, (sockaddr*)&m_serverAddr, sizeof(m_serverAddr) ) == SOCKET_ERROR ) {
        if (WSAGetLastError() == WSAEWOULDBLOCK) {
            fd_set writeSet;
            FD_ZERO(&writeSet);
            FD_SET(m_clientSocket, &writeSet);

            timeval timeout;
            timeout.tv_sec = m_iConnectTimeout_ms / 1000;  // 设置超时时间 (秒)
            timeout.tv_usec = (m_iConnectTimeout_ms % 1000) * 1000;  // 微秒

            // 使用 select 监测连接的状态
            int result = select( 0, nullptr, &writeSet, nullptr, &timeout );
            if (result <= 0) {  // 超时或出错
                m_bConnected = false;
                m_sError = (result == 0) ? "Connection timeout" : "Connection failed: " + std::to_string(WSAGetLastError());
                qDebug() << m_sError.data();
                closeConnection();
                return false;
            }
        } else {
            m_bConnected = false;
            m_sError = "Connection failed: " + std::to_string( WSAGetLastError() );
            qDebug() << m_sError.data();
            closeConnection();
            return false;
        }
    }

    // 恢复为阻塞模式
    mode = 0;
    ioctlsocket(m_clientSocket, FIONBIO, &mode);

    if( !applyReceiveTimeout( m_iRcvTimeout_default_ms ) ){
        m_bConnected = false;
        m_sError = "applyReceiveTimeout failed: " + std::to_string( WSAGetLastError() );
        qDebug() << m_sError.data();
        closeConnection();
        return false;
    }
    m_bConnected = true;

    return true;
}

bool CTcpClient::clearReceiveBuffer()
{
    char buffer[512];
    while( true )
    {
        // 设置接收超时时间
        fd_set readfds;
        struct timeval timeout;
        timeout.tv_sec = 0;
        timeout.tv_usec = 100;

        FD_ZERO( &readfds );
        FD_SET( m_clientSocket, &readfds );

        int activity = select( m_clientSocket + 1, &readfds, nullptr, nullptr, &timeout );
        if( activity < 0 )
        {
            // 选择发生错误
            qDebug() << "select error: " << activity;
            return false;
        }
        else if (activity == 0)
        {
            // 超时，没有数据可读
            return true;
        }
        else
        {
            if( FD_ISSET( socket, &readfds ) )
            {
                // 有数据可读，尝试读取
                ssize_t bytesRead = recv( m_clientSocket, buffer, sizeof( buffer ), 0 );
                if( bytesRead <= 0 )
                {
                    // 连接关闭或发生错误
                    qDebug() << "Receive data error: " << bytesRead;
                    return false;
                }
                // 处理读取的数据，如果需要
            }
            else
            {
                qDebug() << "No data to be received";
                return true;
            }
        }
    }
}

bool CTcpClient::monitorConnection() {
    fd_set readfds;
    FD_ZERO( &readfds );
    FD_SET( m_clientSocket, &readfds );

    timeval timeout = {0, 0};  // Non-blocking
    int result = select(0, &readfds, NULL, NULL, &timeout);

    if (result == SOCKET_ERROR) {
        m_sError = "Select failed: " + std::to_string(WSAGetLastError());
        //std::cerr << m_sError.data() << std::endl;
        return false;
    }

    if (result == 0) {
        return true;  // No activity, connection seems fine
    }

    if( FD_ISSET( m_clientSocket, &readfds ) ) {
        char buffer[1];
        result = recv( m_clientSocket, buffer, sizeof(buffer), MSG_PEEK );
        if (result == 0) {
            // Connection has been closed
            qDebug() << "Connection closed by server.";
            closeConnection();
            return false;
        } else if (result < 0 && WSAGetLastError() != WSAEWOULDBLOCK) {
            qDebug() << "Connection error: " << std::to_string(WSAGetLastError()).data();
            closeConnection();
            return false;
        }
    }
    return true;
}

bool CTcpClient::sendData( const std::string& data )
{
    if( !clearReceiveBuffer() )
    {
        qDebug() << "clearReceiveBuffer error";
    }

    if( send( m_clientSocket, data.c_str(), data.size(), 0) == SOCKET_ERROR ) {
        m_sError = "Send failed: " + std::to_string( WSAGetLastError() );
        qDebug() << m_sError.data();
        return false;
    }
    return true;
}

bool CTcpClient::setRcvTimeout_tmp_ms( int iRcvTimeout_ms )
{
    if( iRcvTimeout_ms <= 0 )
    {
        m_iRcvTimeout_tmp_ms = m_iRcvTimeout_default_ms;
    }
    else
    {
        m_iRcvTimeout_tmp_ms = iRcvTimeout_ms;
    }

    if( applyReceiveTimeout( m_iRcvTimeout_tmp_ms ) )
    {
        return true;
    }
    else
    {
        m_sError = "applyReceiveTimeout failed: " + std::to_string( WSAGetLastError() );
        qDebug() << m_sError.data();
        return false;
    }
}

bool CTcpClient::setRcvTimeout_default_ms( int iRcvTimeout_ms )
{
    if( iRcvTimeout_ms > 0 )
    {
        m_iRcvTimeout_default_ms = iRcvTimeout_ms;
    }

    if( applyReceiveTimeout( m_iRcvTimeout_default_ms ) )
    {
        m_sError = "applyReceiveTimeout ok";
        qDebug() << m_sError.data();
        return true;
    }
    else
    {
        m_sError = "applyReceiveTimeout failed: " + std::to_string( WSAGetLastError() );
        qDebug() << m_sError.data();
        return false;
    }
}

// 设置接收超时时间的函数
bool CTcpClient::applyReceiveTimeout( int timeout_ms )
{
    if ( m_clientSocket == INVALID_SOCKET)
        return false;

    DWORD timeout = timeout_ms;     // 使用DWORD类型
    if ( setsockopt( m_clientSocket, SOL_SOCKET, SO_RCVTIMEO, (char*)&timeout, sizeof(timeout)) == SOCKET_ERROR ) {
        return false;
    }

    return true;
}

bool CTcpClient::receiveData( std::string& dataReceived )
{
    dataReceived = "";
    char buffer[512]={0};
    int bytesReceived = recv( m_clientSocket, buffer, sizeof(buffer) - 1, 0 );
    if ( bytesReceived == SOCKET_ERROR )
    {
        m_sError = "Receive failed: " + std::to_string( WSAGetLastError() );
        qDebug() << m_sError.data();
        return false;
    }

    // 如果bytesReceived为0，表示连接已关闭
    if( bytesReceived == 0 )
    {
        m_sError = "Connection closed by the server.";
        qDebug() << m_sError.data();
        closeConnection();
        return false;
    }

    buffer[bytesReceived] = '\0';
    dataReceived = std::string( buffer );
    qDebug().noquote() << "TCP Socket Receive Row Data:" << dataReceived;
    return true;
}

void CTcpClient::closeConnection()
{
    if ( m_clientSocket != INVALID_SOCKET)
    {
        closesocket( m_clientSocket );
        m_clientSocket = INVALID_SOCKET;
    }
}

// 新增的isConnected函数，检查socket是否仍然有效
bool CTcpClient::isConnected()
{
    if ( m_clientSocket == INVALID_SOCKET)
        return false;
    return m_bConnected;
}

void CTcpClient::startConnMonitoring()
{
    if( m_b_conn_monitoringActive )
        return;  // 如果已经在监测，则不需要再次启动
    m_b_conn_monitoringActive = true;
    m_conn_monitorThread = std::thread( &CTcpClient::monitorConnRun, this );  // 创建并启动监测线程
}

void CTcpClient::monitorConnRun()
{
    static bool bLastConnStatus = false;
    int interval_ms = 50;

    while( true )
    {
        if( !monitorConnection() )
        {
            m_bConnected = false;
            if( m_bConnected != bLastConnStatus )
            {
                emit sigConnBroken();
            }
        }
        else
        {
            m_bConnected = true;
        }
        bLastConnStatus = m_bConnected;

        Sleep( interval_ms );
    }
}

void CTcpClient::startRcvMonitoring()
{
    if( m_b_rcv_monitoringActive )
        return;  // 如果已经在监测，则不需要再次启动
    m_b_rcv_monitoringActive = true;
    m_rcv_monitorThread = std::thread( &CTcpClient::monitorIncomingData, this );  // 创建并启动监测线程
}

void CTcpClient::stopRcvMonitoring()
{
    if( m_b_rcv_monitoringActive )
    {
        m_b_rcv_monitoringActive = false;  // 停止标识
        if( m_rcv_monitorThread.joinable() )
        {
            m_rcv_monitorThread.join();    // 等待线程结束
        }
    }
}

void CTcpClient::monitorIncomingData()
{
    while( m_b_rcv_monitoringActive )
    {
        std::string receivedData;
        if( receiveData( receivedData ) )
        {
            if( !receivedData.empty() ) // 每1秒尝试接收数据
            {
                qDebug() << "Received data: " << receivedData.data();
            }
        }
        std::this_thread::sleep_for( std::chrono::milliseconds(10) );  // 控制接收频率
    }
}

