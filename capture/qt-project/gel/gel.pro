QT += quick core widgets sql printsupport
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += DEMO
DEFINES += GEL_CAPTURE
#DEFINES += UC500_2000
#DEFINES += NO_LOGO
#DEFINES += BETA
#DEFINES += OEM_BR
DEFINES += TOUPCAM

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VERSION = 3.0.0.1
DEFINES += APP_VERSION=\\\"$$VERSION\\\"

contains(DEFINES, OEM_BR) {
    DEFINES += OEM
    QMAKE_TARGET_COMPANY = "BR Biochem Life Sciences Pvt. Ltd."
    QMAKE_TARGET_COPYRIGHT = "BR Biochem Life Sciences Pvt. Ltd."
    QMAKE_TARGET_PRODUCT = "BR Capture Gel"
    QMAKE_DEVELOPMENT_TEAM = "BR Biochem Life Sciences Pvt. Ltd."
    QMAKE_FRAMEWORK_BUNDLE_NAME = "BR Capture Gel"
    contains(DEFINES, BETA) {
        TARGET = "BRCaptureGelBeta"
    } else {
        contains(DEFINES, UC500_2000) {
            TARGET = "BRCaptureGel"
        } else {
            TARGET = "BRCaptureGelN"
        }
    }
}
contains(DEFINES, NO_LOGO) {
    QMAKE_TARGET_COMPANY = "GelCapture"
    QMAKE_TARGET_COPYRIGHT = "GelCapture"
    QMAKE_TARGET_PRODUCT = "GelCapture"
    QMAKE_DEVELOPMENT_TEAM = "GelCapture"
    QMAKE_FRAMEWORK_BUNDLE_NAME = "GelCapture"
    contains(DEFINES, UC500_2000) {
        TARGET = "GelCapture"
    } else {
        TARGET = "GelCaptureN"
    }
}
!contains(DEFINES, OEM) {
    QMAKE_TARGET_COMPANY = "Shenhua Science Technology"
    QMAKE_TARGET_COPYRIGHT = "Shenhua Science Technology"
    QMAKE_TARGET_PRODUCT = "SHST Capture Gel"
    QMAKE_DEVELOPMENT_TEAM = "Shenhua Science Technology"
    QMAKE_FRAMEWORK_BUNDLE_NAME = "SHST Capture Gel"
    contains(DEFINES, BETA) {
        TARGET = "SHSTCaptureGelBeta"
    } else {
        contains(DEFINES, UC500_2000) {
            TARGET = "SHSTCaptureGel"
        } else {
            TARGET = "SHSTCaptureGelN"
        }
    }
}

