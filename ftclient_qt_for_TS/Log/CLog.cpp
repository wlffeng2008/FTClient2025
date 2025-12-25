#include <QCoreApplication>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QDir>
#include <QDebug>

namespace ts_log
{
    static QFile m_logFile;
    static int m_MaxFileSize = 5 * 1024 * 1024;     // 5MB
    static QtMessageHandler originalMessageHandler = nullptr;
    static void checkLogFileSize()
    {
        if (!m_logFile.isOpen() || m_logFile.size() > m_MaxFileSize )
        {
            QString strPath = QCoreApplication::applicationDirPath() + "/log";
            QDir logDir( strPath );
            if( !logDir.exists() ) logDir.mkpath( strPath );

            QString strTime = QDateTime::currentDateTime().toString( "yyyyMMdd_hhmmss" );
            QString strFile = QString( "%1/log_%2.txt" ).arg( strPath, strTime );

            if ( m_logFile.isOpen() ) m_logFile.close();

            m_logFile.setFileName( strFile );
            if( !m_logFile.open( QIODevice::Append | QIODevice::Text ) )
                qCritical() << "无法打开日志文件!";
        }
    }

    static void LogMessageHandler( QtMsgType type, const QMessageLogContext &context, const QString &strMsg )
    {
        QString strType = "UNKNOWN: " ;
        switch ( type )
        {
        case QtDebugMsg:
            strType = "DEBUG: " ;
            break;

        case QtWarningMsg:
            strType = "WARNING: " ;
            break;

        case QtCriticalMsg:
            strType = "CRITICAL: " ;
            break;

        case QtFatalMsg:
            strType = "FATAL: " ;

        case QtInfoMsg:
            strType = "INFO: " ;
            break;
        }

        QString strTime = QDateTime::currentDateTime().toString( "yyyy-MM-dd hh:mm:ss.zzz" );
        QString strOut  = QString("[%1] %2%3").arg(strTime,strType,strMsg) ;
        // fprintf(stderr, "%s", strOut.toStdString().c_str());

        checkLogFileSize();
        QTextStream out( &m_logFile  );
        out << strOut << "\n";

        if (originalMessageHandler)
            originalMessageHandler(type, context, strOut);
    }

    void LogInit(int nMaxFileSize)
    {
        m_MaxFileSize = nMaxFileSize ;
        originalMessageHandler = qInstallMessageHandler( LogMessageHandler );
    }
}
