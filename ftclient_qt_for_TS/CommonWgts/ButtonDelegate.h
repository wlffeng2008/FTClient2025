#ifndef BUTTONDELEGATE_H
#define BUTTONDELEGATE_H

#include <QStyledItemDelegate>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>
#include <QPalette>

// 自定义事件过滤器类
class MyEventFilter : public QObject
{
    Q_OBJECT
public:
    explicit MyEventFilter(QObject *parent = nullptr) : QObject(parent)
    {

    }

protected:
    bool eventFilter(QObject *watched, QEvent *event) override
    {
        qDebug() << "MyEventFilter::eventFilter" << event->type();
        if( event->type() == QEvent::MouseMove)
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);

            return true;
        }
        if( event->type() == QEvent::Leave)
        {
            QApplication::restoreOverrideCursor();

            return true;
        }
        return QObject::eventFilter(watched, event);
    }
};

class ButtonDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:

    MyEventFilter filter;
    ButtonDelegate( const QString& text, QObject *parent = nullptr ) :
        QStyledItemDelegate( parent ), m_sBtnText( text )
    {
        installEventFilter(&filter) ;
    }

    // 绘制按钮
    void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const override
    {
        QString btnText = m_sBtnText;
        QString textCustom = index.data( Qt::UserRole + 1 ).toString().trimmed();
        btnText = textCustom.isEmpty() ? m_sBtnText : textCustom;

        QRect rcTmp = option.rect ;
        rcTmp.adjust(4,3,-4,-3) ;
        QStyleOptionButton button;
        button.rect = rcTmp;
        button.text = btnText;
        button.state = QStyle::State_Enabled;

        if(m_bClicked && m_clickIndex == index)
        {
            //QPalette palette;
            //palette.setColor(QPalette::ButtonText, Qt::red);
            //palette.setColor(QPalette::Button,(QColor(0,0,255)));
            //button.palette = palette ;
            //button.state = QStyle::State_Active;
            //button.icon = QIcon("D:\\A.png") ;
            //button.iconSize = QSize(20,20) ;
        }

        if(m_bClicked && m_clickIndex == index)
        {
            painter->setPen(QPen(Qt::blue)) ;
            painter->setBrush(QBrush(Qt::blue));
        }
        else
        {
            painter->setPen(QPen(QColor(0, 253, 100),2)) ;
            painter->setBrush(QBrush(QColor(0, 128, 0)));

            //QApplication::style()->drawControl( QStyle::CE_PushButton, &button, painter );
        }
        painter->drawRoundedRect(rcTmp,4,4);
        painter->setPen(QPen(Qt::white)) ;
        painter->drawText(rcTmp,Qt::AlignVCenter|Qt::AlignHCenter,btnText);

        QPen fixLine(QBrush(QColor("skyblue")),1);
        painter->setPen(fixLine)  ;
        painter->drawLine(option.rect.bottomLeft(),option.rect.bottomRight()) ;
    }

    // 确保按钮的大小合适
    QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        return QSize( 80, 30 );  // 按钮的大小
    }

    bool editorEvent( QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index ) override
    {
        Q_UNUSED(option)
        Q_UNUSED(model)
        if( event->type() == QEvent::MouseButtonPress )
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>( event );
            if( option.rect.contains( mouseEvent->pos() ) )
            {
                m_bClicked = true ;
                m_clickIndex = index ;
                emit buttonClicked( index );
                return true;
            }
        }
        return false;
    }

signals:
    void buttonClicked( const QModelIndex &index ) const;

private:
    QString m_sBtnText;

    bool m_bClicked = false ;
    QModelIndex m_clickIndex ;
};


#endif // BUTTONDELEGATE_H
