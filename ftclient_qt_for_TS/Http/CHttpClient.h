#ifndef CHTTPCLIENT_H
#define CHTTPCLIENT_H

#include <string>
void setToken(const std::string&strToken);
class CHttpClient {
public:
    CHttpClient();
    ~CHttpClient();

    std::string postRequest(const std::string& hostname, int port, const std::string& path, const std::string& jsonData, const std::string& token = "");
    const std::string& getErrorStr() { return m_sError; }

private:
    int createSocket(const std::string& hostname, int port);
    void sendPostRequest(int sockfd, const std::string& hostname, const std::string& path, const std::string& data, const std::string& token = "");
    std::string receiveResponse(int sockfd);

#ifdef _WIN32
    bool initializeWinsock();
    void cleanupWinsock();
#endif

    int m_iTimeout_ms;
    std::string m_sError;
};

#endif // CHTTPCLIENT_H
