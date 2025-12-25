#include "CHttpClientAgent.h"
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonValue>
#include <QJsonArray>
#include <QString>
#include <QDebug>

static std::string path1 = "/api/v1/codeali/frist";
static std::string path2 = "/api/v1/codeali/second";
static std::string path3 = "/api/v1/codeali/third";
static std::string path4 = "/api/v1/tuya_number";

CHttpClientAgent::CHttpClientAgent()
{
    m_pHttp = new CHttpClient();
    m_sHostIP = "192.168.1.135";
    m_iPort = 3735;

    path1 = "/api/v1/tange/frist";
    path2 = "/api/v1/tange/second";
    path3 = "/api/v1/tange/third";
    path4 = "/api/v1/tange_number";
}

CHttpClientAgent::~CHttpClientAgent()
{

}

bool CHttpClientAgent::getDeviceSN( const QString& sOrderNo, const QString& sOrderType,const QString&strDevicMac,
                                     const QString&strUuid, QJsonObject&jObjRet)
{
    QJsonObject jLogin;
    jLogin["order_id"] = sOrderNo;
    jLogin["device_type"] = sOrderType;
    jLogin["mac"]= strDevicMac;
    //jLogin["uuid"] = strUuid;

    //jLogin["device_name"] = strUuid;

    QJsonDocument doc(jLogin);
    std::string strData = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    qDebug().noquote()<<"getDeviceSN Send:" << strData;
    std::string sResult = m_pHttp->postRequest( m_sHostIP, m_iPort, path1, strData );
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

bool CHttpClientAgent::reportDeviceSN(const QString& sOrderNo,const QString& strType,const QString&strDevicMac,
                    const QString&strName, const QString&strKey,const QString&strSecrect1,const QString&strSecrect2,
                    QJsonObject&jObjRet)
{
    QJsonObject jRept;
    jRept["mac"] = strDevicMac;
    jRept["order_id"] = sOrderNo;
    jRept["device_type"] = strType;

    jRept["uuid"] = strName;
    jRept["key"] = strKey;

    // jRept["product_key"] = strKey;
    // jRept["product_secret"]=strSecrect1;
    // jRept["device_name"] = strName;
    // jRept["device_secret"] = strSecrect2;
    // jRept["result"] = "OK";

    QJsonDocument doc(jRept);
    std::string strData = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    qDebug().noquote()<<"reportDeviceSN Send:" << strData;
    std::string sResult = m_pHttp->postRequest( m_sHostIP, m_iPort, path2, strData );
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

    QJsonArray jArray;
    for (const QString& str : arrSNs) {
        jArray.append(str.trimmed());
    }

    //jRept["device_names_list"] = jArray;
    jRept["uuid_list"] = jArray;

    QJsonDocument doc(jRept);
    std::string strData = doc.toJson(QJsonDocument::JsonFormat::Compact).toStdString();
    qDebug().noquote()<<"reportPackInfo Send:" << strData;
    std::string sResult = m_pHttp->postRequest( m_sHostIP, m_iPort, path3, strData );
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
    std::string sResult = m_pHttp->postRequest( m_sHostIP, m_iPort, path4, strData );
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
