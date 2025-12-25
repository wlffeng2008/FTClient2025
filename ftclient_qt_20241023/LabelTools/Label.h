#ifndef LABEL_H
#define LABEL_H

#include <QString>

namespace label
{
    struct LabelPara
    {
        int iLabelWidth_mm=29;
        int iLabelHeight_mm=29;
        double fZoomScale=26.7f;     // 窗口尺寸与标签尺寸(mm)的比值
        QString strTempl="schema://prod/get/s=GTJtzIFe&u=%1";
    };

    extern QString sTypeInImage;
    extern QString sTypeInQrCode;
    extern QString sTypeInBarCode;
    extern QString sTypeInText;
    extern QString sTypeInBarSN;
}

#endif // LABEL_H
