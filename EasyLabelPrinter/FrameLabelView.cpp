#include "FrameLabelView.h"
#include "ui_FrameLabelView.h"

#include "CLabelSave.h"

#include "zint.h"

#include <QTimer>
#include <QRectF>
#include <QFileDialog>


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
        strcpy_s(symbol->outfile, strFile.c_str()) ;

        const char *lpszText = strText.c_str();
        int nLen = strlen(lpszText);
        int nRet = ::ZBarcode_Encode(symbol,(const unsigned char *)lpszText,nLen); //编码
        if (nRet == 0)
            nRet = ::ZBarcode_Print(symbol,0); //antate angle 旋转角度

        ::ZBarcode_Delete(symbol);

        imgBar.load(strFile.c_str());
    }
    return imgBar ;
}

static QFont s_font;

FrameLabelView::FrameLabelView(QWidget *parent)
    : QFrame(parent)
    , ui(new Ui::FrameLabelView)
{
    ui->setupUi(this);
    s_font.setFamily("方正兰亭黒_GBK");
    s_font.setPointSize(9);

    m_pView = ui->graphicsView;

    m_pScene = new CustomScene( this );

    m_pView->setScene(m_pScene) ;
    m_pScene->setView(m_pView) ;

    m_pScene->setSceneRect(QRectF(0,0,m_pView->size().width(),m_pView->size().width())) ;
    m_pView->setHorizontalScrollBarPolicy( Qt::ScrollBarAlwaysOff );
    m_pView->setVerticalScrollBarPolicy( Qt::ScrollBarAlwaysOff );

    connect( m_pScene, &CustomScene::sigNoItemSelected,this, [=](){
        emit onItemSelected(nullptr) ;
    });
}

FrameLabelView::~FrameLabelView()
{
    delete ui;
}

void FrameLabelView::resizeEvent(QResizeEvent *event)
{
    QSize LSize = ui->graphicsView->size() ;
    int nW = LSize.width() ;
    int nH = LSize.height() ;

    if(nH < nW)
        ui->graphicsView->setFixedWidth(nH) ;
    else
        ui->graphicsView->setFixedHeight(nW) ;

    m_pScene->setSceneRect(QRectF(0,0,m_pView->size().width(),m_pView->size().width())) ;

    QFrame::resizeEvent(event) ;
}

QFont FrameLabelView::GetFont()
{
    if(!m_bCtrlPress)
    {
        for (auto item : m_pScene->items())
        {
            if (auto textItem = dynamic_cast<CustomTextItem*>(item))
            {
                if(textItem->isSelected())
                   return textItem->font();
            }
        }
    }
    return s_font;
}

void FrameLabelView::SetFont(QFont&font)
{
    if(m_bCtrlPress)
        s_font = font;

    for (auto item : m_pScene->items())
    {
        if (auto textItem = dynamic_cast<CustomTextItem*>(item))
        {
            if(textItem->isSelected() || m_bCtrlPress)
                textItem->setFont(font);
        }
    }
}

void FrameLabelView::Load(const QString&srtFile)
{
    m_strTemlate = srtFile ;
    if(srtFile.isEmpty())
        return ;

    emit onItemSelected(nullptr) ;

    CLabelSave::loadSceneWithImages(m_pScene,srtFile) ;

    for (auto item : m_pScene->items())
    {
        if (auto textItem = dynamic_cast<CustomTextItem*>(item))
        {
            connect( textItem, &CustomTextItem::sigRectChanged_ , [this]( QObject *pSender ){
                emit onItemSelected(pSender) ;
            } );
        }
        else if (auto pixmapItem = dynamic_cast<CustomPixmapItem*>(item))
        {
            connect( pixmapItem, &CustomPixmapItem::sigRectChanged_ , [this](QObject *pSender ){
                emit onItemSelected(pSender) ;
            } );
        }
    }
}

void FrameLabelView::Save()
{
    if(m_strTemlate.isEmpty())
    {
        QString strFile = QFileDialog::getSaveFileName(this, "保存模板文件", QApplication::applicationDirPath() + "/config", "Template Files (*.tem);;All Files (*.*)");

        if(strFile.isEmpty())
            return;

        m_strTemlate = strFile;
        setWindowTitle("标签打印模板 - " + strFile) ;
    }
    CLabelSave::saveSceneWithImages(m_pScene,m_strTemlate) ;
}

void FrameLabelView::Preview()
{
    QRectF rc(0,0,m_pView->size().width(),m_pView->size().height()) ;
    m_pView->printPreviewAndPrint(rc) ;
}

void FrameLabelView::Print()
{
    QRectF rc(0,0,m_pView->size().width(),m_pView->size().height()) ;
    m_pView->print(rc) ;
}

void FrameLabelView::AddText(const QString&strText, const QString&strName)
{
    CustomTextItem* textItem = m_pScene->getTextItem(strName);
    if(!textItem)
    {
        textItem = new CustomTextItem( strText );
        m_pScene->addItem( textItem );
        textItem->setPos( 0, m_pView->size().height()/2 );
        textItem->setName(strName) ;

        textItem->setFont(s_font);

        connect( textItem, &CustomTextItem::sigRectChanged_ ,this, [=]( QObject *pSender ){
            emit onItemSelected(pSender) ;
        } );
    }
    textItem->setPlainText(strText) ;
}

void FrameLabelView::AddImage(const QImage & image, const QString&strName)
{
    if(image.isNull())
        return ;
    CustomPixmapItem* imgItem = m_pScene->getPixmapItem(strName);
    if(!imgItem)
    {
        imgItem = new CustomPixmapItem() ;
        m_pScene->addItem( imgItem );
        imgItem->setPos( 0, m_pView->size().height()/2 );
        imgItem->setName(strName) ;

        connect( imgItem, &CustomPixmapItem::sigRectChanged_ , this, [=]( QObject *pSender ){
            emit onItemSelected(pSender) ;
        } );
    }
    imgItem->setPixmap(QPixmap::fromImage(image)) ;
}

void FrameLabelView::AddImageFile(const QString&strFile, const QString&strName)
{
    QImage image = QImage(strFile)  ;
    AddImage(image,strName) ;
}

void FrameLabelView::AddImageQR(const QString&strQrText, const QString&strName)
{
    QImage image = genQrCode(strQrText.toStdString().c_str()) ;
    AddImage(image,strName) ;
}

void FrameLabelView::Delete()
{
    QList<QGraphicsItem *> items = m_pScene->selectedItems();
    for( QGraphicsItem *item : items )
    {
        m_pScene->removeItem( item );
        delete item;
    }
    Save() ;
}

void FrameLabelView::keyReleaseEvent( QKeyEvent *event )
{
    auto nKey = event->key() ;
    qDebug()<< "Key Release:" << Qt::hex << nKey;

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
                fscaleN *= 0.95 ;
            if(event->key()  == Qt::Key_Equal)
                fscaleN *= 1.05 ;

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

    QFrame::keyReleaseEvent(event) ;
}

void FrameLabelView::keyPressEvent( QKeyEvent *event )
{
    auto nKey = event->key() ;
    qDebug()<< "Key Press:" << Qt::hex << event->modifiers() << nKey;
    if (event->key() == Qt::Key_Delete)
    {
        Delete() ;
    }

    if(event->modifiers() == Qt::ControlModifier)
    {
        m_bCtrlPress = true ;
        if(event->key() == Qt::Key_S)
            this->Save() ;
    }

    QFrame::keyPressEvent(event); // 确保调用基类的处理方法
}

