#include "DefaultValuesWidget.h"
#include "ui_DefaultValuesWidget.h"
#include "ButtonDelegate.h"

#include <QDebug>
#include <QMessageBox>

static int ITEM_COLUMN_VAR_NOTE = 0;            // "变量说明"
static int ITEM_COLUMN_VAR_VALUE = 1;           // "变量值"
static int ITEM_COLUMN_VAR_GROUP = 2;            // "默认配置组"
static int ITEM_COLUMN_VAR_NAME = 3;            // "变量名"
static int ITEM_COLUMN_VAR_WRITE_RESULT = 4;    // "写入结果"
static int ITEM_COLUMN_VAR_ENABLE = 5;          // "启用该项"
static int ITEM_COLUMN_VAR_WRITE = 6;           // "写入该项"


DefaultValuesWidget::DefaultValuesWidget( QWidget *parent )
    : QWidget( parent )
    , ui( new Ui::DefaultValuesWidget )
{
    ui->setupUi( this );
    initTable();
    on_pbtn_reset_clicked();
    ui->checkBox_enable_all_items->setCheckState( Qt::Checked );
}

DefaultValuesWidget::~DefaultValuesWidget()
{
    delete ui;
}

void DefaultValuesWidget::initTable()
{
    m_pItemModel = new QStandardItemModel( 1, 7, this );

    m_pItemModel->setHeaderData( ITEM_COLUMN_VAR_NOTE, Qt::Horizontal, "变量说明" );
    m_pItemModel->setHeaderData( ITEM_COLUMN_VAR_VALUE, Qt::Horizontal, "变量值" );
    m_pItemModel->setHeaderData( ITEM_COLUMN_VAR_GROUP, Qt::Horizontal, "默认配置组" );
    m_pItemModel->setHeaderData( ITEM_COLUMN_VAR_NAME, Qt::Horizontal, "变量名");
    m_pItemModel->setHeaderData( ITEM_COLUMN_VAR_WRITE_RESULT, Qt::Horizontal, "写入结果" );
    m_pItemModel->setHeaderData( ITEM_COLUMN_VAR_ENABLE, Qt::Horizontal, "启用" );
    m_pItemModel->setHeaderData( ITEM_COLUMN_VAR_WRITE, Qt::Horizontal, "写入" );

    ui->tableView_default_values->setModel( m_pItemModel);

    ui->tableView_default_values->horizontalHeader()->setStretchLastSection( true );
    ui->tableView_default_values->setSelectionBehavior(QAbstractItemView::SelectRows);

    int totalWidth = ui->tableView_default_values ->viewport()->width();         // 可见区域宽度
    int sum_w = 0;
    int col1Width = 80;     sum_w += col1Width;                                  // 变量值
    int col2Width = 70;     sum_w += col2Width;                                  // 变量类型
    int col4Width = 60;     sum_w += col4Width;                                  // 写入结果
    int col5Width = 60;     sum_w += col5Width;                                  // 是否启用
    int col6Width = 60;     sum_w += col6Width;                                  // 按钮
    int equitQidth = ( totalWidth - sum_w ) * 1 / 2.0f;
    int col0Width = equitQidth;                                     // 变量说明
    int col3Width = totalWidth - sum_w - col0Width;               // 变量名

    ui->tableView_default_values->setColumnWidth( 0, col0Width );
    ui->tableView_default_values->setColumnWidth( 1, col1Width );
    ui->tableView_default_values->setColumnWidth( 2, col2Width );
    ui->tableView_default_values->setColumnWidth( 3, col3Width );
    ui->tableView_default_values->setColumnWidth( 4, col4Width );
    ui->tableView_default_values->setColumnWidth( 5, col5Width );
    ui->tableView_default_values->setColumnWidth( 6, col6Width );
}


