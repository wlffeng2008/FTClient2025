#include "CHttpClient.h"
#include <iostream>
#include <cstring>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <QDebug>


#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    #define close closesocket  // 定义 close 为 closesocket 以兼容 Linux 风格
#else
    #include <unistd.h>
    #include <arpa/inet.h>
    #include <sys/socket.h>
#endif

#define BUFFER_SIZE 4096


CHttpClient::CHttpClient(): m_iTimeout_ms( 1000 ) {
#ifdef _WIN32
    initializeWinsock();
#endif
}

CHttpClient::~CHttpClient() {
#ifdef _WIN32
    cleanupWinsock();
#endif
}

#ifdef _WIN32
bool CHttpClient::initializeWinsock() {
    WSADATA wsaData;
    if( WSAStartup( MAKEWORD( 2, 2 ), &wsaData ) != 0 )
    {
        m_sError = "WSAStartup failed";
        std::cerr << m_sError.data() << std::endl;
        return false;
    }
    return true;
}

void CHttpClient::cleanupWinsock() {
    WSACleanup();
}
#endif

int CHttpClient::createSocket( const std::string& hostname, int port )
{
    int sockfd;
    struct sockaddr_in server_addr;
    if( ( sockfd = socket( AF_INET, SOCK_STREAM, 0 ) ) == INVALID_SOCKET )
    {
        m_sError = "Socket creation error";
        std::cerr << m_sError.data() << std::endl;
        WSACleanup();
        return -1;
    }

    // 设置非阻塞模式
    u_long mode = 1;
    ioctlsocket(sockfd, FIONBIO, &mode);

    // server_addr.sin_family = AF_INET;
    // server_addr.sin_port = htons(port);

    // if( inet_pton(AF_INET, hostname.c_str(), &server_addr.sin_addr) <= 0 )
    // {
    //     m_sError = "Invalid address/ Address not supported";
    //     std::cerr << m_sError.data() << std::endl;
    //     closesocket(sockfd);
    //     WSACleanup();
    //     return -2;
    // }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);

    // 使用 inet_addr 代替 inet_pton
    server_addr.sin_addr.s_addr = inet_addr(hostname.c_str());
    if (server_addr.sin_addr.s_addr == INADDR_NONE) {
        std::string m_sError = "Invalid address/ Address not supported";
        std::cerr << m_sError << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return -2;
    }

    int ret = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
    if ( ret == SOCKET_ERROR && WSAGetLastError() != WSAEWOULDBLOCK ) {
        m_sError = "Connection Failed";
        std::cerr << m_sError.data() << std::endl;
        closesocket(sockfd);
        WSACleanup();
        return -3;
    }

    if (ret == SOCKET_ERROR && WSAGetLastError() == WSAEWOULDBLOCK) {
        fd_set writefds;
        FD_ZERO(&writefds);
        FD_SET(sockfd, &writefds);

        // 设置超时时间
        struct timeval tv;
        tv.tv_sec = m_iTimeout_ms / 1000;
        tv.tv_usec = ( m_iTimeout_ms % 1000 ) * 1000;
        ret = select(0, NULL, &writefds, NULL, &tv);
        if (ret <= 0) { // 连接超时或发生错误
            m_sError = (ret == 0) ? "Connection timeout" : "Connection error";
            std::cerr << m_sError.data() << std::endl;
            closesocket(sockfd);
            WSACleanup();
            return -4;
        }

        int so_error;
        int len = sizeof(so_error);
        getsockopt(sockfd, SOL_SOCKET, SO_ERROR, (char*)&so_error, &len);
        if (so_error != 0) {
            m_sError = "Connection failed with error " + std::to_string(so_error);
            std::cerr << m_sError.data() << std::endl;
            closesocket(sockfd);
            WSACleanup();
            return -5;
        }
    }

    // 连接成功后将套接字设置回阻塞模式
    mode = 0;
    ioctlsocket(sockfd, FIONBIO, &mode);

    return sockfd;
}

static std::string globalToken ="" ;

void setToken(const std::string&strToken)
{
    globalToken = strToken ;
    qDebug() << "-----------------" << strToken.c_str();
}

void CHttpClient::sendPostRequest(int sockfd, const std::string& hostname, const std::string& path, const std::string& data, const std::string& token ) {
    // 构建请求头
    std::string headers = "POST " + path + " HTTP/1.1\r\n" +
                          "Host: " + hostname + "\r\n" +
                          "User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64)\r\n" +
                          "Accept: application/json\r\n" +
                          "Content-Type: application/json\r\n" +
                          "Content-Length: " + std::to_string(data.length()) + "\r\n";

    // 如果 token 不为空，添加 Authorization 头
    if( !globalToken.empty() ) {
        headers += "Authorization: Bearer " + globalToken + "\r\n";
    }

    // 添加 Connection 头和两个 CRLF 以分隔头和主体
    headers += "Connection: close\r\n\r\n";

    // 完整的请求字符串
    std::string request = headers + data;

    // 发送请求
    send(sockfd, request.c_str(), request.length(), 0);
}

std::string CHttpClient::receiveResponse(int sockfd) {
    char buffer[BUFFER_SIZE];
    std::string response;
    int n;

    while ((n = recv(sockfd, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[n] = '\0';
        response.append(buffer);
    }

    return response;
}

std::string CHttpClient::postRequest(const std::string& hostname, int port, const std::string& path, const std::string& jsonData, const std::string &token) {
    int sockfd = createSocket( hostname, port );
    if( sockfd < 0 )
    {
        return "createSocket error";
    }

    sendPostRequest( sockfd, hostname, path, jsonData, token );
    std::string response = receiveResponse( sockfd );
    close( sockfd );  // 使用 close 或 closesocket 关闭 socket
    return response;
}
