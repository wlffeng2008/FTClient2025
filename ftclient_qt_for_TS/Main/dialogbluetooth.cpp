#include "dialogbluetooth.h"
#include "ui_dialogbluetooth.h"
#include <QMessageBox>

bool isBluetoothAvailable()
{
    QBluetoothLocalDevice localDevice;
    QBluetoothAddress address = localDevice.address();

    // 如果地址为空，则蓝牙不可用
    if (address.isNull()) {
        qDebug() << "蓝牙适配器未找到";
        return false;
    }

    // 检查蓝牙是否已开启
    if (localDevice.hostMode() == QBluetoothLocalDevice::HostPoweredOff) {
        qDebug() << "蓝牙已关闭";
        // localDevice.powerOn();
    }

    return true;
}


DialogBlueTooth::DialogBlueTooth(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogBlueTooth)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags() | Qt::MSWindowsFixedSizeDialogHint);

    bool bFindBle = isBluetoothAvailable() ;
    if(bFindBle)
    {
        discoveryAgent = new QBluetoothDeviceDiscoveryAgent(this);
        model = new QStandardItemModel();
        ui->listView->setModel(model);

        connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::deviceDiscovered,
                this,[=](const QBluetoothDeviceInfo&device){
                    QString strName = device.name() ;
                    QString strMac = device.address().toString() ;
                    if(!strName.startsWith("Bluetooth"))
                    {
                        emit reportBleDevice(strMac,strName,ui->checkBox->isChecked()) ;

                        QString itemText = strMac + " -- " +strName;
                        QStandardItem *item = new QStandardItem(itemText);
                        model->appendRow(item);
                        qDebug().noquote() << itemText ;
                    }
                });

        connect(discoveryAgent, &QBluetoothDeviceDiscoveryAgent::errorOccurred,
                this,[=](QBluetoothDeviceDiscoveryAgent::Error error){
                    qDebug()<< "onDeviceDiscoverError: " << error;
                });
    }

    ui->checkBox->setChecked(true) ;

    scanBlueTooth("") ;
}

DialogBlueTooth::~DialogBlueTooth()
{
    delete ui;
}

void DialogBlueTooth::scanBlueTooth(const QString&strMac)
{
    ui->lineEditMac->setText(strMac);
    if(!discoveryAgent)
        return ;
    on_pushButton_clicked() ;
}

void DialogBlueTooth::on_pushButton_clicked()
{
    if(!discoveryAgent)
    {
        QMessageBox::warning(this,"错误","没有找到蓝牙模块！") ;
        return ;
    }

    discoveryAgent->stop() ;
    ui->listView->reset() ;
    model->clear() ;
    int nScanMs = ui->lineEdit->text().toInt() ;
    discoveryAgent->setLowEnergyDiscoveryTimeout(nScanMs);
    discoveryAgent->start(QBluetoothDeviceDiscoveryAgent::LowEnergyMethod) ;
}

