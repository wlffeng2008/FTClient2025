#include "CLabelEdit.h"
#include "CLabelSave.h"
#include "CustomItems.h"
#include "Label.h"
#include "CLabelSettings.h"

#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QToolBar>
#include <QPushButton>
#include <QAction>
#include <QFileDialog>
#include <QDir>
#include <QLineEdit>
#include <QLabel>
#include <QMessageBox>
#include <QDebug>
#include <QTimer>
#include <QPixmap>
#include <QImage>
#include <QFontDialog>
#include "../Zint.h"
#include <QSpacerItem>

CLabelEdit* CLabelEdit::m_pIns = nullptr;

QImage genBarCode(const std::string &strText,const std::string&strFile="./1234567.bmp",int nH=15)
{
    QImage imgBar;
    struct zint_symbol *symbol = ::ZBarcode_Create();
    if (symbol != NULL)
    {
        symbol->scale = 2;
        symbol->option_1 = 1; //容错级别
        symbol->option_2 = 1; //版本，决定图片大小
        symbol->symbology = BARCODE_CODE128;
        symbol->height = nH ;
        symbol->show_hrt = 0; //可显示信息，如果设置为1，则需要设置text值
        //symbol->input_mode = UNICODE_MODE;
        strcpy(symbol->outfile,strFile.c_str()) ;

        const char *lpszText = strText.c_str();
        int nLen = strlen(lpszText);
        int nRet = ZBarcode_Encode(symbol,(const unsigned char *)lpszText,nLen); //编码
        if (nRet == 0)
            nRet = ZBarcode_Print(symbol,0); //antate angle 旋转角度

        ZBarcode_Delete(symbol);

        imgBar.load(strFile.c_str());
    }
    return imgBar ;
}


QImage genQrCode(const std::string &strText,const std::string&strFile="./1234567Qr.bmp",int nH=15)
{
    QImage imgBar;
    struct zint_symbol *symbol = ZBarcode_Create();
    if (symbol != NULL)
    {
        symbol->scale = 4;
        symbol->symbology = BARCODE_QRCODE;
        symbol->show_hrt = 0; //可显示信息，如果设置为1，则需要设置text值
        //symbol->input_mode = UNICODE_MODE;
        strcpy(symbol->outfile, strFile.c_str()) ;

        const char *lpszText = strText.c_str();
        int nLen = strlen(lpszText);
        int nRet = ZBarcode_Encode(symbol,(const unsigned char *)lpszText,nLen); //编码
        if (nRet == 0)
            nRet = ZBarcode_Print(symbol,0); //antate angle 旋转角度

        ZBarcode_Delete(symbol);

        imgBar.load(strFile.c_str());
    }
    return imgBar ;
}

static QFont s_font;
CLabelEdit::CLabelEdit( QWidget *parent )
    : QMainWindow( parent ), m_pLineEdit_x( nullptr ), m_pLineEdit_y( nullptr ), m_pLineEdit_w( nullptr ), m_pLineEdit_h( nullptr )
{
    setWindowTitle( "标签模板编辑工具" );
    checkPara();

    //s_font.setFamily("思源黑体 CN Medium");
    s_font.setFamily("黑体");
    s_font.setPointSize(9);
    s_font.setBold(true);

    s_font.setFamily("Bahnschrift");
    s_font.setStyleName("Condensed");
    s_font.setPointSize(9);
    s_font.setBold(true);

    getView()->setScene( getScene().get() );
    getView()->setRenderHint( QPainter::Antialiasing );

    // 设置画布的固定大小
    getScene()->setSceneRect( 0, 0, m_para.iLabelWidth_mm * m_para.fZoomScale, m_para.iLabelHeight_mm * m_para.fZoomScale );       // 设置场景大小为400x300
    getView()->setFixedSize( getScene()->sceneRect().size().toSize() );
    getView()->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    getView()->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    setCentralWidget( getView().get() );

    initToolBar();

    adjustSize();
    setFixedSize( size() );
}

CLabelEdit::~CLabelEdit()
{
}

void CLabelEdit::showWindow()
{
    getWgtStart()->setPara( m_para );
    getWgtStart()->show();
}


