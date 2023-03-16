#include "ShstToupCameraHandle.h"

ShstToupCameraHandle::ShstToupCameraHandle(QObject *parent)
    : ShstCameraHandle{parent}
{

}

HToupcam ShstToupCameraHandle::handle() const
{
    return m_handle;
}

void ShstToupCameraHandle::setHandle(HToupcam newHandle)
{
    m_handle = newHandle;
}