win32: {
    !contains(DEFINES, OEM) {
        RC_ICONS = icon/app.ico
    }
    contains(DEFINES, OEM_BR) {
        RC_ICONS = icon/app_br.ico
    }
    contains(DEFINES, NO_LOGO) {
        RC_ICONS = camera.ico
    }

    DEFINES += HARDLOCK
    QMAKE_CXXFLAGS += /utf-8

    INCLUDEPATH += \
        $$PWD/../libusb-1.0.23/include/libusb-1.0 \
        $$PWD/include

    LIBS += -lDbghelp

    contains(QT_ARCH, i386) {
        INCLUDEPATH += \
            $$PWD/../libtiff_4.1.0_x32
        LIBS += \
            -L$$PWD/../libtiff_4.1.0_x32 -ltiff \
            -L$$PWD/../libusb-1.0.23/MS32/dll -llibusb-1.0

        Debug:LIBS += \
                -L$$PWD/lib -lexiv2_x86_msvc_debug

        Release:LIBS += \
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
            -L$$PWD/../libusb-1.0.23/MS64/dll -llibusb-1.0

        Debug:LIBS += \
                -L$$PWD/lib -lexiv2_x64_msvc_debug

        Release:LIBS += \
                -L$$PWD/lib -lexiv2_x64_msvc

        HEADERS += \
            $$PWD/../libtiff_4.1.0_x64/tiff.h \
            $$PWD/../libtiff_4.1.0_x64/tiffconf.h \
            $$PWD/../libtiff_4.1.0_x64/tiffio.h \
            $$PWD/../libtiff_4.1.0_x64/tiffvers.h
    }

    contains(DEFINES, PVCAM) {
        HEADERS += \
            pvcam/Inc/master.h \
            pvcam/Inc/pvcam.h \
            pvcam/Inc/pvcam_helper_color.h \
            WzPvCamera.h
        INCLUDEPATH += \
            $$PWD/pvcam/Inc \
            $$PWD/pvcam2/CommonFiles
        LIBS += \
            -L$$PWD/pvcam/Lib/i386 -lpvcam32
        SOURCES += \
            WzPvCamera.cpp
    }

    contains(DEFINES, GEL_CAPTURE) {
        DEFINES += KSJAPI_EXPORTS

        HEADERS += \
            ksj/KSJApi.Inc \
            WzKsjCamera.h

        SOURCES += \
            WzKsjCamera.cpp \
            ksj/common/KSJ_GS.cpp

        INCLUDEPATH += $$PWD/ksj/KSJApi.Inc
        INCLUDEPATH += $$PWD/ksj/common

        contains(QT_ARCH, i386) {
            LIBS += -L$$PWD/ksj/KSJApi.Lib/ -lKSJApiu
        } else {
            LIBS += -L$$PWD/ksj/KSJApi.Lib/ -lKSJApi64u
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

    contains(DEFINES, TOUPCAM) {
        INCLUDEPATH += \
            $$PWD/cpp \
            $$PWD/cpp/toupcam
        DEPENDPATH += $$PWD/cpp/toupcam
        contains(QT_ARCH, i386) {
            LIBS += -L$$PWD/cpp/toupcam/x86 -ltoupcam
            DEPENDPATH += $$PWD/cpp/toupcam/x86
        } else {
            LIBS += -L$$PWD/cpp/toupcam/x64 -ltoupcam
            DEPENDPATH += $$PWD/cpp/toupcam/x64
        }
        HEADERS += cpp/ShstToupCamera.h
        SOURCES += cpp/ShstToupCamera.cpp
    }
}
macx: {
    ICON = Icon.icns

    DEFINES += MAC
    DEFINES += DEMO

    INCLUDEPATH += \
        /usr/local/include \
        /usr/local/include/libusb-1.0 \
        /usr/local/include/exiv2

    LIBS += \
        -L/usr/local/lib -ltiff \
        -L/usr/local/lib -lusb-1.0 \
        -L/usr/local/lib -lexiv2
}

SOURCES += \
    MiniDump.cpp \
    Test.cpp \
    WzAutoFocus.cpp \
    WzCamera.cpp \
    WzCaptureService.cpp \
    WzDatabaseService.cpp \
    WzI18N.cpp \
    WzImageService.cpp \
    WzImageView.cpp \
    WzLibusbThread.cpp \
    WzLiveImageView.cpp \
    WzLogger.cpp \
    WzMcuQml.cpp \
    WzPseudoColor.cpp \
    WzRender.cpp \
    WzRenderThread.cpp \
    WzSingleton.cpp \
    WzUtils.cpp \
    aes/aes.c \
    cpp/ShstCameraHandle.cpp \
    cpp/ShstCameraInstance.cpp \
    cpp/ShstToupCameraHandle.cpp \
    main.cpp \
    WzImageFilter.cpp \
    WzIniSetting.cpp \
    WzAppSingleton.cpp \
    WzSetting2.cpp \
    WzImageBuffer.cpp

RESOURCES += \
    qml/i18n.qrc \
    qml/qml.qrc

contains(DEFINES, OEM_BR) {
    RESOURCES += qml/br.qrc
}
!contains(DEFINES, OEM) {
    RESOURCES += qml/sh.qrc
}

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

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    MiniDump.h \
    Test.h \
    WzAutoFocus.h \
    WzCamera.h \
    WzCaptureService.h \
    WzDatabaseService.h \
    WzGlobalConst.h \
    WzGlobalEnum.h \
    WzI18N.h \
    WzImageService.h \
    WzImageView.h \
    WzLibusbThread.h \
    WzLiveImageView.h \
    WzLogger.h \
    WzMcuQml.h \
    WzPseudoColor.h \
    WzRender.h \
    WzRenderThread.h \
    WzSingleton.h \
    WzUtils.h \
    aes/aes.h \
    aes/aes.hpp \
    WzImageFilter.h \
    WzIniSetting.h \
    WzAppSingleton.h \
    WzSetting2.h \
    WzImageBuffer.h \
    cpp/ShstCameraHandle.h \
    cpp/ShstCameraInstance.h \
    cpp/ShstToupCameraHandle.h

DISTFILES += \
    qml/translations/i18n_en.ts \
    readme.txt

TRANSLATIONS += \
    translations/i18n_en.ts
