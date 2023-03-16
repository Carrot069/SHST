#ifndef WZIMAGEBUFFER_H
#define WZIMAGEBUFFER_H

#include <QObject>
#include <QDebug>
#include <QDateTime>

typedef uint8_t Gray16Table[65536];

class WzImageBuffer {
private:
    // 16位灰阶转8位灰阶的映射表
    Gray16Table gray16Table;

public:
    WzImageBuffer();

    int low = 0;
    int high = 65535;
    QString imageFile;
    QString imageThumbFile;
    uint32_t exposureMs = 0;
    uint32_t width = 0;
    uint32_t height = 0;
    uint32_t bitDepth = 0;
    uint32_t samplesPerPixel = 1;
    uint8_t* bit8Array;
    uint16_t* bit16Array;
    QDateTime captureDateTime;

    // 调用者负责申请和释放内存
    uint8_t* buf = nullptr;
    // 缓冲区字节数, 须外部赋值
    uint32_t bytesCountOfBuf = 0;

    void updateGray16Table();
    void updateGray16Table(const int& low, const int& high);

    // 更新了缓冲区内容后必须调用此函数初始化一些东西
    void update();
    bool getPixelAs8bit(uint32_t row, uint32_t col, uint8_t& byte);

    void getMinMax(int& min, int& max);
};

#endif // WZIMAGEBUFFER_H
