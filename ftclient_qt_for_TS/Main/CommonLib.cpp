#include "CommonLib.h"
#include <QJsonDocument>
#include <QMessageBox>
#include <QDebug>


namespace cb {


bool strToJsonObject( const std::string &str, QJsonObject &jsonObjOut )
{
    QString response = str.data();
    response = response.trimmed();

    // 将字符串转换为 JSON 文档
    QJsonDocument jsonDoc = QJsonDocument::fromJson( response.toUtf8() );

    // 检查解析是否成功
    if( !jsonDoc.isNull() && jsonDoc.isObject() )
    {
        jsonObjOut = jsonDoc.object();
        return true;
    }
    else
    {
        qDebug() << "Failed to parse JSON ---- ";
        return false;
    }
    return true;
}

bool askUserComfirm( const std::string& str_confirm_infor )
{
    QMessageBox::StandardButton reply;
    reply = QMessageBox::question( nullptr, "确认操作", str_confirm_infor.data(),
                                  QMessageBox::Yes | QMessageBox::No);

    return ( reply == QMessageBox::Yes );
}

int findSubByteArrayPos(const QByteArray &byteArray, const QByteArray &subByteArray , int startPos )
{
    if( byteArray.size() <= 0 || subByteArray.size() <= 0 )
    {
        qDebug() << "byteArray or subByteArray is empty";
        return -1;
    }
    if( subByteArray.size() > byteArray.size() )
    {
        qDebug() << "subByteArray size is bigger than byteArray";
        return -2;
    }

    int pos = -1;
    int loop = byteArray.size() - subByteArray.size() + 1;
    for( int iPos=startPos; iPos<loop; iPos++ )
    {
        for( int iSub=0; iSub<subByteArray.size(); iSub++ )
        {
            if( byteArray[iPos+iSub] != subByteArray[iSub] )
            {
                pos = -1;
                break;
            }
            if( ( subByteArray.size() - 1 ) == iSub )
            {
                return iPos;
            }
        }
    }

    return pos;
}

bool isStringInTableItemModelColumn( QStandardItemModel* model, const QString& str, int column )
{
    if( nullptr == model )
    {
        qDebug() << "model is null";
        return false;
    }

    if( column < 0 || model->columnCount() <= column )
    {
        qDebug() << "column error: " << column;
        return false;
    }

    // 遍历所有行
    for( int row = 0; row < model->rowCount(); ++row )
    {
        QStandardItem* item = model->item( row, column ); // 获取指定列的 item
        if( item && item->text() == str )
        {  // 检查字符串是否匹配
            return true; // 找到匹配的字符串
        }
    }

    return false; // 没有找到
}

void getCurQtVersion( unsigned char &major_no, unsigned char &minor_no, unsigned char &patch_no )
{
    major_no = (QT_VERSION >> 16) & 0xFF;   // 获取主版本号
    minor_no = (QT_VERSION >> 8) & 0xFF;    // 获取次版本号
    patch_no = QT_VERSION & 0xFF;           // 获取修订版本号

    qDebug() << "Qt version:" << major_no << "." << minor_no << "." << patch_no;
}

bool qtVerLowerThan6()
{
    unsigned char major_no;
    unsigned char minor_no;
    unsigned char patch_no;
    getCurQtVersion( major_no, minor_no, patch_no );

    return ( major_no < 6 );
}


}
