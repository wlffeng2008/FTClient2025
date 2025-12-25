#ifndef CHTTPCLIENTAGENT_H
#define CHTTPCLIENTAGENT_H

#include "CHttpClient.h"
#include <memory>
#include <QString>
#include <QJsonObject>
#include <QJsonArray>


class CHttpClientAgent
{
public:
    explicit CHttpClientAgent();
    virtual ~CHttpClientAgent();

    static CHttpClientAgent* getInstance()
    {
        if( nullptr == m_pHttpClientAgent)
            m_pHttpClientAgent = new CHttpClientAgent();
        return m_pHttpClientAgent;
    }

    void setPara( const QString& sHostIP, int port )
    {
        m_sHostIP = sHostIP.toStdString();
        m_iPort = port;
    }

    bool userLogin(const QString& strUserName, const QString& strPassword, QJsonObject&jObjRet) ;

    bool getDeviceSN( const QString& strOrderId, const QString& sOrderPwd,const QString&strDevicMac,const QString&strDevicSN,
                     QJsonObject&jObjRet);
    bool reportDeviceSN( const QString& strOrderId,const QString& strType,const QString&strDevicMac,const QString&strDevUuid,
                        const QString&strProduct,const QString&strAuthKey, QJsonObject&jObjRet);
    bool reportPackInfo(const QString&strPackNo,const QStringList&arrSNs, QJsonObject&jObjRet);

    int queryNumber( const QString& strOrderId, QJsonObject&jObjRet);

    QString m_strReturn ;

private:
    static CHttpClientAgent* m_pHttpClientAgent;
    std::shared_ptr<CHttpClient> m_pHttpCient;
    std::string m_sHostIP;
    int m_iPort;

};

#endif // CHTTPCLIENTAGENT_H
