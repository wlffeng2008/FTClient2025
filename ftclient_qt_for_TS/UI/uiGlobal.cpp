#include "uiGlobal.h"

namespace ui_g
{

void installCommonStyle( QApplication &a )
{
    QString strStyle=R"(
        QGroupBox {
            background-color: #F0FFFF;
            border-radius: 5px;
            margin-top: 0px;
            padding: 3px;
        }

        QGroupBox::title {
            background-color: transparent;
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 3px;
            color: black;
        }

        QDialog {background-color: #F0FFFF;}

        QListView { background-color: #F0FCFF;}
        QPushButton { background-color: #303CCF; border-radius: 4px; color:white;border: 2px solid gray;}
        QPushButton:disabled { background-color: #cccccc; color: #999999;}
        QLineEdit {border: 1px solid gray; border-radius: 4px; }

        QCheckBox{spacing: 5px;text-align: AlignVCenter;}
        QCheckBox::indicator { width: 12px; height: 12px; border: 2px solid black;}
        QCheckBox::indicator::checked { image: url(:/UI/check.png); border: 0px solid black; width: 16px; height: 16px;}

        QComboBox {border: 1px solid gray; border-radius: 4px; }
        QComboBox::drop-down {
              subcontrol-origin: padding;
              subcontrol-position: top right;
              width: 20px;
              border-left-width: 1px;
              border-left-color: #ccc;
              border-left-style: solid;
              border-top-right-radius: 4px;
              border-bottom-right-radius: 4px;
              background-color: #e0e0e0;
          }
        QComboBox::down-arrow { image: url(:/UI/down-arrow.png); width: 12px; height: 12px; }

        QTableView QHeaderView::section{background-color:skyblue;border:1px solid #F0FFFF; border-left-color: #ccc;}
        QTableView {background: #F0FFFF;border:none;}
        QTableView::Item{ border: none; color:#000000; padding-left:5px; border-bottom: 1px solid skyblue;}
        QTableView::Item::selected{ background-color: #00bb9e; color:white; outline: none;}
        QTableView::Item::focus{ outline: none; }

        QMessageBox {min-width: 500px; min-height: 300px;}
        QMessageBox QLabel#qt_msgbox_label{min-width: 400px; qproperty-alignment: AlignLeft;}
        QMessageBox QLabel#qt_msgboxex_icon_label{ min-width: 50px; min-height: 50px; qproperty-alignment: AlignTop;}
        QMessageBox QPushButton {background-color: #303CCF; border-radius: 4px; color:white; min-width: 80px;min-height: 28px;}

        VideoAndPtzWidget[PanelStyle="true"] {
        background-color: #F0FFFF;
        border-radius: 4px;
        padding: 0px;
        }

        QGroupBox[PanelStyle="true"]::title {
            background-color: transparent;
            subcontrol-origin: margin;
            subcontrol-position: top left;
            padding: 0 0px;
            color: black;
        }

    )";

    a.setStyleSheet(strStyle );
}


}
