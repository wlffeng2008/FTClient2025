#include "DialogDefaultValues.h"
#include "ui_DialogDefaultValues.h"
#include "ButtonDelegate.h"

#include <QMessageBox>
#include <QDebug>
#include <QJsonObject>
#include <QJsonArray>

DialogDefaultValues::DialogDefaultValues(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogDefaultValues)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags()|Qt::MSWindowsFixedSizeDialogHint);

    initValuesTable();
}

DialogDefaultValues::~DialogDefaultValues()
{
    delete ui;
}

typedef enum
{
    ITEM_COLUMN_VAR_NOTE ,
    ITEM_COLUMN_VAR_VALUE ,
    ITEM_COLUMN_VAR_TYPE ,
    ITEM_COLUMN_VAR_NAME ,
    ITEM_COLUMN_VAR_GROUP ,
    ITEM_COLUMN_VAR_RESULT ,
    ITEM_COLUMN_VAR_ENABLE ,
    ITEM_COLUMN_VAR_WRITE ,
}VarColmnType;

static QMap<int,QString> VarColmns=
    {
        {ITEM_COLUMN_VAR_NOTE ,   "变量说明"},
        {ITEM_COLUMN_VAR_VALUE ,  "变量值"},
        {ITEM_COLUMN_VAR_TYPE ,   "变量类型"},
        {ITEM_COLUMN_VAR_NAME ,   "变量名"},
        {ITEM_COLUMN_VAR_GROUP ,  "变量组"},
        {ITEM_COLUMN_VAR_RESULT , "写入结果"},
        {ITEM_COLUMN_VAR_ENABLE , "启用该项"},
        {ITEM_COLUMN_VAR_WRITE ,  "写入该项"}
};

void DialogDefaultValues::initValuesTable()
{
    m_pItemModel = new QStandardItemModel ( 0, 8, this );
    for(int i=0; i<8; i++)
        m_pItemModel->setHeaderData( i, Qt::Horizontal, VarColmns[i] );

    ui->tableView_default_values->setModel( m_pItemModel );
    ui->tableView_default_values->horizontalHeader()->setStretchLastSection( true );
    QHeaderView *header = ui->tableView_default_values->horizontalHeader() ;
    header->setSectionResizeMode(QHeaderView::Fixed) ;
    header->setSectionResizeMode(0,QHeaderView::Stretch) ;
    header->resizeSection(ITEM_COLUMN_VAR_NAME,160) ;
    header->resizeSection(ITEM_COLUMN_VAR_TYPE,60) ;
    header->resizeSection(ITEM_COLUMN_VAR_GROUP,60) ;
    header->resizeSection(ITEM_COLUMN_VAR_VALUE,60) ;
    header->resizeSection(ITEM_COLUMN_VAR_RESULT,60) ;
    header->resizeSection(ITEM_COLUMN_VAR_ENABLE,60) ;
    header->resizeSection(ITEM_COLUMN_VAR_WRITE,60) ;
}

void DialogDefaultValues::updateItemData( const std::vector<QJsonObject> &vecItems )
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

        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_NOTE,  new QStandardItem(jsonObj["var_note"].toString().trimmed() ) );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_VALUE, new QStandardItem(jsonObj["var_value"].toString().trimmed() ) );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_GROUP, new QStandardItem(jsonObj["var_group"].toString().trimmed() ) );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_TYPE,  new QStandardItem(jsonObj["var_data_type"].toString().trimmed() ) );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_NAME,  new QStandardItem(jsonObj["var_name"].toString().trimmed() ) );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_RESULT, new QStandardItem( "" ) );

        QStandardItem *checkItem = new QStandardItem();
        checkItem->setCheckable( true );
        checkItem->setCheckState( Qt::Checked );
        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_ENABLE, checkItem );

        m_pItemModel->setItem( nRow, ITEM_COLUMN_VAR_WRITE, new QStandardItem("") );

        ui->tableView_default_values->setRowHeight( nRow, 30 );
        nRow++;
    }

    ui->checkBox_enable_all_items->setChecked(true) ;

    static ButtonDelegate *delegate = nullptr;
    if(!delegate)
    {
        delegate = new ButtonDelegate( "写入该项", this );
        ui->tableView_default_values->setItemDelegateForColumn( ITEM_COLUMN_VAR_WRITE, delegate );
        connect( delegate,  &ButtonDelegate::buttonClicked, this, &DialogDefaultValues::handleItemButtonClicked );
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

void DialogDefaultValues::handleItemButtonClicked( const QModelIndex &index )
{
    writeItemVars( index.row() );
}


void DialogDefaultValues::on_pbtn_write_clicked()
{
    writeItemVars( -1 ) ;
}

void DialogDefaultValues::writeItemVars( int row_index )
{
    QJsonArray jSend ;
    for(int i=0; i<m_pItemModel->rowCount(); i++)
    {
        QStandardItem *item_var_name  = m_pItemModel->item( i, ITEM_COLUMN_VAR_NAME );
        QStandardItem *item_var_group = m_pItemModel->item( i, ITEM_COLUMN_VAR_GROUP );
        QStandardItem *item_var_type  = m_pItemModel->item( i, ITEM_COLUMN_VAR_TYPE );
        QStandardItem *item_var_value = m_pItemModel->item( i, ITEM_COLUMN_VAR_VALUE );

        if( (row_index < 0 && m_pItemModel->item( i, ITEM_COLUMN_VAR_ENABLE )->checkState() != Qt::Checked) || row_index == i)
        {
            QJsonObject paraObj;
            paraObj["var_group"] = item_var_group->text();
            paraObj["var_name"]  = item_var_name->text();
            paraObj["var_type"]  = item_var_type->text();
            paraObj["var_value"] = item_var_value->text();
            jSend.append(paraObj) ;
        }
    }

    if(jSend.isEmpty())
        emit writeDefaultValue(jSend) ;
    else
        QMessageBox::warning(this,"提示","没有写入数据！");
}

void DialogDefaultValues::on_pbtn_reset_clicked()
{
    for( int row = 0; row < m_pItemModel->rowCount(); ++row )
    {
        m_pItemModel->item( row, ITEM_COLUMN_VAR_RESULT)->setText("");
    }
    ui->label_status->setText("待写入");
}

void DialogDefaultValues::on_checkBox_enable_all_items_checkStateChanged( Qt::CheckState state )
{
    bool bEnableAll = ( state == Qt::Checked );
    for( int iRow = 0; iRow < m_pItemModel->rowCount(); iRow++ )
    {
        QStandardItem *checkItem = m_pItemModel->item( iRow, ITEM_COLUMN_VAR_ENABLE );
        checkItem->setCheckState( bEnableAll ? Qt::Checked : Qt::Unchecked );
    }
}
