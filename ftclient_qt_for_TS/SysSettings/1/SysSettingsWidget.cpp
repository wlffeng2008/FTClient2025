#include "SysSettingsWidget.h"
#include "ui_SysSettingsWidget.h"
#include "ButtonDelegate.h"
#include "CHttpClientAgent.h"
#include "CLabelEdit.h"
#include "CommonLib.h"
#include <QMessageBox>
#include <QFileDialog>
#include <QMenu>
#include <QAction>
#include <QItemDelegate>
#include <QDir>
#include <QDebug>


SysSettingsWidget::SysSettingsWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::SysSettingsWidget ), m_bDevideTypeUpdating( true )
{
    ui->setupUi( this );
    initSysCfgListUI();

    ui->tableView_device_types->setEditTriggers( QAbstractItemView::DoubleClicked );
    connect( ui->tableView_device_types->itemDelegate(), &QItemDelegate::closeEditor, this, &SysSettingsWidget::onSysStsTableEditingFinished );

    updateSysCfgListData();
    updateCurDevTypeListData();
    updateCurDevType();
    updateNetCfgData();
    updateLabelTemCfgData();
}

SysSettingsWidget::~SysSettingsWidget()
{
    delete ui;
}

void SysSettingsWidget::setCurDeviceType( const QString &newDeviceType )
{
    CSysSettings::getInstance()->setCurDeviceType( newDeviceType );
    updateCurDevType();
}

void SysSettingsWidget::setDevBoundStatus( bool bBound )
{
    //ui->lineEdit_pos_id->setReadOnly( bBound );
}

void SysSettingsWidget::updateUIData()
{
    updateCurDevTypeListData();
    updateSysCfgListData();
    updateCurDevType();
}

void SysSettingsWidget::on_pbtn_add_device_type_clicked()
{
    std::string devType = ui->lineEdit_device_type->text().trimmed().toStdString();
    if( devType.empty() )
    {
        QMessageBox::warning( this, "失败", "设备型号不能为空" );
        return;
    }

    // 创建设备型号文件夹
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
    if( CSysSettings::getInstance()->addDevType( devType, jsonItem ) )
    {
        qDebug() << "devType " << devType.data() << " added ok.";
        updateUIData();
        QMessageBox::information( this, "成功", "设备型号添加成功" );
    }
    else
    {
        qDebug() << "devType " << devType.data() << " added error.";
        QMessageBox::warning( this, "失败", "设备型号添加失败" );
    }
}

void SysSettingsWidget::on_pushButton_select_cfg_file_clicked()
{
    qDebug() << "on_pushButton_select_cfg_file_clicked..";

    // 打开文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择测试指令配置文件",       // 对话框标题
        "",                // 初始目录 ("" 表示当前目录)
        "Text Files (*.cmd);;All Files (*)"  // 过滤器
        );

    // 检查用户是否选择了文件
    if( !fileName.isEmpty() )
    {
        ui->lineEdit_config_file_path->setText( fileName );
    }
}

void SysSettingsWidget::on_pushButton_select_default_var_file_clicked()
{
    qDebug() << "on_pushButton_select_default_var_file_clicked..";

    // 打开文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择默认值配置文件",       // 对话框标题
        "",                // 初始目录 ("" 表示当前目录)
        "Text Files (*.var);;All Files (*)"  // 过滤器
        );

    // 检查用户是否选择了文件
    if( !fileName.isEmpty() )
    {
        ui->lineEdit_default_var_file_path->setText( fileName );
    }
}

void SysSettingsWidget::on_comboBox_cur_device_type_currentTextChanged( const QString &text )
{
    qDebug() << "on_comboBox_cur_device_type_currentTextChanged.." << text;

    if( !m_bDevideTypeUpdating )
    {
        CSysSettings::getInstance()->setCurDeviceType( text );
    }
}

void SysSettingsWidget::resetSysCfgListModel()
{
    while( getItemModelSysCfgs()->rowCount() > 0 )
    {
        getItemModelSysCfgs()->removeRow( 0 );
    }
}

void SysSettingsWidget::initSysCfgListUI()
{
    // 设置表头
    getItemModelSysCfgs()->setHeaderData( 0, Qt::Horizontal, "设备类型" );
    getItemModelSysCfgs()->setHeaderData( 1, Qt::Horizontal, "测试指令配置文件" );
    getItemModelSysCfgs()->setHeaderData( 2, Qt::Horizontal, "默认值配置文件" );
    getItemModelSysCfgs()->setHeaderData( 3, Qt::Horizontal, "操作" );

    // 创建 QTableView 并绑定数据模型
    ui->tableView_device_types->setModel( getItemModelSysCfgs().get() );
    ui->tableView_device_types->horizontalHeader()->setStretchLastSection( true );
}

