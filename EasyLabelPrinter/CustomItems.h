#ifndef CUSTOMITEMS_H
#define CUSTOMITEMS_H

#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QStyleOptionGraphicsItem>
#include <QCursor>
#include <QTextCursor>
#include <QObject>
#include <QDebug>
#include <QPainter>
#include <QRgba64>
#include <QWidget>

class CustomBaseItem : public QObject {
    Q_OBJECT

signals:
    void sigRectChanged(QObject *sender);

public:
    explicit CustomBaseItem( QGraphicsItem *itemIn ) : m_item( itemIn )
    {
        if( m_item )
        {
            m_item->setFlags( QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges );
            m_item->setFlag( QGraphicsItem::ItemIsFocusable );
            m_item->setAcceptHoverEvents(true);
        }
    }

    void emitRectSig()
    {
        emit sigRectChanged( this );
    }

    QRectF getItemCurRect()
    {
        QRectF rect = m_item->boundingRect();
        if( m_item != nullptr )
        {
            qreal scale = m_item->scale();
            if(scale <= 0.05)
                scale = 1;

            rect = QRectF( m_item->pos().x(), m_item->pos().y(), rect.width()*scale, rect.height()*scale );
        }
        return rect;
    }

    void handleMousePressEvent( QGraphicsSceneMouseEvent *event )
    {
        QRectF rect = m_item->boundingRect();
        QPointF pos = event->pos();
        const qreal edgeThreshold = 2.0;

        initialScenePos = event->scenePos();
        initialRect = getItemCurRect();

        if( isNearEdge( pos, rect, edgeThreshold ) )
        {
            resizing = true;
            event->accept();
        }
        else
        {
            moving = true;
            m_item->setCursor(Qt::ClosedHandCursor);  // Change cursor for moving
            event->accept();
        }

        //emitRectSig();
    }

    void handleMouseMoveEvent( QGraphicsSceneMouseEvent *event )
    {
        if( resizing )
        {
            QPointF delta = event->scenePos() - initialScenePos;
            qreal newWidth = initialRect.width() + delta.x();
            qreal newHeight = initialRect.height() + delta.y();
            m_item->setScale( qMax( newWidth / initialRect.width(), newHeight / initialRect.height() ) );  // Scaling logic
            event->accept();
        }
        else if( moving )
        {
            m_item->setPos( m_item->pos() + ( event->scenePos() - initialScenePos ) );  // Move item
            initialScenePos = event->scenePos();
            event->accept();
        }

        //emitRectSig();
    }

    void handleMouseReleaseEvent( QGraphicsSceneMouseEvent *event )
    {
        if (resizing) {
            resizing = false;
            m_item->setCursor(Qt::ArrowCursor);  // Reset cursor
            event->accept();
        } else if (moving) {
            moving = false;
            m_item->setCursor(Qt::ArrowCursor);  // Reset cursor
            m_item->setSelected(true);
            m_item->setFocus();
            event->accept();
        }

        emitRectSig();
    }

    void handleHoverMoveEvent( QGraphicsSceneHoverEvent *event )
    {
        QRectF rect = m_item->boundingRect();
        QPointF pos = event->pos();
        const qreal edgeThreshold = 3.0;

        if ( isNearEdge( pos, rect, edgeThreshold ) )
        {
            m_item->setCursor( Qt::SizeFDiagCursor );  // Change cursor to resizing
        }
        else
        {
            m_item->setCursor( Qt::ArrowCursor );  // Normal cursor
        }
    }

    void setItemRect( const QRectF &rectNew )
    {
        if( m_item != nullptr )
        {
            if( rectNew.topLeft() != m_item->pos() )
            {
                m_item->setPos( rectNew.topLeft() );
            }

            QRectF origRect = m_item->boundingRect();

            QRectF rect_last = getItemCurRect();
            int w_last = rect_last.width();
            int h_last = rect_last.height();
            int w_new = rectNew.width();
            int h_new = rectNew.height();

            qreal newScaleMax = qMax( rectNew.width() / origRect.width(), rectNew.height() / origRect.height() );
            qreal w_scale_new = rectNew.width() / origRect.width();
            qreal h_scale_new = rectNew.height() / origRect.height();
            qreal newScale = 1;
            if( w_last == w_new && h_last != h_new )
            {
                newScale = h_scale_new;
            }
            else if( h_last == h_new && w_last != w_new )
            {
                newScale = w_scale_new;
            }
            else if( h_last != h_new && w_last != w_new )
            {
                newScale = newScaleMax;
            }
            else
            {
                return;
            }

            m_item->setScale( newScale );

            emitRectSig();
        }
    }

private:
    QGraphicsItem *m_item=nullptr;
    bool resizing=false;
    bool moving=false;
    QPointF initialScenePos;
    QRectF initialRect;

