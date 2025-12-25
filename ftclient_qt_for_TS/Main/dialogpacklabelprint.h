#ifndef DIALOGPACKLABELPRINT_H
#define DIALOGPACKLABELPRINT_H

#include <QDialog>

namespace Ui {
class DialogLabelPrint;
}

class DialogLabelPrint : public QDialog
{
    Q_OBJECT

public:
    explicit DialogLabelPrint(QWidget *parent = nullptr);
    ~DialogLabelPrint();

private slots:
    void on_plainTextEdit_textChanged();
signals:
    void reportPachInfo(const QString&strBoxNo,const QStringList&strDeviceNos) ;

private:
    Ui::DialogLabelPrint *ui;
    int m_nPrintCount = 0 ;
    void MakePrintSN() ;
    void startPrint() ;
};

#endif // DIALOGPACKLABELPRINT_H
