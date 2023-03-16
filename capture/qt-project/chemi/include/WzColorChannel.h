#ifndef WZCOLORCHANNEL_H
#define WZCOLORCHANNEL_H

#include <QObject>

#include "WzImageBuffer.h"
#include "WzImageReader.h"
#include "WzGlobalEnum.h"

class WzColorChannel : public QObject
{
    Q_OBJECT

private:
    QString m_imageFile;
    int m_low = 0;
    int m_high = 65535;
    WzEnum::RGBChannel m_channel;
    WzImageBuffer* m_imageBuffer;

public:
    explicit WzColorChannel(QObject *parent = nullptr);
    ~WzColorChannel() override;

    QString imageFile() const;
    void setImageFile(const QString &imageFile);

    int low() const;
    void setLow(int low);

    int high() const;
    void setHigh(int high);

    WzEnum::RGBChannel channel() const;
    void setChannel(const WzEnum::RGBChannel &channel);

    WzImageBuffer *imageBuffer() const;

signals:

};

#endif // WZCOLORCHANNEL_H
