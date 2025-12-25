#include "dialogboxlabelprint.h"
#include "ui_dialogboxlabelprint.h"
#include "CLabelEdit.h"
#include "CSysSettings.h"

DialogBoxLabelPrint::DialogBoxLabelPrint(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::DialogBoxLabelPrint)
{
    ui->setupUi(this);

    setWindowFlags(windowFlags()|Qt::MSWindowsFixedSizeDialogHint|Qt::Dialog);
    ui->checkBoxAutoPrint->setChecked(true);

    connect(ui->pushButton,&QPushButton::clicked,[=](){ EnterPrint() ; });
    connect(&m_TimerPrint,&QTimer::timeout,[=](){ m_TimerPrint.stop(); EnterPrint() ; });
}

DialogBoxLabelPrint::~DialogBoxLabelPrint()
{
    delete ui;
}

void DialogBoxLabelPrint::EnterPrint()
{
    CLabelEdit::getInstance()->setMode( 1 );
    QString curDevLabTem = CSysSettings::getInstance()->getBoxLabelTemplateFile();
    CLabelEdit::getInstance()->loadTemplate( curDevLabTem );
    CLabelEdit::getInstance()->show();
    QString strName = ui->lineEditDevSN->text() ;
    QString strKey = "11001CVIP301" ;

    if(strName.length()>6)
        CLabelEdit::getInstance()->setLabelInfo2(strName, strKey) ;
}

void DialogBoxLabelPrint::on_lineEditDevSN_textChanged(const QString &arg1)
{
    if(ui->checkBoxAutoPrint->isChecked())
    {
        if(m_TimerPrint.isActive())
            m_TimerPrint.stop() ;
        m_TimerPrint.start(200);
    }
}

