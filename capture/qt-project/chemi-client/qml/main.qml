import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

import WzDatabaseService 1.0
import WzUtils 1.0
import WzRender 1.0
import WzJavaService 1.0
import ShstCapture 1.0

import "Components"
import "WzControls"
import "."

ApplicationWindow {
    id: window
    visible: false
    width: 1280
    height: 800
    flags: Qt.Window | Qt.WindowFullscreenButtonHint

    property real scale: {
        if (WzUtils.isPad()) {
            return window.height / 760
        } else {
            var val = window.height / 800
            if (val > 1) {
                return 1
            } else {
                return val
            }
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
        /* issue8 2021-03-08 11:04:28
        if (showPage.opacity === 1) return
        pageChangeAnimation.showPage = showPage
        pageChangeAnimation.hidePage = getPageByIndex(rootView.activePageIndex)
        pageChangeAnimation.running = true
        */
        rootView.activePageIndex = showPageIndex
        pageSwipeView.currentIndex = showPageIndex
    }

    function closeSelf(buttonID) {
        close()
        if (buttonID === 2)
            WzRender.restart()
    }

    function saveAdminSetting(params) {
        pageCapture.saveAdminSetting(params)
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

        saveAdminSetting(pageCapture.adminParams)
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
            //hardlockNotFound()
            return;
        } else {
            renderThread.startThread();
        }

        if (WzUtils.isMobile()) {
            var autoConnectWiFi = dbService.readIntOption("autoConnectWiFi", 1)
            if (autoConnectWiFi) {
                var wifiName = dbService.readStrOption("wifiName", "SH-Infinite523")
                var wifiPassword = dbService.readStrOptionAes("wifiPassword", "shst1234")
                if (undefined !== wifiName && "" !== wifiName &&
                        undefined !== wifiPassword && "" !== wifiPassword) {
                    WzJavaService.connectWiFi(wifiName, wifiPassword)
                }
            }
        } else {
            var imagePath = dbService.readStrOption("image_path", "")
            if (undefined === imagePath || "" === imagePath || !WzUtils.validatePath(imagePath)) {
                rectangleShade.opacity = 0.7
                imagePathSelect.show()
            }
        }

        console.info("scale:", window.scale)
    }

    WzDatabaseService {
        id: dbService
    }
    WzRenderThread {
        id: renderThread
        onRender: {
            if (!msgBox.visible) {
                hardlockNotFound()
            }
        }
    }
    ShstServerInfo {
        id: serverInfo
        function getFullWsUrl() {
            return "ws://" + serverHost + ":" + serverPort
        }
    }
    Heart {
        id: heart
    }

    Item {
        id: rootView
        property int activePageIndex: 0
        width: parent.width / window.scale
        height: parent.height / window.scale
        anchors.top: parent.top
        anchors.left: parent.left
        transform: Scale { xScale: window.scale; yScale: window.scale }

        // issue8 2021-03-08 11:04:28
        SwipeView {
            id: pageSwipeView
            anchors.fill: parent

            PageCapture {
                id: pageCapture
                /* issue8 2021-03-08 11:04:28
                x: 0
                y: 0
                width: parent.width
                height: parent.height
                */
                onNewImage: {
                    console.debug("PageCapture.onNewImage")
                    var showOptions = {
                        autoFitOnce: true,
                        isNewImage: true,
                        isPreload: true // 预加载到缩略图列表, 大图可能还在从服务器下载中
                    }
                    pageImage.openImage(imageFile, showOptions)
                }
                onImageLoaded: {
                    console.debug("PageCapture.onImageLoaded")
                    var showOptions = {
                        autoFitOnce: true,
                        isNewImage: true,
                        isPreload: false
                    }
                    pageImage.openImage(imageFile, showOptions)
                }
                onPageChange: {
                    showPage(pageIndex)
                }
            }
            PageImage {
                id: pageImage
                /* issue8 2021-03-08 11:04:28
                x: 0
                y: 0
                width: parent.width
                height: parent.height
                visible: opacity > 0
                opacity: 0
                */
                onPageChange: {
                    showPage(pageIndex)
                }
            }
            PageOption {
                id: pageOption
                /* issue8 2021-03-08 11:04:28
                y: 0
                width: parent.width
                height: parent.height
                visible: opacity > 0
                opacity: 0
                */
                onPageChange: {
                    showPage(pageIndex)
                }
                onAdminParamsChanged: {
                    pageCapture.adminParams = params
                }
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

        WzBusyBox {
            id: busyBox
            z: rectangleShade.z + 1
            onVisibleChanged: {
                if (visible)
                    rectangleShade.opacity = 0.5
                else
                    rectangleShade.opacity = 0
            }
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

    function showAbout() {
        about.opacity = 1
    }
}
