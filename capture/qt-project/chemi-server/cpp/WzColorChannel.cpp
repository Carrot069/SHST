#include "WzColorChannel.h"

QString WzColorChannel::imageFile() const
{
    return m_imageFile;
}

void WzColorChannel::setImageFile(const QString &imageFile)
{
    qInfo() << "ColorChannel.setImageFile: " << imageFile;
    if (m_imageFile == imageFile)
        return;
    m_imageFile = imageFile;
    if (nullptr != m_imageBuffer->buf) {
        delete [] m_imageBuffer->buf;
        m_imageBuffer->buf = nullptr;
    }

    WzImageReader imageReader;
    WzEnum::ErrorCode ec = imageReader.loadImage(imageFile, m_imageBuffer);
    if (WzEnum::Success != ec) {
        qInfo("ColorChannel read image error: %d", ec);
        return;
    }
    m_imageBuffer->update();
}

int WzColorChannel::low() const
{
    return m_low;
}

void WzColorChannel::setLow(int low)
{
    m_low = low;
}

int WzColorChannel::high() const
{
    return m_high;
}

void WzColorChannel::setHigh(int high)
{
    m_high = high;
}

WzEnum::RGBChannel WzColorChannel::channel() const
{
    return m_channel;
}

void WzColorChannel::setChannel(const WzEnum::RGBChannel &channel)
{
    m_channel = channel;
}

WzImageBuffer *WzColorChannel::imageBuffer() const
{
    return m_imageBuffer;
}

WzColorChannel::WzColorChannel(QObject *parent) : QObject(parent)
{
    m_imageBuffer = new WzImageBuffer;
}

WzColorChannel::~WzColorChannel()
{
    if (nullptr != m_imageBuffer) {
        delete m_imageBuffer;
        m_imageBuffer = nullptr;
    }
}
