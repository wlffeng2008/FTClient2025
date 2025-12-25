#ifndef BATCH_TEST_WIDGET_H
#define BATCH_TEST_WIDGET_H
#include "DialogDefaultValues.h"
#include "ButtonDelegateManPass.h"
#include "ButtonDelegate.h"

#include <QString>
#include <QWidget>
#include <QThread>
#include <QTimer>
#include <QMap>
#include <QStandardItemModel>
#include <QtMqtt/QMqttClient>

#include "dialogbluetooth.h"

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
    void updateTestItemData( const QMap<int, QJsonObject> &mapCmdEx ) ;

    void updateTestResult(const QString&strAction,const QString&strData,const QString&strStatus) ;

    void setMQTTAgent(QMqttClient *pMqtt,const QString&strTopic) ;
    void addMqttLog(const QString&strLog);
    void resetResult() ;
    void batTest() ;
    void autoTestAll() ;
    void setDevId(const QString&strDevId) ;
    bool m_bHeartbeat = false ;

private slots:
    void on_pbtn_batch_test_start_clicked();

    void on_pbtn_reset_clicked();
    void handleItemManPassButtonClicked( const QModelIndex &index );
    void handleItemTestButtonClicked( const QModelIndex &index );

    void on_pbtn_browse_default_values_clicked();
    void on_pbtn_batch_success_clicked();
    void on_checkBox_enable_all_checkStateChanged(const Qt::CheckState &arg1);
    void on_pushButtonBle_clicked();

private:
    void initTestItemTable();

    void checkFinalTestResult();
    bool testRow( int row);
    void addAllItemDataToTableOf( const QMap<int, QJsonObject> &mapCmdEx, bool bNeedManPass );

private:
    Ui::BatchTestWidget *ui;
    QStandardItemModel *m_pModelItems;
    DialogDefaultValues* m_pDefaultValuesWidget = nullptr;
    ButtonDelegateManPass *m_pManPassDelegate = nullptr;
    ButtonDelegate *m_pTestDelegate = nullptr;

    QString m_strDevId ;
    QString m_strDestBle ;
    DialogBlueTooth *m_pBTDlg = nullptr ;

    QMap<int,QJsonObject> m_mapCmdEx;

    bool m_bAutoBatchTestOk=false;
    bool m_bFinalTestOk=false;
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