std::shared_ptr<CustomView> CLabelEdit::getView()
{
    if( nullptr == m_pView )
    {
        m_pView = std::make_shared<CustomView>();
    }
    return m_pView;
}

std::shared_ptr<CustomScene> CLabelEdit::getScene()
{
    if( nullptr == m_pScene )
    {
        m_pScene = std::make_shared<CustomScene>( this );
        QObject::connect( m_pScene.get(), &CustomScene::sigNoItemSelected, [this](){
            this->onNoItemSelected();
        } );
    }
    return m_pScene;
}

std::shared_ptr<CLabelSettings> CLabelEdit::getSettingsWgt()
{
    if( nullptr == m_pSettings )
    {
        m_pSettings = std::make_shared<CLabelSettings>(this);
        QObject::connect( m_pSettings.get(), &CLabelSettings::sigNewPara, [=]( label::LabelPara p ){
            m_para = p;

            this->updateWinSize();
            qDebug() << "Label size updated - new width: " << m_para.iLabelWidth_mm << ", new height: " << m_para.iLabelHeight_mm;
        } );
    }

    return m_pSettings;
}

std::shared_ptr<WgtLabelParaStart> CLabelEdit::getWgtStart()
{
    if( nullptr == m_pWgtStart )
    {
        m_pWgtStart = std::make_shared<WgtLabelParaStart>();
        QObject::connect( m_pWgtStart.get(), &WgtLabelParaStart::sigPara, [=]( label::LabelPara p ){
            m_para = p;
            updateWinSize();
            this->show();
        } );
        QObject::connect( m_pWgtStart.get(), &WgtLabelParaStart::sigStartEdit, [=](){
            this->show();
        } );
    }
    return m_pWgtStart;
}

bool CLabelEdit::findItemByTypeIn( QGraphicsItem **ppItem, const QString& typeIn,int nIndex)
{
    if( nullptr == ppItem || typeIn.trimmed().isEmpty() )
    {
        qDebug() << "Params error";
        return false;
    }

    int nFind = 0 ;
    for( auto item : getScene()->items() )
    {
        if( auto pixmapItem = dynamic_cast<QGraphicsPixmapItem*>(item) )
        {
            QString type_in = pixmapItem->data( label::obj_type_in_data_index ).toString();
            if( typeIn == type_in )
            {
                *ppItem = pixmapItem;
                nFind++;
                if(nFind == nIndex)
                    return true;
            }
        }
        else if( auto textItem = dynamic_cast<QGraphicsTextItem*>(item) )
        {
            QString type_in = textItem->data( label::obj_type_in_data_index ).toString();
            if( typeIn == type_in )
            {
                *ppItem = textItem;
                nFind++;
                if(nFind == nIndex)
                    return true;
            }
        }
    }

    return false;
}


void CLabelEdit::checkPara()
{
    m_para.fZoomScale = ( 0 == m_para.fZoomScale ) ? 26.7 : m_para.fZoomScale;
    m_para.iLabelWidth_mm = ( m_para.iLabelWidth_mm < 1 ) ? 10 : m_para.iLabelWidth_mm;
    m_para.iLabelHeight_mm = ( m_para.iLabelHeight_mm < 1 ) ? 10 : m_para.iLabelHeight_mm;
}


bool CLabelEdit::printPage()
{
    QRectF specifiedArea( 0, 0, m_para.iLabelWidth_mm * m_para.fZoomScale, m_para.iLabelHeight_mm * m_para.fZoomScale );       // 指定需要打印的区域（左上角坐标和宽高）
    getView()->print( specifiedArea );
    return true;
}

