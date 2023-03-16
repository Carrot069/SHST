QT += quick core widgets sql printsupport svg websockets network
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += DEMO
DEFINES += CHEMI_CAPTURE
DEFINES += PVCAM
#DEFINES += ATIK
DEFINES += TCP_SERVER
#DEFINES += TCP_CLIENT
DEFINES += KSJ_CAMERA

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VERSION = 2.0.1.56
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
QMAKE_TARGET_COMPANY="Shenhua Science Technology"
QMAKE_TARGET_COPYRIGHT="Shenhua Science Technology"
QMAKE_TARGET_PRODUCT="SHST Capture"
QMAKE_DEVELOPMENT_TEAM="Shenhua Science Technology"
QMAKE_FRAMEWORK_BUNDLE_NAME="SHST Capture"

contains(DEFINES, ATIK) {
    TARGET = "SHSTCaptureServerA"
} else {
    TARGET = "SHSTCaptureServer"
}

win32: {
    RC_ICONS = app.ico
    DEFINES += HARDLOCK
    QMAKE_CXXFLAGS += /utf-8

    INCLUDEPATH += \
        $$PWD/../libusb-1.0.23/include/libusb-1.0 \
        $$PWD/include \
        $$PWD/cpp

    Debug:LIBS += -lDbghelp

    contains(QT_ARCH, i386) {
        INCLUDEPATH += \
            $$PWD/../libtiff_4.1.0_x32

        LIBS += \
            -L$$PWD/../libtiff_4.1.0_x32 -ltiff \
            -L$$PWD/../libusb-1.0.23/MS32/dll -llibusb-1.0 \
            -L$$PWD/lib -lexiv2_x86_msvc

        HEADERS += \
            $$PWD/../libtiff_4.1.0_x32/tiff.h \
            $$PWD/../libtiff_4.1.0_x32/tiffconf.h \
            $$PWD/../libtiff_4.1.0_x32/tiffio.h \
            $$PWD/../libtiff_4.1.0_x32/tiffvers.h
    } else {
        INCLUDEPATH += \
            $$PWD/../libtiff_4.1.0_x64

        LIBS += \
            -L$$PWD/../libtiff_4.1.0_x64 -ltiff \
            -L$$PWD/../libusb-1.0.23/MS64/dll -llibusb-1.0 \
            -L$$PWD/lib/ -lexiv2_x64_msvc

        HEADERS += \
            $$PWD/../libtiff_4.1.0_x64/tiff.h \
            $$PWD/../libtiff_4.1.0_x64/tiffconf.h \
            $$PWD/../libtiff_4.1.0_x64/tiffio.h \
            $$PWD/../libtiff_4.1.0_x64/tiffvers.h
    }

    contains(DEFINES, ATIK) {
        INCLUDEPATH += \
            include \
            third-party

        HEADERS += \
            include/WzAtikCamera.h \
            third-party/atik/AtikCameras.h
        SOURCES += \
            source/WzAtikCamera.cpp \
            third-party/atik/AtikCameras.cpp
    }

    contains(DEFINES, PVCAM) {
        HEADERS += \
            pvcam/Inc/master.h \
            pvcam/Inc/pvcam.h \
            pvcam/Inc/pvcam_helper_color.h \
            cpp/WzPvCamera.h
        INCLUDEPATH += \
            $$PWD/pvcam/Inc

        contains(QT_ARCH, i386) {
            LIBS += \
                -L$$PWD/pvcam/Lib/i386 -lpvcam32
        } else {
            LIBS += \
                -L$$PWD/pvcam/Lib/amd64 -lpvcam64
        }
        SOURCES += \
            cpp/WzPvCamera.cpp
    }

    contains(DEFINES, KSJ_CAMERA) {
        DEFINES += KSJAPI_EXPORTS

        HEADERS += \
            ksj/KSJApi.Inc \
            cpp/WzKsjCamera.h

        SOURCES += \
            cpp/WzKsjCamera.cpp \
            ksj/common/KSJ_GS.cpp

        INCLUDEPATH += $$PWD/ksj/KSJApi.Inc
        INCLUDEPATH += $$PWD/ksj/common

        contains(QT_ARCH, i386) {
            LIBS += \
                -L$$PWD/ksj/KSJApi.Lib/ -lKSJApiu
        } else {
            LIBS += \
                -L$$PWD/ksj/KSJApi.Lib/ -lKSJApi64u
        }

        INCLUDEPATH += $$PWD/ksj/KSJApi.Lib
        DEPENDPATH += $$PWD/ksj/KSJApi.Lib
    }

    contains(DEFINES, HARDLOCK) {
        INCLUDEPATH += \
            $$PWD/rockey3

        HEADERS += \
            rockey3/RY3_API.h

        contains(QT_ARCH, i386) {
            LIBS += \
                -L$$PWD/rockey3 -lRockey3
        } else {
            LIBS += \
                -L$$PWD/rockey3 -lRockey3_x64
        }
    }

    HEADERS += \
        include/exiv2/exiv2.hpp

}
macx: {
    ICON = Icon.icns

    DEFINES += MAC
    DEFINES += DEMO
    DEFINES -= PVCAM
    DEFINES -= KSJ_CAMERA

    INCLUDEPATH += \
        /usr/local/include \
        /usr/local/include/libusb-1.0 \
        /usr/local/include/exiv2 \
        $$PWD/cpp

    LIBS += \
        -L/usr/local/lib -ltiff \
        -L/usr/local/lib -lusb-1.0 \
        -L/usr/local/lib -lexiv2
}

