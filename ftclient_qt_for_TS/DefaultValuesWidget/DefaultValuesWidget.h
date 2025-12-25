#ifndef DEFAULTVALUESWIDGET_H
#define DEFAULTVALUESWIDGET_H

#include <QWidget>
#include <QStandardItemModel>

#include <QJsonArray>
#include <QJsonObject>

namespace Ui {
class DefaultValuesWidget;
}

class DefaultValuesWidget : public QWidget
{
    Q_OBJECT

public:
    explicit DefaultValuesWidget( QWidget *parent = nullptr );
    virtual ~DefaultValuesWidget();
    void updateItemData( const std::vector<QJsonObject> &vecItems );

private slots:
    void on_pbtn_reset_clicked();
    void on_pbtn_write_clicked();
    void on_pbtn_write_all_clicked();
    void on_checkBox_enable_all_items_checkStateChanged( Qt::CheckState state );
    void handleItemButtonClicked( const QModelIndex &index );

signals:
    void writeDefaultValue(const QJsonArray&jValue) ;

private:
    void initTable();
    void writeAnItemVar( int row_index );

private:
    Ui::DefaultValuesWidget *ui;
    QStandardItemModel *m_pItemModel = nullptr;

};

#endif // DEFAULTVALUESWIDGET_H