void CLabelEdit::initToolBar()
{
    QToolBar *toolBar = addToolBar( "工具条" );

    QAction *buttonAction = nullptr;

    buttonAction = new QAction( "打开...", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onLoadTemplate );

    buttonAction = new QAction( "保存", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onSaveTemplate );

    buttonAction = new QAction( "另存为...", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onNewTemplate );
    toolBar->addSeparator() ;

    buttonAction = new QAction( "添加文字", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onAddText );

    buttonAction = new QAction( "添加SN", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onAddBarSN );

    buttonAction = new QAction( "添加UUID", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onAddUUID );

    buttonAction = new QAction( "添加图片", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onAddImg );

    buttonAction = new QAction( "添加二维码", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onAddQrCode );

    buttonAction = new QAction( "添加条形码", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onAddBarCode );    
    toolBar->addSeparator() ;

    buttonAction = new QAction( "模板参数...", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onSettings );
    toolBar->addSeparator() ;

    buttonAction = new QAction( "打印", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onPrint );

    buttonAction = new QAction( "预览", this );
    toolBar->addAction( buttonAction );
    connect( buttonAction, &QAction::triggered, this, &CLabelEdit::onPrintPreview );

    m_pAutoPrt = new QCheckBox( "自动打印", this );
    toolBar->addWidget( m_pAutoPrt );

    addToolBarBreak();  // 设置下一个 toolbar 放在第二行

    // 第二个工具条
    QToolBar *toolBar_sub = addToolBar( "工具条2" );

    QLabel *label = new QLabel(" X: ",this);
    m_pLineEdit_x = std::make_shared<QLineEdit>();
    QObject::connect( m_pLineEdit_x.get(), &QLineEdit::editingFinished, [this](){
        this->on_label_rect_editingFinished();
    } );
    toolBar_sub->addWidget( label );
    toolBar_sub->addWidget( m_pLineEdit_x.get() );

    label = new QLabel("   Y: ");
    m_pLineEdit_y = std::make_shared<QLineEdit>();
    QObject::connect( m_pLineEdit_y.get(), &QLineEdit::editingFinished, [this](){
        this->on_label_rect_editingFinished();
    } );
    toolBar_sub->addWidget(label);
    toolBar_sub->addWidget( m_pLineEdit_y.get() );

    label = new QLabel("   W: ");
    m_pLineEdit_w = std::make_shared<QLineEdit>();
    QObject::connect( m_pLineEdit_w.get(), &QLineEdit::editingFinished, [this](){
        this->on_label_rect_editingFinished();
    } );
    toolBar_sub->addWidget( label );
    toolBar_sub->addWidget( m_pLineEdit_w.get() );

    label = new QLabel("   H: ");
    m_pLineEdit_h = std::make_shared<QLineEdit>();
    QObject::connect( m_pLineEdit_h.get(), &QLineEdit::editingFinished, [this](){
        this->on_label_rect_editingFinished();
    } );
    toolBar_sub->addWidget( label );
    toolBar_sub->addWidget( m_pLineEdit_h.get() );

    m_pLineEdit_x->setMaximumWidth(70) ;
    m_pLineEdit_y->setMaximumWidth(70) ;
    m_pLineEdit_w->setMaximumWidth(70) ;
    m_pLineEdit_h->setMaximumWidth(70) ;

    toolBar_sub->addSeparator() ;

    QPushButton *pFontBtn = new QPushButton("字体设置...",this);
    toolBar_sub->addWidget( pFontBtn );
    pFontBtn->setFixedSize(100,24) ;
    connect(pFontBtn,&QPushButton::clicked,[this]()
    {
        QFont oldFont = s_font ;
        if(!m_bCtrlPress)
        {
            for( auto item : m_pScene->items() )
            {
                if( item->isSelected())
                {
                    if (auto textItem = dynamic_cast<CustomTextItem*>(item))
                    {
                        oldFont = textItem->font() ;
                        break;
                    }
                }
            }
        }

        bool ok;
        QFont font = QFontDialog::getFont(&ok, oldFont, this);
        if (ok)
        {
            if(m_bCtrlPress)
                s_font = font ;

            for( auto item : m_pScene->items() )
            {
                if( item->isSelected() || m_bCtrlPress)
                {
                    if (auto textItem = dynamic_cast<CustomTextItem*>(item))
                    {
                        textItem->setFont(font) ;
                    }
                }
            }
        }
    });
}

