#include "MainWidget.h"
#include "uiGlobal.h"
#include "CLog.h"

#include <QApplication>
#include <QTranslator>
#include <QLibraryInfo>


int main( int argc, char *argv[] )
{
    QApplication a( argc, argv );

    ui_g::installCommonStyle( a );

    QLocale::setDefault( QLocale(QLocale::Chinese, QLocale::China) );
    QTranslator translator;
    if( translator.load( "qt_zh_CN.qm", QLibraryInfo::path( QLibraryInfo::TranslationsPath ) ) )
    {
        a.installTranslator( &translator );
    }

    // 设置控制台输出为 UTF-8 编码
    // SetConsoleOutputCP( CP_UTF8 );

    ts_log::LogInit();

    MainWidget w;
    w.show();

    return a.exec();
}
