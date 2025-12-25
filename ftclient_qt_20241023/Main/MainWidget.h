#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QJsonArray>
#include <QString>
#include <QMutex>
#include <QTimer>
#include <QtMqtt/QMqttClient>

#include "DialogSysSetting.h"
#include "CHttpClientAgent.h"

#include "dialogpacklabelprint.h"
#include "dialogboxlabelprint.h"


QT_BEGIN_NAMESPACE
namespace Ui {
class MainWidget;
}
QT_END_NAMESPACE

class MainWidget : public QWidget
{
    Q_OBJECT

public:
    explicit MainWidget( QWidget *parent = nullptr );
    virtual ~MainWidget();

private slots:
    void on_pbtn_login_order_clicked();
    void on_pbtn_print_dev_label_clicked();
    void on_pbtn_print_box_label_clicked();
    void on_pbtn_print_pack_label_clicked();

    void on_pbtn_inquire_devices_clicked();

    void on_comboBox_device_type_currentTextChanged( const QString &text );

    void on_pushButtonClear_clicked();

    void on_comboBoxDeviceList_activated(int index);

    void on_pushButtonBind_clicked();

    void on_pushButtonGetOrderCount_clicked();

private:

    void updateDevType();

    void initTestCmdListData();
    void initHttpPara();
    void connectMqtt() ;

    void doDeviceBinding();


private:
    Ui::MainWidget *ui;
    DialogSysSetting *m_pStsWgt = nullptr;

    DialogLabelPrint   *m_pLabel3 ;
    DialogBoxLabelPrint*m_pLabel2 ;

    QMutex m_mtx_devList;

    QString m_strBindId ;
    QString m_strMyIP ;

    QMqttClient m_Client ;
    QString m_strRecvTopic ;
    QString m_strSendTopic ;
    QString m_strRootTopic ;
    QJsonObject  m_jBak ;

    QTimer m_TMAutoBind ;
    QTimer m_TMCheckVideo ;

    quint32 m_lastVideoIn = 0 ;
    bool m_bCanLogin = true ;

    CHttpClientAgent *m_pHttpAli = nullptr ;

    bool m_bCanBindWhenVideoLost = true;

    QJsonObject m_jData ;
    bool m_bLoading = false ;
};
#endif // MAINWIDGET_H
