#include "dialoguserlogin.h"
#include "ui_dialoguserlogin.h"
#include "CHttpClientAgent.h"
#include "CSysSettings.h"

#include <QMessageBox>
#include <QJsonObject>

DialogUserLogin::DialogUserLogin(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogUserLogin)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()|Qt::WindowStaysOnTopHint);

    QString strUserName ;
    QString strPassword ;
    CSysSettings::getInstance()->getLoginPara(strUserName,strPassword);
    ui->lineEditUserName->setText(strUserName.trimmed()) ;
    ui->lineEditPassword->setText(strPassword.trimmed()) ;

    ui->checkBoxSaveLogin->setChecked(true);
}

DialogUserLogin::~DialogUserLogin()
{
    delete ui;
}

void DialogUserLogin::on_pushButtonLogin_clicked()
{
    {
        std::string sHttpIp;
        int iHttpPort=0;
        CSysSettings::getInstance()->getHttpPara( sHttpIp, iHttpPort );
        CHttpClientAgent::getInstance()->setPara( sHttpIp.c_str(), iHttpPort );
    }
    QString strUserName = ui->lineEditUserName->text().trimmed();
    QString strPassword = ui->lineEditPassword->text().trimmed();

    if(strUserName.isEmpty())
        strUserName = "superAdmin";
    if(strPassword.isEmpty())
        strPassword = "super123789";

    QJsonObject jRet ;

    if( CHttpClientAgent::getInstance()->userLogin(strUserName, strPassword,jRet))
    {
        QJsonObject jData =jRet["data"].toObject();
        QJsonObject jUser =jData["user"].toObject();
        m_strUser = jUser["username"].toString() ;
        m_strName = jUser["nickname"].toString() ;
        on_checkBoxSaveLogin_clicked() ;
        accept() ;
    }
    else
    {
        QString strError =  jRet["msg"].toString();
        if(strError.contains("TCPTransport closed=True"))
        {

        }

        QString strInfo = QString("登录失败！-- [%1]").arg(strError);
        QMessageBox::warning(this,"提示",strInfo);
    }
}


void DialogUserLogin::on_pushButtonCancel_clicked()
{
    reject() ;
}


void DialogUserLogin::on_checkBoxPassword_clicked()
{
    bool bIsVisible = ui->checkBoxPassword->isChecked() ;
    ui->lineEditPassword->setEchoMode(bIsVisible ? QLineEdit::Normal : QLineEdit::Password) ;
    ui->lineEditPassword->update() ;
}

void DialogUserLogin::on_checkBoxSaveLogin_clicked()
{
    QString strUserName = ui->lineEditUserName->text().trimmed();
    QString strPassword = ui->lineEditPassword->text().trimmed();
    if(!ui->checkBoxSaveLogin->isChecked())
    {
        strUserName.clear() ;
        strPassword.clear() ;
    }

    CSysSettings::getInstance()->setLoginPara(strUserName,strPassword);
}


void DialogUserLogin::on_pushButtonSysSet_clicked()
{
    m_pSet->setParent(this) ;

    m_pSet->setWindowFlags(Qt::Popup|Qt::Dialog|m_pSet->windowFlags()) ;
    m_pSet->show() ;
}

