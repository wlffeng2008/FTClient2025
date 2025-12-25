#ifndef SERIALTESTDIALOG_H
#define SERIALTESTDIALOG_H

#include "gencomport.h"

#include <QDialog>
#include <QStandardItem>
#include <QStandardItemModel>

namespace Ui {
class SerialTestDialog;
}

class SerialTestDialog : public QDialog
{
    Q_OBJECT

public:
    explicit SerialTestDialog(QWidget *parent = nullptr);
    ~SerialTestDialog();

private slots:
    void on_pushButton_clicked();

    void on_pushButtonLoad_clicked();

    void on_pushButtonSave_clicked();

    void on_pushButtonSaveAs_clicked();

    void on_pushButtonHide_clicked();

    void on_checkBox_clicked();

    void on_comboBox_currentIndexChanged(int index);

    void on_pushButtonSend_clicked();

    void on_pushButtonNew_clicked() ;

    void on_pushButtonDel_clicked() ;

private:
    Ui::SerialTestDialog *ui;

    QStandardItemModel *m_model = nullptr ;

    GenComport *m_pCOM = nullptr ;
    void updateCOMStatus() ;

    QString m_strCurFile ;

    void LoadCmdFile(const QString&strFile) ;

    int m_nCurRow = -1 ;

    bool sendCommand(const QString&strCmd) ;
};

#endif // SERIALTESTDIALOG_H
