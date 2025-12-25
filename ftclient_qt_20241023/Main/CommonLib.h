#ifndef COMMONLIB_H
#define COMMONLIB_H

#include <QJsonObject>
#include <QByteArray>
#include <QStandardItemModel>
#include <string>


namespace cb {

    void getCurQtVersion( unsigned char & major_no, unsigned char& minor_no, unsigned char& patch_no );
    bool qtVerLowerThan6();

    bool strToJsonObject( const std::string& str, QJsonObject & jsonObjOut );
    bool askUserConfirm( const std::string& str_confirm_infor );
    int findSubByteArrayPos( const QByteArray& byteArray, const QByteArray& subByteArray, int startPos = 0 );
    bool isStringInTableItemModelColumn( QStandardItemModel* model, const QString& str, int column );

}

#endif // COMMONLIB_H