SOURCES += \
    cpp/Test.cpp \
    cpp/WzAutoFocus.cpp \
    cpp/WzCamera.cpp \
    cpp/WzCaptureService.cpp \
    cpp/WzColorChannel.cpp \
    cpp/WzDatabaseService.cpp \
    cpp/WzDiskUtils.cpp \
    cpp/WzFileServer.cpp \
    cpp/WzFileService.cpp \
    cpp/WzHaierConf.cpp \
    cpp/WzI18N.cpp \
    cpp/WzImageBuffer.cpp \
    cpp/WzImageReader.cpp \
    cpp/WzImageService.cpp \
    cpp/WzImageView.cpp \
    cpp/WzLibusbThread.cpp \
    cpp/WzLiveImageView.cpp \
    cpp/WzMcuQml.cpp \
    cpp/WzPseudoColor.cpp \
    cpp/WzRender.cpp \
    cpp/WzRenderThread.cpp \
    cpp/WzSetting2.cpp \
    cpp/WzSingleton.cpp \
    cpp/WzUdpBroadcastSender.cpp \
    cpp/WzUtils.cpp \
    aes/aes.c \
    cpp/ShstAbstractService.cpp \
    cpp/ShstFileServer.cpp \
    cpp/ShstServer.cpp \
    cpp/ShstTcpFileIo.cpp \
    cpp/ShstTcpFileService.cpp \
    cpp/ShstTcpIo.cpp \
    cpp/ShstUsbDisk.cpp \
    cpp/service/ShstUsbDiskService.cpp \
    cpp/service/ShstHeartService.cpp \
    cpp/main.cpp \
    cpp/WzImageFilter.cpp \
    cpp/MiniDump.cpp

RESOURCES += \
    qml/haier.qrc \
    qml/i18n.qrc \
    qml/qml.qrc

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH += $$PWD/modules

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

CONFIG(debug,debug|release){
#    win32:LIBS += \
#        ..\chemi\pvcam\Lib\i386\pvcam32.lib
} else {
#    win32:LIBS += \
#        ..\chemi\pvcam\Lib\i386\pvcam32.lib

}

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    cpp/Test.h \
    cpp/WzAutoFocus.h \
    cpp/WzCamera.h \
    cpp/WzCaptureService.h \
    cpp/WzColorChannel.h \
    cpp/WzDatabaseService.h \
    cpp/WzDiskUtils.h \
    cpp/WzFileServer.h \
    cpp/WzFileService.h \
    cpp/WzGlobalConst.h \
    cpp/WzGlobalEnum.h \
    cpp/WzHaierConf.h \
    cpp/WzI18N.h \
    cpp/WzImageBuffer.h \
    cpp/WzImageReader.h \
    cpp/WzImageService.h \
    cpp/WzImageView.h \
    cpp/WzLibusbThread.h \
    cpp/WzLiveImageView.h \
    cpp/WzMcuQml.h \
    cpp/WzPseudoColor.h \
    cpp/WzRender.h \
    cpp/WzRenderThread.h \
    cpp/WzSetting2.h \
    cpp/WzSingleton.h \
    cpp/WzUdpBroadcastSender.h \
    cpp/WzUtils.h \
    aes/aes.h \
    aes/aes.hpp \
    cpp/WzImageFilter.h \
    cpp/MiniDump.h \
    cpp/ShstAbstractService.h \
    cpp/ShstFileServer.h \
    cpp/ShstServer.h \
    cpp/ShstTcpFileIo.h \
    cpp/ShstTcpFileService.h \
    cpp/ShstTcpIo.h \
    cpp/ShstUsbDisk.h \
    cpp/service/ShstUsbDiskService.h \
    cpp/service/ShstHeartService.h

DISTFILES += \
    qml/translations/i18n_en.ts \
    readme.txt

TRANSLATIONS += \
    translations/i18n_en.ts

contains(DEFINES, TCP_SERVER) {
    QT += network
    HEADERS += \
        cpp/WzTcpServer.h \
        cpp/WzTcpServerThread.h
    SOURCES += \
        cpp/WzTcpServer.cpp \
        cpp/WzTcpServerThread.cpp
}