void CLabelEdit::setMode(int nMode)
{
    m_pAutoPrt->setVisible(false) ;
    switch(nMode)
    {
    case 0:
        m_pAutoPrt->setVisible(true) ;
        m_para.fZoomScale = 26.7 ;
        break;

    case 1:
        m_para.fZoomScale = 26.7 ;
        break;

    case 2:
        m_para.fZoomScale = 26.7 ;
        break;

    case 3:
        m_para.fZoomScale = 26.7 ;
        m_para.iLabelWidth_mm  = 30  ;
        m_para.iLabelHeight_mm = 30  ;
        break;
    }
    updateWinSize() ;

    if(nMode == 3)
    {
        int nIndex = 1000;
        char szIndex[20]={0};
        m_pScene->clear() ;
        for(int i=0; i<2; i++)
        {
            for(int j=0; j<10; j++)
            {
                if(i==2 && j==6)
                    break;

                sprintf(szIndex,"%04d",nIndex++);
                QString strText = QString("86000100002500000") + szIndex;
                CustomTextItem* textItem = new CustomTextItem( QString("S/N: ") + strText);
                getScene()->addItem( textItem );
                textItem->setPos(100 + i*400, 72+j*75 );
                textItem->setData( label::obj_type_in_data_index, label::sTypeInBarSN );
                textItem->setFont(s_font) ;
                textItem->setScale(1.2);

                QObject::connect( textItem, &CustomTextItem::sigRectChanged_ , [this]( QRectF rect ){
                    this->onCurItemRectChanged( rect );
                } );


                CustomPixmapItem* pixmapItem = new CustomPixmapItem();
                getScene()->addItem( pixmapItem );

                QImage A = genBarCode(strText.toStdString()) ;
                pixmapItem->setPixmap( QPixmap::fromImage(A) );
                pixmapItem->setPos( 45+i*380, 35 + j*75);
                pixmapItem->setData( label::obj_type_in_data_index, label::sTypeInBarCode );
                pixmapItem->setScale(1) ;

                QObject::connect( pixmapItem, &CustomPixmapItem::sigRectChanged_ , [this]( QRectF rect ){
                    this->onCurItemRectChanged( rect );
                } );
            }
        }
    }
}

void CLabelEdit::setLabelInfo4(const QStringList&ListSN,bool bAutoPrint)
{
    int nCount = ListSN.size() ;
    int nIndex = 0 ;
    m_pScene->clear() ;
    for(int i=0; i<2; i++)
    {
        for(int j=0; j<10; j++)
        {
            if((i==2 && j==6) || nIndex>=nCount)
                break;

            QString strText = ListSN[nIndex++];
            CustomTextItem* textItem = new CustomTextItem( QString("S/N: ") + strText);
            getScene()->addItem( textItem );
            textItem->setPos(105 + i*395, 72+j*75 );
            textItem->setData( label::obj_type_in_data_index, label::sTypeInBarSN );
            textItem->setFont(s_font) ;
            textItem->setScale(1.2);

            QObject::connect( textItem, &CustomTextItem::sigRectChanged_ , [this]( QRectF rect ){
                this->onCurItemRectChanged( rect );
            } );

            CustomPixmapItem* pixmapItem = new CustomPixmapItem();
            getScene()->addItem( pixmapItem );

            QImage A = genBarCode(strText.toStdString()) ;
            pixmapItem->setPixmap( QPixmap::fromImage(A) );
            pixmapItem->setPos( 65+i*385, 35 + j*75);
            pixmapItem->setData( label::obj_type_in_data_index, label::sTypeInBarCode );
            pixmapItem->setScale(0.5) ;

            QObject::connect( pixmapItem, &CustomPixmapItem::sigRectChanged_ , [this]( QRectF rect ){
                this->onCurItemRectChanged( rect );
            } );
        }
    }
    if(nCount>0 && bAutoPrint)
        onPrint();
}


