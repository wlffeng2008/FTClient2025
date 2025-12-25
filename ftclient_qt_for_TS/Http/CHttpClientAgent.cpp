#include "CHttpClientAgent.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QString>
#include <QDebug>


CHttpClientAgent* CHttpClientAgent::m_pHttpClientAgent = nullptr;

static std::string path1 = "/api/v1/tuya/frist";
static std::string path2 = "/api/v1/tuya/second";
static std::string path3 = "/api/v1/tuya/third";
static std::string path4 = "/api/v1/tuya_number";
static std::string strPassword = "eyJhbGciOiJIUzI1NiIsInR5cCI6IkpXVCJ9.eyJleHAiOjE3MzQwNTUyOTIsInN1YiI6IjIifQ.YD6dcfrpqR1Kap8sTA73HtM5OFHKSzMLcKsW819rhEo" ;


CHttpClientAgent::CHttpClientAgent()
{
    m_pHttpCient = std::make_shared<CHttpClient>();
    m_sHostIP = "192.168.1.135";
    m_iPort = 8008;
}

CHttpClientAgent::~CHttpClientAgent()
{

}

bool CHttpClientAgent::userLogin(const QString& strUserName, const QString& strPassword, QJsonObject&jObjRet)
{
    QJsonObject jLogin;
    jLogin["username"] = strUserName;
    jLogin["password"] = strPassword;
    jLogin["captcha"]= "HelloWorld";

    QString strPath="/api/v1/auth/login";
    QJsonDocument doc(jLogin);
    std::string strData = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    qDebug().noquote()<<"userLogin Send:" << strData << m_sHostIP << m_iPort << strPath;
    std::string sResult = m_pHttpCient->postRequest( m_sHostIP, m_iPort, strPath.toStdString(), strData );
    int nFind = sResult.find("\r\n\r\n") ;
    QString strReturn = sResult.substr(nFind+4).c_str() ;
    qDebug().noquote() << "userLogin Response: " << strReturn;
    QJsonDocument jDoc = QJsonDocument::fromJson(strReturn.toUtf8());
    if(jDoc.isObject())
    {
        jObjRet = jDoc.object();
        if(jDoc["code"].toInt() == 200)
        {
            setToken(jDoc["data"].toObject()["access_token"].toString().toStdString()) ;
            return true ;
        }
    }
    return false ;
}

bool CHttpClientAgent::getDeviceSN( const QString& strOrderId, const QString& sOrderType,const QString&strDevicMac,const QString&strDevicSN,
                                      QJsonObject&jObjRet)
{
    QJsonObject jLogin;
    jLogin["order_id"] = strOrderId;
    jLogin["device_type"] = sOrderType;
    jLogin["mac"]= strDevicMac;

    if(!strDevicSN.isEmpty())
    {
        jLogin["uuid"] = strDevicSN;
        //jLogin["pwd"] = strPassword.c_str();
    }

    QJsonDocument doc(jLogin);
    std::string strData = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    qDebug().noquote()<<"getDeviceSN Send:" << strData << m_sHostIP << m_iPort << path1;
    std::string sResult = m_pHttpCient->postRequest( m_sHostIP, m_iPort, path1, strData );
    int nFind = sResult.find("\r\n\r\n") ;
    QString strReturn = sResult.substr(nFind+4).c_str() ;
    qDebug().noquote() << "getDeviceSN Response: " << strReturn;
    QJsonDocument jDoc = QJsonDocument::fromJson(strReturn.toUtf8());
    if(jDoc.isObject())
    {
        jObjRet = jDoc.object();
        if(jDoc["code"].toInt() == 200)
            return true ;
    }
    return false ;
}

bool CHttpClientAgent::reportDeviceSN(const QString& strOrderId,const QString& strDevType,const QString&strDevMac,
                    const QString&strDevUuid, const QString&strProduct, const QString&strAuthKey, QJsonObject&jObjRet)
{
    QJsonObject jRept;
    jRept["mac"] = strDevMac;
    jRept["order_id"] = strOrderId;
    jRept["device_type"] = strDevType;

    jRept["product"] = strProduct;
    jRept["auth_key"]= strAuthKey;
    jRept["uuid"] = strDevUuid;
    //jRept["pwd"] = strPassword.c_str();

    jRept["result"] = "OK";

    QJsonDocument doc(jRept);
    std::string strData = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    qDebug().noquote()<<"reportDeviceSN Send:" << strData;
    std::string sResult = m_pHttpCient->postRequest( m_sHostIP, m_iPort, path2, strData );
    int nFind = sResult.find("\r\n\r\n") ;
    QString strReturn = sResult.substr(nFind+4).c_str() ;
    qDebug().noquote() << "reportDeviceSN Response: " << strReturn;
    QJsonDocument jDoc = QJsonDocument::fromJson(strReturn.toUtf8());
    if(jDoc.isObject())
    {
        jObjRet = jDoc.object();
        if(jDoc["code"].toInt() == 200)
            return true ;
    }
    return false ;
}

bool CHttpClientAgent::reportPackInfo(const QString&strPackNo,const QStringList&arrSNs, QJsonObject&jObjRet)
{
    QJsonObject jRept;
    jRept["box_name"] = strPackNo;
    jRept["device_number"]= arrSNs.size();
    //jRept["pwd"] = strPassword.c_str();

    QJsonArray jArray;
    for (const QString& str : arrSNs) {
        jArray.append(str.trimmed());
    }

    jRept["device_names_list"] = jArray;

    QJsonDocument doc(jRept);
    std::string strData = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    qDebug().noquote()<<"reportPackInfo Send:" << strData;
    std::string sResult = m_pHttpCient->postRequest( m_sHostIP, m_iPort, path3, strData );
    int nFind = sResult.find("\r\n\r\n") ;
    QString strReturn = sResult.substr(nFind+4).c_str() ;
    qDebug().noquote() << "reportPackInfo Response: " << strReturn;
    QJsonDocument jDoc = QJsonDocument::fromJson(strReturn.toUtf8());
    if(jDoc.isObject())
    {
        jObjRet = jDoc.object();
        if(jDoc["code"].toInt() == 200)
            return true ;
    }
    return false ;
}

int CHttpClientAgent::queryNumber( const QString& strOrderId, QJsonObject&jObjRet)
{
    QJsonObject jRept;
    jRept["order_id"] = strOrderId;

    QJsonDocument doc(jRept);
    std::string strData = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    std::string sResult = m_pHttpCient->postRequest( m_sHostIP, m_iPort, path4, strData );
    int nFind = sResult.find("\r\n\r\n") ;
    QString strReturn = sResult.substr(nFind+4).c_str() ;
    qDebug().noquote() << "queryNumber Response: " << strReturn;
    m_strReturn = strReturn ;
    QJsonDocument jDoc = QJsonDocument::fromJson(strReturn.toUtf8());
    if(jDoc.isObject())
    {
        jObjRet = jDoc.object();
        if(jDoc["code"].toInt() == 200)
            return jDoc["data"].toInt() ;
    }
    return -1 ;
}
