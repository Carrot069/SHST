import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

import WzDatabaseService 1.0
import WzIniSetting 1.0
import WzUtils 1.0
import WzRender 1.0

import "Components"
import "Controller"
import "WzControls"
import "."

ApplicationWindow {
    id: window
    visible: false
    width: 1280
    height: 800
    flags: Qt.Window | Qt.WindowFullscreenButtonHint
    property alias pageOption: pageOption
    property bool isMini: WzUtils.isMini()
    // TODO 重命名为 command
    signal debugCommand(string cmd)
    signal commandResponse(string response)
    // TODO 重命名为 sendCommandResponse
    function debugCommandResponse(response){sendCommandResponse(response)}
    function sendCommandResponse(response){func.sendCommandResponse(response)}
    function readStrOption(optionName, defaultValue){return func.readStrOption(optionName, defaultValue)}
    function execCommand(cmd) {func.execCommand(cmd)}

    property real scale: {
        var val = window.height / 800
        if (val > 1) {
            return 1
        } else {
            return val
        }
    }

    function getPageByIndex(pageIndex) {
        switch(pageIndex) {
        case 0: return pageCapture
        case 1: return pageImage
        case 2: return pageOption
        }
    }

    function showPage(showPageIndex) {
        var showPage = getPageByIndex(showPageIndex)
        if (rootView.activePageIndex === showPageIndex)
            return
        pageChangeAnimation.running = false
        pageChangeAnimation.showPage = showPage
        pageChangeAnimation.hidePage = getPageByIndex(rootView.activePageIndex)
        pageChangeAnimation.running = true
        rootView.activePageIndex = showPageIndex
    }

    function closeSelf(buttonID) {
        close()
        if (buttonID === 2)
            WzRender.restart()
    }

    onClosing: {
        // TODO 等待所有线程结束

        renderThread.stopThread();

        pageCapture.mcu.closeAllLight()

        dbService.saveIntOption("windowWidth", window.width)
        dbService.saveIntOption("windowHeight", window.height)
        dbService.saveIntOption("windowLeft", window.x)
        dbService.saveIntOption("windowTop", window.y)
        dbService.saveIntOption("windowVisibility", window.visibility)

        if(1==0){
        dbService.saveAdminSetting(pageCapture.adminParams)
        }

        launcherHelper.exiting()
    }

    Component.onCompleted: {
        var ww = dbService.readIntOption("windowWidth", -1)
        var wh = dbService.readIntOption("windowHeight", -1)
        var wx = dbService.readIntOption("windowLeft", -1)
        var wy = dbService.readIntOption("windowTop", -1)
        if (ww === -1)
            ww = Screen.desktopAvailableWidth <= 1280 ? 1024 : 1280
        if (wh === -1)
            wh = Screen.desktopAvailableHeight <= 800 ? 700 : 800

        if (wx === -1) wx = (Screen.desktopAvailableWidth - ww) / 2
        if (wy === -1) wy = (Screen.desktopAvailableHeight - wh) / 2

        window.x = wx
        window.y = wy
        window.width = ww
        window.height = wh
        window.visibility = dbService.readIntOption("windowVisibility", 0)

        if (wh > Screen.desktopAvailableHeight || ww > Screen.desktopAvailableWidth)
            window.showMaximized()

        visible = true

        if (WzRender.getCount() === 0) {
            hardlockNotFound()
            launcherHelper.started()
            return
        } else {
            renderThread.startThread()
        }

        var imagePath = dbService.readStrOption("image_path", "")
        if (undefined === imagePath || "" === imagePath || !WzUtils.validatePath(imagePath)) {
            rectangleShade.opacity = 0.7
            imagePathSelect.show()
        }

        launcherHelper.started()
    }

    WzDatabaseService {
        id: dbService
    }
    WzIniSetting {
        id: iniSetting
    }
    WzRenderThread {
        id: renderThread
        onRender: {
            if (!msgBox.visible) {
                hardlockNotFound()
            }
        }
    }

    Item {
        id: rootView
        property int activePageIndex: 0
        width: parent.width / window.scale
        height: parent.height / window.scale - (textFieldCmd.visible ? textFieldCmd.height : 0)
        anchors.top: parent.top
        anchors.left: parent.left
        transform: Scale { xScale: window.scale; yScale: window.scale }

        property int keyF2Count: 0
        Keys.onPressed: {
            if (event.key === Qt.Key_F2 && event.modifiers & Qt.ShiftModifier)
                keyF2Count += 1
            if (keyF2Count === 10) {
                keyF2Count = 0
                textFieldCmd.visible = true
                textFieldCmd.forceActiveFocus()
                loaderDebugCommandResponse.active = true
            }
        }

        PageCapture {
            id: pageCapture
            x: 0
            y: 0
            width: parent.width
            height: parent.height
            onNewImage: {
                var showOptions = {
                    autoFitOnce: true,
                    isNewImage: true
                }
                pageImage.openImage(imageFile, showOptions)
            }
            onPageChange: {
                showPage(pageIndex)
            }
            onMarkerImageSaved: {
                pageImage.updateMarkerImage(fileName)
                showPage(1)
            }
        }
        PageImage {
            id: pageImage
            x: 0
            y: 0
            width: parent.width
            height: parent.height
            visible: opacity > 0
            opacity: 0
            onPageChange: {
                showPage(pageIndex)
            }
            onCaptureWhiteImage: {
                showPage(0)
                pageCapture.captureWhiteImage()
            }
        }
        PageOption {
            id: pageOption
            y: 0
            width: parent.width
            height: parent.height
            visible: opacity > 0
            opacity: 0
            onPageChange: {
                showPage(pageIndex)
            }
            onAdminParamsChanged: {
                pageCapture.adminParams = params
            }
        }

        Rectangle {
            id: rectangleShade
            anchors.fill: parent
            opacity: 0
            visible: opacity > 0
            color: "black"
            z: 9999

            Behavior on opacity { NumberAnimation { duration: 200 }}

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                preventStealing: true
            }
        }

        WzMsgBox {
            id: msgBox
            z: rectangleShade.z + 1
            onVisibleChanged: {
                if (visible)
                    rectangleShade.opacity = 0.5
                else
                    rectangleShade.opacity = 0
            }
        }

        WzPopupMsg {
            id: popupMsg
            z: 10000
            anchors.centerIn: parent
        }

        ImagePathSelect {
            id: imagePathSelect
            z: rectangleShade.z + 1
            opacity: 0
            visible: opacity > 0
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            onHide: rectangleShade.opacity = 0
            onImagePathChanged: pageOption.imagePath = imagePath
        }

        About {
            id: about
            opacity: 0
            visible: opacity > 0
            z: rectangleShade.z + 1
            onVisibleChanged: {
                if (visible)
                    rectangleShade.opacity = 0.8
                else
                    rectangleShade.opacity = 0
            }
        }
    }

    LogViewer {
        id: logViewer
        visible: false
        anchors {
            fill: parent
            topMargin: position === "top" ? 0 : parent.height * 0.6
            bottomMargin: position === "bottom" ? textFieldCmd.height : parent.height * 0.6
        }
    }
    WzLoader {
        id: loaderDebugCommandResponse
        active: textFieldCmd.visible
        anchors.bottom: textFieldCmd.top
        visible: false
        width: parent.width
        source: "qrc:/Components/DebugCommandResponse.qml"
        Connections {
            target: loaderDebugCommandResponse.item
            onClose: loaderDebugCommandResponse.visible = false
            onShow: loaderDebugCommandResponse.visible = true
        }
    }
    TextField {
        id: textFieldCmd
        visible: false
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: parent.right
        anchors.rightMargin: 10
        Keys.onPressed: {
            if (event.key !== Qt.Key_Return)
                return
            if (text === "exit")
                textFieldCmd.visible = false
            else {
                debugCommand(textFieldCmd.text)
            }
        }
    }

    SettingController {
        id: settingController
        onResponse: loaderDebugCommandResponse.item.append(response)
    }

    ParallelAnimation {
        id: pageChangeAnimation
        running: false
        property var showPage
        property var hidePage

        NumberAnimation {
            target: pageChangeAnimation.showPage === undefined ? null : pageChangeAnimation.showPage
            duration: 700
            properties: "x"
            from: pageChangeAnimation.showPage === undefined ? 0 : -pageChangeAnimation.showPage.width
            to: 0
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: pageChangeAnimation.hidePage === undefined ? null : pageChangeAnimation.hidePage
            duration: 700
            properties: "x"
            from: 0
            to: pageChangeAnimation.hidePage === undefined ? 0 : pageChangeAnimation.hidePage.width
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: pageChangeAnimation.showPage === undefined ? null : pageChangeAnimation.showPage
            duration: 900
            properties: "opacity"
            from: 0
            to: 1
            easing.type: Easing.InOutQuad
        }
        NumberAnimation {
            target: pageChangeAnimation.hidePage === undefined ? null : pageChangeAnimation.hidePage
            duration: 900
            properties: "opacity"
            from: 1
            to: 0
            easing.type: Easing.InOutQuad
        }
    }

    function hardlockNotFound() {
        pageCapture.visible = false
        pageImage.visible = false
        pageOption.visible = false
        msgBox.buttonClicked.connect(closeSelf)
        msgBox.show(qsTr("没有找到加密锁，请连接仪器后重新打开本软件。"), qsTr("退出"))
    }

    function showAbout() {
        about.opacity = 1
    }

    QtObject {
        id: func
        function sendCommandResponse(response) {
            if (loaderDebugCommandResponse.active) {
                if (!loaderDebugCommandResponse.visible)
                    loaderDebugCommandResponse.visible = true
                loaderDebugCommandResponse.item.append(response)
            }
            window.commandResponse(response)
        }
        function readStrOption(optionName, defaultValue) {
            return dbService.readStrOption(optionName, defaultValue)
        }
        function execCommand(cmd) {
            debugCommand(cmd)
        }
    }
}
