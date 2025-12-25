#ifndef RESIZABLEITEMS_H
#define RESIZABLEITEMS_H

#include <QGraphicsItem>
#include <QGraphicsPixmapItem>
#include <QGraphicsTextItem>
#include <QGraphicsSceneMouseEvent>
#include <QCursor>
#include <QTextCursor>


class Resizable : public QObject {
    Q_OBJECT

public:
    explicit Resizable(QGraphicsItem *item) : item(item), resizing(false), moving(false)
    {
        item->setFlags( QGraphicsItem::ItemIsMovable | QGraphicsItem::ItemIsSelectable | QGraphicsItem::ItemSendsGeometryChanges );
        item->setFlag( QGraphicsItem::ItemIsFocusable );
        item->setAcceptHoverEvents(true);
    }

    void handleMousePressEvent(QGraphicsSceneMouseEvent *event)
    {
        QRectF rect = item->boundingRect();
        QPointF pos = event->pos();
        const qreal edgeThreshold = 5.0;

        initialScenePos = event->scenePos();
        initialItemPos = QPointF( initialScenePos - rect.topLeft() );
        initialRect = rect;
        qDebug() << "scenePos: " << initialScenePos << ", itemPos: " << initialItemPos << ", pos: " << pos << ", rect: " << rect;

        if (isNearEdge(pos, rect, edgeThreshold)) {
            resizing = true;
            event->accept();
            qDebug() << "isNearEdge true";
        } else {
            qDebug() << "isNearEdge false";
            moving = true;
            item->setCursor(Qt::ClosedHandCursor);  // Change cursor for moving
            event->accept();
        }
    }

    void handleMouseMoveEvent(QGraphicsSceneMouseEvent *event)
    {
        if (resizing) {
            QPointF delta = event->scenePos() - initialScenePos;
            qreal newWidth = initialRect.width() + delta.x();
            qreal newHeight = initialRect.height() + delta.y();
            item->setScale(qMin(newWidth / initialRect.width(), newHeight / initialRect.height()));  // Scaling logic
            event->accept();
        } else if (moving) {
            item->setPos(item->pos() + (event->scenePos() - initialScenePos));  // Move item
            initialScenePos = event->scenePos();
            event->accept();
        }
    }

    void handleMouseReleaseEvent(QGraphicsSceneMouseEvent *event)
    {
        if (resizing) {
            resizing = false;
            item->setCursor(Qt::ArrowCursor);  // Reset cursor
            event->accept();
        } else if (moving) {
            moving = false;
            item->setCursor(Qt::ArrowCursor);  // Reset cursor
            item->setSelected(true);
            item->setFocus();
            event->accept();
        }
    }

    void handleHoverMoveEvent(QGraphicsSceneHoverEvent *event)
    {
        QRectF rect = item->boundingRect();
        QPointF pos = event->pos();
        const qreal edgeThreshold = 5.0;

        if (isNearEdge(pos, rect, edgeThreshold)) {
            item->setCursor(Qt::SizeFDiagCursor);  // Change cursor to resizing
        } else {
            item->setCursor(Qt::ArrowCursor);  // Normal cursor
        }
    }

private:
    QGraphicsItem *item;
    bool resizing;
    bool moving;
    QPointF initialScenePos;
    QPointF initialItemPos;
    QRectF initialRect;

    bool isNearEdge( const QPointF &pos, const QRectF &rect, qreal threshold )
    {
        return (qAbs(pos.x() - rect.left()) < threshold ||
                qAbs(pos.x() - rect.right()) < threshold ||
                qAbs(pos.y() - rect.top()) < threshold ||
                qAbs(pos.y() - rect.bottom()) < threshold);
    }
};


class ResizableTextItem : public QGraphicsTextItem {
public:
    ResizableTextItem( const QString &text, QGraphicsItem *parent = nullptr )
        : QGraphicsTextItem(text, parent), resizable(this) {}

protected:
    Resizable resizable;  // Instance of Resizable to handle resizing

    // 重写事件处理
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        resizable.handleMousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override {
        resizable.handleMouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
        resizable.handleMouseReleaseEvent(event);
    }

    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override {
        resizable.handleHoverMoveEvent(event);
    }

    QRectF boundingRect() const override {
        return QGraphicsTextItem::boundingRect();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        QGraphicsTextItem::paint(painter, option, widget);
    }

    // 捕获双击事件
    void mouseDoubleClickEvent(QGraphicsSceneMouseEvent *event) override {
        // 启用编辑模式，允许用户编辑文本
        setTextInteractionFlags(Qt::TextEditorInteraction);

        // 将光标移动到双击的位置
        QTextCursor cursor = textCursor();
        cursor.clearSelection();  // 清除任何选中状态
        setTextCursor(cursor);

        // 继续处理双击事件
        QGraphicsTextItem::mouseDoubleClickEvent(event);
    }

    // 捕获失去焦点事件，当文本框失去焦点时，禁用编辑模式
    void focusOutEvent(QFocusEvent *event) override {
        // 禁用编辑模式，防止文本随时被修改
        setTextInteractionFlags(Qt::NoTextInteraction);

        // 调用基类的焦点失去处理函数
        QGraphicsTextItem::focusOutEvent(event);
    }
};


class ResizablePixmapItem : public QGraphicsPixmapItem {
public:
    ResizablePixmapItem( const QPixmap &pixmap, QGraphicsItem *parent = nullptr )
        : QGraphicsPixmapItem( pixmap, parent ), resizable(this) {}

protected:
    Resizable resizable;  // Instance of Resizable to handle resizing

    // 重写事件处理
    void mousePressEvent(QGraphicsSceneMouseEvent *event) override {
        resizable.handleMousePressEvent(event);
    }

    void mouseMoveEvent(QGraphicsSceneMouseEvent *event) override {
        resizable.handleMouseMoveEvent(event);
    }

    void mouseReleaseEvent(QGraphicsSceneMouseEvent *event) override {
        resizable.handleMouseReleaseEvent(event);
    }

    void hoverMoveEvent(QGraphicsSceneHoverEvent *event) override {
        resizable.handleHoverMoveEvent(event);
    }

    QRectF boundingRect() const override {
        return QGraphicsPixmapItem::boundingRect();
    }

    void paint(QPainter *painter, const QStyleOptionGraphicsItem *option, QWidget *widget) override {
        QGraphicsPixmapItem::paint(painter, option, widget);
    }
};


#endif // RESIZABLEITEMS_H