bool SysSettingsWidget::updateSysCfgListData()
{
    resetSysCfgListModel();

    int index = 0;
    std::map<std::string,QJsonObject> map_para = CSysSettings::getInstance()->getDevTypeSettings();
    for( const auto& pair : map_para )
    {
        getItemModelSysCfgs()->setItem( index, 0, new QStandardItem( pair.first.data() ) );
        getItemModelSysCfgs()->setItem( index, 1, new QStandardItem( pair.second["test_cmd_file"].toString() ) );
        getItemModelSysCfgs()->setItem( index, 2, new QStandardItem( pair.second["default_var_value_file"].toString() ) );
        getItemModelSysCfgs()->setItem( index, 3, new QStandardItem( "删除" ) );

        // 设置行高
        ui->tableView_device_types->setRowHeight( index, 25 );
        index++;
    }

    // 设置自定义委托 - 每一行的删除按钮
    ButtonDelegate *delegate = new ButtonDelegate( "删除该行", this );
    ui->tableView_device_types->setItemDelegateForColumn(3, delegate); // 将按钮放在第3列
    connect( delegate, &ButtonDelegate::buttonClicked, this, &SysSettingsWidget::handleItemButtonClicked );

    return true;
}

void SysSettingsWidget::updateSysCfgListTableSize()
{
    // 计算各列宽度的比例
    int totalWidth = ui->tableView_device_types->viewport()->width();    // 可见区域宽度
    int sum_w = 0;

    int col3Width = 80;
    sum_w += col3Width;

    int col0Width = ( totalWidth - col3Width ) * 1 / 5.0f;
    sum_w += col0Width;               // 第一列宽度占 1/3

    int col1Width = ( totalWidth - sum_w ) / 2.0f;                  // 第二列宽度占 2/3
    sum_w += col1Width;

    int col2Width = totalWidth - sum_w;                  // 第二列宽度占 2/3

    // 设置各列宽度
    ui->tableView_device_types->setColumnWidth( 0, col0Width );
    ui->tableView_device_types->setColumnWidth( 1, col1Width );
    ui->tableView_device_types->setColumnWidth( 2, col2Width );
    ui->tableView_device_types->setColumnWidth( 3, col3Width );
}

void SysSettingsWidget::handleItemButtonClicked( const QModelIndex &index )
{
    qDebug() << "Button clicked at row" << index.row() << "column" << index.column();
    if( index.row() >= 0 )
    {
        QMessageBox::StandardButton reply;
        reply = QMessageBox::question( this, "确认操作", "你确定要删除这条记录吗？",
                                      QMessageBox::Yes | QMessageBox::No);

        if( reply == QMessageBox::Yes ) {
            qDebug() << "User chose Yes. Proceeding with deletion.";

            QString curDevType = getItemModelSysCfgs()->item( index.row(), 0 )->text();
            CSysSettings::getInstance()->removeDevType( curDevType.toStdString() );
            updateUIData();

        } else {
            qDebug() << "User chose No. Deletion canceled.";
        }
    }
}

void SysSettingsWidget::onSysStsTableEditingFinished( QWidget *editor, QAbstractItemDelegate::EndEditHint hint )
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
        QMessageBox::warning( this, "错误", "保存设置失败" );
    }

    // 更新当前设备类型
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

    // 更新 UI 数据
    updateUIData();
}

std::shared_ptr<QStandardItemModel> &SysSettingsWidget::getItemModelSysCfgs()
{
    if( nullptr == m_pItemModelSysCfgs )
    {
        m_pItemModelSysCfgs = std::make_shared<QStandardItemModel>( 1, 4, this );
    }
    return m_pItemModelSysCfgs;
}

bool SysSettingsWidget::updateCurDevTypeListData()
{
    m_bDevideTypeUpdating = true;

    ui->comboBox_cur_device_type->clear();
    std::vector<std::string> allTypes = CSysSettings::getInstance()->getAllDeviceTyps();
    for( const auto& devType : allTypes )
    {
        ui->comboBox_cur_device_type->addItem( devType.data() );
    }

    m_bDevideTypeUpdating = false;
    return true;
}

void SysSettingsWidget::updateCurDevType()
{
    m_bDevideTypeUpdating = true;
    ui->comboBox_cur_device_type->setCurrentText( CSysSettings::getInstance()->getCurDeviceType().data() );
    m_bDevideTypeUpdating = false;
}


