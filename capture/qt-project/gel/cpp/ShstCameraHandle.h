#ifndef SHSTCAMERAHANDLE_H
#define SHSTCAMERAHANDLE_H

#include <QObject>

class ShstCameraHandle : public QObject
{
    Q_OBJECT
public:
    explicit ShstCameraHandle(QObject *parent = nullptr);

signals:

};

#endif // SHSTCAMERAHANDLE_H
