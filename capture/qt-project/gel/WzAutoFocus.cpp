#include "WzAutoFocus.h"

double WzAutoFocus::s_gaussFilter[51][101];

WzAutoFocus::WzAutoFocus(QObject *parent) : QObject(parent)
{
    initGaussFilter();
    connect(&tmrTurnOn, &QTimer::timeout, this, &WzAutoFocus::tmrTurnOnTimer);
    connect(&tmrTurnOff, &QTimer::timeout, this, &WzAutoFocus::tmrTurnOffTimer);
    connect(&tmrTurnBack, &QTimer::timeout, this, &WzAutoFocus::tmrTurnBackTimer);
}

WzAutoFocus::~WzAutoFocus()
{
    if (nullptr != m_image) {
        delete [] m_image;
        m_image = nullptr;
    }
    tmrTurnOn.stop();
    tmrTurnOff.stop();
    tmrTurnBack.stop();
}

void WzAutoFocus::start()
{
    clearFrames();

    m_isToFar = true;
    doFocusFar();

    tmrTurnOn.setInterval(TIME_S1);
    tmrTurnOff.setInterval(TIME_S2);
    tmrTurnOn.start();
    tmrTurnOff.stop();
}

void WzAutoFocus::stop()
{
    doLog("stop auto focus");
    tmrTurnOn.stop();
    tmrTurnOff.stop();

    doFocusStop();
    doFinished();
}

void WzAutoFocus::addFrame(uint8_t *bmp, QSize size, QRect rect)
{    
    if (nullptr == bmp)
        return;

    if (nullptr == m_image)
        m_image = new uint8_t[size.width() * size.height()];
    else if (m_imageSize.width() * m_imageSize.height() < size.width() * size.height()) {
        delete [] m_image;
        m_image = new uint8_t[size.width() * size.height()];
    }

    memcpy(m_image, bmp, size.width() * size.height());
    m_imageSize = size;

    m_framesDiff << getDiffCount(bmp, size, rect);
    if (m_framesDiff.count() < COUNT_OF_LINE)
        return;
    gaussFilter(m_framesDiff, m_frames);

    if (isUp())
        m_lineState = afsUp;
    else if (isDown())
        m_lineState = afsDown;
    else if (isKeep())
        m_lineState = afsKeep;
    else
        m_lineState = afsUnknown;

    m_countOfState[m_lineState] += 1;

    m_maxDiff = getMaxDiff();
}

void WzAutoFocus::clearFrames()
{
    m_frames.clear();
    m_countOfState.clear();
}

int WzAutoFocus::countOfState(AutoFocusState Index)
{
    return m_countOfState[Index];
}

int64_t WzAutoFocus::getDiffCount(uint8_t *Bitmap, QSize size, QRect rect)
{
    uint8_t *bmp = Bitmap;
    int RGBPtr[3];
    int64_t LDiffCount = 0;
    for (int LRow = rect.top(); LRow <= rect.bottom() - 2; LRow++) {
        RGBPtr[0] = LRow * size.width();
        RGBPtr[1] = LRow * size.width();
        RGBPtr[2] = (LRow+1) * size.width();
        for (int i = 0; i < 3; i++)
            RGBPtr[i] += rect.left();
        RGBPtr[1]++;
        for (int LCol = rect.left(); LCol <= rect.right() - 2; LCol++) {
            LDiffCount = LDiffCount +
                    qAbs(bmp[RGBPtr[1]] - bmp[RGBPtr[0]]) +
                    qAbs(bmp[RGBPtr[1]] - bmp[RGBPtr[2]]);
            RGBPtr[0]++;
            RGBPtr[1]++;
            RGBPtr[2]++;
        }
    }

    return LDiffCount;
}

void WzAutoFocus::gaussFilter(const QList<int64_t> originalLine, QList<double> &filteredLine, int count)
{
    const int filterWidth = 7;
    if (count == -1)
        count = originalLine.count();

    filteredLine.clear();
    for (int i = 0; i < originalLine.count(); i++)
        filteredLine << 0;

    for (int j = 0; j < count; j++) {
        for (int i = 0; i <= 2 * filterWidth; i++) {
            int k = j + i - filterWidth;
            if (k >= 0 && k < count)
                filteredLine[j] = filteredLine[j] + originalLine[k] * s_gaussFilter[filterWidth][i];
            else if (k < 0)
                filteredLine[j] = filteredLine[j] + originalLine[0] * s_gaussFilter[filterWidth][i];
            else
                filteredLine[j] = filteredLine[j] + originalLine[count - 2] * s_gaussFilter[filterWidth][i];
        }
    }
}

