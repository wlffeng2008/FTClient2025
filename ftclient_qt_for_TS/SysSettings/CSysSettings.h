#ifndef CSYSSETTINGS_H
#define CSYSSETTINGS_H

#include <map>
#include <string>
#include <vector>
#include <QJsonObject>
#include <QJsonArray>


class CSysSettings
{
public:
    explicit CSysSettings();
    virtual ~CSysSettings();

    static CSysSettings* getInstance()
    {
        if( nullptr == m_pThis )  m_pThis = new CSysSettings();
        return m_pThis;
    }

    const std::string& getConfigPath() { return m_sSysCfgPath; }

    const std::map<std::string,QJsonObject>& getDevTypeSettings()
    {
        return m_mapDevTypeSettings;
    }

    bool updateDevTypeSettings(const std::map<std::string,QJsonObject>& newSettings )
    {
        m_mapDevTypeSettings = newSettings;
        return saveSysConfig();
    }

    std::string getWorkPosId() {return m_sWorkPosId;}
    void setWorkPosId(const std::string&strWorkPos) {m_sWorkPosId=strWorkPos;saveSysConfig();}

    const std::string getCurDeviceComType() {return m_sCurDeviceComType;}
    bool setCurDeviceComType( const QString& curDeviceComType );

    const std::string getCurDeviceType() {return m_sCurDeviceType;}
    const std::vector<std::string> getAllDeviceTyps();

    const QJsonObject& getNetCfg() { return m_jsonNetCfg;}
    bool setNetCfg( const QJsonObject& newNetCfg );

    bool getHttpPara(  std::string &strIP, int &nPort );
    bool getMqttPara( std::string& strIP, int&nPort,std::string&strUser,std::string& strPassword);
    bool getRtmpPara( std::string& sIP, int& iPort );
    bool getRtmpDelay_ms( int& iDelay_ms );

    bool setCurDeviceType( const QString& curDeviceType );
    bool addDevType( const std::string& devType, const QJsonObject& objConfig );
    bool removeDevType( const std::string& devType );

    const QJsonObject& getLabelTemCfg()   { return m_jsonLabelTemCfg; }
    bool setLabelTemCfg( const QJsonObject& newLabelTemCfg );
    bool setDevLabelTemplateFile( const QString& devLabelTemplateFile );
    QString getDevLabelTemplateFile();
    bool setBoxLabelTemplateFile( const QString& boxLabelTemplateFile );
    QString getBoxLabelTemplateFile();
    bool setPackLabelTemplateFile( const QString& packLabelTemplateFile );
    QString getPackLabelTemplateFile();

    bool loadDevTypeTestCmds( const std::string& devType, std::map<int,QJsonObject>& mapTestItems );
    bool loadCurDevTypeTestCmds( std::map<int,QJsonObject>& mapTestItems );

    bool loadDevTypeSetDefaultCmds( const std::string& devType, std::vector<QJsonObject> &vecItems );
    bool loadCurDevTypeSetDefaultCmds( std::vector<QJsonObject>& vecItems );

    bool loadSysConfig();
    bool saveSysConfig();

    bool getLoginPara(QString&strUser,QString&strPWord) ;
    bool setLoginPara(const QString&strUser,const QString&strPWord) ;

private:
    static CSysSettings* m_pThis;
    std::string m_sSysCfgPath;
    std::string m_sSysCfgFilePath;
    std::map<std::string,QJsonObject> m_mapDevTypeSettings;      // 设备型号->型号配置文件
    std::string m_sWorkPosId;
    std::string m_sCurDeviceType;           // TS101K, TS201Q, ...
    std::string m_sCurDeviceComType;        // wifi, 4g, ...
    QJsonObject m_jsonNetCfg;
    QJsonObject m_jsonLabelTemCfg;

};

#endif // CSYSSETTINGS_H
