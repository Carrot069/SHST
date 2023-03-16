#ifndef SHSTCAMERAINSTANCE_H
#define SHSTCAMERAINSTANCE_H

#include <QObject>

class ShstCameraInstance : public QObject
{
    Q_OBJECT
public:
    ShstCameraInstance(QObject *parent = nullptr);

    virtual QString getName() const;
    virtual QString getDisplayName() const;
    virtual QString getModel() const;
signals:

};

#endif // SHSTCAMERAINSTANCE_H
