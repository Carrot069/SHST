QT += quick core widgets

CONFIG += c++11

DEFINES += ONLY_CHINESE

# You can make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

VERSION = 1.0.0.1
DEFINES += APP_VERSION=\\\"$$VERSION\\\"
QMAKE_TARGET_COMPANY="Shenhua Science Technology"
QMAKE_TARGET_COPYRIGHT="Shenhua Science Technology"
QMAKE_TARGET_PRODUCT="SHST Capture Crash"
QMAKE_DEVELOPMENT_TEAM="Shenhua Science Technology"
QMAKE_FRAMEWORK_BUNDLE_NAME="SHST Capture"
QMAKE_TARGET_BUNDLE_PREFIX="bio.shenhua"

SOURCES += \
        WzI18N.cpp \
        StartProcess.cpp \
        main.cpp

RESOURCES += qml.qrc \
    i18n.qrc

win32: {
    RC_ICONS = app.ico
}

# Additional import path used to resolve QML modules in Qt Creator's code model
QML_IMPORT_PATH =

# Additional import path used to resolve QML modules just for Qt Quick Designer
QML_DESIGNER_IMPORT_PATH =

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

HEADERS += \
    WzI18N.h \
    StartProcess.h

TRANSLATIONS += \
    translations/i18n_en.ts
