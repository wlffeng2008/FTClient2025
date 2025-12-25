#ifndef DIALOGBLUETOOTH_H
#define DIALOGBLUETOOTH_H

#include <QDialog>

#include <QBluetoothLocalDevice>
#include <QBluetoothDeviceDiscoveryAgent>

#include <QStandardItemModel>


namespace Ui {
class DialogBlueTooth;
}

class DialogBlueTooth : public QDialog
{
    Q_OBJECT

public:
    explicit DialogBlueTooth(QWidget *parent = nullptr);
    ~DialogBlueTooth();

    void scanBlueTooth(const QString&strMac) ;

signals:
    void reportBleDevice(const QString &strDeviceMac,const QString &strDeviceName,bool show);

private slots:
    void on_pushButton_clicked();

private:
    Ui::DialogBlueTooth *ui;
    QBluetoothDeviceDiscoveryAgent *discoveryAgent=nullptr;
    QStandardItemModel *model=nullptr;
};

#endif // DIALOGBLUETOOTH_H
