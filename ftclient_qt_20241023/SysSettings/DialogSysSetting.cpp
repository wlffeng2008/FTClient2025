#include "DialogSysSetting.h"
#include "ui_DialogSysSetting.h"

#include "ButtonDelegate.h"
#include "BatchTestWidget.h"

#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QItemDelegate>
#include <QDir>
#include <QDebug>


DialogSysSetting::DialogSysSetting(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogSysSetting)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags()|Qt::MSWindowsFixedSizeDialogHint);
    initSysCfgListUI();

    ui->tableView_device_types->setEditTriggers( QAbstractItemView::DoubleClicked );
    connect( ui->tableView_device_types->itemDelegate(), &QItemDelegate::closeEditor, this, &DialogSysSetting::onSysStsTableEditingFinished );

    ui->label_LocalIP->setText(getMyIP() + ": ") ;
    updateSysCfgListData();
    updateCurDevTypeListData();
    updateCurDevType();
    updateNetCfgData();
    updateLabelTemCfgData();

    const QList<QPushButton*> btns = findChildren<QPushButton*>();
    for(QPushButton *btn : std::as_const(btns))
    {
        btn->setFocusPolicy(Qt::NoFocus);
        btn->setCursor(Qt::PointingHandCursor);
        btn->setAutoRepeat(false);
    }
}

DialogSysSetting::~DialogSysSetting()
{
    delete ui;
}


void DialogSysSetting::setCurDeviceType( const QString &newDeviceType )
{
    CSysSettings::getInstance()->setCurDeviceType( newDeviceType );
    updateCurDevType();
}

void DialogSysSetting::updateUIData()
{
    updateCurDevTypeListData();
    updateSysCfgListData();
    updateCurDevType();
}

void DialogSysSetting::on_pbtn_add_device_type_clicked()
{
    QString devType = ui->lineEdit_device_type->text().trimmed();
    if( devType.isEmpty() )
    {
        QMessageBox::warning( this, "失败", "设备型号不能为空!" );
        return;
    }

    QDir currentDir;
    QString dirPath = QString( CSysSettings::getInstance()->getConfigPath().data() ) + QString( devType.data() );
    if( currentDir.mkpath( dirPath ) )
    {
        qDebug() << "目录创建成功：" << dirPath;
    } else {
        qDebug() << "目录创建失败：" << dirPath;
    }

    std::string cmdfilePath = ui->lineEdit_config_file_path->text().trimmed().toStdString();
    std::string varfilePath = ui->lineEdit_default_var_file_path ->text().trimmed().toStdString();
    QJsonObject jsonItem;
    jsonItem["test_cmd_file"] = cmdfilePath.data();
    jsonItem["default_var_value_file"] = varfilePath.data();
    if( CSysSettings::getInstance()->addDevType( devType.toStdString(), jsonItem ) )
    {
        qDebug() << "devType " << devType << " added ok.";
        updateUIData();
        QMessageBox::information( this, "成功", "设备型号添加成功!" );
    }
    else
    {
        qDebug() << "devType " << devType<< " added error.";
        QMessageBox::warning( this, "失败", "设备型号添加失败!" );
    }
}

void DialogSysSetting::on_pushButton_select_cfg_file_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择测试指令配置文件",
        "",
        "Text Files (*.cmd);;All Files (*)"
        );

    if( !fileName.isEmpty() )
    {
        ui->lineEdit_config_file_path->setText( fileName );
    }
}

void DialogSysSetting::on_pushButton_select_default_var_file_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择默认值配置文件",
        "",
        "Text Files (*.var);;All Files (*)"
        );

    if( !fileName.isEmpty() )
    {
        ui->lineEdit_default_var_file_path->setText( fileName );
    }
}

void DialogSysSetting::on_comboBox_cur_device_type_currentTextChanged( const QString &text )
{
    CSysSettings::getInstance()->setCurDeviceType( text );
}