void CLabelEdit::setLabelInfo(const QString&strUuid,const QString&strSN)
{
    if(strUuid.isEmpty() )
        return ;

    QGraphicsItem *pItemFound = nullptr;
    if( findItemByTypeIn( &pItemFound, label::sTypeInQrCode ) )
    {
        QGraphicsPixmapItem *qrCodeItem = dynamic_cast<QGraphicsPixmapItem*>(pItemFound) ;
        QString strQR = QString("schema://prod/get/s=GTJtzIFe&u=%1").arg(strUuid);
        strQR = m_para.strTempl ;
        strQR.replace("%1",strUuid);
        QImage img = genQrCode(strQR.toStdString()) ;
        qrCodeItem->setPixmap(QPixmap::fromImage(img)) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInBarCode ) )
    {
        QGraphicsPixmapItem *barCodeItem = dynamic_cast<QGraphicsPixmapItem*>(pItemFound) ;
        QImage img = genBarCode(strSN.toStdString()) ;
        barCodeItem->setPixmap(QPixmap::fromImage(img)) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInUUID ))
    {
        CustomTextItem *barSNItem = dynamic_cast<CustomTextItem*>(pItemFound) ;
        barSNItem->setPlainText(strUuid) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInBarSN ))
    {
        CustomTextItem *barSNItem = dynamic_cast<CustomTextItem*>(pItemFound) ;
        barSNItem->setPlainText("S/N: " + strSN) ;
    }

    if(m_pAutoPrt->isChecked())
        onPrint() ;
}


void CLabelEdit::setLabelInfo2(const QString&strSN,const QString&strKey)
{
    QGraphicsItem *pItemFound = nullptr;
    if( findItemByTypeIn( &pItemFound, label::sTypeInQrCode ) )
    {
        QGraphicsPixmapItem *qrCodeItem = dynamic_cast<QGraphicsPixmapItem*>(pItemFound) ;

        QString strQR = strSN;
        QImage img = genQrCode(strQR.toStdString()) ;
        qrCodeItem->setPixmap(QPixmap::fromImage(img)) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInBarCode ) )
    {
        QGraphicsPixmapItem *barCodeItem = dynamic_cast<QGraphicsPixmapItem*>(pItemFound) ;
        QImage img = genBarCode(strKey.toStdString()) ;
        barCodeItem->setPixmap(QPixmap::fromImage(img)) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInBarSN ) )
    {
        CustomTextItem *barSNItem = dynamic_cast<CustomTextItem*>(pItemFound) ;
        barSNItem->setPlainText("S/N: " + strSN) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInText ) )
    {
        CustomTextItem *barPNItem = dynamic_cast<CustomTextItem*>(pItemFound) ;
        barPNItem->setPlainText("P/N: " + strKey) ;
    }
}

void CLabelEdit::setLabelInfo3(const QString&strSN,const QString&strKey,const QString&strPN,bool bAutoPrint)
{
    QGraphicsItem *pItemFound = nullptr;
    if( findItemByTypeIn( &pItemFound, label::sTypeInQrCode ) )
    {
        QGraphicsPixmapItem *qrCodeItem = dynamic_cast<QGraphicsPixmapItem*>(pItemFound) ;

        QString strQR = strSN;
        QImage img = genQrCode(strQR.toStdString());//QR.generateQr(strQR) ;
        img.save("D:\\qr12345678.png");
        qrCodeItem->setPixmap(QPixmap::fromImage(img.scaled(150,150))) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInBarCode ) )
    {
        QGraphicsPixmapItem *barCodeItem = dynamic_cast<QGraphicsPixmapItem*>(pItemFound) ;
        QImage img = genBarCode(strKey.toStdString()) ;
        barCodeItem->setPixmap(QPixmap::fromImage(img)) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInBarSN ) )
    {
        CustomTextItem *barSNItem = dynamic_cast<CustomTextItem*>(pItemFound) ;
        barSNItem->setPlainText("P/N: " + strKey) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInBarSN, 2) )
    {
        CustomTextItem *barPNItem = dynamic_cast<CustomTextItem*>(pItemFound) ;
        barPNItem->setPlainText("箱号: " + strPN) ;
    }

    if( findItemByTypeIn( &pItemFound, label::sTypeInBarCode,2 ) )
    {
        QGraphicsPixmapItem *barCodeItem = dynamic_cast<QGraphicsPixmapItem*>(pItemFound) ;
        QImage img = genBarCode(strPN.toStdString()) ;
        barCodeItem->setPixmap(QPixmap::fromImage(img)) ;
    }

    if( bAutoPrint)
        onPrint();
}


