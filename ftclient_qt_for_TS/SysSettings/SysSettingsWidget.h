#ifndef SYSSETTINGSWIDGET_H
#define SYSSETTINGSWIDGET_H

#include <memory>
#include <QWidget>
#include <QAbstractItemDelegate>
#include <QStandardItemModel>
#include "CSysSettings.h"

namespace Ui {
class SysSettingsWidget;
}


class SysSettingsWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SysSettingsWidget(QWidget *parent = nullptr);
    virtual ~SysSettingsWidget();

    const std::string getCurDeviceType()
    {
        return CSysSettings::getInstance()->getCurDeviceType();
    }

    const std::string getCurWorkPosId()
    {
        return CSysSettings::getInstance()->getWorkPosId();
    }

    void setCurDeviceType( const QString& newDeviceType );
    const std::vector<std::string> getAllDeviceTyps()
    {
        return CSysSettings::getInstance()->getAllDeviceTyps();
    }
    bool loadCurDevTypeTestItems( std::map<int,QJsonObject>& mapTestItems )
    {
        return CSysSettings::getInstance()->loadCurDevTypeTestCmds( mapTestItems );
    }

signals:
    void sigWindowHidden();

private slots:
    void on_pbtn_add_device_type_clicked();
    void on_pushButton_select_cfg_file_clicked();
    void on_pushButton_select_default_var_file_clicked();
    void on_comboBox_cur_device_type_currentTextChanged( const QString &text );
    void handleItemButtonClicked( const QModelIndex &index );
    void onSysStsTableEditingFinished( QWidget *editor, QAbstractItemDelegate::EndEditHint hint );

    // 标签模板
    void on_pBtn_select_dev_label_tem_clicked();
    void on_pBtn_select_box_label_tem_clicked();
    void on_pBtn_select_pack_label_tem_clicked();

    void on_pbtn_add_save_config_clicked();

private:
    bool updateCommonData();
    bool updateCurDevTypeListData();
    void updateCurDevType();

    void resetSysCfgListModel();
    void initSysCfgListUI();
    bool updateSysCfgListData();
    void updateSysCfgListTableSize();
    void updateUIData();

    void updateNetCfgData();
    void saveAllNetCfgData();

    void updateLabelTemCfgData();
    void saveAllLabelTemCfgData();

    void closeEvent( QCloseEvent *event ) override;
    void paintEvent( QPaintEvent *event ) override;

    std::shared_ptr<QStandardItemModel>& getItemModelSysCfgs();

private:
    Ui::SysSettingsWidget *ui;
    std::shared_ptr<QStandardItemModel> m_pItemModelSysCfgs;
    bool m_bDevideTypeUpdating;

};

#endif // SYSSETTINGSWIDGET_H
