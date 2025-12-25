#ifndef CLABELSETTINGS_H
#define CLABELSETTINGS_H

#include <QWidget>
#include "Label.h"

namespace Ui {
class CLabelSettings;
}

class CLabelSettings : public QWidget
{
    Q_OBJECT

public:
    explicit CLabelSettings(QWidget *parent = nullptr);
    virtual ~CLabelSettings();

    void setLabelPara( const label::LabelPara& p )
    {
        m_para = p;
        applyPara();
    }

    label::LabelPara getLabelPara()
    {
        return m_para;
    }

private slots:
    void on_pBtn_ok_clicked();
    void on_pBtn_cancel_clicked();

private:
    void applyPara();

signals:
    void sigNewPara( label::LabelPara p );

private:
    Ui::CLabelSettings *ui;
    label::LabelPara m_para;
};

#endif // CLABELSETTINGS_H
