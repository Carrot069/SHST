#include "ShstCameraInstance.h"

ShstCameraInstance::ShstCameraInstance(QObject *parent)
    : QObject{parent}
{

}

QString ShstCameraInstance::getName() const
{
    return "";
}

QString ShstCameraInstance::getDisplayName() const
{
    return "";
}

QString ShstCameraInstance::getModel() const
{
    return "";
}
