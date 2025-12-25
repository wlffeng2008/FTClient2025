#include "dialogpacklabelprint.h"
#include "ui_dialogpacklabelprint.h"
#include "CLabelEdit.h"
#include "CSysSettings.h"
#include <QTextDocument>
#include <QDateTime>
#include <QSettings>

static QSettings settings("FTClient", "PrintInfo");

DialogLabelPrint::DialogLabelPrint(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogLabelPrint)
{
    ui->setupUi(this);
    setWindowFlags(windowFlags()|Qt::MSWindowsFixedSizeDialogHint|Qt::Dialog);
    ui->plainTextEdit->setMaximumBlockCount(100);

    ui->lineEditModel->setText(settings.value("model","CVIP3").toString())  ;
    ui->lineEditFactory->setText(settings.value("fcode","6").toString());

    MakePrintSN() ;
    ui->checkBoxAutoPrint->setChecked(true) ;

    connect(ui->pushButton,&QPushButton::clicked,[=](){ startPrint();  });
    connect(ui->pushButtonUpload,&QPushButton::clicked,[=](){

        QString strSNs = ui->plainTextEdit->toPlainText().trimmed() ;
        if(strSNs.isEmpty())
            return;
        QStringList strDeviceNos = strSNs.split('\n') ;
        if(strDeviceNos.size()>0)
        {
            QString strBox = ui->lineEdit->text() ;
            emit reportPachInfo(strBox,strDeviceNos) ;
            MakePrintSN();
            ui->plainTextEdit->clear();
        }
    });

    connect(ui->pushButtonClear,&QPushButton::clicked,[=](){ ui->plainTextEdit->clear(); });
    connect(ui->pushButtonAllBars,&QPushButton::clicked,[=](){

        bool bNoviewPrint = ui->checkBoxPrintNoview->isChecked() ;
        CLabelEdit::getInstance()->setMode( 3 );
        QString strSNs = ui->plainTextEdit->toPlainText().trimmed() ;
        QStringList SNList = strSNs.split('\n') ;
        if(SNList.size()>0)
            CLabelEdit::getInstance()->setLabelInfo4(SNList, bNoviewPrint);
        if(!bNoviewPrint || SNList.size() < 2)
            CLabelEdit::getInstance()->show();
    });
}

DialogLabelPrint::~DialogLabelPrint()
{
    delete ui;
}

void DialogLabelPrint::MakePrintSN()
{
    QDateTime dateTime = QDateTime::currentDateTime();
    QString strNow = dateTime.toString("yyyy-MM-dd");
    QString strOld = settings.value("packdate").toString() ;

    m_nPrintCount = settings.value("packcount").toInt() ;
    if(strNow != strOld)
        m_nPrintCount = 0 ;

    char szIndex[20]={0};
    sprintf(szIndex,"%04d",m_nPrintCount+1);
    QString strModel = ui->lineEditModel->text();
    QString strFactory= ui->lineEditFactory->text() ;
    QString strBN = QString("%1%2%3%4").arg(strModel).arg(strFactory).arg(QDateTime::currentDateTime().toString("yyMMdd")).arg(szIndex);
    ui->lineEdit->setText(strBN) ;
    ui->labelPrintCount->setText(QString::number(m_nPrintCount)) ;
    m_nPrintCount++ ;
    settings.setValue("model",strModel);
    settings.setValue("fcode",strFactory);
    settings.setValue("packdate",strNow);
    settings.setValue("packcount",m_nPrintCount);
}

void DialogLabelPrint::startPrint()
{
    bool bNoviewPrint = ui->checkBoxPrintNoview->isChecked() ;
    CLabelEdit::getInstance()->setMode( 2 );
    QString curDevLabTem = CSysSettings::getInstance()->getPackLabelTemplateFile();
    CLabelEdit::getInstance()->loadTemplate( curDevLabTem );
    if(!bNoviewPrint)
        CLabelEdit::getInstance()->show();

    QString strSNs = ui->plainTextEdit->toPlainText().trimmed() ;
    if(strSNs.isEmpty())
        return;
    QStringList strDeviceNos = strSNs.split('\n') ;
    if(strDeviceNos.size()>0)
    {
        QString strBox = ui->lineEdit->text() ;
        QString strKey = "11001CVIP301" ;
        strSNs.replace("\n\r\n","\n");
        strSNs.replace("\n","\r\n");
        CLabelEdit::getInstance()->setLabelInfo3(strSNs,strKey,strBox) ;
    }
}

void DialogLabelPrint::on_plainTextEdit_textChanged()
{
    QTextDocument* document = ui->plainTextEdit->document();
    int nLineCount = document->blockCount()-1;
    ui->labelScanCount->setText(QString::number(nLineCount));
    int nLimt = ui->lineEditLimit->text().toInt() ;
    if(ui->checkBoxAutoPrint->isChecked() && nLineCount>=nLimt)
    {
        startPrint() ;
    }
}

void DialogLabelPrint::on_lineEditModel_editingFinished()
{
    m_nPrintCount--;
    MakePrintSN();
}


void DialogLabelPrint::on_lineEditFactory_editingFinished()
{
    m_nPrintCount--;
    MakePrintSN();
}

