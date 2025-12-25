#ifndef WGTLABELPARASTART_H
#define WGTLABELPARASTART_H

#include <QWidget>
#include "Label.h"


namespace Ui {
class WgtLabelParaStart;
}


class WgtLabelParaStart : public QWidget
{
    Q_OBJECT

public:
    explicit WgtLabelParaStart( QWidget *parent = nullptr );
    virtual ~WgtLabelParaStart();
    void setPara( const label::LabelPara& p );

private slots:
    void on_pBtn_cancel_clicked();
    void on_pBtn_create_clicked();

signals:
    void sigPara( label::LabelPara p );
    void sigStartEdit();

private:
    void updateParaOnUI();

private:
    Ui::WgtLabelParaStart *ui;
    label::LabelPara m_para;
};

#endif  // WGTLABELPARASTART_H