void WzAutoFocus::doLog(QString s)
{
    emit log(QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz") + ':' + s);
}

void WzAutoFocus::doFocusNear(int Step)
{
    doLog("DoFocusNear");
    emit focusNear(Step);
}

void WzAutoFocus::doFocusFar(int Step)
{
    doLog("DoFocusFar");
    emit focusFar(Step);
}

void WzAutoFocus::doFocusStop()
{
    doLog("DoFocusStop");
    emit focusStop();
}

void WzAutoFocus::doFinished()
{
    doFocusStop();
    emit finished();
}

bool WzAutoFocus::checkLine()
{
    doLog("CheckLine");

    emit getImage();

    bool result = false;

    AutoFocusState afs = getState();
    // 持平次数大于X代表镜头转到头了
    if (afs == afsKeep && countOfState(afsKeep) > COUNT_OF_LINE) {
        // 如果之前是上升的就停掉转动
        if (countOfState(afsUp) > 0) {
            result = true;
            doFinished();
        }
        // 现在需要朝另一个方向转
        else {
            for (int i = afsFirst; i != afsLast; i++) {
                m_countOfState[static_cast<AutoFocusState>(i)] = 0;
            };
            m_isToFar = !m_isToFar;
        };
    }
    // 出现下降趋势
    else if (afs == afsDown) {
        // 直接出现的下降趋势 }
        if (countOfState(afsUp) == 0 && countOfState(afsDown) == 1) {
            doLog("直接出现下降趋势, 往回转");
            result = false;
            m_isToFar = !m_isToFar;
        }
        // 有过上升之后的下降才算聚焦清楚了
        else if (countOfState(afsUp) > 0) {
            doLog("找到了最清晰的位置, 开始逆向转并开启停止聚焦的定时器");
            result = true;
            // 往回转的时间是 判断趋势的帧数*每采集一帧马达转动的时间
            tmrTurnBack.setInterval(trunc(COUNT_OF_LINE * TIME_S1 * 1.8));
            tmrTurnBack.start();
            // 向USB电路板发送指令, 若其正在执行其它指令, 就会忽略后续指令, 所以在此多
            // 发几次
            QThread::msleep(200); // 卡一下再发新指令，不然可能被下位机扔了
            if (m_isToFar) {
                //for i = 0 to 19 do
                doFocusNear(tmrTurnBack.interval());
            } else {
                //for i = 0 to 19 do
                doFocusFar(tmrTurnBack.interval());
            };
        };
    };
    doNewFrame();
    return result;
}

void WzAutoFocus::tmrTurnOnTimer()
{
    //DoFocusStop();
    tmrTurnOn.stop();
    tmrTurnOff.start();
}

void WzAutoFocus::tmrTurnOffTimer()
{
    tmrTurnOff.stop();
    if (checkLine())
        return;
    if (m_isToFar)
        doFocusFar();
    else
        doFocusNear();
    tmrTurnOn.start();
}

void WzAutoFocus::tmrTurnBackTimer()
{
    tmrTurnBack.stop();
    doFinished();
}

AutoFocusState WzAutoFocus::getState()
{
    return m_lineState;
}

bool WzAutoFocus::isUp()
{
    // 已添加的帧数小于趋势判断帧数
    if (m_frames.count() < COUNT_OF_LINE)
        return false;
    // 从最后的帧往前循环, 如果每一帧比前一帧都大
    for (int i = m_frames.count() - 1; i > m_frames.count() - COUNT_OF_LINE; i--)
        if (m_frames[i] < m_frames[i - 1])
            return false;

    return true;
}

bool WzAutoFocus::isKeep()
{
    const double PERCENT = 0.01;

    // 根据最大值的百分之得出的波动数值, 在此范围内视为趋势保持水平
    double fluctuateDiff;

    // 已添加的帧数小于趋势判断帧数
    if (m_frames.count() < COUNT_OF_LINE)
        return false;

    // 从最后的帧往前循环, 如果每一帧和前一帧相减的绝对值在设置的浮动区间内则视为
    // 趋势正在维持
    fluctuateDiff = m_maxDiff * PERCENT;
    for (int i = m_frames.count() - 1; i > m_frames.count() - COUNT_OF_LINE; i--) {
        if (qAbs(m_frames[i] - m_frames[i - 1]) >= fluctuateDiff)
            return false;
    }
    return true;
}

bool WzAutoFocus::isDown()
{
    // 已添加的帧数小于趋势判断帧数
    if (m_frames.count() < COUNT_OF_LINE)
        return false;

    // 从最后的帧往前循环, 如果每一帧比前一帧都小
    for (int i = m_frames.count() - 1; i > m_frames.count() - COUNT_OF_LINE; i--)
        if (m_frames[i] > m_frames[i - 1])
            return false;

    return true;
}

double WzAutoFocus::getMaxDiff()
{
    double result = 0;
    for (int i = 0; i < m_frames.count(); i++)
        if (result < m_frames[i])
            result = m_frames[i];
    return result;
}

void WzAutoFocus::doNewFrame()
{
    emit newFrame();
}

void WzAutoFocus::initGaussFilter()
{
    for (int k = 0; k < 51; k++)
        for (int j = 0; j < 101; j++)
            s_gaussFilter[k][j] = 0;

    for (int k = 1; k < 51; k++) {
        double ggm = 0;
        for (int j = 0; j <= 2 * k; j++){
            s_gaussFilter[k][j] = 1.0 / exp(static_cast<double>((j - k) * (j - k)) / static_cast<double>(k * k));
            ggm = ggm + s_gaussFilter[k][j];
        }
        if (ggm > 0)
            for (int j = 0; j <= 2 * k; j++)
                s_gaussFilter[k][j] = s_gaussFilter[k][j] / ggm;
    }
}