void SysSettingsWidget::on_lineEdit_rtmp_server_ip_editingFinished()
{
    if( !ui->lineEdit_rtmp_server_ip->text().trimmed().isEmpty() )
    {
        QJsonObject netCfg = CSysSettings::getInstance()->getNetCfg();
        netCfg["rtmp_server_ip"] = ui->lineEdit_rtmp_server_ip->text().trimmed();
        saveAllNetCfgData();
    }
}

void SysSettingsWidget::on_lineEdit_rtmp_server_port_editingFinished()
{
    if( !ui->lineEdit_rtmp_server_port->text().trimmed().isEmpty() )
    {
        QJsonObject netCfg = CSysSettings::getInstance()->getNetCfg();
        netCfg["rtmp_server_port"] = ui->lineEdit_rtmp_server_port->text().toInt();
        saveAllNetCfgData();
    }
}

void SysSettingsWidget::on_lineEdit_rtmp_delay_ms_editingFinished()
{
    if( !ui->lineEdit_rtmp_delay_ms->text().trimmed().isEmpty() )
    {
        QJsonObject netCfg = CSysSettings::getInstance()->getNetCfg();
        netCfg["rtmp_delay_ms"] = ui->lineEdit_rtmp_delay_ms->text().toInt();
        saveAllNetCfgData();
    }
}

void SysSettingsWidget::on_pBtn_select_dev_label_tem_clicked()
{
    qDebug() << "on_pBtn_select_dev_label_tem_clicked..";

    // 打开文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择设备标签模板文件",       // 对话框标题
        "",                // 初始目录 ("" 表示当前目录)
        "Template Files (*.tem);;All Files (*.*)"  // 过滤器
        );

    // 检查用户是否选择了文件
    if( !fileName.isEmpty() )
    {
        ui->lineEdit_dev_label_tem->setText( fileName );
        CSysSettings::getInstance()->setDevLabelTemplateFile( fileName );
    }
}

void SysSettingsWidget::on_pBtn_select_box_label_tem_clicked()
{
    qDebug() << "on_pBtn_select_box_label_tem_clicked..";

    // 打开文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择机盒标签模板文件",       // 对话框标题
        "",                // 初始目录 ("" 表示当前目录)
        "Template Files (*.tem);;All Files (*.*)"  // 过滤器
        );

    // 检查用户是否选择了文件
    if( !fileName.isEmpty() )
    {
        ui->lineEdit_box_label_tem->setText( fileName );
        CSysSettings::getInstance()->setBoxLabelTemplateFile( fileName );
    }
}

void SysSettingsWidget::on_pBtn_select_pack_label_tem_clicked()
{
    qDebug() << "on_pBtn_select_pack_label_tem_clicked..";

    // 打开文件选择对话框
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择包装箱标签模板文件",       // 对话框标题
        "",                // 初始目录 ("" 表示当前目录)
        "Template Files (*.tem);;All Files (*.*)"  // 过滤器
        );

    // 检查用户是否选择了文件
    if( !fileName.isEmpty() )
    {
        ui->lineEdit_pack_label_tem->setText( fileName );
        CSysSettings::getInstance()->setPackLabelTemplateFile( fileName );
    }
}

void SysSettingsWidget::on_lineEdit_dev_label_tem_editingFinished()
{
    saveAllLabelTemCfgData();
}

void SysSettingsWidget::on_lineEdit_box_label_tem_editingFinished()
{
    saveAllLabelTemCfgData();
}

void SysSettingsWidget::on_lineEdit_pack_label_tem_editingFinished()
{
    saveAllLabelTemCfgData();
}

void SysSettingsWidget::on_lineEdit_pos_id_editingFinished()
{
    CSysSettings::getInstance()->setWorkPosId(ui->lineEdit_pos_id->text().trimmed().toStdString());
}

void SysSettingsWidget::updateNetCfgData()
{
    const QJsonObject& netCfg = CSysSettings::getInstance()->getNetCfg();
    if(!netCfg.contains("mqtt_server_ip"))
        return;

    ui->lineEdit_mqtt_server_ip->setText( netCfg["mqtt_server_ip"].toString() );
    ui->lineEdit_mqtt_server_port->setText( QString( "%1" ).arg( netCfg["mqtt_server_port"].toInt() ) );

    ui->lineEdit_http_server_ip->setText( netCfg["http_server_ip"].toString() );
    ui->lineEdit_http_server_port->setText( QString( "%1" ).arg( netCfg["http_server_port"].toInt() ) );

    ui->lineEdit_mqtt_user->setText( netCfg["mqtt_user"].toString() );
    ui->lineEdit_mqtt_password->setText( netCfg["mqtt_password"].toString() );

    ui->lineEdit_rtmp_server_ip->setText( netCfg["rtmp_server_ip"].toString() );
    ui->lineEdit_rtmp_server_port->setText( QString( "%1" ).arg( netCfg["rtmp_server_port"].toInt() ) );
    ui->lineEdit_rtmp_delay_ms->setText( QString( "%1" ).arg( netCfg["rtmp_delay_ms"].toInt() ) );
    ui->lineEdit_pos_id->setText( CSysSettings::getInstance()->getWorkPosId().c_str());
}

