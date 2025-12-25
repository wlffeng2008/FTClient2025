#ifndef DIALOGDEFAULTVALUES_H
#define DIALOGDEFAULTVALUES_H


#include <QDialog>
#include <QStandardItemModel>

namespace Ui {
class DialogDefaultValues;
}

class DialogDefaultValues : public QDialog
{
    Q_OBJECT

public:
    explicit DialogDefaultValues(QWidget *parent = nullptr);
    ~DialogDefaultValues();

    void updateItemData( const std::vector<QJsonObject> &vecItems );

private slots:
    void on_pbtn_reset_clicked();
    void on_pbtn_write_clicked();
    void on_checkBox_enable_all_items_checkStateChanged( Qt::CheckState state );
    void handleItemButtonClicked( const QModelIndex &index );

signals:
    void writeDefaultValue(const QJsonArray&jValue) ;

private:
    void initValuesTable();
    void writeItemVars( int row_index = -1);

private:
    Ui::DialogDefaultValues *ui;
    QStandardItemModel *m_pItemModel = nullptr;

private:
};

#endif // DIALOGDEFAULTVALUES_H
