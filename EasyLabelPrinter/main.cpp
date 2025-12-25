#include "mainwindow.h"

#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    QString strStyle=R"(

        /*QGroupBox {
            background-color: rgb(0, 192, 128);
            border-radius: 4px;
            margin-top: 0px;
            padding: 2px;
        }*/

        QPushButton:focus{background-color: #30B0EF;}
        QPushButton:hover{background-color: #C0B0BF;}
        QPushButton {background-color: #3030EF; border-radius: 4px; color:white; border:2px solid gray; min-width: 80px; min-height: 22px;}
        QLineEdit {border: 1px solid gray; border-radius: 4px; }

        QTableView { background: #F0FFFF; border:1px solid gray;}
        QTableView QHeaderView::section{background-color:skyblue; border:1px solid gray; border-left: none;border-top: none;}
        QTableView::Item{ border:1px solid gray; border-left: none;border-top: none; color:#000000; padding-left:2px; }
        QTableView::Item::selected{ background-color: #00bb9e; color:white;}

        QMessageBox {min-width: 400px; min-height: 300px;}
        QMessageBox QLabel#qt_msgbox_label{max-width: 300px; min-width: 300px;min-height: 120px; qproperty-alignment: AlignLeft;}
        QMessageBox QLabel#qt_msgboxex_icon_label{ min-width: 50px; min-height: 80px; qproperty-alignment: AlignTop;}
        QMessageBox QPushButton {background-color: #303CCF; border-radius: 4px; color:white; min-width: 80px;min-height: 24px;}

    )";

    a.setStyleSheet(strStyle );
    QLocale::setDefault( QLocale(QLocale::Chinese, QLocale::China) );
    QTranslator translator;
    if( translator.load( "qt_zh_CN.qm", QLibraryInfo::path( QLibraryInfo::TranslationsPath ) ) )
    {
        a.installTranslator( &translator );
    }

    MainWindow w;
    w.show();
    return a.exec();
}