// QJsonObject:
// {
//      "var_name": "name",
//      "var_value": "value",
//      "var_data_type": "data_type",
//      "var_note": "note"
// }
void DefaultValuesWidget::updateItemData( const std::vector<QJsonObject> &vecItems )
{
    while( m_pItemModel->rowCount() > 0 )
    {
        m_pItemModel->removeRow( 0 );
    }

    int nRow = 0;
    for( const auto &item : vecItems )
    {
        const QJsonObject &jsonObj = item;
        if( jsonObj["var_name"].toString().trimmed().isEmpty() )
            continue;

        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_NOTE, new QStandardItem( jsonObj["var_note"].toString() ) );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_VALUE, new QStandardItem( jsonObj["var_value"].toString() ) );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_GROUP, new QStandardItem( jsonObj["var_group"].toString() ) );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_NAME, new QStandardItem( jsonObj["var_name"].toString() ) );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_WRITE_RESULT, new QStandardItem( "" ) );

        QStandardItem *checkItem = new QStandardItem();
        checkItem->setCheckable( true );
        checkItem->setCheckState( Qt::Checked );
        checkItem->setTextAlignment(Qt::AlignCenter) ;
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_ENABLE, checkItem );

        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_WRITE, new QStandardItem("") );

        ui->tableView_default_values->setRowHeight( nRow, 25 );
        nRow++;
    }

    static ButtonDelegate *delegate = nullptr;
    if(!delegate)
    {
        delegate = new ButtonDelegate( "写入该项", this );
        ui->tableView_default_values->setItemDelegateForColumn( ITEM_COLUMN_VAR_WRITE, delegate );
        connect( delegate,  &ButtonDelegate::buttonClicked, this, &DefaultValuesWidget::handleItemButtonClicked );
    }
    int nCount = m_pItemModel->rowCount() ;
    for(int i=0; i<nCount; i++)
    {
        for(int j=0; j<6; j++)
        {
            m_pItemModel->item(i,j)->setEditable(j == ITEM_COLUMN_VAR_VALUE || j == ITEM_COLUMN_VAR_GROUP) ;
        }
    }
}



// 执行当前测试项
void DefaultValuesWidget::handleItemButtonClicked( const QModelIndex &index )
{
    qDebug() << "handleItemButtonClicked,row: " << index.row();
    int row = index.row();
    writeAnItemVar( row );
}


void DefaultValuesWidget::on_pbtn_write_clicked()
{
    QModelIndexList selIndexs = ui->tableView_default_values->selectionModel()->selectedIndexes() ;
    if(selIndexs.count())
    {
        writeAnItemVar(selIndexs[0].row());
        return  ;
    }
    QMessageBox::warning( this, "错误", "请选中一项数据!" );
}

void DefaultValuesWidget::writeAnItemVar( int row_index )
{
    int nWCount = 0 ;
    QJsonArray jGroup;
    for( int iRow = 0; iRow < m_pItemModel->rowCount(); iRow++ )
    {
        QStandardItem *checkItem = m_pItemModel->item( iRow, ITEM_COLUMN_VAR_ENABLE );
        if((row_index == -1 && checkItem->checkState() == Qt::Checked) || row_index == iRow)
        {
            QStandardItem *item_key   = m_pItemModel->item( iRow, ITEM_COLUMN_VAR_NAME );
            QStandardItem *item_group = m_pItemModel->item( iRow, ITEM_COLUMN_VAR_GROUP );
            QStandardItem *item_value = m_pItemModel->item( iRow, ITEM_COLUMN_VAR_VALUE );

            QJsonObject jItem;
            jItem["group"] = item_group->text();
            jItem["key"] = item_key->text();
            jItem["value"] = item_value->text();
            jGroup.append(jItem) ;

            nWCount++ ;
        }
    }

    if(nWCount>0)
        emit writeDefaultValue(jGroup) ;
    else
        QMessageBox::warning(nullptr,"提示","请勾选需要写入的数据项！");
}

void DefaultValuesWidget::on_pbtn_write_all_clicked()
{
    writeAnItemVar(-1) ;
}

void DefaultValuesWidget::on_pbtn_reset_clicked()
{
    for( int row = 0; row < m_pItemModel->rowCount(); ++row )
    {
        m_pItemModel->item( row, ITEM_COLUMN_VAR_WRITE_RESULT)->setText("");
    }
    ui->label_status->setText("待写入");
}

void DefaultValuesWidget::on_checkBox_enable_all_items_checkStateChanged( Qt::CheckState state )
{
    bool bEnableAll = ( state == Qt::Checked );
    for( int iRow = 0; iRow < m_pItemModel->rowCount(); iRow++ )
    {
        QStandardItem *checkItem = m_pItemModel->item( iRow, ITEM_COLUMN_VAR_ENABLE );
        if( checkItem )
        {
            checkItem->setCheckState( bEnableAll ? Qt::Checked : Qt::Unchecked );
        }
    }
}



