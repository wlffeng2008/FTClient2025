#ifndef CLABELEDIT_H
#define CLABELEDIT_H

#include <QMainWindow>
#include <QGraphicsView>
#include <QGraphicsScene>
#include <QGraphicsTextItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsRectItem>
#include <QGraphicsItem>
#include <QMouseEvent>
#include <QGraphicsSceneMouseEvent>
#include <QPrinter>
#include <QPrintDialog>
#include <QPrintPreviewDialog>
#include <QLineEdit>
#include <QCheckBox>
#include <memory>
#include "CLabelSettings.h"
#include "WgtLabelParaStart.h"
#include "CLabelSave.h"


class CustomScene : public QGraphicsScene {
    Q_OBJECT

public:
    CustomScene( QObject *parent = nullptr ) : QGraphicsScene( parent )
    {
        QObject::connect( this, &QGraphicsScene::selectionChanged, [=](){
            on_selectionChanged();
        } );
    }

signals:
    void sigNoItemSelected();

private slots:
    void on_selectionChanged()
    {
        QList<QGraphicsItem *> selectedItems = this->selectedItems();
        if( selectedItems.size() > 1 )
        {
            for( int i = 0; i < selectedItems.size(); ++i )
            {
                if( m_pLastClickedItem != selectedItems[i] )
                {
                    selectedItems[i]->setSelected(false);
                }
            }
        }
    }

protected:
    void mousePressEvent( QGraphicsSceneMouseEvent *event ) override
    {
        // 获取点击位置的 item
        QGraphicsItem *clickedItem = itemAt( event->scenePos(), QTransform() );

        // 如果点击的是空白区域 (clickedItem == nullptr)，取消所有选中状态
        if( !clickedItem )
        {
            clearSelection();  // 取消所有选中的 item
            emit sigNoItemSelected();
        }
        m_pLastClickedItem = clickedItem;

        // 调用父类的 mousePressEvent 以保持其他默认行为
        QGraphicsScene::mousePressEvent( event );
    }

private:
    QGraphicsItem *m_pLastClickedItem;
};

class CustomView : public QGraphicsView {
    Q_OBJECT

public:
    CustomView(QWidget *parent = nullptr) : QGraphicsView(parent) {
        this->setBackgroundBrush( QBrush( Qt::transparent ) );  // 示例：设置为白色
    }

    QPixmap grabSpecifiedArea(const QRectF &area) {
        if (area.isEmpty()) {
            return QPixmap();  // 如果指定区域为空，返回空
        }

        QPixmap pixmap(area.width(), area.height());
        pixmap.fill(Qt::white);  // 背景色为白色

        // 使用 QPainter 将指定区域的内容渲染到 pixmap 上
        QPainter painter(&pixmap);
        this->scene()->render(&painter, QRectF(pixmap.rect()), area);  // 将场景的指定区域绘制到 QPixmap
        return pixmap;
    }

    void printPreviewAndPrint(const QRectF &area) {
        QPrinter printer(QPrinter::HighResolution);  // 使用高分辨率的 QPrinter
        printer.setOutputFormat(QPrinter::NativeFormat);  // 使用系统默认的打印机
        //printer.setPageSize(QPageSize(QPageSize::A4));  // 默认设置为 A4 纸

        QPrintPreviewDialog previewDialog(&printer, this);

        // 连接预览对话框的信号，当需要预览时调用槽函数 printArea
        connect(&previewDialog, &QPrintPreviewDialog::paintRequested, this, [=](QPrinter *printer) {
            this->printArea(printer, area);
        });

        previewDialog.exec();
    }

    void print(const QRectF &area) {
        QPrinter printer(QPrinter::HighResolution);      // 使用高分辨率的 QPrinter
        printer.setOutputFormat(QPrinter::NativeFormat);  // 使用系统默认的打印机
        //printer.setPageSize(QPageSize(QPageSize::A4));  // 默认设置为 A4 纸
        printArea( &printer, area );
    }

private:
    void printArea(QPrinter *printer, const QRectF &area) {
        QPixmap pixmap = grabSpecifiedArea(area);
        if (!pixmap.isNull())
        {
            QPainter painter(printer);
            QRect rect = painter.viewport();
            QSize size = pixmap.size();
            size.scale(rect.size(), Qt::KeepAspectRatio);  // 保持比例缩放
            painter.setViewport(rect.x(), rect.y(), size.width(), size.height());
            painter.setWindow(pixmap.rect());
            painter.drawPixmap(0, 0, pixmap);
        }
    }
};


class CLabelEdit : public QMainWindow
{
    Q_OBJECT

public:
    explicit CLabelEdit( QWidget *parent = nullptr );
    virtual ~CLabelEdit();
    void showWindow();

    static CLabelEdit* getInstance(){
        if( nullptr == m_pIns ){
            m_pIns = new CLabelEdit();
        }
        return m_pIns;
    }

    bool loadTemplate( const QString& tempFilePath );
    bool loadTemplate();

    void setLabelInfo(const QString&strUuid,const QString&strSN) ;
    void setLabelInfo2(const QString&strSN,const QString&strKey) ;
    void setLabelInfo3(const QString&strSN,const QString&strKey,const QString&strPN,bool bAutoPrint=false) ;
    void setLabelInfo4(const QStringList&strDeviceNos,bool bAutoPrint=false);

    bool printPage();

    void setMode(int nMode);

private slots:
    void onNewTemplate();
    void onSaveTemplate();
    void onLoadTemplate();
    void onPrint();
    void onPrintPreview();
    void onAddImg();
    void onAddText();
    void onAddBarSN();
    void onAddUUID();
    void onAddQrCode();
    void onAddBarCode();
    void onSettings();

    void onDeleteSelectedItem();
    void onCurItemRectChanged( QRectF rect );

    void on_label_rect_editingFinished();
    void onNoItemSelected();

private:
    std::shared_ptr<CustomView> getView();
    std::shared_ptr<CustomScene> getScene();
    std::shared_ptr<CLabelSettings> getSettingsWgt();
    std::shared_ptr<WgtLabelParaStart> getWgtStart();
    void initToolBar();
    void updateWinSize();
    void addTestObjs();
    bool findItemByTypeIn( QGraphicsItem **ppItem, const QString& typeIn,int nIndex=1 );
    void checkPara();

    void resizeEvent( QResizeEvent *event ) override;
    void keyPressEvent( QKeyEvent *event ) override;
    void keyReleaseEvent( QKeyEvent *event ) override;

private:
    static CLabelEdit* m_pIns;
    std::shared_ptr<CustomView > m_pView;
    std::shared_ptr<CustomScene> m_pScene;
    std::shared_ptr<CLabelSettings> m_pSettings;
    std::shared_ptr<WgtLabelParaStart> m_pWgtStart;
    QString m_sCurTemplateFileName;
    label::LabelPara m_para;

    QCheckBox *m_pAutoPrt = nullptr;

    std::shared_ptr<QLineEdit> m_pLineEdit_x;
    std::shared_ptr<QLineEdit> m_pLineEdit_y;
    std::shared_ptr<QLineEdit> m_pLineEdit_w;
    std::shared_ptr<QLineEdit> m_pLineEdit_h;

    TemplInfo m_TI;

    int m_nMode = 0  ;
    bool m_bCtrlPress = false ;
};

#endif  // CLABELEDIT_H
