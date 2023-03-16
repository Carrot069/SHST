#include "WzRender.h"

WzRender::WzRender(QObject *parent) : QObject(parent)
{

}

int WzRender::getCount() {
#ifdef HARDLOCK
    unsigned char v1 = 0x34;
    unsigned char v2 = 0x32;
    unsigned char v3 = 0x38;
    unsigned char v4 = 0x30;
    unsigned char v5 = 0x35;
    unsigned char v6 = 0x39;
    unsigned char v7 = 0x32;
    unsigned char v8 = 0x38;

    char vid[9];
    vid[8] = 0;

    for (int i = 0; i < 9; i++) {
        switch (i) {
            case 0: vid[i] = v1; break;
            case 1: vid[i] = v2; break;
            case 2: vid[i] = v3; break;
            case 3: vid[i] = v4; break;
            case 4: vid[i] = v5; break;
            case 5: vid[i] = v6; break;
            case 6: vid[i] = v7; break;
            case 7: vid[i] = v8; break;
            case 8: vid[i] = 0; break;
        }
    }

    int count = 0;
    RY3_Find(vid, &count);
    return count;
#else
    return 1;
#endif
}

void WzRender::restart() {
    qApp->quit();
    QProcess::startDetached(qApp->arguments()[0], qApp->arguments());
}