void CLabelEdit::updateWinSize()
{
    int w_win = m_para.fZoomScale * m_para.iLabelWidth_mm+2;
    int h_win = m_para.fZoomScale * m_para.iLabelHeight_mm + 60;
    setFixedSize( w_win, h_win );

    getScene()->setSceneRect( 0, 0, w_win-5, h_win - 60 );
    getView()->setFixedSize( getScene()->sceneRect().size().toSize() );
}

void CLabelEdit::onNewTemplate()
{
    QString strBak = m_sCurTemplateFileName;
    m_sCurTemplateFileName = "";
    onSaveTemplate() ;
}

void CLabelEdit::onSaveTemplate()
{
    if( m_sCurTemplateFileName.trimmed().isEmpty() )
    {
        QString filePath = QFileDialog::getSaveFileName(nullptr, "保存模板文件", "", "Template Files (*.tem);;All Files (*.*)");

        if(filePath.isEmpty() )
            return;

        m_sCurTemplateFileName = filePath;
        setWindowTitle("标签打印模板 - " + filePath) ;
    }

    m_TI.width=m_para.iLabelWidth_mm;
    m_TI.height=m_para.iLabelHeight_mm;
    m_TI.strTempl=m_para.strTempl ;

    if( CLabelSave::saveSceneWithImages( getScene().get(), m_sCurTemplateFileName,m_TI) )
    {
        QMessageBox::information( this, "提示", "模板文件保存成功!" );
    }
    else
    {
        QMessageBox::warning( this, "提示", "模板文件保存失败!" );
    }
}


bool CLabelEdit::loadTemplate( const QString &tempFilePath )
{
    m_sCurTemplateFileName = tempFilePath ;
    return loadTemplate() ;
}

bool CLabelEdit::loadTemplate()
{
    QString strFile = m_sCurTemplateFileName ;
    if( CLabelSave::loadSceneWithImages( getScene().get(), strFile,m_TI) )
    {
        if(m_TI.strTempl.isEmpty())
            m_TI.strTempl="schema://prod/get/s=GTJtzIFe&u=%1" ;
        m_para.iLabelWidth_mm=m_TI.width;
        m_para.iLabelHeight_mm=m_TI.height;
        m_para.strTempl=m_TI.strTempl ;

        s_font.setBold(m_TI.fontBold);
        s_font.setFamily(m_TI.fontName);
        s_font.setStyleName(m_TI.fontStyle);
        s_font.setPointSize(m_TI.fontSize);

        updateWinSize() ;
        qDebug() << "Label template loading ok: " << strFile;
        setWindowTitle("标签打印模板 - " + strFile) ;
        return true;
    }
    qDebug() << "Label template loading error: " << strFile;
    return false ;
}

void CLabelEdit::onLoadTemplate()
{
    QString fileName = QFileDialog::getOpenFileName(
        nullptr,
        "请选择模板文件",       // 对话框标题
        "",                     // 初始目录 ("" 表示当前目录)
        "Template Files (*.tem);;All Files (*.*)"  // 过滤器
        );
    if( fileName.isEmpty() )
        return ;

    m_sCurTemplateFileName = fileName;
    if(loadTemplate())
    {
        QMessageBox::information( this, "成功", "模板载入成功！" );
    }
    else
    {
        QMessageBox::warning( this, "失败", "模板载入失败！" );
    }
}

void CLabelEdit::onPrint()
{
    QRectF specifiedArea( 0, 0, m_para.iLabelWidth_mm * m_para.fZoomScale, m_para.iLabelHeight_mm * m_para.fZoomScale );       // 指定需要打印的区域（左上角坐标和宽高）
    getView()->print( specifiedArea );
}

void CLabelEdit::onPrintPreview()
{
    QRectF specifiedArea( 0, 0, m_para.iLabelWidth_mm * m_para.fZoomScale, m_para.iLabelHeight_mm * m_para.fZoomScale );       // 指定需要打印的区域（左上角坐标和宽高）
    getView()->printPreviewAndPrint( specifiedArea );
}

