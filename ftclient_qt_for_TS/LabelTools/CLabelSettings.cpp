#include "CLabelSettings.h"
#include "ui_CLabelSettings.h"

CLabelSettings::CLabelSettings(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::CLabelSettings)
{
    ui->setupUi( this );
    setWindowFlags(windowFlags()|Qt::Popup|Qt::Dialog) ;
}

CLabelSettings::~CLabelSettings()
{
    delete ui;
}

void CLabelSettings::on_pBtn_ok_clicked()
{
    m_para.iLabelWidth_mm = ui->lineEdit_label_width->text().toInt();
    m_para.iLabelHeight_mm = ui->lineEdit_label_height->text().toInt();
    m_para.fZoomScale = ui->lineEdit_scale->text().toFloat();
    m_para.strTempl = ui->lineEdit_qrtempl->text().trimmed();
    emit sigNewPara( m_para );
    this->close();
}

void CLabelSettings::on_pBtn_cancel_clicked()
{
    this->close();
}

void CLabelSettings::applyPara()
{
    ui->lineEdit_label_width->setText( QString( "%1" ).arg( m_para.iLabelWidth_mm ) );
    ui->lineEdit_label_height->setText( QString( "%1" ).arg( m_para.iLabelHeight_mm ) );
    ui->lineEdit_scale->setText( QString( "%1" ).arg( m_para.fZoomScale ) );
    ui->lineEdit_qrtempl->setText( QString( "%1" ).arg( m_para.strTempl ) );
}

