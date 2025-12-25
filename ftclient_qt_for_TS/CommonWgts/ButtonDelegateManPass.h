#ifndef BUTTONDELEGATE_MAN_PASS_H
#define BUTTONDELEGATE_MAN_PASS_H

#include <QStyledItemDelegate>
#include <QPushButton>
#include <QApplication>
#include <QMouseEvent>
#include <QPainter>

class ButtonDelegateManPass : public QStyledItemDelegate
{
    Q_OBJECT

public:
    ButtonDelegateManPass( const QString& text_passed, const QString& text_unpassed, QObject *parent = nullptr ) : QStyledItemDelegate(parent),
        m_sBtnText_passed( text_passed ), m_sBtnText_unpassed( text_unpassed ){}

    // 绘制按钮
    void paint( QPainter *painter, const QStyleOptionViewItem &option, const QModelIndex &index ) const override
    {
        int row = index.row();
        painter->setPen(QColor(255,0,0)) ;
        if( rowsNoButton.contains( row ) ) {
            // 其他行不显示按钮，调用基类的 paint 方法
            QStyledItemDelegate::paint(painter, option, index);
        }
        else
        {
            if( row >= 0 )
            {
                const QString btnText = getButtonText( row );
                QRect rcTmp = option.rect ;
                rcTmp.adjust(4,3,-4,-3) ;

                painter->setPen(QPen(QColor(0, 253, 100),2)) ;
                painter->setBrush(QBrush(QColor(0, 128, 0)));

                painter->drawRoundedRect(rcTmp,4,4);
                painter->setPen(QPen(Qt::white)) ;
                painter->drawText(rcTmp,Qt::AlignVCenter|Qt::AlignHCenter,btnText);
                QPen fixLine(QBrush(QColor("skyblue")),1);
                painter->setPen(fixLine)  ;
                painter->drawLine(option.rect.bottomLeft(),option.rect.bottomRight()) ;
            }
        }
    }

    // 确保按钮的大小合适
    QSize sizeHint( const QStyleOptionViewItem &option, const QModelIndex &index ) const override
    {
        Q_UNUSED(option)
        Q_UNUSED(index)
        return QSize( 80, 30 );  // 按钮的大小
    }

    // 处理点击事件
    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override
    {
        Q_UNUSED(model)
        Q_UNUSED(option)
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (option.rect.contains(mouseEvent->pos())) {
                emit buttonClicked(index);
                return true;
            }
        }
        return false;
    }

    void switchStatus( int row ) { rowButtonStatus[row] = !rowButtonStatus[row]; }

    bool getPassStatus( int row ) { return rowButtonStatus[row]; }
    bool getAllPassed()
    {
        for( bool status : std::as_const(rowButtonStatus) )
        {
            if( !status )
                return false;
        }
        return true;
    }

    void markAllStatus( bool bPassed )
    {
        for( auto &value : rowButtonStatus )
        {
            value = bPassed;
        }
    }

    void setHideButtonRow( int row ){ rowsNoButton.insert( row ); }
    void initButtonRow( int row ){ rowButtonStatus[row] = false; }

signals:
    void buttonClicked(const QModelIndex &index) const;

private:
    const QString getButtonText( int row ) const
    {
        if( rowButtonStatus[row] )
            return m_sBtnText_passed;
        else
            return m_sBtnText_unpassed;
    }

private:
    QString m_sBtnText_passed, m_sBtnText_unpassed;
    QMap<int, bool> rowButtonStatus;
    QSet<int> rowsNoButton;  // 保存需要显示按钮的行号
};

#endif      // BUTTONDELEGATE_MAN_PASS_H
