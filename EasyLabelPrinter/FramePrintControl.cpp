#include "FramePrintControl.h"
#include "ui_FramePrintControl.h"

#include "FrameLabelView.h"

#include <QTimer>
#include <QMessageBox>
#include <QFileDialog>
#include <QFontDialog>
#include <QDir>
#include <QFile>

FramePrintControl::FramePrintControl(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::FramePrintControl)
{
    ui->setupUi(this);

    m_pSet = new QSettings(QApplication::applicationDirPath() + "/config/global.ini",QSettings::IniFormat) ;

    m_strConfigFile = m_pSet->value("lastConfigFile", QApplication::applicationDirPath() + "/config/default.ini").toString();
    m_strTemplFile  = m_pSet->value("lastTemplFile" , QApplication::applicationDirPath() + "/config/default.tem").toString();

    connect(ui->spinBoxX,&QSpinBox::valueChanged,this,[=]{updateItemSize();}) ;
    connect(ui->spinBoxY,&QSpinBox::valueChanged,this,[=]{updateItemSize();}) ;
    connect(ui->spinBoxW,&QSpinBox::valueChanged,this,[=]{updateItemSize();}) ;
    connect(ui->spinBoxH,&QSpinBox::valueChanged,this,[=]{updateItemSize();}) ;
    connect(ui->spinBoxS,&QDoubleSpinBox::valueChanged,this,[=]{updateItemSize();}) ;
    connect(ui->lineEditName,&QLineEdit::editingFinished,this,[=]{updateItemSize();}) ;

    connect(ui->pushButtonAddText,&QPushButton::clicked,this,[=]{
        m_pLabelView->AddText("请修改你的文字",ui->lineEditName->text().trimmed());
    });
    connect(ui->pushButtonAddImage,&QPushButton::clicked,this,[=]{
        QString strFile = QFileDialog::getOpenFileName(this, "打开图片文件", QApplication::applicationDirPath() + "/images", "图片文件 (*.png;*.jpg;*.bmp;*.jpeg;*.tif);;All Files (*.*)");

        if(strFile.isEmpty())
            return;
        m_pLabelView->AddImageFile(strFile,ui->lineEditName->text().trimmed());
    });

    connect(ui->pushButtonSave,&QPushButton::clicked,this,[=]{
        m_pLabelView->Save() ;
        QMessageBox::information(this,"提示","标签模版保存！");
    });

    connect(ui->pushButtonSaveAs,&QPushButton::clicked,this,[=]{
        QString strFile = QFileDialog::getSaveFileName(this, "保存模板文件", QApplication::applicationDirPath() + "/config", "Template Files (*.tem);;All Files (*.*)");

        if(strFile.isEmpty())
            return;

        m_pLabelView->Load(strFile) ;
        m_pLabelView->Save() ;
        QMessageBox::information(this,"提示","标签模版保存！");
    });

    connect(ui->pushButtonLoad,&QPushButton::clicked,this,[=]{
        QString strFile = QFileDialog::getOpenFileName(this, "打开模板文件", QApplication::applicationDirPath() + "/config", "Template Files (*.tem);;All Files (*.*)");

        if(strFile.isEmpty())
            return;

        m_pLabelView->Load(strFile) ;
    });

    connect(ui->pushButtonSetFont,&QPushButton::clicked,this,[=]{
        QFont oldFont = m_pLabelView->GetFont() ;
        bool ok = false;
        QFont font = QFontDialog::getFont(&ok, oldFont, this);
        if (ok)
        {
            m_pLabelView->SetFont(font) ;
        }
    });

    connect(ui->pushButtonDelete,&QPushButton::clicked,this,[=]{
        if(m_pItemPixmap || m_pItemText)
        {
            if(QMessageBox::question(this,"提示",QString("\"%1\" ").arg(ui->lineEditName->text().trimmed()) +" 删除后不可恢复，确定要删除吗？") != QMessageBox::Yes)
            {
                return ;
            }
            m_pLabelView->Delete() ;
        }
    });

    connect(ui->pushButtonOpenCfg,&QPushButton::clicked,this,[=]{
        QString strFile = QFileDialog::getOpenFileName(this, "打开配置文件", QApplication::applicationDirPath() + "/config", "配置文件(*.ini);;All Files (*.*)");

        if(strFile.isEmpty())
            return;
        LoadConfig(strFile) ;
        SaveConfig() ;
    });

    connect(ui->pushButtonSaveCfg,&QPushButton::clicked,this,[=]{
        SaveConfig() ;
    });

    connect(ui->pushButtonSaveAsCfg,&QPushButton::clicked,this,[=]{
        QString strFile = QFileDialog::getOpenFileName(this, "保存配置文件", QApplication::applicationDirPath() + "/config", "配置文件(*.ini);;All Files (*.*)");

        if(strFile.isEmpty())
            return;
        m_strConfigFile = strFile ;
        SaveConfig() ;
        LoadConfig(strFile) ;
    });

    connect(ui->lineEdit_9,&QLineEdit::textChanged,this,[=](const QString&text){
        m_TMCreate.stop();
        m_TMCreate.start(200);
    });

    connect(&m_TMCreate,&QTimer::timeout,this,[=]{
        m_TMCreate.stop();
        QString strDID = ui->lineEdit_9->text().trimmed() ;
        if(strDID.isEmpty())
            return ;
        ui->pushButtonCreate->click() ;
    });

    connect(ui->pushButtonCreate,&QPushButton::clicked,this,[=]{
        QString strRes = "PASS" ;
        bool bFound = false ;
        QString strDID = ui->lineEdit_9->text().trimmed() ;
        if(strDID.isEmpty())
            return ;

        if(!ui->checkBoxManual->isChecked())
        {
            foreach (const DataItem&DI, m_DMap) {
                if(DI.strDID == strDID)
                {
                    QString strTmp = DI.strMac ;
                    ui->lineEdit_8->setText(strTmp.remove(':'));
                    ui->lineEdit_10->setText(DI.strDate.left(7));
                    strRes = DI.strQPass;
                    bFound = true ;
                    break;
                }
            }
            if(!bFound)
            {
                QMessageBox::warning(this,"提示","没找到对应的MAC数据!\n" + strDID);
                return ;
            }
        }

        int nIndex = ui->lineEdit_7->text().trimmed().toInt() + 1;
        ui->lineEdit_7->setText(QString("%1").arg(nIndex,6,10,QLatin1Char('0'))) ;
        QString strSN = QString("%1%2%3%4%5%6%7").arg(
            ui->lineEdit_1->text().trimmed(),
            ui->lineEdit_2->text().trimmed(),
            ui->lineEdit_3->text().trimmed(),
            ui->lineEdit_4->text().trimmed(),
            ui->lineEdit_5->text().trimmed(),
            ui->lineEdit_6->text().trimmed(),
            ui->lineEdit_7->text().trimmed() ) ;
        QString strMac = ui->lineEdit_8->text().trimmed();
        QString strDate = ui->lineEdit_10->text().trimmed();

        m_pLabelView->AddText(QString("SN:")+strSN,"SN") ;
        m_pLabelView->AddText(QString("MAC:")+strMac,"MAC") ;
        m_pLabelView->AddText(QString("DID:")+strDID,"DID") ;
        m_pLabelView->AddText(QString("Date:")+strDate,"Date") ;
        m_pLabelView->AddText(QString("QC ")+strRes,"QPass") ;
        m_pLabelView->AddImageQR(strSN,"QRCode") ;

        SaveConfig() ;
        if(ui->checkBoxAutoPrint->isChecked())
            ui->pushButtonPrint->click() ;

        if(ui->checkBoxManual->isChecked())
            QTimer::singleShot(500,this,[=]{ ui->lineEdit_9->setText(""); });

        QFile EF(QCoreApplication::applicationDirPath() + "/binding.csv") ;
        if(EF.open(QIODevice::Append) || EF.open(QIODevice::WriteOnly))
        {
            if(EF.size() == 0) EF.write("SN,MAC,DID\n") ;
            QString strLine = strSN + "," + strMac + "," + strDID + "\n";
            EF.write(strLine.toStdString().c_str()) ;
            EF.flush();
            EF.close();
        }
        else
        {
            QMessageBox::critical(this,"提示","文件 binding.csv 被其他应用程序占用，数据写入失败，请仔细检查后重试！");
            return ;
        }
    });

    ui->label_17->installEventFilter(this);
    ui->label_18->installEventFilter(this);
    ui->label_17->setMouseTracking(true);
    ui->label_18->setMouseTracking(true);

    LoadConfig(m_strConfigFile) ;

    QList<QPushButton*> allButtons = findChildren<QPushButton*>();
    foreach (QPushButton *button, allButtons) {
        button->setFocusPolicy(Qt::ClickFocus) ;
        button->setCursor(Qt::PointingHandCursor);
    }

    LoadMap() ;

    QList<QPushButton*>btns = findChildren<QPushButton*>();
    for(QPushButton*btn:btns)
    {
        btn->setFocusPolicy(Qt::NoFocus) ;
        btn->setCursor(Qt::PointingHandCursor) ;
    }
}

