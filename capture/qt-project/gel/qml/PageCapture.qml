import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

import WzCapture 1.0
import WzUtils 1.0
import WzI18N 1.0
import "WzControls"
import "WzControls.2"
import "Components"

Item {
    id: root

    signal newImage(string imageFile)
    signal pageChange(int pageIndex)

    property bool mcuConnected: mcu.connected
    property bool cameraConnected: false
    property bool isPreviewing: false
    property bool confirmCaptureWithoutPreviewing: false
    property bool isAutoFocusSelectRect: false
    property bool isAutoFocusing: false

    property alias captureProgress: captureProgress
    property var adminParams

    WzMcu {
        id: mcu
        onLightSwitched: {
            if (!isOpened)
                lightControl.closeAll()
            else {
                if (lightType === "white_up" || lightType === "white_down") {
                    switchToFilter0Timer.running = true
                } else if (lightType === "uv_penetrate") {
                    fluorescence.filter4.active = true
                }
            }
        }
        onDevicesChanged: {
            console.debug("device available count:", availableCount)
            console.debug(devices)
            if (availableCount === 0) {
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
                // 2020-12-10 暂时屏蔽记忆上次打开过的设备这个功能 if ("" !== previousOpenedSn) {
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
        }
        onError: {
            msgBox.show(errorMsg, qsTr("确定"))
        }
    }
    Timer {
        id: switchToFilter0Timer
        repeat: false
        running: false
        interval: 800
        onTriggered: {
            mcu.switchFilterWheel(0)
            fluorescence.filter1.active = true
        }
    }
    Timer {
        id: getTemperatureTimer
        interval: 1000
        repeat: true
        onTriggered: {
            previewControl.cameraTemperature = captureService.getCameraTemperature();
        }
    }

    Timer {
        id: changePreviewExposureMs
        interval: 500
        repeat: false
        running: false
        onTriggered: {
            if (WzUtils.isGelCapture())
                captureService.previewExposureMs = exposureTime.exposureMs
            else
                captureService.previewExposureMs = previewControl.exposureTime.exposureMs
        }
    }

    WzCaptureService {
        id: captureService
        isAutoExposure: checkBoxExposureTimeAuto.checked
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
                break;
            case WzCaptureService.AutoExposureFinished:
                exposureTime.exposureMs = captureService.autoExposureMs
                break;
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
                }
                pageChange(1)
                if (mcu.latestLightOpened &&
                        (mcu.latestLightType === "uv_penetrate" || mcu.latestLightType === "uv_penetrate_force")) {
                    mcu.closeAllLight()
                }

                break
            }
        }
        onCameraStateChanged: {
            switch(state) {
            case WzCameraState.Connecting:
                previewControl.cameraState = "cameraConnecting"
                break;
            case WzCameraState.Connected:
            case WzCameraState.CaptureFinished:
                cameraConnected = true
                previewControl.cameraState = "cameraConnected"
                getTemperatureTimer.running = true
                break;
            case WzCameraState.Disconnected:
                cameraConnected = false
                getTemperatureTimer.running = false
                previewControl.cameraState = "cameraDisconnected"
                break;
            case WzCameraState.PreviewStarted:
                isPreviewing = true
                previewControl.previewState = "cameraPreviewStarted"
                if (!mcu.latestLightOpened) {
                    mcu.switchLight("white_up", true)
                    lightControl.buttonLightWhiteUp.checked = true
                }
                break
            case WzCameraState.PreviewStopped:
                isPreviewing = false
                previewControl.previewState = "cameraPreviewStopped"
                liveImageView.clearImage()
                switchPreview.checked = false
                if (mcu.latestLightOpened && mcu.latestLightType === "white_up") {
                    if (dbService.readIntOption("noCloseWhiteUpWhenCapture", 0)) {

                    } else {
                        mcu.closeAllLight()
                    }
                }
                break
            case WzCameraState.CameraNotFound:
                previewControl.cameraState = "cameraNotFound"
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
        onActiveCameraManufacturerChanged: {
            if (activeCameraManufacturer === "Toup") {

            } else {
                buttonFocusAuto.visible = true
            }
        }
    }

    Component.onCompleted: {
        captureService.connectCamera()
        mcu.init()
        adminParams = dbService.readAdminSetting()
    }
    Connections {
        target: window
        onClosing: {
            mcu.closeAllLight()
            mcu.uninit()
        }
    }

    Rectangle {
        id: rectangleLeft
        color: "#cccccc"
        anchors.left: parent.left
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.right: rectangleRight.left

        //[[实时画面显示控件]]//
        LiveImageView {
            id: liveImageView
            width: parent.width
            height: parent.height
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
                    onClicked: captureService.stopAutoFocus()
                }
            }
            Rectangle {
                id: selectRect
                color: "transparent"
                border.color: "red"
                visible: false
            }
            MouseArea {
                anchors.fill: parent
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
                        selectRect.x = mouseX - 1
                        selectRect.y = mouseY - 1
                        selectRect.width = 1
                        selectRect.height = 1
                        selectRect.visible = true
                    }
                }
                onReleased: {
                    if (isAutoFocusSelectRect) {
                        isAutoFocusSelectRect = false
                        isAutoFocusing = true
                        selectRect.visible = false
                        liveImageView.selectRect(selectRect.x, selectRect.x + selectRect.width,
                                                 selectRect.y, selectRect.y + selectRect.height)
                        captureService.startAutoFocus()
                    }
                }
                onPositionChanged: {
                    if (pressed) {
                        var x1 = Math.min(pressedX, mouseX)
                        var x2 = Math.max(pressedX, mouseX)
                        var y1 = Math.min(pressedY, mouseY)
                        var y2 = Math.max(pressedY, mouseY)
                        selectRect.x = x1
                        selectRect.y = y1
                        selectRect.width = x2 - x1
                        selectRect.height = y2 - y1
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
            visible: !WzUtils.isGelCapture()

            Component.onCompleted: previewControl.exposureTime.setExposureMs(dbService.readIntOption("preview_exposure_ms", 10))
            Component.onDestruction: dbService.saveIntOption("preview_exposure_ms", previewControl.exposureTime.getExposureMs())

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

        //[[显示 图片页面]]//
        //[[显示 图片页面]]//
    }

    Rectangle {
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
            anchors.right: buttonCapture.right
            anchors.bottomMargin: 19
            anchors.bottom: fluorescence.top
        }

        // 样品名
        Rectangle {
            id: rectSampleName
            anchors.left: parent.left
            anchors.leftMargin: 22
            anchors.bottom: lightControl.top
            anchors.bottomMargin: 19
            anchors.right: parent.right
            anchors.rightMargin: 20
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
            anchors.bottomMargin: 0
            anchors.left: parent.left
            anchors.leftMargin: 1
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: fluorescence.visible ? lineFluorescence.top : lineApertureSlider.top
            onSwitchLight: {
                if (isOpen)
                    fluorescence.closeAll()
                mcu.switchLight(lightType, isOpen)
            }
        }

        WzButton {
            id: buttonCapture
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            width: 92
            height: 99
            label.font.pixelSize: 20
            label.anchors.verticalCenterOffset: 26
            text: qsTr("拍摄")
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_capture.png"
            image.anchors.verticalCenterOffset: -16
            radius: 5
            enabled: root.cameraConnected

            // 在按钮右上角画一个三角
            Canvas {
                id: multiCaptureCanvas
                property bool hovered: false
                property bool pressed: false
                anchors.right: parent.right
                anchors.rightMargin: 3
                anchors.top: parent.top
                anchors.topMargin: 3
                width: 20
                height: 20
                onPaint: {
                    var ctx = getContext("2d")
                    ctx.fillStyle = pressed ? "#444444" : (hovered ? "#333333" : "#111111")
                    ctx.moveTo(1, 1)
                    ctx.lineTo(width - 1, 1)
                    ctx.lineTo(width - 1, height - 1)
                    ctx.lineTo(1, 1)
                    ctx.fill()
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: {
                        multiCaptureCanvas.hovered = true
                        multiCaptureCanvas.requestPaint()
                    }
                    onExited: {
                        multiCaptureCanvas.hovered = false
                        multiCaptureCanvas.requestPaint()
                    }
                    onPressed: {
                        multiCaptureCanvas.pressed = true
                        multiCaptureCanvas.requestPaint()
                    }
                    onReleased: {
                        multiCaptureCanvas.pressed = false
                        multiCaptureCanvas.requestPaint()
                    }
                    onClicked: {
                        multiCapture.show()
                    }
                }
            }

            onClicked: {
                forceActiveFocus()
                switchMultiFrameCapture.checked = false
                capture()
            }
        }

        Text {
            id: textMultiFrameCapture
            font.pixelSize: 18
            text: qsTr("多帧拍摄")
            color: "#828282"
            visible: !WzUtils.isGelCapture()
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
                if (showOptionsOnChecked) {
                    if (checked) {
                        multiCapture.show()
                        rectShade.opacity = 0.7
                        rectShade.color = "#00000"
                        checkBoxExposureTimeAuto.checked = false
                    } else {
                        multiCapture.hide()
                        rectShade.opacity = 0
                    }
                }
            }
            Component.onCompleted: {
                showOptionsOnChecked = false
                checked = dbService.readIntOption("is_multi_capture", 0)
                showOptionsOnChecked = true
            }
            Component.onDestruction: {
                dbService.saveIntOption("is_multi_capture", checked)
            }
        }

        Text {
            id: textPreviewSwitch
            font.pixelSize: 18
            text: qsTr("预览开关")
            color: "#828282"
            visible: WzUtils.isGelCapture()
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
            onCheckedChanged: {
                if (checked) {
                    captureService.previewExposureMs = exposureTime.exposureMs
                    captureService.enableCameraPreview(true, buttonBinning.binning)
                } else {
                    captureService.enableCameraPreview(false, buttonBinning.binning)
                }
            }
        }

        Text {
            id: textBinning
            font.pixelSize: 18
            text: qsTr("像素合并")
            color: "#828282"
            visible: !WzUtils.isGelCapture()
            anchors.leftMargin: 15
            anchors.left: textMultiFrameCapture.right
            anchors.bottom: textMultiFrameCapture.bottom
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
            onBinningChanged: {text = binning + "x" + binning}

            onClicked: {
                menuBinning.popup(0, -menuBinning.height - 10)
            }

            Component.onCompleted: {binning = dbService.readIntOption("binning", 4)}
            Component.onDestruction: {dbService.saveIntOption("binning", binning)}

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
            anchors.leftMargin: textPreviewSwitch.visible ? 30 : 15
            anchors.left: textPreviewSwitch.visible ? textPreviewSwitch.right : textBinning.right
            anchors.bottom: textMultiFrameCapture.bottom
            anchors.bottomMargin: textPreviewSwitch.visible ? -1 : 0
            MouseArea {
                anchors.fill: parent
                //onClicked: menuExposureTimePlans.popup(0, menuExposureTimePlans.height*-1)
            }
            Text {
                id: textExposureTimeAuto
                font.pixelSize: 14
                text: qsTr("[自动计算")
                color: "#828282"
                anchors.leftMargin: 4
                anchors.left: textExposureTime.right
                anchors.verticalCenter: parent.verticalCenter
                visible: checkBoxExposureTimeAuto.visible
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        checkBoxExposureTimeAuto.checked = !checkBoxExposureTimeAuto.checked
                    }
                }
            }
            WzCheckBox {
                id: checkBoxExposureTimeAuto
                scale: 0.7
                width: 24
                visible: {
                    if (adminParams) {
                        if (adminParams.autoExposure) {
                            return true
                        }
                    }
                    return false
                }
                anchors.left: textExposureTimeAuto.right
                anchors.verticalCenter: parent.verticalCenter
                onCheckedChanged: {
                    // 自动曝光和多帧拍摄逻辑上有冲突, 所以开启了自动曝光后自动关闭多帧拍摄
                    if (checked && switchMultiFrameCapture.checked)
                        switchMultiFrameCapture.checked = false
                }
                Component.onCompleted: checked = dbService.readIntOption("auto_exposure", 0)
                Component.onDestruction: dbService.saveIntOption("auto_exposure", checked)
            }
            Text {
                id: textExposureTimeAutoEnd
                font.pixelSize: 14
                text: qsTr("]")
                color: "#828282"
                anchors.left: checkBoxExposureTimeAuto.right
                anchors.verticalCenter: parent.verticalCenter
                visible: checkBoxExposureTimeAuto.visible
            }
            WzMenu {
                id: menuExposureTimePlans
                width: 90
                indicatorWidth: 8
                font.pixelSize: 15
                font.family: "Arial"

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
            onExposureMsChanged: {
                changePreviewExposureMs.restart()
            }
            Component.onCompleted: {
                var defaultExposureMs = 10 * 1000
                if (WzUtils.isGelCapture())
                    defaultExposureMs = 10;
                exposureTime.setExposureMs(dbService.readIntOption("capture_exposure_ms", defaultExposureMs))
            }
            Component.onDestruction: {
                dbService.saveIntOption("capture_exposure_ms", exposureTime.getExposureMs())
            }
        }

        Rectangle {
            id: lineCapture
            anchors.left: textMultiFrameCapture.left
            anchors.leftMargin: 2
            anchors.right: buttonCapture.right
            anchors.bottom: buttonCapture.top
            anchors.bottomMargin: 22
            height: 1
            color: "#161616"
        }

        //[[镜头]]//
        WzButton {
            id: buttonFocusAuto
            enabled: mcuConnected
            opacity: enabled ? 1 : 0.4
            visible: false
            width: {
                switch(WzI18N.language) {
                case "zh": return 140
                case "en": return 145
                }
            }
            height: 43
            radius: 4
            text: qsTr("自动聚焦")
            anchors.bottom: lineCapture.top
            anchors.bottomMargin: 22
            anchors.right: buttonFocusFar.left
            anchors.rightMargin: 10
            label.font.pixelSize: 18
            label.horizontalAlignment: Text.AlignRight
            label.anchors.rightMargin: {
                switch(WzI18N.language) {
                case "zh": return 20
                case "en": return 15
                }
            }
            label.anchors.fill: label.parent
            imageVisible: true
            imageAlign: Qt.AlignLeft | Qt.AlignVCenter
            imageSourceNormal: "qrc:/images/focus.svg"
            image.sourceSize.width: 22
            image.sourceSize.height: 22
            image.anchors.leftMargin: {
                switch(WzI18N.language) {
                case "zh": return 18
                case "en": return 12
                }
            }
            image.anchors.verticalCenterOffset: 0
            onClicked: {
                if (isAutoFocusing) return
                if (!switchPreview.checked) return
                isAutoFocusSelectRect = !isAutoFocusSelectRect
            }
        }

        WzButton {
            id: buttonFocusNear
            enabled: mcuConnected
            opacity: enabled ? 1 : 0.4
            width: {
                switch(WzI18N.language) {
                case "zh": return 124
                case "en": return 152
                }
            }
            height: 43
            radius: 4
            text: qsTr("聚焦近")
            anchors.bottom: lineCapture.top
            anchors.bottomMargin: 22
            anchors.right: buttonCapture.right
            label.font.pixelSize: 18
            label.horizontalAlignment: Text.AlignRight
            label.anchors.rightMargin: {
                switch(WzI18N.language) {
                case "zh": return 20
                case "en": return 17
                }
            }
            label.anchors.fill: label.parent
            imageVisible: true
            imageAlign: Qt.AlignLeft | Qt.AlignVCenter
            imageSourceNormal: "qrc:/images/button_minus.png"
            image.anchors.leftMargin: {
                switch(WzI18N.language) {
                case "zh": return 18
                case "en": return 15
                }
            }
            image.anchors.verticalCenterOffset: 0
            onPressed: {
                if (captureService.activeCameraManufacturer == "Toup")
                    mcu.focusStartNear2(mouse.x / width)
                else
                    mcu.focusStartNear()
            }
            onReleased: mcu.focusStop()
        }

        WzButton {
            id: buttonFocusFar
            enabled: mcuConnected
            opacity: enabled ? 1 : 0.4
            width: {
                switch(WzI18N.language) {
                case "zh": return 124
                case "en": return 143
                }
            }
            height: 43
            radius: 3
            text: qsTr("聚焦远")
            anchors.bottom: WzUtils.isGelCapture() ? buttonFocusNear.bottom : buttonFocusNear.top
            anchors.bottomMargin: WzUtils.isGelCapture() ? 0 : 7
            anchors.right: WzUtils.isGelCapture() ? buttonFocusNear.left : buttonCapture.right
            anchors.rightMargin: WzUtils.isGelCapture() ? 10 : 0
            label.font.pixelSize: 18
            label.horizontalAlignment: Text.AlignRight
            label.anchors.rightMargin: {
                switch(WzI18N.language) {
                case "zh": return 20
                case "en": return 17
                }
            }
            label.anchors.fill: label.parent
            imageVisible: true
            imageAlign: Qt.AlignLeft | Qt.AlignVCenter
            imageSourceNormal: "qrc:/images/button_plus.png"
            image.anchors.leftMargin: {
                switch(WzI18N.language) {
                case "zh": return 18
                case "en": return 15
                }
            }
            image.anchors.verticalCenterOffset: 0
            onPressed: {
                if (captureService.activeCameraManufacturer == "Toup")
                    mcu.focusStartFar2(mouse.x / width)
                else
                    mcu.focusStartFar()
            }
            onReleased: mcu.focusStop()
        }

        Loader {
            id: focusSliderGelLoader
            height: 70
            anchors.bottom: apertureSliderLoader.top
            anchors.bottomMargin: 15
            anchors.left: lineApertureSlider.left
            anchors.right: buttonFocusNear.right
            anchors.rightMargin: 0
            visible: WzUtils.isGelCapture()
            Component.onCompleted: {
                if (WzUtils.isGelCapture())
                    sourceComponent = focusSliderGelComponent
            }
        }

        Component {
            id: focusSliderGelComponent
            FocusSliderGel {
                id: focusSliderGel
                anchors.fill: parent
                enabled: mcuConnected
                onSwitchFocus: mcu.switchFocus(value)
                Component.onCompleted: focusSliderGel.value = dbService.readIntOption("focus", 6)
                Component.onDestruction: {dbService.saveIntOption("focus", focusSliderGel.value)}

            }
        }

        Loader {
            id: apertureSliderLoader
            height: 70
            anchors.bottom: WzUtils.isGelCapture() ? buttonFocusFar.top : buttonFocusNear.bottom
            anchors.bottomMargin: WzUtils.isGelCapture() ? 15 : 34
            anchors.left: lineApertureSlider.left
            anchors.right: buttonFocusNear.right
            anchors.rightMargin: 0
            sourceComponent: WzUtils.isGelCapture() ? apertureSliderGelComponent : apertureSliderComponent
        }

        Component {
            id: apertureSliderComponent
            ApertureSlider {
                id: apertureSlider
                anchors.fill: parent
                enabled: mcuConnected
                Component.onCompleted: apertureSlider.value = dbService.readIntOption("aperture", 6)
                Component.onDestruction: {dbService.saveIntOption("aperture", apertureSlider.value)}
                onSwitchAperture: mcu.switchAperture(value)
            }
        }
        Component {
            id: apertureSliderGelComponent
            ApertureSliderGel {
                id: apertureSlider
                anchors.fill: parent
                enabled: mcuConnected
                Component.onCompleted: apertureSlider.value = dbService.readIntOption("aperture", 6)
                Component.onDestruction: {dbService.saveIntOption("aperture", apertureSlider.value)}
                onSwitchAperture: mcu.switchAperture(value)
            }
        }

        Text {
            id: textLens
            text: qsTr("电动镜头")
            font.pixelSize: 18
            color: "#707070"
            anchors.left: apertureSliderLoader.left
            anchors.leftMargin: 0
            anchors.bottom: focusSliderGelLoader.visible ? focusSliderGelLoader.top : apertureSliderLoader.top
            anchors.bottomMargin: 13
        }
        //[[镜头]]//

        Rectangle {
            id: lineApertureSlider
            anchors.left: lineCapture.left
            anchors.leftMargin: lineCapture.anchors.leftMargin
            anchors.right: buttonCapture.right
            anchors.bottom: textLens.top
            anchors.bottomMargin: 18
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
            anchors.bottomMargin: 2
            height: 152
            anchors.leftMargin: -15
            anchors.rightMargin: 20
            onSwitchLight: {
                if (isOpen) {
                    mcu.switchLight(lightType, true)
                    lightControl.closeAll()
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
            anchors.bottomMargin: 14
        }
        //[[荧光和滤镜轮]]//
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
        anchors.leftMargin: -40
        anchors.right: captureProgress.right
        anchors.rightMargin: -40
        anchors.top: captureProgress.top
        anchors.topMargin: -30
        anchors.bottom: captureProgress.bottom
        anchors.bottomMargin: -20
        radius: 3
        color: "#a0a0a0"
        opacity: 0.9
        visible: captureProgress.visible
    }
    CaptureProgress {
        id: captureProgress
        anchors.verticalCenter: rectangleLeft.verticalCenter
        anchors.horizontalCenter: rectangleLeft.horizontalCenter
        z: 20
        visible: false

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
        z: 20
        visible: opacity > 0
        opacity: 0
        anchors.left: rectangleRight.left
        anchors.leftMargin: textMultiFrameCapture.x
        anchors.bottom: rectangleRight.bottom
        anchors.bottomMargin: rectangleRight.height - textMultiFrameCapture.y
        Component.onCompleted: {
            var params = JSON.parse(dbService.readStrOption("multi_capture_params", "{}"))
            multiCapture.setParams(params)
        }
        onMultiCapture: {
            multiCapture.hide()
            captureService.isAutoExposure = false
            switchMultiFrameCapture.checked = true
            root.capture()
        }
    }
    //[[多帧拍摄]]//

    function captureWithoutPreviewingClicked(buttonIndex) {
        if (buttonIndex === 2) {
            confirmCaptureWithoutPreviewing = true
            capture()
        } else {
            previewControl.startPreview()
            mcu.switchLight("white_up", true)
            lightControl.buttonLightWhiteUp.checked = true
        }
        msgBox.buttonClicked.disconnect(captureWithoutPreviewingClicked)
    }

    function capture() {
        confirmCaptureWithoutPreviewing = false
        if (mcu.latestLightOpened) {
            if (mcu.latestLightType === "white_up") {
                if (dbService.readIntOption("noCloseWhiteUpWhenCapture", 0)) {

                } else {
                    mcu.closeAllLight()
                }
                switchMultiFrameCapture.checked = false
            }
            else if (mcu.latestLightType === "uv_penetrate" || mcu.latestLightType === "uv_penetrate_force" ||
                     mcu.latestLightType === "white_down") {
                switchMultiFrameCapture.checked = false
            } else if (mcu.latestLightType === "red" || mcu.latestLightType === "green" || mcu.latestLightType === "blue") {
                switchMultiFrameCapture.checked = false
            }
        }
        rectShade.color = "#262626"
        rectShade.opacity = 0.7
        captureProgress.progress = 0
        captureProgress.visible = true
        captureService.imagePath = dbService.readStrOption("image_path", "");
        if (switchMultiFrameCapture.checked) {
            var params = multiCapture.params()
            params.exposureMs = exposureTime.getExposureMs()
            params.binning = buttonBinning.binning
            captureService.captureMulti(params)
        } else {
            var params1 = {
                binning: buttonBinning.binning,
                exposureMs: exposureTime.getExposureMs(),
                openedLightType: mcu.latestLightType,
                isLightOpened: mcu.latestLightOpened,
                sampleName: textInputSampleName.text
            }
            captureService.capture(params1)
        }
    }
}


/*##^##
Designer {
    D{i:0;autoSize:true;height:800;width:1280}D{i:7;anchors_width:497}
}
##^##*/