    bool isNearEdge( const QPointF &pos, const QRectF &rect, qreal threshold )
    {
        return ( qAbs(pos.x() - rect.right()) < threshold ||
                qAbs(pos.y() - rect.bottom()) < threshold );
    }
};


class CustomTextItem : public QGraphicsTextItem
{
     Q_OBJECT

signals:
    void sigRectChanged_( QObject *sender );
public:
    CustomTextItem( const QString &text, QGraphicsTextItem *parentGraph = nullptr )
        : QGraphicsTextItem(text, parentGraph), base(this)
    {
        connect( &base, &CustomBaseItem::sigRectChanged, [=]( QObject *sender ){
            emit sigRectChanged_( this );
        } );
    }

    QString m_strName ;
    bool m_bShowRect = false ;
    void setName(const QString&strName) {m_strName = strName;}
    QString getName(){return m_strName;}

    void setItemRect( const QRectF&rectNew ){ base.setItemRect( rectNew ); }
    QRectF getItemRect(){  return base.getItemCurRect(); }

protected:
    CustomBaseItem base;  // Instance of Resizable to handle resizing

    // 重写事件处理
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        base.handleMousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override {
        base.handleMouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
        base.handleMouseReleaseEvent(event);
    }

    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override {
        base.handleHoverMoveEvent(event);
    }

    QRectF boundingRect() const override {
        return QGraphicsTextItem::boundingRect();
    }

    void paint( QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget ) override {
        QGraphicsTextItem::paint( painter, option, widget );
        QRectF A =  this->boundingRect();
        if(this->isSelected())
        {
            QPen pen = QPen(Qt::red) ;
            painter->setPen(pen) ;

            A.adjust(0,0,-1,-1) ;
            painter->drawRect(A) ;
        }
        if(m_bShowRect)
        {
            QPen pen = QPen(Qt::black) ;
            painter->setPen(pen) ;
            QRectF A =  this->boundingRect();
            A.adjust(2,2,-2,-2) ;
            painter->drawRect(A) ;
        }
    }

    QTextCursor cursor = textCursor();
    // 捕获双击事件
    void mouseDoubleClickEvent( QGraphicsSceneMouseEvent *event ) override {
        // 启用编辑模式，允许用户编辑文本
        setTextInteractionFlags( Qt::TextEditorInteraction );

        // 将光标移动到双击的位置
        cursor.clearSelection();  // 清除任何选中状态
        setTextCursor( cursor );

        QGraphicsTextItem::mouseDoubleClickEvent( event );
    }

    // 捕获失去焦点事件，当文本框失去焦点时，禁用编辑模式
    void focusOutEvent( QFocusEvent *event ) override {
        cursor.clearSelection();

        // 禁用编辑模式，防止文本随时被修改
        setTextInteractionFlags( Qt::NoTextInteraction );
        unsetCursor() ;
        QGraphicsTextItem::focusOutEvent( event );
    }
};


class CustomPixmapItem : public QObject, public QGraphicsPixmapItem
{
    Q_OBJECT

signals:
    void sigRectChanged_( QObject *sender );

public:
    CustomPixmapItem( const QPixmap &pixmap, QGraphicsItem *parent = nullptr )
        : QObject(), QGraphicsPixmapItem( pixmap, parent ), base(this)
    {
        connect( &base, &CustomBaseItem::sigRectChanged, this, [=]( QObject *sender ){
            emit sigRectChanged_( this );
        } );
    }

    CustomPixmapItem( QGraphicsItem *parent = nullptr )
        : QGraphicsPixmapItem( parent ), base(this)
    {
        connect( &base, &CustomBaseItem::sigRectChanged, this, [=]( QObject *sender ){
            emit sigRectChanged_( this );
        } );
    }

    QString m_strName ;
    bool m_bShowRect = false ;

    void setName(const QString&strName) {m_strName = strName;}
    QString getName(){return m_strName;}

    void setItemRect( const QRectF rectNew ){  base.setItemRect( rectNew ); }
    QRectF getItemRect(){  return base.getItemCurRect(); }

protected:
    CustomBaseItem base;

    void mousePressEvent(QGraphicsSceneMouseEvent *event) override { base.handleMousePressEvent(event);}
    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override { base.handleMouseMoveEvent(event); }
    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override { base.handleMouseReleaseEvent(event); }
    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override { base.handleHoverMoveEvent(event); }

    QRectF boundingRect() const override { return QGraphicsPixmapItem::boundingRect(); }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        //QGraphicsPixmapItem::paint(painter, option, widget);
        QPixmap pix = pixmap() ;
        QRectF A = boundingRect();
        painter->drawImage(A,pix.toImage());
        if(this->isSelected())
        {
            painter->setPen(Qt::red) ;
            painter->drawRect(A) ;
        }
    }
};


#endif // CUSTOMITEMS_H
