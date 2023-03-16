#ifndef SHSTTOUPCAMERAHANDLE_H
#define SHSTTOUPCAMERAHANDLE_H

#include "ShstCameraHandle.h"
#include "toupcam.h"

class ShstToupCameraHandle : public ShstCameraHandle
{
    Q_OBJECT
public:
    explicit ShstToupCameraHandle(QObject *parent = nullptr);

    HToupcam handle() const;
    void setHandle(HToupcam newHandle);

private:
    HToupcam m_handle;
};

#endif // SHSTTOUPCAMERAHANDLE_H
