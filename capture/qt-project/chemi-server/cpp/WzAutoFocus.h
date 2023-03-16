#ifndef WZAUTOFOCUS_H
#define WZAUTOFOCUS_H

#include <QObject>
#include <QMutex>
#include <QMutexLocker>
#include <QSize>
#include <QTimer>
#include <QRect>
#include <QMap>
#include <QThread>
#include <QDateTime>

#include <math.h>

// TODO 测试通过后按照C++风格重命名变量名、函数名等

enum FocusPos {
    fpFarthest,
    fpNearest,
    fpPresetUp,
    fpPresetDown
};
// 自动聚焦过程中的三个状态: 曲线正在爬升, 曲线正在保持原样, 曲线正在下降
enum AutoFocusState {
    afsUnknown,
    afsUp,
    afsKeep,
    afsDown,
    afsFirst = afsUnknown,
    afsLast = afsDown
};

class WzAutoFocus : public QObject {

    Q_OBJECT
public:
    static double s_gaussFilter[51][101];

    // 判断趋势的线的帧数
    const double COUNT_OF_LINE = 7.0;
    // 马达运行时间 S1
    const double TIME_S1 = 100;
    // 稳定时间 S2
    const double TIME_S2 = 50;

    explicit WzAutoFocus(QObject *parent = nullptr);
    ~WzAutoFocus() override;

    void start();
    void stop();
    void addFrame(uint8_t *bmp, QSize size, QRect rect);
    void clearFrames();

    // 每种状态出现的数量
    int countOfState(AutoFocusState Index);

    static int64_t getDiffCount(uint8_t *image, QSize size, QRect rect);

    static void gaussFilter(const QList<int64_t> originalLine,
        QList<double> &filteredLine, int Count = -1);

private:
    // 经过高斯滤波的每一帧的 Diff 值
    QList<double> m_frames;

    // 没有经过高斯滤波的原始的每一帧的 Diff 值
    QList<int64_t> m_framesDiff;

    uint8_t *m_image = nullptr;
    QSize m_imageSize;

    // 添加了新的一帧之后曲线的状态
    AutoFocusState m_lineState;
    // 是否往远处聚焦
    bool m_isToFar;
    QMap<AutoFocusState, int> m_countOfState;
    // 所有帧的差值的最大值
    double m_maxDiff = 0;

    // 控制电机转动
    QTimer tmrTurnOn;
    // 控制电机停止
    QTimer tmrTurnOff;
    // 往回转的定时器
    QTimer tmrTurnBack;

    void doLog(QString s);

    void doFocusNear(int Step = 200);
    void doFocusFar(int Step = 200);
    void doFocusStop();
    void doFinished();

    bool checkLine();

    void tmrTurnOnTimer();
    void tmrTurnOffTimer();
    void tmrTurnBackTimer();

    // 曲线的状态
    AutoFocusState getState();

    bool isUp();
    bool isKeep();
    bool isDown();

    double getMaxDiff();

    void doNewFrame();

    static void initGaussFilter();
signals:
    void getImage();
    void focusFar(const int step);
    void focusNear(const int step);
    void focusStop();
    void log(const QString &msg);
    void finished();
    void newFrame();
};

#endif // WZAUTOFOCUS_H
