#ifndef DIALOGUSERLOGIN_H
#define DIALOGUSERLOGIN_H

#include "SysSettingsWidget.h"
#include <QDialog>

namespace Ui {
class DialogUserLogin;
}

class DialogUserLogin : public QDialog
{
    Q_OBJECT

public:
    explicit DialogUserLogin(QWidget *parent = nullptr);
    ~DialogUserLogin();

    SysSettingsWidget *m_pSet = nullptr ;

    QString m_strUser;
    QString m_strName;
private slots:
    void on_pushButtonLogin_clicked();

    void on_pushButtonCancel_clicked();

    void on_checkBoxPassword_clicked();

    void on_checkBoxSaveLogin_clicked();

    void on_pushButtonSysSet_clicked();

private:
    Ui::DialogUserLogin *ui;
};

#endif // DIALOGUSERLOGIN_H