FramePrintControl::~FramePrintControl()
{
    delete ui;
}

void FramePrintControl::LoadMap()
{
    QString strInPath = ui->lineEdit_11->text().trimmed();
    QString strOutPath = ui->lineEdit_12->text().trimmed();

    QDir A(strInPath) ;
    if(!A.exists()) A.mkdir(strInPath) ;
    QStringList nameFilters = {"*.log"};
    QDir::Filters filters = QDir::Files | QDir::NoDotAndDotDot | QDir::Readable;
    QDir::SortFlags sort = QDir::Name | QDir::IgnoreCase;

    QStringList files = A.entryList(nameFilters, filters, sort);

    m_DMap.clear() ;
    for(int i=0 ; i<files.count(); i++)
    {
        QString strFile = files[i];

        QSettings Get(strInPath + "/" + strFile,QSettings::IniFormat) ;
        QString strStage = strFile;
        strStage.replace(".log","");
        QString strMac = Get.value(strStage+"/MAC").toString();
        QString strDID = Get.value(strStage+"/DID").toString();
        QString strDate = Get.value(strStage+"/DATE").toString();
        QString strQPass = Get.value(strStage+"/TEST_RESULT").toString();

        DataItem DI ;
        DI.strMac  = strMac;
        DI.strDate = strDate;
        DI.strDID  = strDID;
        DI.strQPass  = strQPass;
        qDebug() << strDID;

        m_DMap.push_back(DI) ;

        // QString strOldFile = strInPath  + "/" + strFile ;
        // QString strNewFile = strOutPath + "/" + strFile ;

        // if(!A.rename(strOldFile,strNewFile))
        // {
        //     A.remove(strOldFile) ;
        //     qDebug() << "Remove File:" << strFile ;
        // }
    }
}

