QT += quick core widgets sql printsupport svg
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
#DEFINES += TCP_SERVER
#DEFINES += TCP_CLIENT
#DEFINES += EXPOSURE_TIME_DOUBLE
DEFINES += EXPOSURE_AREA
#DEFINES += ONLY_CHINESE
#DEFINES += OEM
#DEFINES += OEM_JP
#DEFINES += MINI
#DEFINES += NO_LOGO

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VERSION = 2.0.1.107
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
contains(DEFINES, OEM_JP) {
    QMAKE_TARGET_PRODUCT="JPCapture"
    DEFINES += OEM
}
contains(DEFINES, NO_LOGO) {
    QMAKE_TARGET_PRODUCT="ChemiCapture"
    DEFINES += OEM
}
!contains(DEFINES, OEM) {
    QMAKE_TARGET_COMPANY="Shenhua Science Technology"
    QMAKE_TARGET_COPYRIGHT="Shenhua Science Technology"
    contains(DEFINES, MINI) {
        QMAKE_TARGET_PRODUCT="SHST Capture Mini"
        QMAKE_FRAMEWORK_BUNDLE_NAME="SHST Capture Mini"
    } else {
        QMAKE_TARGET_PRODUCT="SHST Capture"
        QMAKE_FRAMEWORK_BUNDLE_NAME="SHST Capture"
    }
    QMAKE_DEVELOPMENT_TEAM="Shenhua Science Technology"
    QMAKE_TARGET_BUNDLE_PREFIX="bio.shenhua"
}

contains(DEFINES, CHEMI_CAPTURE) {
    contains(DEFINES, OEM_JP) {
        TARGET = "JPCapture"
    }
    contains(DEFINES, NO_LOGO) {
        contains(DEFINES, MINI) {
            contains(DEFINES, ATIK) {
                TARGET = "ChemiCaptureMiniA"
            } else {
                TARGET = "ChemiCaptureMini"
            }
        } else {
            contains(DEFINES, ATIK) {
                TARGET = "ChemiCaptureA"
            } else {
                TARGET = "ChemiCapture"
            }
        }
    }
    !contains(DEFINES, OEM) {
        contains(DEFINES, MINI) {
            contains(DEFINES, ATIK) {
                TARGET = "SHSTCaptureMiniA"
            } else {
                TARGET = "SHSTCaptureMini"
            }
        } else {
            contains(DEFINES, ATIK) {
                TARGET = "SHSTCaptureA"
            } else {
                TARGET = "SHSTCapture"
            }
        }
    }
}

win32: {
    DEFINES += MED_FIL_TIFF8
    contains(DEFINES, OEM_JP) {
        RC_ICONS = icon/app_jp.ico
    }
    contains(DEFINES, NO_LOGO) {
        RC_ICONS = icon/camera.ico
    }
    !contains(DEFINES, OEM) {
        RC_ICONS = icon/app.ico
    }
    DEFINES += HARDLOCK
    QMAKE_CXXFLAGS += /utf-8

    INCLUDEPATH += \
        $$PWD/../libusb-1.0.23/include/libusb-1.0 \
        $$PWD/include \
        $$PWD/third-party/exiv2/include

    LIBS += -lDbghelp

    contains(QT_ARCH, i386) {
        INCLUDEPATH += \
            $$PWD/../libtiff_4.1.0_x32

        LIBS += \
            -L$$PWD/../libtiff_4.1.0_x32 -ltiff \
            -L$$PWD/../libusb-1.0.23/MS32/dll -llibusb-1.0

        Debug:LIBS += \
                -L$$PWD/third-party/exiv2/lib -lexiv2_x86_msvc_debug

        Release:LIBS += \
                -L$$PWD/third-party/exiv2/lib -lexiv2_x86_msvc

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
                -L$$PWD/third-party/exiv2/lib -lexiv2_x64_msvc_debug

        Release:LIBS += \
                -L$$PWD/third-party/exiv2/lib -lexiv2_x64_msvc

        HEADERS += \
            $$PWD/../libtiff_4.1.0_x64/tiff.h \
            $$PWD/../libtiff_4.1.0_x64/tiffconf.h \
            $$PWD/../libtiff_4.1.0_x64/tiffio.h \
            $$PWD/../libtiff_4.1.0_x64/tiffvers.h
    }

    contains(DEFINES, PVCAM) {
        HEADERS += \
            third-party/pvcam/Inc/master.h \
            third-party/pvcam/Inc/pvcam.h \
            third-party/pvcam/Inc/pvcam_helper_color.h \
            include/WzPvCamera.h
        INCLUDEPATH += \
            $$PWD/third-party/pvcam/Inc

        contains(QT_ARCH, i386) {
            LIBS += \
                -L$$PWD/third-party/pvcam/Lib/i386 -lpvcam32
        } else {
            LIBS += \
                -L$$PWD/third-party/pvcam/Lib/amd64 -lpvcam64
        }
        SOURCES += \
            source/WzPvCamera.cpp
    }

    contains(DEFINES, ATIK) {
        INCLUDEPATH += atik
        HEADERS += \
            include/WzAtikCamera.h \
            third-party/atik/AtikCameras.h
        SOURCES += \
            source/WzAtikCamera.cpp \
            third-party/atik/AtikCameras.cpp
    }

    contains(DEFINES, HARDLOCK) {
        INCLUDEPATH += \
            $$PWD/third-party/rockey3

        HEADERS += \
            third-party/rockey3/RY3_API.h

        contains(QT_ARCH, i386) {
            LIBS += \
                -L$$PWD/third-party/rockey3 -lRockey3
        } else {
            LIBS += \
                -L$$PWD/third-party/rockey3 -lRockey3_x64
        }
    }

    contains(DEFINES, MED_FIL_TIFF8) {
        INCLUDEPATH += \
            $$PWD/../opencv/include

        contains(QT_ARCH, i386) {
            Debug:LIBS += \
                -L$$PWD/../opencv/build_win32/lib/Debug -lopencv_core453d \
                -L$$PWD/../opencv/build_win32/lib/Debug -lopencv_imgcodecs453d \
                -L$$PWD/../opencv/build_win32/lib/Debug -lopencv_imgproc453d
            Release:LIBS += \
                -L$$PWD/../opencv/build_win32/lib/Release -lopencv_core453 \
                -L$$PWD/../opencv/build_win32/lib/Release -lopencv_imgcodecs453 \
                -L$$PWD/../opencv/build_win32/lib/Release -lopencv_imgproc453
        } else {
            Debug:LIBS += \
                -L$$PWD/../opencv/build_win64/lib/Debug -lopencv_core453d \
                -L$$PWD/../opencv/build_win64/lib/Debug -lopencv_imgcodecs453d \
                -L$$PWD/../opencv/build_win64/lib/Debug -lopencv_imgproc453d
            Release:LIBS += \
                -L$$PWD/../opencv/build_win64/lib/Release -lopencv_core453 \
                -L$$PWD/../opencv/build_win64/lib/Release -lopencv_imgcodecs453 \
                -L$$PWD/../opencv/build_win64/lib/Release -lopencv_imgproc453
        }
    }

    HEADERS += \
        third-party/exiv2/include/exiv2/exiv2.hpp

}
macx: {
    ICON = Icon.icns

    DEFINES += MAC
    DEFINES += DEMO

    INCLUDEPATH += \
        /usr/local/include \
        /usr/local/include/libusb-1.0 \
        /usr/local/include/exiv2 \
        $$PWD/include

    LIBS += \
        -L/usr/local/lib -ltiff \
        -L/usr/local/lib -lusb-1.0 \
        -L/usr/local/lib -lexiv2
}