void DialogSysSetting::initSysCfgListUI()
{
    // 设置表头
    getItemModelSysCfgs()->setHeaderData( 0, Qt::Horizontal, "设备型号" );
    getItemModelSysCfgs()->setHeaderData( 1, Qt::Horizontal, "测试指令配置文件" );
    getItemModelSysCfgs()->setHeaderData( 2, Qt::Horizontal, "默认值配置文件" );
    getItemModelSysCfgs()->setHeaderData( 3, Qt::Horizontal, "操作" );

    // 创建 QTableView 并绑定数据模型
    ui->tableView_device_types->setModel( getItemModelSysCfgs().get() );

    QHeaderView *pHD = ui->tableView_device_types->horizontalHeader() ;
    pHD->setSectionResizeMode(QHeaderView::Stretch);

    pHD->setSectionResizeMode(0,QHeaderView::Fixed);
    pHD->setSectionResizeMode(3,QHeaderView::Fixed);
    pHD->resizeSection(0,120) ;
    pHD->resizeSection(3,80) ;
}

bool DialogSysSetting::updateSysCfgListData()
{
    while( getItemModelSysCfgs()->rowCount() > 0 )
        getItemModelSysCfgs()->removeRow( 0 );

    int index = 0;
    std::map<std::string,QJsonObject> map_para = CSysSettings::getInstance()->getDevTypeSettings();
    for( const auto& pair : map_para )
    {
        getItemModelSysCfgs()->setItem( index, 0, new QStandardItem( pair.first.data() ) );
        getItemModelSysCfgs()->setItem( index, 1, new QStandardItem( pair.second["test_cmd_file"].toString() ) );
        getItemModelSysCfgs()->setItem( index, 2, new QStandardItem( pair.second["default_var_value_file"].toString() ) );
        getItemModelSysCfgs()->setItem( index, 3, new QStandardItem( "删除" ) );

        ui->tableView_device_types->setRowHeight( index, 30 );
        index++;
    }

    // 设置自定义委托 - 每一行的删除按钮
    ButtonDelegate *delegate = new ButtonDelegate( "删除该行", this );
    ui->tableView_device_types->setItemDelegateForColumn(3, delegate); // 将按钮放在第3列
    connect( delegate, &ButtonDelegate::buttonClicked, this, &DialogSysSetting::handleItemButtonClicked );

    return true;
}

void DialogSysSetting::handleItemButtonClicked( const QModelIndex &index )
{
    qDebug() << "Button clicked at row" << index.row() << "column" << index.column();
    if( index.row() >= 0 )
    {
        auto reply = QMessageBox::question( this, "确认操作", "你确定要删除这条记录吗？");

        if( reply == QMessageBox::Yes )
        {
            QString curDevType = getItemModelSysCfgs()->item( index.row(), 0 )->text();
            CSysSettings::getInstance()->removeDevType( curDevType.toStdString() );
            updateUIData();
        }
    }
}

void DialogSysSetting::onSysStsTableEditingFinished( QWidget *editor, QAbstractItemDelegate::EndEditHint hint )
{
    Q_UNUSED( editor );
    Q_UNUSED( hint );

    QModelIndex index = ui->tableView_device_types->currentIndex();
    QString newText = m_pItemModelSysCfgs->data( index ).toString();
    qDebug() << "Editing finished for cell (" << index.row() << "," << index.column() << ") with new content: " << newText;

    // 保存更新后的系统设置
    std::map<std::string, QJsonObject> mapSts;
    for( int i=0; i<m_pItemModelSysCfgs->rowCount(); i++ )
    {
        std::string devType = m_pItemModelSysCfgs->item( i, 0 )->text().toStdString();
        QString cmdCfgFile = m_pItemModelSysCfgs->item( i, 1 )->text();
        QString varCfgFile = m_pItemModelSysCfgs->item( i, 2 )->text();
        QJsonObject itemObj;
        itemObj["test_cmd_file"] = cmdCfgFile;
        itemObj["default_var_value_file"] = varCfgFile;
        mapSts[ devType ] = itemObj;
    }

    if( !CSysSettings::getInstance()->updateDevTypeSettings( mapSts ) )
    {
        QMessageBox::warning( this, "错误", "保存设置失败!" );
    }

    bool bDevTypeChanged = ( index.column() == 0 );
    if( bDevTypeChanged )
    {
        std::vector<std::string> allTypes = CSysSettings::getInstance()->getAllDeviceTyps();
        std::string curDevType = CSysSettings::getInstance()->getCurDeviceType();
        bool bCurDevTypeFound = std::find( allTypes.begin(), allTypes.end(), curDevType ) != allTypes.end();
        if( !bCurDevTypeFound )
        {
            CSysSettings::getInstance()->setCurDeviceType( newText );
        }
    }

    updateUIData();
}

