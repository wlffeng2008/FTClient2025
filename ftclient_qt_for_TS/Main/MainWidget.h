#ifndef MAINWIDGET_H
#define MAINWIDGET_H

#include <QWidget>
#include <QStandardItemModel>
#include <QJsonArray>
#include <QString>
#include <QMutex>
#include <QTimer>
#include "SysSettingsWidget.h"

#include "dialogpacklabelprint.h"
#include "dialogboxlabelprint.h"
#include "dialogbluetooth.h"
#include "dialoguserlogin.h"
#include "SerialTestDialog.h"

#include <QtMqtt/QMqttClient>

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

    void on_pushButtonReadSN_clicked();

    void on_pbtn_bluetooth_scan_clicked();

    void on_pushButtonWriteSN_clicked();

    void on_lineEdit_order_no_editingFinished();

    void on_lineEditDeviceUuid_textChanged(const QString &arg1);

    void on_comboBoxDeviceList_activated(int index);

    void on_pushButtonBind_clicked();

    void on_pushButton_clicked();

    void on_lineEdit_order_no_textEdited(const QString &arg1);

    void on_pbtn_serialsetting_clicked();

private:
    void updateDevType();

    void initTestCmdListData();
    void initHttpPara();
    void connectMqtt() ;

    void doDeviceBinding();

private:
    Ui::MainWidget *ui;
    SysSettingsWidget *m_pStsWgt = nullptr;

    DialogLabelPrint   *m_pLabel3 ;
    DialogBoxLabelPrint*m_pLabel2 ;
    DialogUserLogin    *m_pLogin ;
    SerialTestDialog *m_pCOMDlg ;

    DialogBlueTooth *m_pBTDlg ;
    QString m_strBindId ;
    QString m_strDeviceSN="ABCDEF1234567890";

    QMqttClient m_Client ;
    QString m_strRecvTopic ;
    QString m_strSendTopic ;
    QString m_strRootTopic ;

    QTimer  m_TTrimText ;
    bool m_bWriteSN_OK = false ;

    QTimer m_TMAutoBind ;
    QTimer m_TMCheckVideo ;
    QJsonObject m_jData ;

    bool m_bCanBindWhenVideoLost = true;
};
#endif // MAINWIDGET_H