SOURCES += \
    source/LauncherHelper.cpp \
    source/LogToFile.cpp \
    source/MiniDump.cpp \
    source/ShstLogToQml.cpp \
    source/Test.cpp \
    source/WzAppSingleton.cpp \
    source/WzAutoFocus.cpp \
    source/WzCamera.cpp \
    source/WzCaptureService.cpp \
    source/WzColorChannel.cpp \
    source/WzDatabaseService.cpp \
    source/WzI18N.cpp \
    source/WzImageBuffer.cpp \
    source/WzImageExporter.cpp \
    source/WzImageReader.cpp \
    source/WzImageService.cpp \
    source/WzImageView.cpp \
    source/WzIniSetting.cpp \
    source/WzLibusbThread.cpp \
    source/WzLiveImageView.cpp \
    source/WzLocalServer.cpp \
    source/WzLocalServerService.cpp \
    source/WzLogger.cpp \
    source/WzMcuQml.cpp \
    source/WzPseudoColor.cpp \
    source/WzRender.cpp \
    source/WzRenderThread.cpp \
    source/WzSetting2.cpp \
    source/WzSingleton.cpp \
    source/WzUtils.cpp \
    third-party/aes/aes.c \
    source/main.cpp \
    source/WzImageFilter.cpp

RESOURCES += \
    i18n/i18n.qrc \
    qml/qml.qrc \
    images/images.qrc

contains(DEFINES, OEM_JP) {
    RESOURCES += images/jp/jp.qrc
}
!contains(DEFINES, OEM) {
    RESOURCES += images/sh/sh.qrc
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

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

INCLUDEPATH += \
    include \
    third-party \
    third-party/aes

HEADERS += \
    include/LauncherHelper.h \
    include/LogToFile.h \
    include/MiniDump.h \
    include/ShstLogToQml.h \
    include/Test.h \
    include/WzAppSingleton.h \
    include/WzAutoFocus.h \
    include/WzCamera.h \
    include/WzCaptureService.h \
    include/WzColorChannel.h \
    include/WzDatabaseService.h \
    include/WzGlobalConst.h \
    include/WzGlobalEnum.h \
    include/WzI18N.h \
    include/WzImageBuffer.h \
    include/WzImageExporter.h \
    include/WzImageReader.h \
    include/WzImageService.h \
    include/WzImageView.h \
    include/WzIniSetting.h \
    include/WzLibusbThread.h \
    include/WzLiveImageView.h \
    include/WzLocalServer.h \
    include/WzLocalServerService.h \
    include/WzLogger.h \
    include/WzMcuQml.h \
    include/WzPseudoColor.h \
    include/WzRender.h \
    include/WzRenderThread.h \
    include/WzSetting2.h \
    include/WzSingleton.h \
    include/WzUtils.h \
    third-party/aes/aes.h \
    third-party/aes/aes.hpp \
    include/WzImageFilter.h

DISTFILES += \
    i18n/i18n_en.ts \
    readme.txt \
    update.txt

TRANSLATIONS += \
    i18n/i18n_en.ts

contains(DEFINES, TCP_SERVER) {
    QT += network
    HEADERS += \
        include/WzTcpServer.h \
        include/WzTcpServerThread.h
    SOURCES += \
        include/WzTcpServer.cpp \
        include/WzTcpServerThread.cpp
}
