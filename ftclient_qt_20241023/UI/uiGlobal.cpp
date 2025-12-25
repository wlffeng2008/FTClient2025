#include "uiGlobal.h"

namespace ui_g
{
    void installCommonStyle( QApplication &a )
    {
        QString strStyle = R"(

            QGroupBox {
                background-color: skyblue;
                border-radius: 4px;
                margin-top: 0px;
                padding: 2px; }

            QPushButton:focus{background-color: #30B0EF;}
            QPushButton:hover{background-color: #C0B0BF;}
            QPushButton {background-color: #3030EF; border-radius: 4px; color:white; border:2px solid gray; min-width: 80px; min-height: 24px;}
            QLineEdit {border: 1px solid gray; border-radius: 4px; }
            QCheckBox{spacing: 5px;text-align: AlignVCenter;}
            QCheckBox::indicator { width: 12px; height: 12px; border: 2px solid black;}
            QCheckBox::indicator::checked { image: url(:/UI/check.png); border: 0px solid black; width: 16px; height: 16px;}

            QTableView { background: #F0FFFF; border:1px solid gray;}
            QTableView QHeaderView::section{background-color:lightgreen; border:1px solid gray; border-left: none;border-top: none;}
            QTableView::Item{ border:1px solid gray; border-left: none;border-top: none; color:#000000; padding-left:2px; }
            QTableView::Item::selected{ background-color: #00bb9e; color:white;}

            QTableView::indicator{ min-width: 18px; min-height: 18px;}
            QTableView::indicator:checked { image: url(:/UI/BoxChecked.png); }
            QTableView::indicator:unchecked { image: url(:/UI/BoxUncheck.png); }

            QMessageBox {min-width: 500px; min-height: 250px;}
            QMessageBox QLabel#qt_msgbox_label{ font-size:12px;font-weight: bold; max-width: 420px; min-width: 430px; min-height: 120px; max-height: 520px; qproperty-alignment: AlignTop; white-space: pre-wrap;}
            QMessageBox QLabel#qt_msgboxex_icon_label{ min-width: 32px; min-height: 32px; qproperty-alignment: AlignTop;}
            QMessageBox QPushButton { background-color: #303CCF; border-radius: 4px; color:white; min-width: 80px;min-height: 24px;}

        )";

        a.setStyleSheet(strStyle );
    }
}
