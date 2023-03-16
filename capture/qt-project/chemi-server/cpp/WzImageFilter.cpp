#include "WzImageFilter.h"

void WzImageFilter::add4Line(uint16_t *buffer, uint16_t *newBuffer, const int &imageWidth, const int &imageHeight)
{
    if (nullptr == buffer) {
        qDebug() << "line4 buffer nullptr";
        return;
    }
    if (nullptr == newBuffer) {
        qDebug() << "line4 newBuffer nullptr";
    }

    uint16_t *pSrc = buffer;
    uint16_t *pDest = newBuffer;

    *pDest = *pSrc;
    pDest++;
    memcpy(pDest, pSrc, imageWidth * sizeof(uint16_t));
    pDest += imageWidth;
    *pDest = *pSrc;
    pDest++;

    for (int row = 0; row < imageHeight; row++) {
        *pDest = *pSrc;
        pDest++;
        memcpy(pDest, pSrc, imageWidth * sizeof(uint16_t));
        pDest = pDest + imageWidth;
        pSrc = (pSrc + imageWidth - 1);
        *pDest = *pSrc;
        pSrc++;
        pDest++;
    }
    pSrc -= imageWidth;
    *pDest = *pSrc;
    pDest++;
    memcpy(pDest, pSrc, imageWidth * sizeof(uint16_t));
    pDest++;
    *pDest = *pSrc;
}

void WzImageFilter::remove4Line(uint16_t *buffer, uint16_t *newBuffer,
                                const int &imageWidth, const int &imageHeight)
{
    if (nullptr == buffer) {
        qDebug() << "line4 buffer nullptr";
        return;
    }
    if (nullptr == newBuffer) {
        qDebug() << "line4 newBuffer nullptr";
    }

    int newWidth = imageWidth - 2;
    int newHeight = imageHeight - 2;
    uint16_t *pDest = newBuffer;
    uint16_t *pSrc = buffer;

    pSrc += imageWidth + 1;
    for (int row = 0; row < newHeight; row++) {
        memcpy(pDest, pSrc, newWidth * sizeof(uint16_t));

        pDest += newWidth;
        pSrc += imageWidth;
    }
}

WzImageFilter::WzImageFilter()
{

}

uint16_t mediumOfWord(uint16_t x1, uint16_t x2, uint16_t x3) {
    if (x1 > x2) {
        if (x3 > x2) {
            if (x3 > x1)
                x2 = x1;
            else
                x2 = x3;
        }
    } else {
        if (x2 > x3) {
            if (x3 > x1)
                x2 = x3;
            else
                x2 = x1;
        }
    }
    return x2;
}