std::shared_ptr<QStandardItemModel> &DialogSysSetting::getItemModelSysCfgs()
{
    if( !m_pItemModelSysCfgs )
    {
        m_pItemModelSysCfgs = std::make_shared<QStandardItemModel>( 1, 4, this );
    }
    return m_pItemModelSysCfgs;
}

bool DialogSysSetting::updateCurDevTypeListData()
{
    ui->comboBox_cur_device_type->clear();
    std::vector<std::string> allTypes = CSysSettings::getInstance()->getAllDeviceTyps();
    for( const auto& devType : allTypes )
    {
        ui->comboBox_cur_device_type->addItem( devType.data() );
    }

    return true;
}

void DialogSysSetting::updateCurDevType()
{
    ui->comboBox_cur_device_type->setCurrentText( CSysSettings::getInstance()->getCurDeviceType().data() );
}

void DialogSysSetting::on_pBtn_select_dev_label_tem_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择设备标签模板文件",
        "",
        "Template Files (*.tem);;All Files (*.*)"
        );

    if( !fileName.isEmpty() )
    {
        ui->lineEdit_dev_label_tem->setText( fileName );
        CSysSettings::getInstance()->setDevLabelTemplateFile( fileName );
    }
}

void DialogSysSetting::on_pBtn_select_box_label_tem_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择机盒标签模板文件",
        "",
        "Template Files (*.tem);;All Files (*.*)"
        );

    if( !fileName.isEmpty() )
    {
        ui->lineEdit_box_label_tem->setText( fileName );
        CSysSettings::getInstance()->setBoxLabelTemplateFile( fileName );
    }
}

void DialogSysSetting::on_pBtn_select_pack_label_tem_clicked()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择包装箱标签模板文件",
        "",
        "Template Files (*.tem);;All Files (*.*)"
        );

    if( !fileName.isEmpty() )
    {
        ui->lineEdit_pack_label_tem->setText( fileName );
        CSysSettings::getInstance()->setPackLabelTemplateFile( fileName );
    }
}

void DialogSysSetting::updateNetCfgData()
{
    const QJsonObject& netCfg = CSysSettings::getInstance()->getNetCfg();
    if(!netCfg.contains("mqtt_server_ip"))
        return;
    ui->lineEdit_mqtt_server_ip->setText( netCfg["mqtt_server_ip"].toString() );
    ui->lineEdit_mqtt_server_port->setText( QString( "%1" ).arg( netCfg["mqtt_server_port"].toInt() ) );
    ui->lineEdit_mqtt_user->setText( netCfg["mqtt_user"].toString() );
    ui->lineEdit_mqtt_password->setText( netCfg["mqtt_password"].toString() );
    ui->lineEdit_pos_id->setText( CSysSettings::getInstance()->getWorkPosId().c_str());

    ui->lineEdit_rtmp_server_ip->setText( netCfg["rtmp_server_ip"].toString() );
    ui->lineEdit_rtmp_server_port->setText( QString( "%1" ).arg( netCfg["rtmp_server_port"].toInt() ) );

    ui->lineEdit_http_server_ip->setText( netCfg["http_server_ip"].toString() );
    ui->lineEdit_http_server_port->setText( QString( "%1" ).arg( netCfg["http_server_port"].toInt() ) );

    ui->checkBoxLocalRtmp->setChecked(netCfg["rtmp_local_enable"].toBool());
    int nPort = netCfg["rtmp_local_port"].toInt()  ;
    if(nPort<=0)
        nPort = 1935 ;
    ui->lineEdit_rtmp_local_port->setText( QString( "%1" ).arg( nPort) );
    ui->checkBoxRtsp->setChecked(netCfg["rtsp_enable"].toBool());
}

