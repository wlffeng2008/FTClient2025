#ifndef CHTTPCLIENTAGENT_H
#define CHTTPCLIENTAGENT_H

#include "CHttpClient.h"
#include <memory>
#include <string>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>


class CHttpClientAgent
{
public:
    explicit CHttpClientAgent();
    virtual ~CHttpClientAgent();

    void setPara( const std::string& sHostIP, int port )
    {
        m_sHostIP = sHostIP;
        m_iPort = port;
    }

    bool getDeviceSN( const QString& sOrderNo, const QString& sOrderPwd,const QString&strDevicMac,const QString&strUuid,
                     QJsonObject&jObjRet);
    bool reportDeviceSN( const QString& sOrderNo,const QString& strType,const QString&strDevicMac,const QString&strName,
                        const QString&strKey,const QString&strSecrect1,const QString&strSecrect2, QJsonObject&jObjRet);
    bool reportPackInfo(const QString&strPackNo,const QStringList&arrSNs, QJsonObject&jObjRet);

    int queryNumber( const QString& strOrderId, QJsonObject&jObjRet);
    QString m_strReturn ;

private:
    CHttpClient* m_pHttp;
    std::string m_sHostIP;
    std::string m_sUuidIP;
    int m_iPort;

};

#endif // CHTTPCLIENTAGENT_H
