#ifndef WZIMAGESERVICE_H
#define WZIMAGESERVICE_H

#include <math.h>
#include <memory>

#include <QObject>
#include <QDebug>
#include <QDateTime>
#include <QFile>
#include <QSqlDatabase>
#include <QSqlError>
#include <QSqlQuery>
#include <QtGlobal>
#include <QPrinter>
#include <QPrintPreviewDialog>
#include <QPainter>
#include <QToolBar>

#include "tiff.h"
#include "tiffio.h"

#include "exiv2/exiv2.hpp"

#include "WzGlobalConst.h"
#include "WzGlobalEnum.h"
#include "WzImageView.h"
#include "WzImageBuffer.h"
#include "WzColorChannel.h"
#include "WzImageReader.h"

class WzImageServiceThread: public QThread {
    Q_OBJECT
    void run() override;
    enum Action {
        OpenImage
    };
private:
    QList<Action> m_actions;
public:
    void addAction(Action action);
    void openImage(const QString& filename, WzImageBuffer* imageBuffer);
};

/****************************************************************************
 * 提供图片处理、显示功能。
 * 该类与QML有交互，业务逻辑在QML中控制。
 * 内存缓冲区只存在于C++代码中，其中的内容通过在QML代码中调用的函数调入。
 ****************************************************************************/
class WzImageService : public QObject
{
    Q_OBJECT
private:
    WzImageBuffer* m_imageBuffer;
    unsigned char m_grayTable[65536];
    unsigned char m_grayTableMarker[65536];
    uint8_t* m_image8bit = nullptr;
    int m_low = 0;
    int m_high = 65535;
    int m_lowMarker = 0;
    int m_highMarker = 65535;
    bool m_invert = true;
    bool m_showMarker = false;
    bool m_showChemi = true;
    QString m_markerImageName;
    QImage* m_markerImage = nullptr;
    WzImageBuffer* m_markerBuffer = nullptr; // 16bit tiff marker
    uint8_t* m_markerBuffer8bit = nullptr; // 8bit marker from 16bit tiff
    QVariantList m_pseudoList;
    QImage* m_printImage = nullptr;

    const int COLOR_CHANNEL_COUNT = 4;
    WzColorChannel** m_colorChannels;
    bool m_isColorChannel = false;

    void setMarkerImageName(const QString& markerImageName);
    void scanPalettePath(const QString& path);
    void makeGrayPalette();
    void loadDefaultPalette();
    QJsonArray getPseudoList();

    int getLow() const;
    int getHigh() const;
    int getLowMarker() const;
    int getHighMarker() const;
    void setLow(int low);
    void setHigh(int high);
    void setLowMarker(int lowMarker);
    void setHighMarker(int highMarker);

    void updateViewRGB();
    bool getIsColorChannel() const;
    void setIsColorChannel(bool isColorChannel);

public:

    static WzEnum::ErrorCode loadImage(const QString& filename, WzImageBuffer* imageBuffer);
    static bool saveImageAsTiff(const WzImageBuffer& imageBuffer, const QString& filename);
    // 根据原始图片的完整文件名生成缩略图文件名, 若路径中的文件夹未创建则自动创建
    static QString getThumbFileName(const QString& imageFileName);

    // 为16位灰阶的图生成一个8位灰阶缩略图, 缩略图预期尺寸由 thumbBuffer.width/height 传入,
    // 而 thumbBuffer 的其他成员变量由函数负责赋值, 比如 buf 的内存申请
    static void createThumb(WzImageBuffer& thumbBuffer,
                     WzImageBuffer& imageBuffer,
                     const uint16_t thumbMaxGrayLimit, const bool isNegative);
    static QString createThumb(WzImageBuffer& imageBuffer, QString imageFileName, const bool isNegative = true);

    explicit WzImageService(QObject *parent = nullptr);
    ~WzImageService() override;

    // 更新UI中的ImageView
    void updateCurrentImage(const WzImageBuffer& imageBuffer);

    // 通过界面中的打开按钮打开一张图片, 打开成功/失败后触发 imageOpened 信号
    Q_INVOKABLE void openImage(const QString& filename);
    // 根据参数中的 low/high 更新灰阶映射表, 但不会更新到界面view, 需要调用 updateView 更新
    Q_INVOKABLE void updateLowHigh(const int& low, const int& high);
    Q_INVOKABLE void updateLowHighMarker(const int& low, const int& high);
    // 根据参数生成新的图像并更新到界面 view
    Q_INVOKABLE void updateView();
    // 计算最佳的Low值和High值, 异步函数, 计算完成后触发 lowHighCalculated
    Q_INVOKABLE void calculateLowHigh();
    Q_INVOKABLE void calculateLowHighMarker();
    Q_INVOKABLE void calculateLowHighRGB(const int channel);
    // 获取当前图像的信息
    Q_INVOKABLE QVariantMap getActiveImageInfo();
    // 删除图片文件
    Q_INVOKABLE void deleteImage(QVariantMap imageInfo);
    // 关闭当前图片的显示
    Q_INVOKABLE void closeActiveImage();
    // 是否可以另存图片
    Q_INVOKABLE bool canSaveAsImage();
    // 另存图片
    Q_INVOKABLE bool saveAsImage(const QString& fileName, const QString& format);
    // 打印当前图片
    Q_INVOKABLE bool printImage(const QString& language);

    Q_INVOKABLE void setColorChannelFile(const QString& fileName, const int colorChannel);
    Q_INVOKABLE void setColorChannelLowHigh(const int colorChannel, const int low, const int high);
    Q_INVOKABLE QString getColorChannelFile(const int colorChannel);

    Q_INVOKABLE QVariantMap getPaletteByName(const QString& paletteName);

    Q_PROPERTY(bool invert MEMBER m_invert)
    Q_PROPERTY(bool showMarker MEMBER m_showMarker)
    Q_PROPERTY(QString markerImageName MEMBER m_markerImageName WRITE setMarkerImageName)
    Q_PROPERTY(bool showChemi MEMBER m_showChemi)
    Q_PROPERTY(QJsonArray pseudoList READ getPseudoList NOTIFY pseudoListChanged)
    Q_PROPERTY(int low READ getLow WRITE setLow)
    Q_PROPERTY(int high READ getHigh WRITE setHigh)
    Q_PROPERTY(int lowMarker READ getLowMarker WRITE setLowMarker)
    Q_PROPERTY(int highMarker READ getHighMarker WRITE setHighMarker)

    Q_PROPERTY(bool isColorChannel READ getIsColorChannel WRITE setIsColorChannel);

signals:

    void imageOpened(const WzEnum::ErrorCode errorCode);
    void lowHighCalculated(const WzEnum::ErrorCode errorCode, const int low, const int high);
    void lowHighMarkerCalculated(const WzEnum::ErrorCode errorCode, const int low, const int high);
    void lowHighRGBCalculated(const WzEnum::ErrorCode errorCode, const int low, const int high);
    void pseudoListChanged(const QJsonArray pseudoList);

public slots:
private slots:
    void printPreview(QPrinter *printer);
};

extern WzImageService* g_imageService;

#endif // WZIMAGESERVICE_H
