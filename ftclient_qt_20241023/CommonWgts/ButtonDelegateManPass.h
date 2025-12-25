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
        QStyledItemDelegate::paint(painter, option, index);
        int row = index.row();
        if( rowsNoButton.contains( row ) )
        {
            return ;
        }

        const QString btnText = getButtonText( row );

        QStyleOptionButton button;
        QRect rect = option.rect;
        rect.adjust(2,2,-2,-2) ;
        button.rect = rect;
        button.text = btnText;
        button.state = QStyle::State_Enabled;
        QApplication::style()->drawControl( QStyle::CE_PushButton, &button, painter );
    }

    bool editorEvent(QEvent *event, QAbstractItemModel *model, const QStyleOptionViewItem &option, const QModelIndex &index) override
    {
        Q_UNUSED(model)
        if (event->type() == QEvent::MouseButtonPress) {
            QMouseEvent *mouseEvent = static_cast<QMouseEvent *>(event);
            if (option.rect.contains(mouseEvent->pos())) {
                if(!rowsNoButton.contains(index.row()))
                emit buttonClicked(index);
                return true;
            }
        }
        return false;
    }

    void switchStatus( int row ) {
        rowButtonStatus[row] = !rowButtonStatus[row];
    }
    bool getPassStatus( int row )
    {
        return rowButtonStatus[row];
    }

    bool setPassStatus( int row,bool bPass )
    {
        return rowButtonStatus[row] = bPass ;
    }

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

    void setHideButtonRow( int row ){
        rowsNoButton.insert( row );
    }
    void initButtonRow( int row ){
        rowButtonStatus[row] = false;
    }

signals:
    void buttonClicked(const QModelIndex &index) ;

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
