#ifndef DIALOGBOXLABELPRINT_H
#define DIALOGBOXLABELPRINT_H

#include <QDialog>
#include <QTimer>

namespace Ui {
class DialogBoxLabelPrint;
}

class DialogBoxLabelPrint : public QDialog
{
    Q_OBJECT

public:
    explicit DialogBoxLabelPrint(QWidget *parent = nullptr);
    ~DialogBoxLabelPrint();

private slots:
    void on_lineEditDevSN_textChanged(const QString &arg1);

private:
    Ui::DialogBoxLabelPrint *ui;

    QTimer m_TimerPrint ;
    void EnterPrint() ;
};

#endif // DIALOGBOXLABELPRINT_H
