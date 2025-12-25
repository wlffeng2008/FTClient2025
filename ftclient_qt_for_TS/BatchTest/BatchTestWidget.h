#ifndef BATCH_TEST_WIDGET_H
#define BATCH_TEST_WIDGET_H
#include "DefaultValuesWidget.h"
#include "ButtonDelegateManPass.h"
#include "ButtonDelegate.h"

#include <QString>
#include <QWidget>
#include <QTimer>
#include <memory>
#include <QThread>
#include <QStandardItemModel>
#include "dialogbluetooth.h"
#include <QtMqtt/QMqttClient>

QT_BEGIN_NAMESPACE
namespace Ui {
class BatchTestWidget;
}
QT_END_NAMESPACE

QString getMyIP() ;

class BatTestThread;

class BatchTestWidget : public QWidget
{
    Q_OBJECT

public:
    explicit BatchTestWidget( QWidget *parent = nullptr );
    virtual ~BatchTestWidget();
    void updateTestItemData( const std::map<int, QJsonObject> &mapCmdEx ) ;

    void updateTestResult(const QString&strAction,const QString&strData,const QString&strStatus) ;

    void setMQTTAgent(QMqttClient *pMqtt,const QString&strTopic) ;
    void addMqttLog(const QString&strLog);
    void resetResult() ;

    QString m_strDevId ;
    bool m_bHeartbeat = false ;

    void reset() ;
    void setBlueToothDlg(DialogBlueTooth *pBleDlg) ;

    void batTest() ;

private slots:
    void on_pbtn_batch_test_start_clicked();
    void on_pbtn_reset_clicked();
    void handleItemManPassButtonClicked( const QModelIndex &index );
    void handleItemTestButtonClicked( const QModelIndex &index );
    void on_pbtn_browse_default_values_clicked();

    void on_pbtn_batch_success_clicked();

    void on_checkBox_enable_all_checkStateChanged(const Qt::CheckState &arg1);
    void onBlueTooth(const QString &strDeviceMac,const QString &strDeviceName,bool bShow) ;

private:
    void initTestItemTable();
    void updateDeviceListTableSize();
    void updateTestItemTableSize();
    std::shared_ptr<DefaultValuesWidget> &getDefaultValuesWidget();

    void checkFinalTestResult();
    bool testRow( int row );
    void addAllItemDataToTableOf( const std::map<int, QJsonObject> &mapCmdEx, bool bNeedManPass );

private:
    Ui::BatchTestWidget *ui;
    std::shared_ptr<QStandardItemModel> m_pItemModelTestItems;
    std::shared_ptr<DefaultValuesWidget> m_pDefaultValuesWidget;
    std::shared_ptr<ButtonDelegateManPass> m_pButtonDelegateManPass;
    std::shared_ptr<ButtonDelegate> m_pButtonDelegateTest;

    std::map<int,QJsonObject> m_mapCmdEx;

    bool m_bAutoBatchTestOk;
    bool m_bFinalTestOk;

    DialogBlueTooth *m_pBleDlg =nullptr;
    QTimer m_TimeBle ;

    QString m_strDestBle ;

    QString m_strAct ;
    BatTestThread *m_pBThread = nullptr ;
    QString m_strMqtt ;
    QTimer m_TMSendCmd ;
    QTimer m_TMHeart ;
};

class BatTestThread : public QThread
{
    Q_OBJECT
public:
    BatTestThread(QObject* parent = NULL):QThread(parent){ m_pTWidget = (BatchTestWidget *)parent ; start(); }
    ~BatTestThread(){ }
    void run() { if(m_pTWidget) m_pTWidget->batTest() ; };
private:
    BatchTestWidget *m_pTWidget = nullptr;
};

#endif // BATCH_TEST_WIDGET_H
