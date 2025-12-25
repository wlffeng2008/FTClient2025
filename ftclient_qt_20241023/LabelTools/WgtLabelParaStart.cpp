#include "WgtLabelParaStart.h"
#include "ui_WgtLabelParaStart.h"

WgtLabelParaStart::WgtLabelParaStart(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::WgtLabelParaStart)
{
    ui->setupUi(this);
}

WgtLabelParaStart::~WgtLabelParaStart()
{
    delete ui;
}

void WgtLabelParaStart::setPara( const label::LabelPara &p )
{
    m_para = p;
    updateParaOnUI();
}

void WgtLabelParaStart::on_pBtn_cancel_clicked()
{
    emit sigStartEdit();
    this->close();
}

void WgtLabelParaStart::on_pBtn_create_clicked()
{
    label::LabelPara p;
    p.iLabelHeight_mm = ui->lineEdit_label_height->text().toInt();
    p.iLabelWidth_mm = ui->lineEdit_label_width->text().toInt();
    emit sigPara( p );
    close();
}

void WgtLabelParaStart::updateParaOnUI()
{
    ui->lineEdit_label_width->setText( QString( "%1" ).arg( m_para.iLabelWidth_mm ) );
    ui->lineEdit_label_height->setText( QString( "%1" ).arg( m_para.iLabelHeight_mm ) );
}
