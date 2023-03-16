#include "WzImageBuffer.h"

WzImageBuffer::WzImageBuffer() {
    bit8Array = nullptr;
    bit16Array = nullptr;
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

bool WzImageBuffer::getPixelAs16bit(uint32_t row, uint32_t col, uint16_t &pixel)
{
    if (bitDepth != 16) {
        qWarning("ImageBuffer::getPixelAs16bit, bitDepth != 16");
        return false;
    }
    pixel = 0;
    if (row < height && col < width) {
        pixel = bit16Array[row*width+col];
        return true;
    } else {
        return false;
    }
}

bool WzImageBuffer::getPixel(int row, int col, int &gray)
{
    if (bitDepth == 16) {
        uint16_t gray16 = 0;
        bool ret = getPixelAs16bit(row, col, gray16);
        gray = gray16;
        return ret;
    } else if (bitDepth == 8) {
        uint8_t gray8 = 0;
        bool ret = getPixelAs8bit(row, col, gray8);
        gray = gray8;
        return ret;
    } else {
        return false;
    }
}

uint8_t *WzImageBuffer::lines8bit(int row)
{
    if (bitDepth != 8) {
        qWarning("ImageBuffer::lines8bit, bitDepth != 8");
        return nullptr;
    }
    uint8_t *p = bit8Array;
    p += row * width;
    return p;
}

uint16_t *WzImageBuffer::lines(int row)
{
    if (bitDepth != 16) {
        qWarning("ImageBuffer::lines, bitDepth != 16");
        return nullptr;
    }
    uint16_t *p = bit16Array;
    p += row * width;
    return p;
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
