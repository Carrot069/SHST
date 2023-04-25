import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import QtGraphicalEffects 1.12

import WzCapture 1.0
import WzUtils 1.0
import WzI18N 1.0
import WzEnum 1.0
import "WzControls"
import "Components"
import "Controller/PageCaptureController.js" as PageCaptureController
import "Controller"

Item {
    id: root

    signal newImage(string imageFile)
    signal pageChange(int pageIndex)
    signal markerImageSaved(string fileName)

    property bool mcuConnected: mcu.connected || WzUtils.isDemo()
    property bool cameraConnected: false
    property bool isPreviewing: false
    property bool captureWithoutMarker: false
    property bool isAutoFocusSelectRect: false
    property bool isAutoFocusing: false
    property bool previewImageControlEnabled: false
    property alias captureProgress: captureProgress
    property alias mcu: mcu
    property int cameraTemperature: 0
    property bool isRemoveFluorCircle: {
        return 1 === dbService.readIntOption("removeFluorCircle", 0)
    }

    property var adminParams

    function captureWhiteImage() {
        captureWhiteImageRect.opacity = 1
        if (!switchPreview.checked)
            switchPreview.checked = true
    }
    function getStorageWhite(m_checked)
    {
        captureService.storageWhite=m_checked;
    }

    WzMcu {
        id: mcu
        onLightSwitched: {
            console.info("Mcu.onLightSwitched:", lightType, isOpened)
            switchLightTimer.running = false
            if (!isOpened) {
                lightControl.closeAll()
                fluorescence.closeAllLight()
            } else {                
                lightControl.setLightChecked(lightType, true)
                if (lightType === "red" || lightType === "green" || lightType === "blue") {
                    lightControl.closeAll()
                    fluorescence.setLightChecked(lightType)
                } else {
                    fluorescence.closeAllLight()
                    // 荧光方案下, 切换到白光时自动把滤镜轮转到空位
                    if (PageCaptureController.capturePlanIsFluor() && lightType === "white_up") {
                        switchToFilter0Timer.running = true
                    }
                }
            }
        }
        onDevicesChanged: {
            console.debug("device available count:", availableCount)
            console.debug(devices)
            if (availableCount === 0) {
                if (!WzUtils.isDemo())
                    msgBox.show(qsTr("没有发现可用的主控器"), qsTr("确定"))
            } else if (availableCount === 1) {
                for (var j = 0; j < devices.length; j++) {
                    if (devices[j]["sn"] !== undefined && devices[j]["sn"] !== "") {
                        var bus = devices[j]["bus"]
                        var address = devices[j]["address"]
                        var sn = devices[j]["sn"]
                        console.info("Only one device was found, open it now.", bus, address, sn);
                        mcu.openDevice(bus, address)
                    }
                }
            } else {
                var previousOpenedSn = dbService.readStrOption("opened_usb_sn", "")
                // 如果之前打开过usb设备则尝试找到这个sn的usb设备并打开
                // 暂时屏蔽打开上次打开过的设备 if ("" !== previousOpenedSn) {
                    for (var i = 0; i < devices.length; i++) {
                        //if (devices[i]["sn"] === previousOpenedSn) {
                        if (devices[i]["sn"] !== "") {
                            var bus2 = devices[i]["bus"]
                            var address2 = devices[i]["address"]
                            mcu.openDevice(bus2, address2)
                            return
                        }
                    }
                //}

                // 在没有解决一台仪器中多个设备(加密锁/主控电路板/相机)可以绑定在一起被识别出一台仪器之前暂时不弹出对话框让用户选择

            }
        }
        onOpened: {
            dbService.saveStrOption("opened_usb_sn", sn)
            console.info("USB Device is opened, sn:", sn)
            //mcu.getPowerOnTick()
            //getPowerOnTickTimer.running = true
        }
        onError: {
            msgBox.show(errorMsg, qsTr("确定"))
        }
        onPowerOnTick: {
            //getPowerOnTickTimer.isGot = true
            //getPowerOnTickTimer.tick = tick
            console.info("PowerOnTick:", tick)
        }
    }
    McuController {
        id: mcuController
        onReady: {
            capturePlan.init()
        }
        onResponseTimeout: {
            msgBox.show(qsTr("USB通信响应超时，可尝试拔插USB数据线"), qsTr("确定"))
        }
    }
    Timer {
        id: getPowerOnTickTimer
        property bool isGot: false
        property int tick: -1
        running: false
        interval: 500
        repeat: true
        onTriggered: {            
            if (!isGot)
                mcu.getPowerOnTick()
            else {
                interval = 1000
                tick++
            }
        }
    }
    Timer {
        id: switchBinningTimer
        repeat: false
        running: false
        interval: 5000
        onTriggered: {
                capturePlan.comboBoxCapturePlan.enabled=true
                switchPreview.checked=true
        }
    }
    Timer {
        id: switchToFilter0Timer
        repeat: false
        running: false
        interval: 800
        onTriggered: {
            mcu.switchFilterWheel(0)
            fluorescence.setActiveFilter(0)
        }
    }
    Timer {
        id: getTemperatureTimer
        interval: 1000
        repeat: true
        property bool isVirtual: false
        property bool isVirtual2: false
        property int targetTemp: 0
        property real currentTemp: 0
        onTriggered: {
            if (!PageCaptureController.atikCameraTemperature()) {
                var realTemp = captureService.getCameraTemperature()
                if (realTemp <= -30 && isVirtual)
                    isVirtual = false

                if (isVirtual) {
                    cameraTemperature = Math.round(currentTemp)
                    if (currentTemp > targetTemp) {
                        currentTemp = currentTemp + ((-37.0 - currentTemp) / 100)
                    } else if (currentTemp < targetTemp)
                        currentTemp = currentTemp + 0.16
                    if (currentTemp < -30)
                        currentTemp = -30
                } else {
                    cameraTemperature = realTemp
                    currentTemp = realTemp
                }
            }
            previewControl.cameraTemperature = cameraTemperature
            textCameraState.text = qsTr("相机温度: ") + cameraTemperature
            if (isMini)
                PageCaptureController.miniAutoStartPreview()
        }
    }

    Timer {
        id: switchFilterTimer
        repeat: true
        running: false
        interval: 5000
    }

    Timer {
        id: switchLightTimer
        property int count: 5
        property int lightType: 0
        repeat: true
        running: false
        interval: 500
        onTriggered: {
            // 2021-09-23 14:21:04
            // 测试过程中，累积拍摄时继电器会响, 暂时去掉这个机制
            running = false
            /*
            mcu.switchLight(lightType, true)
            count--
            if (count === 0) {
                running = false
                msgBox.show(qsTr("无法打开光源"), qsTr("确定"))
            }
            */
        }
    }

    Timer {
        id: changePreviewExposureMs
        interval: 500
        repeat: false
        running: false
        onTriggered: {
            if (isMini) {}
            else if (PageCaptureController.capturePlanIsChemiFluor())
                captureService.previewExposureMs = previewExposure.exposureTime.value
            else
                captureService.previewExposureMs = exposureTime.exposureMs
            captureService.resetPreview()
        }
    }

    WzCaptureService {
        id: captureService
        //isAutoExposure: textExposureTimeAutoCheckBox.isChecked
        onCaptureState: {
            switch(state) {
            case WzCaptureService.Init:
                captureProgress.buttonCancel2.visible = true
                captureProgress.isProgress = false
                captureProgress.text = qsTr("正在初始化")
                break
            case WzCaptureService.AutoExposure:
                captureProgress.buttonCancel2.visible = false
                captureProgress.isProgress = false
                captureProgress.text = qsTr("正在自动曝光")
                break
            case WzCaptureService.AutoExposureFinished:
                exposureTime.exposureMs = captureService.autoExposureMs
                break
            case WzCaptureService.Exposure:
                // 如果是多帧拍摄则显示当前已拍摄和总的拍摄帧数, 如果是单帧拍摄则不显示任何文本
                if (captureCount === 1)
                    captureProgress.countProgress = ""
                else {
                    captureProgress.countProgress = (captureIndex + 1) + "/" + captureCount
                }

                if (captureService.exposurePercent === -1) {
                    captureProgress.isProgress = false
                    captureProgress.buttonCancel2.visible = false
                    captureProgress.text = qsTr("正在曝光")
                } else {
                    captureProgress.isProgress = true
                    captureProgress.progress = captureService.exposurePercent
                    captureProgress.leftTime = captureService.leftExposureTime
                    captureProgress.elapsedTime = captureService.elapsedExposureTime
                }
                break
            case WzCaptureService.Aborted:
                captureProgress.visible = false
                rectShade.opacity = 0
                if (adminParams.repeatSetFilter) {
                    mcu.repeatSetFilter = true
                }
                break
            case WzCaptureService.Image:
                captureProgress.buttonCancel2.visible = true
                captureProgress.isProgress = false
                captureProgress.text = qsTr("正在处理图片")
                break
            case WzCaptureService.Finished:
                newImage(latestImageFile)
                if (captureCount === capturedCount) {
                    captureProgress.visible = false
                    rectShade.opacity = 0
                    exposureArea.visible = false

                    if (privateObject.isStartPreviewAfterCaptured) {
                        privateObject.isStartPreviewAfterCaptured = false
                        switchPreview.checked = true
                    }
                }
                pageChange(1)
                if (mcu.latestLightOpened &&
                        (mcu.latestLightType === "uv_penetrate" || mcu.latestLightType === "uv_penetrate_force")) {
                    mcu.closeAllLight()
                    lightControl.closeAll()
                }
                if (adminParams.repeatSetFilter) {
                    mcu.repeatSetFilter = true
                }
                break
            }
        }
        onCameraStateChanged: {
            switch(state) {
            case WzCameraState.Connecting:
                previewControl.cameraState = "cameraConnecting"
                textCameraState.text = qsTr("正在连接相机")
                break;
            case WzCameraState.Connected:
            case WzCameraState.CaptureFinished:
                cameraConnected = true
                previewControl.cameraState = "cameraConnected"
                textCameraState.text = qsTr("相机已连接")
                getTemperatureTimer.running = true
                break;
            case WzCameraState.Disconnected:
                cameraConnected = false
                getTemperatureTimer.running = false
                previewControl.cameraState = "cameraDisconnected"
                textCameraState.text = qsTr("相机已断开")
                break;
            case WzCameraState.PreviewStarted:
                isPreviewing = true
                previewControl.previewState = "cameraPreviewStarted"
                //textCameraState.text = qsTr("正在预览")
                if (!mcu.latestLightOpened) {
                    var lightType = capturePlan.getLightNameFromIndex(capturePlan.params.light)
                    mcu.switchLight(lightType, true)
                    if (WzUtils.isMini()) {
                        switchLightTimer.lightType = lightType
                        switchLightTimer.count = 5
                        switchLightTimer.running = true
                    }
                }                
                break
            case WzCameraState.PreviewStopped:
                // 因为切换binning导致的预览停止, 要重新开始预览
                if (privateObject.isStartPreviewForChangeBinning) {
                    privateObject.isStartPreviewForChangeBinning = false
                    captureService.enableCameraPreview(true, buttonBinning.binning)
                    return
                }

                isPreviewing = false
                previewControl.previewState = "cameraPreviewStopped"
                //textCameraState.text = qsTr("预览已停止")
                liveImageView.clearImage()
                switchPreview.checked = false
                if (mcu.latestLightOpened && mcu.latestLightType === "white_up") {
                    mcu.closeAllLight()
                }
                break
            case WzCameraState.CameraNotFound:
                previewControl.cameraState = "cameraNotFound"
                textCameraState.text = qsTr("相机未连接")
                break;
            }           
        }  
        onAutoFocusFar: mcu.focusFar(step)
        onAutoFocusNear: mcu.focusNear(step)
        onAutoFocusStop: {
            isAutoFocusing = false
            mcu.focusStop()
        }
        onAutoFocusFinished: {
            isAutoFocusing = false
            mcu.focusStop()
        }
    }

    Component.onCompleted: {
        captureService.connectCamera()
        mcu.init()
        captureService.imagePath = dbService.readStrOption("image_path", "");

        adminParams = dbService.readAdminSetting()
        if (adminParams === undefined) {
            msgBox.show(qsTr("无法读取高级设置"), qsTr("确定"))
            return
        }
        if (adminParams.errorCode !== undefined) {
            msgBox.show(qsTr("无法读取高级设置, 错误代码: " + adminParams.errorCode), qsTr("确定"))
        }
        if (adminParams.repeatSetFilter) {
            mcu.repeatSetFilter = true
        }
        if (adminParams.isFilterWheel8 !== undefined)
            fluorescence.isFilterWheel8 = adminParams.isFilterWheel8
        if (adminParams.customFilterWheel && adminParams.filters) {
            fluorescence.setFilterOptions(adminParams.filters)
        }
        focus2.setLayerPWMCount(adminParams.layerPWMCount)

        var capturePlanIndex = dbService.readIntOption("capture_plan", 0)
        console.debug("capture_plan, index: ", capturePlanIndex)
        capturePlan.comboBoxCapturePlan.currentIndex = capturePlanIndex
        capturePlan.comboBoxCapturePlan.enabled = false
        capturePlan.firstSelectPlanTimer.running = true
    }
    Connections {
        target: window
        onClosing: {
            mcu.uninit()
        }
    }
    Connections {
        target: pageOption
        onBinningChanged: {
            if (isMini) {
                if (buttonBinning.binning !== pageOption.binning) {
                    buttonBinning.binning = pageOption.binning
                    if (isPreviewing) {
                        captureService.enableCameraPreview(false, buttonBinning.binning)
                        privateObject.isStartPreviewForChangeBinning = true
                        // 不能马上启用预览, 因为停止和开始预览的代码是在多线程中异步执行的, 虽然停止预览的函数立即返回了
                        // 但线程中可能还没有停止预览, 所以要在收到多线程发出的已经停止预览的状态后再重新预览
                        //captureService.enableCameraPreview(true, buttonBinning.binning)
                    }
                }
            }
        }
    }

    Connections {
        target: window
        onDebugCommand: {
            if (cmd.startsWith("set exposure ms")) {
                var stringList = cmd.split(" ")
                if (stringList.length > 3) {
                    var exposureMs = parseInt(stringList[3])
                    if (exposureMs)
                        exposureTime.exposureMs = exposureMs
                }
            } else if (cmd.startsWith("mcu,")) {
                mcu.exec(cmd.substring(4))
            }
            // TODO 2022-5-12 暂时存放在这里
            else if (cmd.startsWith("adminparams")) {
                var adminParamsCmdList = cmd.split(",")
                if (adminParamsCmdList.length === 1) {
                    var adminParams2 = dbService.readAdminSetting()
                    sendCommandResponse(JSON.stringify(adminParams2))
                }
                else if (adminParamsCmdList[1] === "save") {
                    dbService.saveAdminSetting(adminParams)
                }
            }
        }
    }

    WzBusyIndicator {
        id: busyLoadMini
        width: 150
        height: 150
        anchors.centerIn: parent
        visible: isMini
        color: "white"
    }
    Loader {
        id: miniCaptureLoader
        active: isMini
        asynchronous: true
        source: "Components/Mini/CaptureMini.qml"
        anchors.fill: parent
        onStatusChanged:
            if (status === Loader.Ready) {
                busyLoadMini.visible = false
                loaderMiniPreviewControl.active = true
                loaderMiniPreviewControl.visible = true
            }
    }

    Rectangle {
        id: miniPreviewRectangle
        anchors.fill: rectangleLeft
        radius: 10
        visible: false
        color: "white"
        opacity: 0.01
    }

    Rectangle {
        id: rectangleLeft
        color: isMini ? "transparent" : "#cccccc"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: rectangleRight.left
        anchors.leftMargin: isMini ? 21 : 0
        anchors.topMargin: isMini ? 21 : 0
        anchors.bottomMargin: isMini ? 21 : 0
        anchors.rightMargin: isMini ? -129 : 0
        clip: true
        radius: isMini ? 10 : 0
        visible: !isMini

        //[[实时画面显示控件]]//
        Flickable {
            id: imageViewWrapper
            anchors.fill: parent
            clip: true
            interactive: false // 禁用鼠标拖动内容区域是因为区域自动曝光需要用鼠标拖动框选区域
            contentWidth: (liveImageView.showWidth < width || !previewImageControlEnabled) ? width : liveImageView.showWidth
            contentHeight: (liveImageView.showHeight < height || !previewImageControlEnabled) ? height : liveImageView.showHeight
            property real previousWidthRatio
            property real previousHeightRatio
            property real previousXPos
            property real previousYPos
            property bool isIgnorePosChanged
            visibleArea.onWidthRatioChanged: {
                if (previousWidthRatio === 1) {
                    imageViewHorizontalScrollBar.position = (1 - visibleArea.widthRatio) / 2
                } else {
                    imageViewHorizontalScrollBar.position = (previousXPos / (1 - previousWidthRatio)) * (1 - visibleArea.widthRatio)
                }
                previousWidthRatio = visibleArea.widthRatio
                previousXPos = visibleArea.xPosition
            }
            visibleArea.onHeightRatioChanged: {
                if (previousHeightRatio === 1) {
                    imageViewVerticalScrollBar.position = (1 - visibleArea.heightRatio) / 2
                } else {
                    imageViewVerticalScrollBar.position = (previousYPos / (1 - previousHeightRatio)) * (1 - visibleArea.heightRatio)
                }
                previousHeightRatio = visibleArea.heightRatio
                previousYPos = visibleArea.yPosition
            }
            visibleArea.onXPositionChanged: {
                if (isIgnorePosChanged) {
                    return
                }
                previousXPos = visibleArea.xPosition
                liveImageView.x = contentX
            }
            visibleArea.onYPositionChanged: {
                if (isIgnorePosChanged) {
                    return
                }
                previousYPos = visibleArea.yPosition
                liveImageView.y = contentY
            }
            Component.onCompleted: {
                previousWidthRatio = visibleArea.widthRatio
                previousHeightRatio = visibleArea.heightRatio
                previousXPos = visibleArea.xPosition
                previousYPos = visibleArea.yPosition
            }

            ScrollBar.horizontal: ScrollBar {
                id: imageViewHorizontalScrollBar
                policy: imageViewWrapper.contentWidth > imageViewWrapper.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                opacity: 0.8

                background: Rectangle {
                    color: "transparent"
                }
                contentItem: Rectangle {
                    implicitHeight: 15
                    color: "#333333"
                    radius: 15
                }
            }
            ScrollBar.vertical: ScrollBar {
                id: imageViewVerticalScrollBar
                policy: imageViewWrapper.contentHeight > imageViewWrapper.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                opacity: 0.8

                background: Rectangle {
                    color: "transparent"
                }

                contentItem: Rectangle {
                    implicitWidth: 15
                    color: "#333333"
                    radius: 15
                }
            }

            Rectangle {
                width: imageViewWrapper.width
                height: imageViewWrapper.height
                x: liveImageView.x
                y: liveImageView.y
                color: "#7f7f7f"
                visible: !isMini
            }

            LiveImageView {
                id: liveImageView
                layer.enabled: isMini
                layer.effect: OpacityMask {
                    maskSource: miniPreviewRectangle
                }

                zoomEnabled: root.previewImageControlEnabled
                onZoomChanging: imageViewWrapper.isIgnorePosChanged = true
                onZoomChanged: {
                    imageViewWrapper.isIgnorePosChanged = false
                    x = imageViewWrapper.contentX
                    y = imageViewWrapper.contentY
                }
                width: imageViewWrapper.width
                height: imageViewWrapper.height

                // auto focus //
                Rectangle {
                    id: autoFocusSelectRect
                    color: "transparent"
                    border.color: "red"
                    visible: false
                }
                MouseArea {
                    id: autoFocusMouseArea
                    anchors.fill: parent
                    visible: isAutoFocusSelectRect
                    propagateComposedEvents: true
                    property int pressedX: 0
                    property int pressedY: 0
                    onClicked: {
                        parent.forceActiveFocus()
                        mouse.accepted = false
                    }
                    onPressed: {
                        if (isAutoFocusSelectRect) {
                            pressedX = mouseX - 1
                            pressedY = mouseY - 1
                            autoFocusSelectRect.x = mouseX - 1
                            autoFocusSelectRect.y = mouseY - 1
                            autoFocusSelectRect.width = 1
                            autoFocusSelectRect.height = 1
                            autoFocusSelectRect.visible = true
                        }
                    }
                    onReleased: {
                        if (isAutoFocusSelectRect) {
                            isAutoFocusSelectRect = false
                            isAutoFocusing = true
                            autoFocusSelectRect.visible = false
                            liveImageView.selectRect(autoFocusSelectRect.x, autoFocusSelectRect.x + autoFocusSelectRect.width,
                                                     autoFocusSelectRect.y, autoFocusSelectRect.y + autoFocusSelectRect.height)
                            captureService.startAutoFocus()
                        }
                    }
                    onPositionChanged: {
                        if (pressed) {
                            var x1 = Math.min(pressedX, mouseX)
                            var x2 = Math.max(pressedX, mouseX)
                            var y1 = Math.min(pressedY, mouseY)
                            var y2 = Math.max(pressedY, mouseY)
                            autoFocusSelectRect.x = x1
                            autoFocusSelectRect.y = y1
                            autoFocusSelectRect.width = x2 - x1
                            autoFocusSelectRect.height = y2 - y1
                        }
                    }
                }
                // auto focus //

                Rectangle {
                    id: exposureArea
                    visible: false
                    color: "transparent"
                    border.width: 3
                    border.color: "red"
                }
                MouseArea {
                    id: exposureAreaMosueArea
                    visible: !autoFocusHintRect.visible
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: parent.forceActiveFocus()
                    onPressed: {
                        if (WzUtils.isExposureArea()) {
                            exposureArea.x = mouseX
                            exposureArea.y = mouseY
                            exposureArea.width = 1
                            exposureArea.height = 1
                            exposureArea.visible = false
                        }
                    }
                    onPositionChanged: {
                        if (WzUtils.isExposureArea()) {
                            if (pressed) {
                                if (mouseX > exposureArea.x && mouseY > exposureArea.y) {
                                    exposureArea.visible = true
                                    exposureArea.width = mouseX - exposureArea.x
                                    exposureArea.height = mouseY - exposureArea.y
                                }
                            }
                        }
                    }
                    onWheel: {
                        if (!root.previewImageControlEnabled) return
                        var zoom = liveImageView.zoom
                        if (wheel.angleDelta.y > 0) {
                            zoom = Math.round(zoom * 1.1)
                        } else {
                            zoom = Math.round(zoom * 0.9)
                        }
                        liveImageView.zoom = zoom
                    }
                }

                Rectangle {
                    id: autoFocusHintRect
                    radius: 5
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.top
                    anchors.topMargin: 20
                    color: "#070707"
                    height: 40
                    width: 200
                    visible: isAutoFocusSelectRect || isAutoFocusing
                    Text {
                        anchors.centerIn: parent
                        color: "#dddddd"
                        font.pixelSize: 17
                        text: {
                            if (isAutoFocusSelectRect)
                                return qsTr("请划选一个聚焦区域")
                            else if (isAutoFocusing)
                                return qsTr("正在自动聚焦")
                            else
                                return ""
                        }
                    }
                    MouseArea {
                        anchors.fill: parent
                        //onClicked: captureService.stopAutoFocus()
                        onClicked: focus2.stopFocus()
                    }
                }
            }

        }
        //[[实时画面显示控件]]//

        //[[预览控制盘]]//
        PreviewControl {
            id: previewControl
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 20
            visible: false // 隐藏不用, 2020-5-15 by wangzhe

            //Component.onCompleted: previewControl.exposureTime.setExposureMs(dbService.readIntOption("preview_exposure_ms", 10))
            //Component.onDestruction: dbService.saveIntOption("preview_exposure_ms", previewControl.exposureTime.getExposureMs())

            onPreviewStart: {
                captureService.previewExposureMs = previewControl.exposureTime.exposureMs
                captureService.enableCameraPreview(true, buttonBinning.binning)
            }
            onPreviewStop: {
                captureService.enableCameraPreview(false, buttonBinning.binning)
            }

            exposureTime.onExposureMsChanged: {
                changePreviewExposureMs.restart()
            }

            Behavior on opacity {NumberAnimation {duration: 500}}
        }
        //[[预览控制盘]]//

        //[[单独拍摄白光的选项]]//
        Rectangle {
            id: captureWhiteImageRect
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: parent.top
            anchors.topMargin: 20
            radius: 5
            color: "#A0A0A0"
            width: 300
            height: 80
            visible: opacity > 0
            opacity: 0

            Behavior on opacity { NumberAnimation { duration: 500 } }

            WzButton {
                id: buttonCaptureWhiteImage
                opacity: enabled ? 1 : 0.4
                width: 110
                height: 43
                radius: 3
                text: qsTr("确定拍摄")
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                anchors.rightMargin: 30
                normalColor: "#141414"
                label.font.pixelSize: 18
                label.anchors.fill: label.parent
                onClicked: {
                    PageCaptureController.captureWhiteImageConfirm(0)
                }
            }
            WzButton {
                id: buttonCaptureWhiteImageCancel
                opacity: enabled ? 1 : 0.4
                width: 110
                height: 43
                radius: 3
                text: qsTr("取消拍摄")
                anchors.verticalCenter: parent.verticalCenter
                anchors.left: parent.left
                anchors.leftMargin: 30
                normalColor: "#141414"
                label.font.pixelSize: 18
                label.anchors.fill: label.parent
                onClicked: {
                    captureWhiteImageRect.opacity = 0
                }
            }
        }
        Rectangle {
            id: previewStartingRect
            anchors.centerIn: parent
            width: 300
            height: 200
            visible: opacity > 0
            opacity: 0
            radius: 5
            color: "#A0A0A0"
            Behavior on opacity { NumberAnimation { duration: 500 } }
            Text {
                text: qsTr("请稍等, 正在开启预览")
                font.pixelSize: 18
                anchors.centerIn: parent
            }
        }
        Timer {
            id: tmrCaptureWhiteImage
            onTriggered: {
                previewStartingRect.opacity = 0
                PageCaptureController.captureWhiteImageConfirm(1)
            }
        }
        //[[单独拍摄白光的选项]]//

        //[[相机状态]]//
        Text {
            id: textCameraState
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            text: qsTr("请稍候")
            color: "#555555"
            font.pixelSize: 18
            visible: !isMini
        }
        //[[相机状态]]//

        //[[显示 图片页面]]//
        //[[显示 图片页面]]//
    }

    Rectangle {
        visible: !isMini
        id: rectangleRight
        color: "black"
        anchors.top: parent.top
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        width: 500

        MouseArea {
            anchors.fill: parent
            onClicked: {
                forceActiveFocus()
            }
        }

        /********************** Logo **********************/
        LogoNav {
            anchors.left: parent.left
            anchors.leftMargin: 20
            anchors.top: parent.top
            anchors.topMargin: 15
            anchors.right: parent.right
        }

        Rectangle {
            id: lineFluorescence
            height: 1
            visible: fluorescence.visible
            color: lineCapture.color
            anchors.left: lineCapture.left
            anchors.leftMargin: lineCapture.anchors.leftMargin
            anchors.right: buttonCaptureAE.right
            anchors.bottomMargin: 9
            anchors.bottom: fluorescence.top
        }

        // 拍摄方案
        CapturePlan {
            id: capturePlan
            anchors.left: parent.left
            anchors.leftMargin: 22
            anchors.bottom: lightControl.top
            anchors.bottomMargin: 19
            anchors.right: parent.right
            anchors.rightMargin: parent.width * 0.45
            height: 50
        }

        // 样品名
        Rectangle {
            id: rectSampleName
            anchors.left: capturePlan.right
            anchors.leftMargin: 5
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.verticalCenter: capturePlan.verticalCenter
            border.width: textInputSampleName.focus || textInputSampleName.hovered ? 2 : 1
            border.color: textInputSampleName.focus ? "#00ccff" : (textInputSampleName.hovered ? "#dddddd" : "#222222")
            color: "transparent"
            height: 38
            radius: 2

            TextInput {
                id: textInputSampleName
                clip: true
                width: parent.width - 14
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 1
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#838383"
                font.pixelSize: 18
                selectByMouse: true
                property string placeholderText: qsTr("样品名")
                property var hovered
                Text {
                    text: qsTr("此处可输入样品名")
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.left: parent.left
                    anchors.leftMargin: 10
                    visible: parent.text === "" && !parent.focus
                    color: "#222222"
                    font: parent.font
                }
            }
            MouseArea {
                anchors.fill: parent
                propagateComposedEvents: true
                cursorShape: Qt.IBeamCursor
                hoverEnabled: true
                onClicked: mouse.accepted = false
                onPressed: mouse.accepted = false
                onReleased: mouse.accepted = false
                onDoubleClicked: mouse.accepted = false
                onPositionChanged: mouse.accepted = false
                onPressAndHold: mouse.accepted = false
                onEntered: textInputSampleName.hovered = true
                onExited: textInputSampleName.hovered = false
            }
        }

        // 光源
        Light {
            id: lightControl
            enabled: mcuConnected
            height: 146
            anchors.bottomMargin: -2
            anchors.left: parent.left
            anchors.leftMargin: 1
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: fluorescence.visible ? lineFluorescence.top : lineApertureSlider.top
            buttonLightBluePenetrate2.visible: adminParams.bluePenetrateAlone
            buttonLightUVPenetrate.visible: !adminParams.hideUvPenetrate
            buttonLightUVPenetrateForce.visible: !adminParams.hideUvPenetrateForce
            onSwitchLight: {
                if (isOpen) {
                    fluorescence.closeAll()
                    var lt = WzUtils.lightTypeFromStr(lightType)
                    console.info("Light.onSwitchLight:", lt, isOpen)
                    if (lt === WzEnum.Light_UvReflex1 || lt === WzEnum.Light_UvReflex2) {
                        // 如果之前开了白光而且正在预览, 当切换成荧光光源/紫外反射的时候就需要保留一张白光图
                        if (mcu.latestLightOpened && mcu.latestLightType === "white_up" && isPreviewing) {
                            captureService.getMarkerImage()
                            var isCheckMarkerDark = "1" === dbService.readStrOption("isCheckMarkerDark", "0")
                            var checkMarkerDarkThreshold = parseInt(dbService.readStrOption("checkMarkerDarkThreshold", "100"))
                            if (isCheckMarkerDark && !captureService.hasMarkerImage(checkMarkerDarkThreshold)) {
                                msgBox.buttonCount = 1
                                msgBox.show(qsTr("自动保存Marker图时发生了意外, 请点击“确定”按钮后重新拍摄。"),
                                            qsTr("确定"))
                                return
                            } else {
                                capturePlan.noClearMarker = true
                            }
                        }
                        capturePlan.switchFluorescence(lightType)
                        capturePlan.noClearMarker = false
                    }
                }
                mcu.switchLight(lightType, isOpen)
                capturePlan.switchLight(lightType)
            }
        }

        WzButton {
            id: buttonCapture
            visible: !textBinning.visible
            anchors.right: buttonCaptureAE.left
            anchors.rightMargin: 2
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            width: 78
            height: 85
            label.font.pixelSize: 14
            label.anchors.verticalCenterOffset: 24
            text: qsTr("手动曝光")
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_capture.png"
            image.anchors.verticalCenterOffset: -12
            radius: 5
            enabled: root.cameraConnected;
            onClicked: {
                forceActiveFocus()
                captureService.isAutoExposure = false
                switchMultiFrameCapture.checked = false
                capture()
            }
        }

        WzButton {
            id: buttonCaptureAE
            visible: !textBinning.visible
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            width: 78
            height: 85
            label.font.pixelSize: 14
            label.anchors.verticalCenterOffset: 24
            text: qsTr("自动曝光")
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_capture.png"
            image.anchors.verticalCenterOffset: -12
            radius: 5
            enabled: root.cameraConnected;
            onClicked: {
                forceActiveFocus()
                captureService.isAutoExposure = true
                switchMultiFrameCapture.checked = false
                capture()
            }
        }

        WzButton {
            id: buttonCaptureSmall
            visible: textBinning.visible
            anchors.right: buttonCaptureAESmall.right
            anchors.bottom: buttonCaptureAESmall.top
            anchors.bottomMargin: 4
            width: 110
            height: 42
            label.font.pixelSize: 16
            label.anchors.horizontalCenterOffset: 15
            label.anchors.verticalCenterOffset: 1
            text: qsTr("手动曝光")
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_capture_small.png"
            image.anchors.horizontalCenterOffset: -33
            radius: 3
            enabled: root.cameraConnected
            Component.onCompleted: clicked.connect(buttonCapture.clicked)
        }

        WzButton {
            id: buttonCaptureAESmall
            visible: textBinning.visible
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            width: 110
            height: 42
            label.font.pixelSize: 16
            label.anchors.horizontalCenterOffset: 15
            label.anchors.verticalCenterOffset: 1
            text: qsTr("自动曝光")
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_capture_small.png"
            image.anchors.horizontalCenterOffset: -33
            radius: 3
            enabled: root.cameraConnected;
            Component.onCompleted: clicked.connect(buttonCaptureAE.clicked)
        }

        Text {
            id: textMultiFrameCapture
            font.pixelSize: 18
            text: qsTr("多帧拍摄")
            color: "#828282"
            visible: false // 隐藏不用, 2020-5-15 by wangzhe
            anchors.leftMargin: 17
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 63
            property bool hovered

            MouseArea {
                anchors.fill: parent
                hoverEnabled: true
                ToolTip.delay: 200
                ToolTip.timeout: 2000
                ToolTip.visible: textMultiFrameCapture.hovered
                ToolTip.text: qsTr("点击此处进行设置")
                onEntered: parent.hovered = true
                onExited: parent.hovered = false
                onClicked: {
                    multiCapture.show()
                    rectShade.opacity = 0.7
                    rectShade.color = "#000000"
                }
            }
        }
        Rectangle {
            x: 12
            anchors.fill: textMultiFrameCapture
            anchors.margins: -3
            color: "transparent"
            border.width: 1
            border.color: "#222222"
            visible: textMultiFrameCapture.hovered
        }

        WzSwitch {
            id: switchMultiFrameCapture
            visible: textMultiFrameCapture.visible
            checked: false
            anchors.horizontalCenter: textMultiFrameCapture.horizontalCenter
            width: 66
            anchors.verticalCenter: buttonBinning.verticalCenter
            anchors.verticalCenterOffset: 3
            property bool showOptionsOnChecked: true
            onCheckedChanged: {
                /* 2020-5-19 注释
                if (showOptionsOnChecked) {
                    if (checked) {
                        multiCapture.show()
                        rectShade.opacity = 0.7
                        rectShade.color = "#00000"
                        textExposureTimeAutoCheckBox.isChecked = false
                    } else {
                        multiCapture.hide()
                        rectShade.opacity = 0
                    }
                }
                */
            }
            Component.onCompleted: {
                //checked = dbService.readIntOption("is_multi_capture", 0)
            }
            Component.onDestruction: {
                //dbService.saveIntOption("is_multi_capture", checked)
            }
        }

        Text {
            id: textPreviewSwitch
            font.pixelSize: 18
            text: qsTr("预览开关")
            color: "#828282"
            visible: true
            anchors.leftMargin: 17
            anchors.left: parent.left
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 63
        }
        WzSwitch {
            id: switchPreview
            visible: textPreviewSwitch.visible
            checked: false
            anchors.horizontalCenter: textPreviewSwitch.horizontalCenter
            width: 66
            anchors.verticalCenter: switchMultiFrameCapture.verticalCenter
            anchors.verticalCenterOffset: -1
            property bool noStopPreview: false
            onCheckedChanged: {
                console.info("switchPreview.onCheckedChanged, planName:", capturePlan.planName, ", checked:", checked)

                if (!checked && noStopPreview)
                    return

                PageCaptureController.enablePreview(checked)
            }
        }

        Text {
            id: textBinning
            font.pixelSize: 18
            text: qsTr("像素合并")
            color: "#828282"
            visible: adminParams.binningVisible
            anchors.leftMargin: {
                switch (WzI18N.language) {
                case "zh": return 10
                case "en": return 18
                }
            }
            anchors.left: textPreviewSwitch.right
            anchors.bottom: textPreviewSwitch.bottom
        }

        WzButton {
            id: buttonBinning
            visible: textBinning.visible
            anchors.horizontalCenter: textBinning.horizontalCenter
            anchors.bottom: buttonCapture.bottom
            width: 63
            height: 32
            radius: 3
            text: "1x1"
            label.font.pixelSize: 18
            label.font.family: "Arial"
            label.horizontalAlignment: Text.AlignLeft
            label.anchors.leftMargin: 5
            label.anchors.fill: label.parent
            imageVisible: true
            imageAlign: Qt.AlignRight | Qt.AlignVCenter
            imageSourceNormal: "qrc:/images/combox_arrow_down_gray.png"
            image.sourceSize.height: 9
            image.sourceSize.width: 15
            image.anchors.rightMargin: 7
            image.anchors.verticalCenterOffset: 2
            property int binning: 1
            onBinningChanged: {
                text = binning + "x" + binning
                capturePlan.setBinning(binning)
                pageOption.binning = binning
                if(!isMini)
                {
                   switchPreview.checked=false
                   capturePlan.comboBoxCapturePlan.enabled=false
                   switchBinningTimer.running=true
                }

            }

            onClicked: {
                menuBinning.popup(0, -menuBinning.height - 10)
            }

            WzMenu {
                id: menuBinning
                width: parent.width * 1.3
                indicatorWidth: 8
                font.pixelSize: 18
                font.family: "Arial"

                Action {text: "1x1"; onTriggered: {buttonBinning.binning = 1}}
                WzMenuSeparator {width: parent.width; color: "#525252"}
                Action {text: "2x2"; onTriggered: {buttonBinning.binning = 2}}
                WzMenuSeparator {width: parent.width; color: "#525252"}
                Action {text: "3x3"; onTriggered: {buttonBinning.binning = 3}}
                WzMenuSeparator {width: parent.width; color: "#525252"}
                Action {text: "4x4"; onTriggered: {buttonBinning.binning = 4}}
            }
        }

        Text {
            id: textExposureTime
            font.pixelSize: 18
            text: qsTr("曝光时间")
            color: "#828282"
            anchors.leftMargin: {
                switch (WzI18N.language) {
                case "zh": return textBinning.visible ? 10 : 20
                case "en": return textBinning.visible ? 15 : 25
                }
            }
            anchors.left: textBinning.visible ? textBinning.right : textPreviewSwitch.right
            anchors.bottom: textPreviewSwitch.bottom
            MouseArea {
                anchors.fill: parent
                onClicked: menuExposureTimePlans.popup(0, menuExposureTimePlans.height*-1)
            }
            WzMenu {
                id: menuExposureTimePlans
                width: {
                    switch(WzI18N.language) {
                    case "zh": return 90
                    case "en": return 110
                    }
                }
                indicatorWidth: 8
                font.pixelSize: 15
                font.family: WzI18N.font.family

                Action {text: qsTr("10分"); onTriggered: {exposureTime.setExposureMs(10*60*1000)}}
                WzMenuSeparator {width: parent.width; color: "#525252"}
                Action {text: qsTr("5分"); onTriggered: {exposureTime.setExposureMs(5*60*1000)}}
                WzMenuSeparator {width: parent.width; color: "#525252"}
                Action {text: qsTr("3分"); onTriggered: {exposureTime.setExposureMs(3*60*1000)}}
                WzMenuSeparator {width: parent.width; color: "#525252"}
                Action {text: qsTr("1分"); onTriggered: {exposureTime.setExposureMs(1*60*1000)}}
                WzMenuSeparator {width: parent.width; color: "#525252"}
                Action {text: qsTr("30秒"); onTriggered: {exposureTime.setExposureMs(30*1000)}}
                WzMenuSeparator {width: parent.width; color: "#525252"}
                Action {text: qsTr("10秒"); onTriggered: {exposureTime.setExposureMs(10*1000)}}
                WzMenuSeparator {width: parent.width; color: "#525252"}
                Action {text: qsTr("5秒"); onTriggered: {exposureTime.setExposureMs(5*1000)}}
            }
        }

        WzExposureTime {
            id: exposureTime
            anchors.left: textExposureTime.left
            anchors.leftMargin: -4
            anchors.bottom: buttonCapture.bottom
            anchors.bottomMargin: -9
            isShowMinute: !WzUtils.isGelCapture()
            maxSecond: !WzUtils.isGelCapture() ? 59 : 10
            property bool noSaveExposureMs: false
            onExposureMsChanged: {
                if (noSaveExposureMs)
                    return
                if (capturePlan.planName === "chemi") {
                    adminParams.chemi.exposureMs = exposureTime.exposureMs
                    dbService.saveValueToDisk("chemi/exposureMs", adminParams.chemi.exposureMs)
                    return
                } else if (capturePlan.planName === "rna")
                {
                    adminParams.rna.exposureMs = exposureTime.exposureMs
                    dbService.saveValueToDisk("rna/exposureMs", adminParams.rna.exposureMs)
                }
                else if (capturePlan.planName === "protein")
                {
                    adminParams.protein.exposureMs = exposureTime.exposureMs
                    dbService.saveValueToDisk("protein/exposureMs", adminParams.protein.exposureMs)
                }
                else if (capturePlan.planName === "fluor") {
                    if (mcu.latestLightType === "red")
                    {
                        adminParams.red.exposureMs = exposureTime.exposureMs
                        dbService.saveValueToDisk("red/exposureMs", adminParams.red.exposureMs)
                    }
                    else if (mcu.latestLightType === "green")
                    {
                        adminParams.green.exposureMs = exposureTime.exposureMs
                        dbService.saveValueToDisk("green/exposureMs", adminParams.green.exposureMs)
                    }
                    else if (mcu.latestLightType === "blue")
                    {
                        adminParams.blue.exposureMs = exposureTime.exposureMs
                        dbService.saveValueToDisk("blue/exposureMs", adminParams.blue.exposureMs)
                    }
                }

                changePreviewExposureMs.restart()
            }
        }

        Rectangle {
            id: lineCapture
            anchors.left: textMultiFrameCapture.left
            anchors.leftMargin: 2
            anchors.right: buttonCaptureAE.right
            anchors.bottom: buttonCapture.top
            anchors.bottomMargin: 18
            height: 1
            color: "#161616"
        }

        //[[镜头]]//
        FocusNearButton {id: buttonFocusNear}
        FocusFarButton {id: buttonFocusFar}

        ApertureSlider {
            id: apertureSlider
            height: 70
            visible: false // 隐藏不用, 2020-5-15 by wangzhe
            anchors.bottom: buttonFocusNear.bottom
            anchors.bottomMargin: WzUtils.isGelCapture() ? 15 : 34
            anchors.left: lineApertureSlider.left
            anchors.right: buttonFocusNear.right
            anchors.rightMargin: 0
            enabled: mcuConnected
            onSwitchAperture: mcu.switchAperture(value)
        }

        Text {
            id: textLens
            text: qsTr("电动镜头")
            font.pixelSize: 18
            color: "#707070"
            anchors.left: lineApertureSlider.left
            anchors.leftMargin: 0
            anchors.bottom: buttonFocusFar.top
            anchors.bottomMargin: 13

            LensPopupMenu {
                id: lensPopupMenu
            }

            MouseArea {
                anchors.fill: parent
                visible: captureService.isSecondChemi
                onClicked: lensPopupMenu.open(mouse.x, mouse.y)
            }
        }

        // 自动聚焦按钮
        WzButton {
            id: buttonFocusAuto
            enabled: mcuConnected && cameraConnected       
            visible: captureService.isSecondChemi
            opacity: enabled ? 1 : 0.4
            width: 20
            height: 20
            anchors.left: textLens.right
            anchors.leftMargin: 5
            anchors.verticalCenter: textLens.verticalCenter
            radius: 3
            normalColor: "transparent"
            imageVisible: true
            imageSourceNormal: "qrc:/images/focus.svg"
            image.sourceSize.width: 16
            image.sourceSize.height: 16

            ToolTip.delay: 1000
            ToolTip.timeout: 5000
            ToolTip.visible: buttonFocusAuto.state === "entered"
            ToolTip.text: qsTr("开始自动聚焦")

            onClicked: {
                if (isAutoFocusing) return
                if (!switchPreview.checked) {
                    popupMsg.showMsg(qsTr("请先打开预览"))
                    return
                }
                //isAutoFocusSelectRect = !isAutoFocusSelectRect
                //if (isAutoFocusSelectRect)
                //    exposureArea.visible = false                
                if (buttonBinning.binning > 2)
                    focus2.waitDiffMs = 200
                else
                    focus2.waitDiffMs = 500
                focus2.autoFocus()
                root.isAutoFocusing = true
            }
        }
        //[[镜头]]//

        //[[多帧拍摄入口]]//
        Text {
            id: textMultiFrameCapture2
            text: qsTr("多帧拍摄")
            font.pixelSize: 18
            color: "#707070"
            anchors.left: buttonMultiFrameCaptureSetting.left
            anchors.leftMargin: 0
            anchors.bottom: buttonFocusFar.top
            anchors.bottomMargin: 13
        }
        WzButton {
            id: buttonMultiFrameCaptureSetting
            opacity: enabled ? 1 : 0.4
            width: {
                switch(WzI18N.language) {
                case "zh": return 190
                case "en": return 150
                }
            }
            height: 48
            radius: 3
            text: qsTr("多帧拍摄设置")
            anchors.bottom: lineCapture.top
            anchors.bottomMargin: 18
            anchors.right: lineCapture.right
            anchors.rightMargin: 0
            normalColor: "#141414"
            label.font.pixelSize: buttonFocusNear.label.font.pixelSize
            label.horizontalAlignment: Text.AlignRight
            label.anchors.rightMargin: {
                switch(WzI18N.language) {
                case "zh": return 22
                case "en": return 30
                }
            }
            label.anchors.fill: label.parent
            imageVisible: true
            imageAlign: Qt.AlignLeft | Qt.AlignVCenter
            imageSourceNormal: "qrc:/images/button_multi_frame.png"
            image.anchors.leftMargin: {
                switch(WzI18N.language) {
                case "zh": return 20
                case "en": return 30
                }
            }
            image.anchors.verticalCenterOffset: 0
            onClicked: {
                multiCapture.show()
            }
        }

        //[[多帧拍摄入口]]//

        Rectangle {
            id: lineApertureSlider
            anchors.left: lineCapture.left
            anchors.leftMargin: lineCapture.anchors.leftMargin
            anchors.right: buttonCaptureAE.right
            anchors.bottom: textLens.top
            anchors.bottomMargin: 13
            height: 1
            color: lineCapture.color
        }

        //[[荧光和滤镜轮]]//
        Fluorescence {
            id: fluorescence
            enabled: mcuConnected
            visible: !WzUtils.isGelCapture()
            anchors.left: textMultiFrameCapture.left
            anchors.right: parent.right
            anchors.bottom: lineApertureSlider.top
            anchors.bottomMargin: 0
            height: 152
            anchors.leftMargin: -15
            anchors.rightMargin: 20
            onSwitchLight: {
                if (isOpen) {
                    // 如果之前开了白光而且正在预览, 当切换成荧光光源的时候就需要保留一张白光图
                    if (mcu.latestLightOpened && mcu.latestLightType === "white_up" && isPreviewing) {
                        captureService.getMarkerImage();
                        var isCheckMarkerDark = "1" === dbService.readStrOption("isCheckMarkerDark", "0")
                        var checkMarkerDarkThreshold = parseInt(dbService.readStrOption("checkMarkerDarkThreshold", "100"))
                        if (isCheckMarkerDark && !captureService.hasMarkerImage(checkMarkerDarkThreshold)) {
                            msgBox.buttonCount = 1
                            msgBox.show(qsTr("自动保存Marker图时发生了意外, 请点击“确定”按钮后重新拍摄。"),
                                        qsTr("确定"))
                            return
                        } else {
                            capturePlan.noClearMarker = true
                        }
                    }
                    lightControl.closeAll()
                    mcu.switchLight(lightType, true)
                    capturePlan.switchFluorescence(lightType)
                    capturePlan.noClearMarker = false
                } else {
                    mcu.closeAllLight()
                }
            }
            onSwitchFilter: {
                mcu.switchFilterWheel(filterIndex)
            }
        }

        Rectangle {
            id: lineLight
            visible: fluorescence.visible
            height: 1
            color: lineCapture.color
            anchors.left: lineCapture.left
            anchors.leftMargin: lineCapture.anchors.leftMargin
            anchors.right: lineCapture.right
            anchors.bottom: lightControl.top
            anchors.bottomMargin: 10
        }
        //[[荧光和滤镜轮]]//


        Rectangle {
            id: rectAdjustHardwareShade
            anchors.top: capturePlan.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            color: "black"
            opacity: capturePlan.comboBoxCapturePlan.enabled ? 0 : 0.7
            visible: opacity > 0
            z: 3
            MouseArea {
                anchors.fill: parent
                propagateComposedEvents: true
                hoverEnabled: true
            }
            Behavior on opacity { NumberAnimation { duration: 500 }}
        }

        Rectangle {
            id: rectAdjustHardware
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 100
            width: 300
            height: 200
            opacity: capturePlan.comboBoxCapturePlan.enabled ? 0 : 1
            visible: opacity > 0
            color: "#151515"
            border.color: "#313131"
            z: 4

            WzBusyIndicator {
                id: busyAdjustHardware
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -30
                anchors.horizontalCenter: parent.horizontalCenter
                width: 70
                height: 70
                color: "#535353"
            }

            Text {
                id: textAdjustHardware
                text: qsTr("正在调整硬件")
                anchors.top: busyAdjustHardware.bottom
                anchors.topMargin: 25
                anchors.horizontalCenter: parent.horizontalCenter
                color: "#535353"
                font.pixelSize: 20
            }
            Behavior on opacity { NumberAnimation { duration: 500 }}
        }
    }

    PreviewImageControl {
        anchors.bottom: previewExposure.top
        anchors.bottomMargin: 5
        anchors.right: previewExposure.right
        opacity: previewExposure.opacity
        visible: previewExposure.visible && previewImageControlEnabled
        z: 2
    }

    PreviewExposure {
        id: previewExposure
        anchors.right: rectangleRight.left
        anchors.rightMargin: PageCaptureController.previewExposureIsVisible() ? 5 : -120
        anchors.top: rectangleRight.top
        anchors.topMargin: textPreviewSwitch.y - 10
        opacity: PageCaptureController.previewExposureIsVisible() ? 0.9 : 0
        z: 2
        exposureTime.onValueChanged: {
            if (PageCaptureController.capturePlanIsChemiFluor()) {
                if (PageCaptureController.capturePlanIsChemi())
                {
                    adminParams.chemi.previewExposureMs = previewExposure.exposureTime.value
                    dbService.saveValueToDisk("chemi/previewExposureMs", previewExposure.exposureTime.value)
                }
                else if (PageCaptureController.capturePlanIsFluor())
                    adminParams.fluorPreviewExposureMs = previewExposure.exposureTime.value
                    dbService.saveValueToDisk("common/fluorPreviewExposureMs", previewExposure.exposureTime.value)
                captureService.previewExposureMs = previewExposure.exposureTime.value
                changePreviewExposureMs.restart()
            }
        }
        MouseArea {
            anchors.fill: previewExposure.textExposureTime
            // 双击时把预览时间同步到拍摄时间
            onDoubleClicked: {
                exposureTime.exposureMs = previewExposure.exposureTime.value
            }
        }
    }
    // Mini 预览曝光时间控制和相机状态显示
    Loader {
        id: loaderMiniPreviewControl
        asynchronous: true
        source: "qrc:/Components/Mini/PreviewControl.qml"
        anchors.horizontalCenter: rectangleLeft.horizontalCenter
        anchors.bottom: rectangleLeft.bottom
        anchors.bottomMargin: 5
        visible: false
        z: 2
    }

    Rectangle {
        id: rectShade
        anchors.fill: parent
        color: "#000000"
        visible: opacity > 0
        opacity: 0
        z: 10
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            preventStealing: true
        }
        Behavior on opacity {NumberAnimation {duration: 300}}
    }

    //[[进度条]]//
    Rectangle {
        id: rectangleCaptureProgress
        anchors.left: captureProgress.left
        anchors.leftMargin: -50
        anchors.right: captureProgress.right
        anchors.rightMargin: -50
        anchors.top: captureProgress.top
        anchors.topMargin: -40
        anchors.bottom: captureProgress.bottom
        anchors.bottomMargin: -30
        radius: 7
        color: isMini ? "white" : "#a0a0a0"
        opacity: isMini ? 0.2 : 0.9
        visible: captureProgress.visible
    }
    Rectangle {
        anchors.fill: rectangleCaptureProgress
        anchors.margins: 1
        visible: isMini && rectangleCaptureProgress.visible
        border.color: "#606060"
        border.width: 1
        color: "transparent"
        radius: rectangleCaptureProgress.radius
    }
    CaptureProgress {
        id: captureProgress
        anchors.verticalCenter: rectangleLeft.verticalCenter
        anchors.horizontalCenter: rectangleLeft.horizontalCenter
        z: 20
        visible: false
        fontColor: isMini ? "#dddddd" : "111111"

        onCancel: {
            cancelCapture()
        }
    }
    function cancelCapture() {
        captureService.abortCapture()
        captureProgress.isProgress = false
        captureProgress.text = qsTr("正在取消")
        captureProgress.buttonCancel2.visible = false
    }
    WzImageButton {
        id: buttonGoToImagePage
        visible: captureProgress.visible
        z: captureProgress.z + 1
        width: 27
        height: 22
        image.source: "qrc:/images/image2_normal.png"
        image.sourceSize.height: 22
        image.sourceSize.width: 27
        imageHot.source: "qrc:/images/image2_hot.png"
        imageHot.sourceSize.height: 22
        imageHot.sourceSize.width: 27
        imageDown.source: "qrc:/images/thumbnail_image_down.png"
        imageDown.sourceSize.height: 22
        imageDown.sourceSize.width: 27
        anchors.right: rectangleCaptureProgress.right
        anchors.rightMargin: 8
        anchors.bottom: rectangleCaptureProgress.bottom
        anchors.bottomMargin: 8
        onClicked: pageChange(1)
    }
    //[[进度条]]//

    //[[多帧拍摄]]//
    MultiCapture {
        id: multiCapture
        width: 450
        height: 10
        visible: opacity > 0
        opacity: 0
        anchors.right: rectangleRight.right
        anchors.rightMargin: 26
        anchors.bottom: rectangleRight.bottom
        anchors.bottomMargin: rectangleRight.height - buttonMultiFrameCaptureSetting.y + 20
        Component.onCompleted: {
            var params = JSON.parse(dbService.readStrOption("multi_capture_params", "{}"))
            multiCapture.setParams(params)
        }
        onMultiCapture: {
            captureService.isAutoExposure = false
            switchMultiFrameCapture.checked = true
            root.capture()
        }
    }
    //[[多帧拍摄]]//

    Focus2 {
        id: focus2
        anchors {
            bottom: parent.bottom
            bottomMargin: 20
            left: parent.left
            leftMargin: 20
        }
        visible: false
        onMcuCmd: {
            mcu.exec("send_data," + mcu.getOpenId() + "," + cmd)
        }
        onRequestImageDiff: {
            setImageDiff(captureService.getPreviewImageDiff())
        }
        onAfDone: isAutoFocusing = false
        onTimeout: {
            isAutoFocusing = false
            popupMsg.showMsg(msg)
        }
        onStop: {
            root.isAutoFocusing = false
        }
    }

    function captureWithoutPreviewingClicked(buttonIndex) {
        console.info("captureWithoutPreviewingClicked")
        if (buttonIndex === 2) {
            console.info("\tcapture")
            captureWithoutMarker = true
            capture(captureParamsObject.captureParams)
        } else {
            console.info("\topen preview")
            if (!switchPreview.checked)
                switchPreview.checked = true
            if (capturePlan.isPlan(WzEnum.CaptureMode_Fluor)) {
                mcu.switchLight(WzUtils.lightTypeStr(WzEnum.Light_WhiteUp), true)
            }
        }
        msgBox.buttonClicked.disconnect(captureWithoutPreviewingClicked)
    }

    QtObject {
        id: captureParamsObject
        property var captureParams
    }
    QtObject {
        id: privateObject
        property bool isStartPreviewForChangeBinning: false
        property bool isStartPreviewAfterCaptured: false
    }

    function askCaptureMarker() {
        msgBox.buttonCount = 2
        msgBox.buttonClicked.connect(captureWithoutPreviewingClicked)
        msgBox.show(qsTr("当前没有开启预览, 这样在拍摄后无法叠加 Marker 图片, 请在下面作出选择:"),
                    qsTr("开启预览"), qsTr("确定拍摄"))
    }

    function capture(captureParams) {
        captureParamsObject.captureParams = captureParams
        console.log("capture function")

        if (undefined === captureParams) {
            captureParams = {}
            console.info("\tparams === undefined")
        }

        var isCheckMarkerDark = "1" === dbService.readStrOption("isCheckMarkerDark", "0")
        var checkMarkerDarkThreshold = parseInt(dbService.readStrOption("checkMarkerDarkThreshold", "100"))

        if (PageCaptureController.capturePlanIsChemi()) {
                console.log("\tis chemi")

            if (captureWithoutMarker) {

            } else if (isPreviewing) {
                // 需要在关闭光源之前把预览图备份出来, 否则可能在关灯后仍然进行短时间的预览并使其覆盖掉正常预览图
                captureService.getMarkerImage()
                if (isCheckMarkerDark && !captureService.hasMarkerImage(checkMarkerDarkThreshold)) {
                    msgBox.buttonCount = 1
                    msgBox.show(qsTr("自动保存Marker图时发生了意外, 请点击“确定”按钮后重新拍摄。"),
                                qsTr("确定"))
                    return
                }
            } else if (isCheckMarkerDark && captureService.hasMarkerImage(checkMarkerDarkThreshold)) {

            } else {
                askCaptureMarker()
                return
            }
        } else if (PageCaptureController.capturePlanIsFluor()) {
            console.log("is fluor")
            if (!isCheckMarkerDark)
                checkMarkerDarkThreshold = -1
            if (captureWithoutMarker) {

            }
            // 如果没有选择检查marker是否过暗也要进行此分支, 否则会进行 else 里的分支提示没有拍摄白光图
            // 前面的代码如果判断不检测marker过暗则将阈值设置为-1，这样hasMarkerImage函数中的逻辑就不会
            // 检查marker是否过暗
            else if (captureService.hasMarkerImage(checkMarkerDarkThreshold)) {

            } else {
                askCaptureMarker()
                return
            }
        }

        // 2022-2-25
        // 如果开启了选项: 拍摄后自动开启预览, 且拍摄前也正在预览, 就做一个标记, 等
        // 拍完之后检查这个标记, 如果需要就自动打开预览
        privateObject.isStartPreviewAfterCaptured = false
        var autoStartPreviewAfterCaptured = dbService.readBoolOption("autoStartPreviewAfterCaptured", false)
        if (autoStartPreviewAfterCaptured === true) {
            if (isPreviewing) {
                privateObject.isStartPreviewAfterCaptured = true
            }
        }

        mcu.repeatSetFilter = false
        captureWithoutMarker = false
        captureService.setCameraParam("readoutFrequency", 0)
        if (mcu.latestLightOpened) {
            if (mcu.latestLightType === "white_up") {
                mcu.closeAllLight()
                exposureTime.noSaveExposureMs = true
                switchPreview.noStopPreview = true
                switchPreview.checked = false
                switchPreview.noStopPreview = false
                exposureTime.noSaveExposureMs = false
            } else if (mcu.latestLightType === "uv_penetrate" || mcu.latestLightType === "uv_penetrate_force" ||
                     mcu.latestLightType === "white_down") {
                switchMultiFrameCapture.checked = false
                captureService.setCameraParam("readoutFrequency", 25)
            } else if (mcu.latestLightType === "red" || mcu.latestLightType === "green" || mcu.latestLightType === "blue" ||
                       mcu.latestLightType === "uv_reflex1" || mcu.latestLightType === "uv_reflex2") {
                switchMultiFrameCapture.checked = false
                captureService.setCameraParam("readoutFrequency", 25)
            }
        }
        if (isMini) {
            liveImageView.visible = false
            rectShade.color = "black"
            rectShade.opacity = 0.6
        } else {
            rectShade.color = "#262626"
            rectShade.opacity = 0.7
        }
        captureProgress.progress = 0
        captureProgress.visible = true
        captureService.imagePath = dbService.readStrOption("image_path", "");

        // 向前兼容代码
        if (captureParams.multiCapture === undefined) {
            console.info("\tmultiCapture === undefined")
            if (switchMultiFrameCapture.checked) {
                console.info("\t\tswitchMultiFrameCapture is true")
                captureParams.multiCapture = true
                captureParams.multiCaptureParams = multiCapture.params()
                captureParams.multiCaptureParams.sampleName = textInputSampleName.text
                captureParams.multiCaptureParams.exposureMs = exposureTime.getExposureMs()
                captureParams.multiCaptureParams.binning = buttonBinning.binning
                captureParams.multiCaptureParams.grayAccumulateAddExposure = adminParams.grayAccumulateAddExposure
                captureParams.multiCaptureParams.removeFluorCircle = isRemoveFluorCircle
            } else {
                console.info("\t\tswitchMultiFrameCapture is false")
            }
        }

        if (captureParams.multiCapture) {
            console.info("\tmultiCapture is true")
            captureService.captureMulti(captureParams.multiCaptureParams)
        } else {
            console.info("\tmultiCapture is false")

            // 向前兼容代码
            if (captureParams.singleCaptureParams === undefined) {
                console.info("\t\singleCaptureParams is undefined")
                captureParams.singleCaptureParams = {
                    sampleName: textInputSampleName.text,
                    binning: buttonBinning.binning,
                    exposureMs: exposureTime.getExposureMs(),
                    openedLightType: mcu.latestLightType,
                    isLightOpened: mcu.latestLightOpened,
                    grayAccumulate: false,
                    removeFluorCircle: isRemoveFluorCircle
                }
            }

            captureService.capture(captureParams.singleCaptureParams)
        }
    }


}


/*##^##
Designer {
    D{i:0;autoSize:true;height:800;width:1280}D{i:7;anchors_width:497}
}
##^##*/
