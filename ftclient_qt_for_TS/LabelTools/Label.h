#ifndef LABEL_H
#define LABEL_H

#include <QString>

namespace label
{
    struct LabelPara
    {
        int iLabelWidth_mm=29;
        int iLabelHeight_mm=29;
        float fZoomScale=26.7f;     // 窗口尺寸与标签尺寸(mm)的比值
        QString strTempl="schema://prod/get/s=GTJtzIFe&u=%1";
    };

    extern int obj_type_in_data_index;
    extern QString sTypeInImage;
    extern QString sTypeInQrCode;
    extern QString sTypeInBarCode;
    extern QString sTypeInText;
    extern QString sTypeInBarSN;
    extern QString sTypeInUUID;
}

#endif // LABEL_H