void WzImageFilter::medianFilter(uint16_t *buffer, const int &imageWidth, const int imageHeight, const int threshold)
{
    uint16_t *Vpr1 = nullptr;
    int V11, V12, V13, V21, V22, V23, V31, V32, V33, tt;
    uint16_t Min_of_Max, Med_of_Med, Max_of_Min;
    int Max_of_4;
    int kmax = 0;

    int filterImageWidth = imageWidth + 2;
    int filterImageHeight = imageHeight + 2;

    uint16_t *filteredBuffer = new uint16_t[filterImageWidth * filterImageHeight];

    add4Line(buffer, filteredBuffer, imageWidth, imageHeight);

    for (int x = 1; x <= imageWidth; x++) {
        for (int y = 1; y <= imageHeight; y++) {
            Vpr1 = filteredBuffer;
            Vpr1 += (y - 1) * filterImageWidth + x - 1;
            V11 = *Vpr1;
            Vpr1++;
            V12 = *Vpr1;
            Vpr1++;
            V13 = *Vpr1;

            Vpr1 = filteredBuffer;
            Vpr1 += y * filterImageWidth + x - 1;
            V21 = *Vpr1;
            Vpr1++;
            V22 = *Vpr1;
            Vpr1++;
            V23 = *Vpr1;

            Vpr1 = filteredBuffer;
            Vpr1 += (y + 1) * filterImageWidth + x - 1;
            V31 = *Vpr1;
            Vpr1++;
            V32 = *Vpr1;
            Vpr1++;
            V33 = *Vpr1;

            kmax = 0;

            if (V22 > V11) kmax++;
            if (V22 > V12) kmax++;
            if (V22 > V13) kmax++;
            if (V22 > V21) kmax++;
            if (V22 > V23) kmax++;
            if (V22 > V31) kmax++;
            if (V22 > V32) kmax++;
            if (V22 > V33) kmax++;

            if (kmax == 0)
            {
                /*/第一行排序
                if (V11 > V12) {
                    tt = V11; V11 = V12; V12 = tt;
                }
                if (V12 > V13) {
                    tt = V12; V12 = V13; V13 = tt;
                }
                if (V11 > V12) {
                    tt = V11; V11 = V12; V12 = tt;
                }

                //第二行排序
                if (V21 > V22) {
                    tt = V21; V21 = V22; V22 = tt;
                }
                if (V22 > V23) {
                    tt = V22; V22 = V23; V23 = tt;
                }
                if (V21 > V22) {
                    tt = V21; V21 = V22; V22 = tt;
                }

                //第三行排序
                if (V31 > V32) {
                    tt = V31; V31 = V32; V32 = tt;
                }
                if (V32 > V33) {
                    tt = V32; V32 = V33; V33 = tt;
                }
                if (V31 > V32) {
                    tt = V31; V31 = V32; V32 = tt;
                }

                Min_of_Max = V13;
                if (V13 > V23) Min_of_Max = V23;
                if (V23 > V33) Min_of_Max = V33;

                Max_of_Min = V11;
                if (V11 < V21) Max_of_Min = V21;
                if (V21 < V31) Max_of_Min = V31;

                Med_of_Med = mediumOfWord(V12, V22, V32);

                //则最后滤波结果
                Vpr1 = filteredBuffer;
                Vpr1 += y * filterImageWidth + x;
                *Vpr1 = mediumOfWord(Min_of_Max, Med_of_Med, Max_of_Min);
                */
            } else {
                if (kmax > 6)
                {
                    Max_of_4 = (V22 - V12) + (V22 - V32) + (V22 - V21) + (V22 - V23);

                    if (Max_of_4 > threshold)
                    {
                        //第一行排序
                        if (V11 > V12) {
                            tt = V11; V11 = V12; V12 = tt;
                        }
                        if (V12 > V13) {
                            tt = V12; V12 = V13; V13 = tt;
                        }
                        if (V11 > V12) {
                            tt = V11; V11 = V12; V12 = tt;
                        }

                        //第二行排序
                        if (V21 > V22) {
                            tt = V21; V21 = V22; V22 = tt;
                        }
                        if (V22 > V23) {
                            tt = V22; V22 = V23; V23 = tt;
                        }
                        if (V21 > V22) {
                            tt = V21; V21 = V22; V22 = tt;
                        }

                        //第三行排序
                        if (V31 > V32) {
                            tt = V31; V31 = V32; V32 = tt;
                        }
                        if (V32 > V33) {
                            tt = V32; V32 = V33; V33 = tt;
                        }
                        if (V31 > V32) {
                            tt = V31; V31 = V32; V32 = tt;
                        }

                        Min_of_Max = V13;
                        if (V13 > V23) Min_of_Max = V23;
                        if (V23 > V33) Min_of_Max = V33;

                        Max_of_Min = V11;
                        if (V11 < V21) Max_of_Min = V21;
                        if (V21 < V31) Max_of_Min = V31;

                        Med_of_Med = mediumOfWord(V12, V22, V32);

                        //则最后滤波结果
                        Vpr1 = filteredBuffer;
                        Vpr1 += y * filterImageWidth + x;
                        *Vpr1 = mediumOfWord(Min_of_Max, Med_of_Med, Max_of_Min);
                    }

                }
            }
        }

        remove4Line(filteredBuffer, buffer, filterImageWidth, filterImageHeight);
    }

    delete [] filteredBuffer;
}

void WzImageFilter::filterLightspot(uint16_t *buffer, const int &imageWidth, const int imageHeight)
{
    // 当前像素是否高于两侧各3个像素的平均值的X倍, 如果是则使用这个平均值
    for (int i = 3; i < imageHeight * imageWidth - 3; i++) {
        int64_t allGraysOf5Pixels = 0;
        int avgGray = 0;
        //qDebug() << "-----";
        for (int j = i - 3; j < i + 4; j++) {
            if (i != j) {
                allGraysOf5Pixels += buffer[j];
                //qDebug() << j;
            }
        }
        avgGray = allGraysOf5Pixels / 6;
        if (buffer[i] > avgGray * 3) {
            qDebug() << "Hit!" << buffer[i];
            buffer[i] = avgGray;
        }
    }
    // 起始和最末3个像素的处理
    for (int i = 0; i < 3; i++) {
        int64_t allGraysOf5Pixels = 0;
        int avgGray = 0;

        for (int j = i + 1; j < i + 6; j++) {
            allGraysOf5Pixels += buffer[j];
        }
        avgGray = allGraysOf5Pixels / 6;
        if (buffer[i] > avgGray * 3) {
            qDebug() << "Hit!" << buffer[i];
            buffer[i] = avgGray;
        }
    }
    for (int i = imageWidth * imageHeight - 3; i < imageWidth * imageHeight; i++) {
        int64_t allGraysOf5Pixels = 0;
        int avgGray = 0;

        for (int j = i - 6; j < i; j++) {
            allGraysOf5Pixels += buffer[j];
        }
        avgGray = allGraysOf5Pixels / 6;
        if (buffer[i] > avgGray * 3) {
            qDebug() << "Hit!" << buffer[i];
            buffer[i] = avgGray;
        }
    }
}
