QT += quick core widgets sql printsupport svg websockets network
CONFIG += c++11

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Refer to the documentation for the
# deprecated API to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS
#DEFINES += DEMO
DEFINES += CHEMI_CAPTURE
#DEFINES += GEL_CAPTURE
DEFINES += PVCAM
#DEFINES += TCP_SERVER
DEFINES += TCP_CLIENT
#DEFINES += PAD
#DEFINES += MOBILE
#DEFINES += TCP_DEBUG

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VERSION = 2.0.1.54
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
QMAKE_TARGET_COMPANY="Shenhua Science Technology"
QMAKE_TARGET_COPYRIGHT="Shenhua Science Technology"
QMAKE_TARGET_PRODUCT="SHST Capture"
QMAKE_DEVELOPMENT_TEAM="Shenhua Science Technology"
QMAKE_FRAMEWORK_BUNDLE_NAME="SHST Capture"

TARGET = "SHSTCapture"

win32: {
    RC_ICONS = app.ico
    QMAKE_CXXFLAGS += /utf-8

    INCLUDEPATH += \
        $$PWD/../libusb-1.0.23/include/libusb-1.0 \
        $$PWD/include \
        $$PWD/cpp

    contains(QT_ARCH, i386) {
        INCLUDEPATH += \
            $$PWD/../libtiff_4.1.0_x32

        LIBS += \
            -L$$PWD/../libtiff_4.1.0_x32 -ltiff


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
            -L$$PWD/../libtiff_4.1.0_x64 -ltiff

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

    HEADERS += \
        include/exiv2/exiv2.hpp

}
macx: {
    ICON = Icon.icns

    DEFINES += MAC
    #DEFINES += DEMO

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

contains(ANDROID_TARGET_ARCH,arm64-v8a) {
    QT += androidextras
    #DEFINES += DEMO
    DEFINES += ANDROID
    DEFINES += MOBILE

    INCLUDEPATH += \
        $$PWD/include \
        $$PWD/../libtiff_4.1.0_arm/include \
        $$PWD/cpp

    LIBS += \
        -L$$PWD/../libtiff_4.1.0_arm/lib -ltiff \
        -L$$PWD/lib -lexiv2_arm

    HEADERS += \
        $$PWD/../libtiff_4.1.0_arm/include/tiff.h \
        $$PWD/../libtiff_4.1.0_arm/include/tiffconf.h \
        $$PWD/../libtiff_4.1.0_arm/include/tiffio.h \
        $$PWD/../libtiff_4.1.0_arm/include/tiffvers.h

    ANDROID_EXTRA_LIBS = \
        $$PWD/../libtiff_4.1.0_arm/lib/libtiff.so

    DISTFILES += \
        android/AndroidManifest.xml \
        android/build.gradle \
        android/gradle/wrapper/gradle-wrapper.jar \
        android/gradle/wrapper/gradle-wrapper.properties \
        android/gradlew \
        android/gradlew.bat \
        android/res/values/libs.xml \
        android/res/xml/filepaths.xml \
        android/libs/androidx.jar

    ANDROID_PACKAGE_SOURCE_DIR = \
        $$PWD/android
}

SOURCES += \
    cpp/Test.cpp \
    cpp/WzCamera.cpp \
    cpp/WzCaptureService.cpp \
    cpp/WzColorChannel.cpp \
    cpp/WzDatabaseService.cpp \
    cpp/WzFileDownloader.cpp \
    cpp/WzI18N.cpp \
    cpp/WzImageBuffer.cpp \
    cpp/WzImageReader.cpp \
    cpp/WzImageService.cpp \
    cpp/WzImageView.cpp \
    cpp/WzJavaService.cpp \
    cpp/WzLibusbNet.cpp \
    cpp/WzLiveImageView.cpp \
    cpp/WzLocalServer.cpp \
    cpp/WzLocalServerService.cpp \
    cpp/WzMcuQml.cpp \
    cpp/WzNetCamera.cpp \
    cpp/WzPseudoColor.cpp \
    cpp/WzRender.cpp \
    cpp/WzRenderThread.cpp \
    cpp/WzSetting2.cpp \
    cpp/WzShareFile.cpp \
    cpp/WzSingleton.cpp \
    cpp/WzUdpBroadcastReceiver.cpp \
    cpp/WzUtils.cpp \
    aes/aes.c \
    cpp/ShstFileToUsbDiskClient.cpp \
    cpp/ShstServerInfo.cpp \
    cpp/ShstTcpFileIo.cpp \
    cpp/ShstTcpIo.cpp \
    cpp/ziputils.cpp \
    cpp/main.cpp \
    cpp/WzImageFilter.cpp

RESOURCES += \
    demotiff.qrc \
    images.qrc \
    qml/i18n.qrc \
    qml/qml.qrc

android {
    RESOURCES += fonts.qrc
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

HEADERS += \
    cpp/Test.h \
    cpp/WzCamera.h \
    cpp/WzCaptureService.h \
    cpp/WzColorChannel.h \
    cpp/WzDatabaseService.h \
    cpp/WzFileDownloader.h \
    cpp/WzGlobalConst.h \
    cpp/WzGlobalEnum.h \
    cpp/WzI18N.h \
    cpp/WzImageBuffer.h \
    cpp/WzImageReader.h \
    cpp/WzImageService.h \
    cpp/WzImageView.h \
    cpp/WzJavaService.h \
    cpp/WzLibusbNet.h \
    cpp/WzLiveImageView.h \
    cpp/WzLocalServer.h \
    cpp/WzLocalServerService.h \
    cpp/WzMcuQml.h \
    cpp/WzNetCamera.h \
    cpp/WzPseudoColor.h \
    cpp/WzRender.h \
    cpp/WzRenderThread.h \
    cpp/WzSetting2.h \
    cpp/WzShareFile.h \
    cpp/WzSingleton.h \
    cpp/WzUdpBroadcastReceiver.h \
    cpp/WzUtils.h \
    aes/aes.h \
    aes/aes.hpp \
    cpp/WzImageFilter.h \
    cpp/ShstFileToUsbDiskClient.h \
    cpp/ShstServerInfo.h \
    cpp/ShstTcpFileIo.h \
    cpp/ShstTcpIo.h \
    cpp/ziputils.h

DISTFILES += \
    android/AndroidManifest.xml \
    android/build.gradle \
    android/gradle/wrapper/gradle-wrapper.jar \
    android/gradle/wrapper/gradle-wrapper.properties \
    android/gradlew \
    android/gradlew.bat \
    android/res/values/libs.xml \
    android/src/bio/shenhua/ShareFileService.java \
    android/src/bio/shenhua/WiFiService.java \
    android/src/bio/shenhua/ZipFileNotifier.java \
    android/src/bio/shenhua/ZipFileUtil.java \
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

ANDROID_ABIS = arm64-v8a