void CLabelEdit::onAddImg()
{
    QString fileName = QFileDialog::getOpenFileName(
        this,
        "请选择图片文件",       // 对话框标题
        "",                     // 初始目录 ("" 表示当前目录)
        "Image Files(*.png *.jpg *.bmp);;All Files (*)"  // 过滤器
        );

    if( !fileName.isEmpty() )
    {
        qDebug() << "img file path: " << fileName;
        CustomPixmapItem* pixmapItem = new CustomPixmapItem( QPixmap( fileName ) );
        getScene()->addItem( pixmapItem );
        pixmapItem->setPos( width()*0.3, height()*0.3 );
        pixmapItem->setData( label::obj_type_in_data_index, label::sTypeInImage );

        QObject::connect( pixmapItem, &CustomPixmapItem::sigRectChanged_ , [this]( QRectF rect ){
            this->onCurItemRectChanged( rect );
        } );
    }
}

void CLabelEdit::onAddText()
{
    CustomTextItem* textItem = new CustomTextItem( "TEXT" );
    getScene()->addItem( textItem );
    textItem->setPos( 0, 0 );
    textItem->setData( label::obj_type_in_data_index, label::sTypeInText );
    textItem->setFont(s_font) ;

    QObject::connect( textItem, &CustomTextItem::sigRectChanged_ , [this]( QRectF rect ){
        this->onCurItemRectChanged( rect );
    } );
}

void CLabelEdit::onAddBarSN()
{
    CustomTextItem* textItem = new CustomTextItem( "860001000025000001225");
    getScene()->addItem( textItem );
    textItem->setPos( 0, 0 );
    textItem->setData( label::obj_type_in_data_index, label::sTypeInBarSN );
    textItem->setFont(s_font) ;
    QObject::connect( textItem, &CustomTextItem::sigRectChanged_ , [this]( QRectF rect ){
        this->onCurItemRectChanged( rect );
    } );
}

void CLabelEdit::onAddUUID()
{
    CustomTextItem* textItem = new CustomTextItem( "aixx9ccc1d336d1fe09f");
    getScene()->addItem( textItem );
    textItem->setPos( 0, 0 );
    textItem->setData( label::obj_type_in_data_index, label::sTypeInUUID);
    textItem->setFont(s_font) ;
    QObject::connect( textItem, &CustomTextItem::sigRectChanged_ , [this]( QRectF rect ){
        this->onCurItemRectChanged( rect );
    } );
}

void CLabelEdit::onAddQrCode()
{
    CustomPixmapItem* pixmapItem = new CustomPixmapItem();
    getScene()->addItem( pixmapItem );

    QImage B = genQrCode("cm\n12345678\n860001000025000001225") ;
    pixmapItem->setPixmap(QPixmap::fromImage(B)) ;
    pixmapItem->setPos( width()*0.3, height()*0.3 );
    pixmapItem->setData( label::obj_type_in_data_index, label::sTypeInQrCode );

    bool b = QObject::connect( pixmapItem, &CustomPixmapItem::sigRectChanged_ , [this]( QRectF rect ){
        this->onCurItemRectChanged( rect );
    } );
}

void CLabelEdit::onAddBarCode()
{
    CustomPixmapItem* pixmapItem = new CustomPixmapItem();
    getScene()->addItem( pixmapItem );

    QImage A = genBarCode("8600014545677898") ;
    pixmapItem->setPixmap( QPixmap::fromImage(A) );
    pixmapItem->setPos( width()*0.3, height()*0.3 );
    pixmapItem->setData( label::obj_type_in_data_index, label::sTypeInBarCode );
    pixmapItem->setScale(0.5) ;

    QObject::connect( pixmapItem, &CustomPixmapItem::sigRectChanged_ , [this]( QRectF rect ){
        this->onCurItemRectChanged( rect );
    } );
}

void CLabelEdit::onSettings()
{
    getSettingsWgt()->setLabelPara( m_para );
    getSettingsWgt()->show();
}