void SysSettingsWidget::saveAllNetCfgData()
{
    qDebug()<<"saveAllNetCfgData-------------";
    QJsonObject netCfg = CSysSettings::getInstance()->getNetCfg();

    netCfg["mqtt_server_ip"] = ui->lineEdit_mqtt_server_ip->text().trimmed();
    netCfg["mqtt_server_port"] = ui->lineEdit_mqtt_server_port->text().toInt();

    netCfg["mqtt_user"] = ui->lineEdit_mqtt_user->text().trimmed();
    netCfg["mqtt_password"] = ui->lineEdit_mqtt_password->text().trimmed();

    netCfg["rtmp_server_ip"] = ui->lineEdit_rtmp_server_ip->text().trimmed();
    netCfg["rtmp_server_port"] = ui->lineEdit_rtmp_server_port->text().trimmed().toInt();
    netCfg["rtmp_delay_ms"] = ui->lineEdit_rtmp_delay_ms->text().trimmed().toInt();

    netCfg["http_server_ip"] = ui->lineEdit_http_server_ip->text().trimmed();
    netCfg["http_server_port"] = ui->lineEdit_http_server_port->text().trimmed().toInt();

    CSysSettings::getInstance()->setWorkPosId(ui->lineEdit_pos_id->text().trimmed().toStdString());
    if( !CSysSettings::getInstance()->setNetCfg( netCfg ) )
    {
        QMessageBox::warning( this, "错误", "保存参数错误" );
    }
}

void SysSettingsWidget::updateLabelTemCfgData()
{
    const QJsonObject& temCfg = CSysSettings::getInstance()->getLabelTemCfg();
    ui->lineEdit_dev_label_tem->setText( temCfg["devLabelTem"].toString() );
    ui->lineEdit_box_label_tem->setText( temCfg["boxLabelTem"].toString() );
    ui->lineEdit_pack_label_tem->setText( temCfg["packLabelTem"].toString() );
}

void SysSettingsWidget::saveAllLabelTemCfgData()
{
    QJsonObject temCfg = CSysSettings::getInstance()->getLabelTemCfg();
    temCfg["devLabelTem"] = ui->lineEdit_dev_label_tem->text();
    temCfg["boxLabelTem"] = ui->lineEdit_box_label_tem->text();
    temCfg["packLabelTem"] = ui->lineEdit_pack_label_tem->text();
    if( !CSysSettings::getInstance()->setLabelTemCfg( temCfg ) )
    {
        QMessageBox::warning( this, "错误", "保存参数错误" );
    }
}

void SysSettingsWidget::closeEvent( QCloseEvent *event )
{
    Q_UNUSED( event )
    // emit sigWindowHidden();
}

void SysSettingsWidget::paintEvent( QPaintEvent *event )
{
    Q_UNUSED( event )
    updateSysCfgListTableSize();
    updateCurDevType();
}

void SysSettingsWidget::on_lineEdit_mqtt_password_editingFinished()
{
    saveAllNetCfgData();
}


void SysSettingsWidget::on_lineEdit_mqtt_user_editingFinished()
{
    saveAllNetCfgData();
}


void SysSettingsWidget::on_lineEdit_mqtt_server_port_editingFinished()
{
    saveAllNetCfgData();
}

void SysSettingsWidget::on_lineEdit_mqtt_server_ip_editingFinished()
{
    saveAllNetCfgData();
}

void SysSettingsWidget::on_lineEdit_http_server_port_editingFinished()
{
    saveAllNetCfgData();
}


void SysSettingsWidget::on_lineEdit_http_server_ip_editingFinished()
{
    saveAllNetCfgData();
}

void SysSettingsWidget::on_pushButton_clicked()
{
    saveAllNetCfgData();
    emit sigWindowHidden();
    QMessageBox::information( this, "提示", "参数设置已成功保存！" );
}

void SysSettingsWidget::setOrderId(const QString&strOrderId)
{
    CSysSettings::getInstance()->setOrderId(strOrderId);
}

QString SysSettingsWidget::getOrderid()
{
    return CSysSettings::getInstance()->getOrderId() ;
}