bool FramePrintControl::eventFilter(QObject *watched, QEvent *event)
{
    if(event->type() == QEvent::Enter)
    {
        if(watched == ui->label_17 || watched == ui->label_18)
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor) ;
            return true;
        }
    }
    if(event->type() == QEvent::Leave)
    {
        if(watched == ui->label_17 || watched == ui->label_18)
        {
            QApplication::setOverrideCursor(Qt::ArrowCursor) ;
            return true;
        }
    }

    if(event->type() == QEvent::MouseButtonRelease)
    {
        if(watched == ui->label_17)
        {
            QString strDir = QFileDialog::getExistingDirectory(
                this,  "选择数据源目录", QDir::homePath() );

            if(!strDir.isEmpty())
                ui->label_17->setText(strDir) ;
        }

        if(watched == ui->label_18)
        {
            QString strDir = QFileDialog::getExistingDirectory(
                this, "选择转存目录", QDir::homePath() );

            if(!strDir.isEmpty())
                ui->label_18->setText(strDir) ;
        }
    }

    return QFrame::eventFilter(watched,event);
}

void FramePrintControl::updateItemSize()
{
    if(!m_bCanSet)
        return ;

    QRectF rc(ui->spinBoxX->value(),ui->spinBoxY->value(),ui->spinBoxW->value(),ui->spinBoxH->value()) ;
    qreal fScale = ui->spinBoxS->value() ;
    QString strName = ui->lineEditName->text().trimmed() ;
    if(m_pItemText)
    {
        m_pItemText->setItemRect(rc) ;
        m_pItemText->setScale(fScale) ;
        m_pItemText->setName(strName) ;
    }

    if(m_pItemPixmap)
    {
        m_pItemPixmap->setItemRect(rc) ;
        m_pItemPixmap->setScale(fScale) ;
        m_pItemPixmap->setName(strName) ;
    }
}


void FramePrintControl::LoadTemplate(const QString&strFile){
    m_pLabelView->Load(strFile) ;

    int nFind = strFile.lastIndexOf('/') ;
    ui->groupBox1->setTitle(QString("模板[%1]").arg(strFile.mid(nFind+1)));
}

