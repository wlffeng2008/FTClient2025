#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>
#include "MainWidget.h"
#include "uiGlobal.h"
#include "Log/CLog.h"


int main( int argc, char *argv[] )
{
    QApplication a( argc, argv );

    QLocale::setDefault( QLocale(QLocale::Chinese, QLocale::China) );
    // 加载 Qt 翻译文件 - 用于自动翻译标准按钮文字
    QTranslator translator;
    if( translator.load( "qt_zh_CN.qm", QLibraryInfo::path( QLibraryInfo::TranslationsPath ) ) )
    {
        a.installTranslator( &translator );
    }
    ts_log::LogInit();

    ui_g::installCommonStyle(a );
    MainWidget w;
    w.show();

    return a.exec();
}
