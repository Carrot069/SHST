#include "WzImageBuffer.h"

WzImageBuffer::WzImageBuffer() {
    for (int i=0; i<65536; i++)
        gray16Table[i] = static_cast<uchar>(i / 256);
}

void WzImageBuffer::updateGray16Table() {
    if (bitDepth != 16) return;
    uint16_t maxGray = 0;
    uint16_t* bufPtr = reinterpret_cast<uint16_t*>(buf);
    for (uint32_t i=0; i<width*height; i++) {
        if (maxGray < *bufPtr)
            maxGray = *bufPtr;
        bufPtr++;
    }
#ifdef QT_DEBUG
    qDebug() << "maxGray:" << maxGray;
#endif
    double scale = maxGray / 256;
    memset(&gray16Table, 0, sizeof(gray16Table));
    for (int i=0; i<maxGray; i++)
        gray16Table[i] = static_cast<uchar>(i / scale);
    for(int i=maxGray; i<65536; i++)
        gray16Table[i] = 255;
}

void WzImageBuffer::updateGray16Table(const int &low, const int &high)
{
    int l = low;
    int h = high;

    if (l < 0) l = 0;
    if (h > 65535) h = 65535;

    if (l > h) l = h;
    if (l == h) {
        qWarning() << "updateGray16Table, low与high相同";
        for(int i=0; i<65536; i++)
            gray16Table[i] = 127; // 灰色显示
        return;
    }
    for(int i=0; i<=l; i++) {
        gray16Table[i] = 0;
    }
    for(int i=h; i<=65535; i++) {
        gray16Table[i] = 255;
    }
    int j = h - l;
    if (j == 1) {
        qWarning() << "updateGray16Table, low与high只相差了1";
        return;
    }
    double scale = static_cast<double>(j+1) / 256.0;
    for(int i=l; i<=h; i++) {
        double k = ((i-l) / scale);
        if (k < 0)
            k = 0;
        else if (k > 255)
            k = 255;
        gray16Table[i] = static_cast<unsigned char>(k);
    }
    this->low = low;
    this->high = high;
}

// 更新了缓冲区内容后必须调用此函数初始化一些东西
void WzImageBuffer::update() {
    bit8Array = static_cast<uint8_t*>(buf);
    bit16Array = reinterpret_cast<uint16_t*>(buf);
}

bool WzImageBuffer::getPixelAs8bit(uint32_t row, uint32_t col, uint8_t& byte) {
    byte = 0;
    if (row < height && col < width) {
        if (bitDepth == 16) {
            byte = gray16Table[bit16Array[row*width+col]];
            return true;
        } else if (bitDepth == 8) {
            byte = bit8Array[row*width+col];
            return true;
        } else {
            return false;
        }
    } else {
        return false;
    }
}

void WzImageBuffer::getMinMax(int& min, int& max) {
    if (bitDepth == 16) {
        if (bit16Array) {
            min = bit16Array[0];
            max = bit16Array[0];
            for(uint n = 0; n < width * height; n++) {
                if (min > bit16Array[n]) min = bit16Array[n];
                if (max < bit16Array[n]) max = bit16Array[n];
            }
        } else {
            qWarning() << "getMinMax, 无效的16位指针";
        }
    } else if (bitDepth == 8) {
        if (bit8Array) {
            min = bit8Array[0];
            max = bit8Array[0];
            for(uint n = 0; n < width * height; n++) {
                if (min > bit8Array[n]) min = bit8Array[n];
                if (max < bit8Array[n]) max = bit8Array[n];
            }
        } else {
            qWarning() << "getMinMax, 无效的8位指针";
        }
    } else {
        qWarning() << "getMinMax, 不支持的位数";
    }
}