void CLabelEdit::onDeleteSelectedItem()
{
    QList<QGraphicsItem *> selectedItems = getScene()->selectedItems();
    for( QGraphicsItem *item : selectedItems )
    {
        getScene()->removeItem( item );     // 从场景中移除
        delete item;                        // 删除对象
    }
}

void CLabelEdit::onCurItemRectChanged( QRectF rect )
{
    if(m_pLineEdit_x)
    {
        m_pLineEdit_x->setText( QString( "%1" ).arg( rect.x() ) );
        m_pLineEdit_y->setText( QString( "%1" ).arg( rect.y() ) );
        m_pLineEdit_w->setText( QString( "%1" ).arg( rect.width() ) );
        m_pLineEdit_h->setText( QString( "%1" ).arg( rect.height() ) );
    }
}

void CLabelEdit::on_label_rect_editingFinished()
{
    qreal x = m_pLineEdit_x->text().toFloat();
    qreal y = m_pLineEdit_y->text().toFloat();
    qreal w = m_pLineEdit_w->text().toFloat();
    qreal h = m_pLineEdit_h->text().toFloat();
    QRectF rect( x, y, w, h );

    qDebug() << "Item new rect: " << rect;
    if(w==0 || h== 0)
        return ;

    for( auto item : m_pScene->items() )
    {
        if( item->isSelected() )
        {
            if( auto textItem = dynamic_cast<CustomTextItem*>( item ) )
            {
                textItem->setItemRect( rect );
            }
            else if( auto pixmapItem = dynamic_cast<CustomPixmapItem*>( item ) )
            {
                pixmapItem->setItemRect( rect );
            }
            break;
        }
    }
}

void CLabelEdit::onNoItemSelected()
{
    m_pLineEdit_x->setText( "" );
    m_pLineEdit_y->setText( "" );
    m_pLineEdit_w->setText( "" );
    m_pLineEdit_h->setText( "" );
}

void CLabelEdit::resizeEvent( QResizeEvent *event )
{
    //qDebug() << "scene size( w, h ): " << getScene()->width() << "," << getScene()->height();
    //qDebug() << "view size( w, h ): " << getView()->width() << "," << getView()->height();
    //qDebug() << "mainwin size( w, h ): " << this->width() << "," << this->height();
}

void CLabelEdit::keyReleaseEvent( QKeyEvent *event )
{
    auto nKey = event->key() ;
    qDebug()<< "Key Release:" << nKey;

    bool bCtrlPress = (event->modifiers() & Qt::ControlModifier) ;
    m_bCtrlPress = bCtrlPress;

    for( auto item : m_pScene->items() )
    {
        if( item->isSelected() || bCtrlPress)
        {
            QPointF pos = item->pos() ;
            qreal fscale = item->scale() ;
            qreal fscaleN = item->scale() ;

            int x = pos.x() ;
            int y = pos.y() ;
            int nx = x ;
            int ny = y ;

            if(event->key() == Qt::Key_Left)
                nx -=5 ;
            if(event->key() == Qt::Key_Right)
                nx +=5 ;

            if(event->key() == Qt::Key_Up)
                ny -=5 ;
            if(event->key()  == Qt::Key_Down)
                ny +=5 ;

            if(event->key() == Qt::Key_Minus)
               fscaleN *= 0.9 ;
            if(event->key()  == Qt::Key_Equal)
               fscaleN *= 1.1 ;

            if(nx<0)
                nx=0;
            if(ny<0)
                ny=0;

            if(nx != x || ny != y)
                item->setPos(nx,ny);

            if(fscaleN != fscale)
                item->setScale(fscaleN);
        }
    }
}

void CLabelEdit::keyPressEvent( QKeyEvent *event )
{
    auto nKey = event->key() ;
    qDebug()<< "Key Press:" << event->modifiers() << nKey;
    if (event->key() == Qt::Key_Delete)
    {
        this->onDeleteSelectedItem();
    }

    if(event->modifiers() == Qt::ControlModifier)
    {
        m_bCtrlPress = true ;
        if(event->key() == Qt::Key_S)
            this->onSaveTemplate() ;
    }

    QWidget::keyPressEvent(event); // 确保调用基类的处理方法
}
