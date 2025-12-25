#ifndef BUTTONDELEGATE_H
#define BUTTONDELEGATE_H

#include <QStyledItemDelegate>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>


class ButtonDelegate : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ButtonDelegate( const QString& text, QObject *parent = nullptr ) : QStyledItemDelegate( parent ), m_sBtnText( text ) {

    }

    void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const override
    {
        QStyledItemDelegate::paint(painter, option, index);

        QString textCustom = index.data( Qt::UserRole + 1 ).toString().trimmed();
        QString btn_text = textCustom.isEmpty() ? m_sBtnText : textCustom;

        QStyleOptionButton button;
        QRect rctmp = option.rect ;
        rctmp.adjust(2,2,-2,-3) ;
        button.rect = rctmp;
        button.text = btn_text;
        //button.features = QStyleOptionButton::HasMenu;
        button.state = QStyle::State_Enabled;
        painter->setBackground(QBrush(Qt::blue));
        QApplication::style()->drawControl( QStyle::CE_PushButton, &button, painter );
    }


    bool editorEvent( QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index ) override
    {
        Q_UNUSED(model)
        QApplication::restoreOverrideCursor();
        if( event->type() == QEvent::MouseButtonRelease )
        {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>( event );
            if( option.rect.contains( mouseEvent->pos() ) && mouseEvent->button() == Qt::LeftButton )
            {
                emit buttonClicked( index );
                return true;
            }
        }

        if (event->type() == QEvent::Enter) {
            QApplication::setOverrideCursor(Qt::PointingHandCursor);
            return true;
        }

        if(event->type() == QEvent::MouseMove)
        {
            QApplication::setOverrideCursor(Qt::PointingHandCursor) ;
            return true;
        }
        return false;
    }

signals:
    void buttonClicked( const QModelIndex &index ) ;

private:
    QString m_sBtnText;

};


#endif // BUTTONDELEGATE_H