void FramePrintControl::LoadConfig(const QString&strFile)
{
    m_strConfigFile = strFile ;
    QSettings Set(strFile, QSettings::IniFormat) ;
    QStringList SNSets = Set.value("SNSetting").toStringList() ;
    if(SNSets.isEmpty())
    {
        SNSets.append("A") ;
        SNSets.append("K02B2") ;
        SNSets.append("US") ;
        SNSets.append("W") ;
        SNSets.append("5") ;
        SNSets.append("7") ;
        SNSets.append("000008") ;
        SNSets.append("1234567890AB") ;
        SNSets.append("YRUSYA2666001040914") ;
        SNSets.append("2025.07") ;
        SNSets.append(QApplication::applicationDirPath() + "/DataIn") ;
        SNSets.append(QApplication::applicationDirPath() + "/DataOut") ;

        QTimer::singleShot(200,this,[=]{ SaveConfig(); }) ;
    }

    ui->lineEdit_1->setText(SNSets[0]) ;
    ui->lineEdit_2->setText(SNSets[1]) ;
    ui->lineEdit_3->setText(SNSets[2]) ;
    ui->lineEdit_4->setText(SNSets[3]) ;
    ui->lineEdit_5->setText(SNSets[4]) ;
    ui->lineEdit_6->setText(SNSets[5]) ;
    ui->lineEdit_7->setText(SNSets[6]) ;
    ui->lineEdit_8->setText(SNSets[7]) ;
    //ui->lineEdit_9->setText(SNSets[8]) ;
    ui->lineEdit_10->setText(SNSets[9]) ;
    ui->lineEdit_11->setText(SNSets[10]) ;
    ui->lineEdit_12->setText(SNSets[11]) ;

    int nFind = strFile.lastIndexOf('/') ;
    ui->groupBox2->setTitle(QString("配置[%1]").arg(strFile.mid(nFind+1)));
}

void FramePrintControl::SaveConfig()
{
    QSettings Set(m_strConfigFile, QSettings::IniFormat) ;
    QStringList SNSets = {
        ui->lineEdit_1->text().trimmed(),
        ui->lineEdit_2->text().trimmed(),
        ui->lineEdit_3->text().trimmed(),
        ui->lineEdit_4->text().trimmed(),
        ui->lineEdit_5->text().trimmed(),
        ui->lineEdit_6->text().trimmed(),
        ui->lineEdit_7->text().trimmed(),
        ui->lineEdit_8->text().trimmed(),
        ui->lineEdit_9->text().trimmed(),
        ui->lineEdit_10->text().trimmed(),
        ui->lineEdit_11->text().trimmed(),
        ui->lineEdit_12->text().trimmed() } ;
    Set.setValue("SNSetting",SNSets) ;

    m_pSet->setValue("lastConfigFile",m_strConfigFile);
    m_pSet->setValue("lastTemplFile",m_strTemplFile);
}

void FramePrintControl::BindLabelView(FrameLabelView *pView)
{
    m_pLabelView = pView ;
    LoadTemplate(m_strTemplFile) ;

    connect(pView,&FrameLabelView::onItemSelected,this,[=](QObject *pSender)
    {
        m_pItemText   = nullptr ;
        m_pItemPixmap = nullptr ;

        bool bEnable = (pSender == nullptr) ;

        ui->spinBoxX->setDisabled(bEnable);
        ui->spinBoxY->setDisabled(bEnable);
        ui->spinBoxW->setDisabled(bEnable);
        ui->spinBoxH->setDisabled(bEnable);
        ui->spinBoxS->setDisabled(bEnable);
        // ui->lineEdit->setDisabled(bEnable);
        ui->lineEditName->clear() ;

        m_bCanSet = false ;
        QTimer::singleShot(200,this,[=]{ m_bCanSet = true; }) ;

        if (auto textItem = dynamic_cast<CustomTextItem*>(pSender))
        {
            m_pItemText = textItem ;

            QRectF rc = textItem->getItemRect() ;
            ui->spinBoxX->setValue(rc.x()) ;
            ui->spinBoxY->setValue(rc.y()) ;
            ui->spinBoxW->setValue(rc.width()) ;
            ui->spinBoxH->setValue(rc.height()) ;
            ui->spinBoxS->setValue(textItem->scale()) ;
            ui->lineEditName->setText(textItem->getName()) ;
        }
        else if (auto pixmapItem = dynamic_cast<CustomPixmapItem*>(pSender))
        {
            m_pItemPixmap = pixmapItem ;

            QRectF rc = pixmapItem->getItemRect() ;
            ui->spinBoxX->setValue(rc.x()) ;
            ui->spinBoxY->setValue(rc.y()) ;
            ui->spinBoxW->setValue(rc.width()) ;
            ui->spinBoxH->setValue(rc.height()) ;
            ui->spinBoxS->setValue(pixmapItem->scale()) ;
            ui->lineEditName->setText(pixmapItem->getName()) ;
        }
    }) ;
}

void FramePrintControl::on_pushButtonPreview_clicked()
{
    m_pLabelView->Preview() ;
}

void FramePrintControl::on_pushButtonPrint_clicked()
{
    /*if(time(nullptr)>1754832595 || time(nullptr)<1754324773)
    {
        QMessageBox::information(this,"提示","免费试用期限已到！");
        return ;
    }*/

    int nCount = ui->spinBoxPrintCount->value() ;
    for(int i=0; i<nCount; i++)
        m_pLabelView->Print() ;
}