bool DialogSysSetting::isRtspEnable()
{
    return ui->checkBoxRtsp->isChecked();
}

void DialogSysSetting::saveAllNetCfgData()
{
    CSysSettings *pSet = CSysSettings::getInstance() ;
    QJsonObject netCfg = CSysSettings::getInstance()->getNetCfg();

    netCfg["mqtt_server_ip"] = ui->lineEdit_mqtt_server_ip->text().trimmed();
    netCfg["mqtt_server_port"] = ui->lineEdit_mqtt_server_port->text().toInt();
    netCfg["mqtt_user"] = ui->lineEdit_mqtt_user->text().trimmed();
    netCfg["mqtt_password"] = ui->lineEdit_mqtt_password->text().trimmed();
    pSet->setWorkPosId(ui->lineEdit_pos_id->text().trimmed().toStdString());

    netCfg["rtmp_server_ip"] = ui->lineEdit_rtmp_server_ip->text().trimmed();
    netCfg["rtmp_server_port"] = ui->lineEdit_rtmp_server_port->text().trimmed().toInt();

    netCfg["http_server_ip"] = ui->lineEdit_http_server_ip->text().trimmed();
    netCfg["http_server_port"] = ui->lineEdit_http_server_port->text().trimmed().toInt();

    netCfg["rtmp_local_enable"] = ui->checkBoxLocalRtmp->isChecked();
    netCfg["rtmp_local_port"] = ui->lineEdit_rtmp_local_port->text().trimmed().toInt();

    netCfg["rtsp_enable"] = ui->checkBoxRtsp->isChecked();

    int nRows=0;
    int nCols=0;

    nRows=ui->lineEditDevRows->text().trimmed().toInt();
    nCols=ui->lineEditDevCols->text().trimmed().toInt();
    pSet->setPrintMatrix(0,nRows,nCols) ;

    nRows=ui->lineEditBoxRows->text().trimmed().toInt();
    nCols=ui->lineEditBoxCols->text().trimmed().toInt();
    pSet->setPrintMatrix(1,nRows,nCols) ;

    nRows=ui->lineEditPackRows->text().trimmed().toInt();
    nCols=ui->lineEditPackCols->text().trimmed().toInt();
    pSet->setPrintMatrix(2,nRows,nCols) ;

    if( !CSysSettings::getInstance()->setNetCfg( netCfg ) )
    {
        QMessageBox::warning( this, "错误", "保存参数错误!" );
    }
}

void DialogSysSetting::updateLabelTemCfgData()
{
    const QJsonObject& temCfg = CSysSettings::getInstance()->getLabelTemCfg();
    ui->lineEdit_dev_label_tem->setText( temCfg["devLabelTem"].toString() );
    ui->lineEdit_box_label_tem->setText( temCfg["boxLabelTem"].toString() );
    ui->lineEdit_pack_label_tem->setText( temCfg["packLabelTem"].toString() );

    CSysSettings *pSet = CSysSettings::getInstance() ;
    int nRows=0;
    int nCols=0;

    pSet->getPrintMatrix(0,nRows,nCols) ;
    ui->lineEditDevRows->setText(QString::number(nRows));
    ui->lineEditDevCols->setText(QString::number(nCols));

    pSet->getPrintMatrix(1,nRows,nCols) ;
    ui->lineEditBoxRows->setText(QString::number(nRows));
    ui->lineEditBoxCols->setText(QString::number(nCols));

    pSet->getPrintMatrix(2,nRows,nCols) ;
    ui->lineEditPackRows->setText(QString::number(nRows));
    ui->lineEditPackCols->setText(QString::number(nCols));
}


void DialogSysSetting::closeEvent( QCloseEvent *event )
{
    Q_UNUSED( event )
    saveAllNetCfgData();
    emit sigWindowHidden();
}


void DialogSysSetting::on_pbtn_add_save_config_clicked()
{
    saveAllNetCfgData();
    QMessageBox::information(this,"提示","配置参数已保存！") ;
}

