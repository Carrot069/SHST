import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

import WzCapture 1.0
import WzUtils 1.0
import WzI18N 1.0
import "WzControls"
import "Components"

Item {
    id: root

    signal newImage(string imageFile)
    signal pageChange(int pageIndex)

    property bool mcuConnected: true
    property bool cameraConnected: false
    property real cameraTemperature: 0
    property bool isPreviewing: false
    property bool confirmCaptureWithoutPreviewing: false
    property bool chemiHasWhiteImage: false // 化学发光模式下预览过此变量为 true, 切换模式后为 false, 需要再次预览

    property alias captureProgress: captureProgress
    property alias mcu: mcu
    property alias captureService: captureService

    property var adminParams

    WzMcu {
        id: mcu
        onLightSwitched: {
            if (!isOpened) {
                lightControl.closeAll()
                fluorescence.closeAllLight()
            } else {
                lightControl.setLightChecked(lightType, true)
                if (lightType === "red" || lightType === "green" || lightType === "blue") {
                    fluorescence.setLightChecked(lightType)
                } else {
                    fluorescence.closeAllLight()
                }
            }
        }
        onDoorSwitchEnter: {
            console.info("onDoorSwitchEnter")
            // 2021-11-2 注释 by wz, 原因: 硬件层面已经实现 "触碰开关后开关抽屉"
            // mcu.switchDoor()
        }
        onDoorSwitchLeave: {
            console.info("onDoorSwitchLeave")
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
            cameraTemperature = captureService.getCameraTemperature()
            previewControl.cameraTemperature = cameraTemperature
            textCameraState.text = qsTr("相机温度: ") + cameraTemperature
        }
    }

    Timer {
        id: changePreviewExposureMs
        interval: 500
        repeat: false
        running: false
        onTriggered: {
            if (capturePlan.planName === "chemi")
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
                //pageChange(1)
                if (mcu.latestLightOpened &&
                        (mcu.latestLightType === "uv_penetrate" || mcu.latestLightType === "uv_penetrate_force")) {
                    mcu.closeAllLight()
                    lightControl.closeAll()
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
                if (1 == 0) {
                if (!mcu.latestLightOpened) {
                    var lightType = capturePlan.getLightNameFromIndex(capturePlan.params.light)
                    mcu.switchLight(lightType, true)
                }
                }
                break
            case WzCameraState.PreviewStopped:
                isPreviewing = false
                previewControl.previewState = "cameraPreviewStopped"
                //textCameraState.text = qsTr("预览已停止")
                liveImageView.clearImage()
                switchPreview.checked = false
                if (1 == 0) {
                if (mcu.latestLightOpened && mcu.latestLightType === "white_up") {
                    mcu.closeAllLight()
                }
                }
                break
            case WzCameraState.CameraNotFound:
                previewControl.cameraState = "cameraNotFound"
                textCameraState.text = qsTr("相机未连接")
                break;
            }
        }
        onPreviewImageRefreshed: {
            if (capturePlan.planName === "chemi") {
                chemiHasWhiteImage = true
            }
        }
        onCameraCountChanged: {

        }
        onTcpClientDisconnected: {
            if (connectionCount == 0) {
                console.info("All of tcp clients are disconnected")
                captureService.enableCameraPreview(false, 4)
                mcu.closeAllLight()
            }
        }
        onAutoFocusFar: mcu.focusFar(step, isGel)
        onAutoFocusNear: mcu.focusNear(step, isGel)
        onAutoFocusStop: {
            mcu.focusStop()
        }
        onAutoFocusFinished: {
            mcu.focusStop()
        }
    }

    Component.onCompleted: {
        //captureService.initCameras()
        captureService.imagePath = dbService.readStrOption("image_path", "")
        captureService.connectCamera()
        mcu.init()
        adminParams = dbService.readAdminSetting()        
    }
    Connections {
        target: window
        onClosing: {
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
            MouseArea {
                anchors.fill: parent
                onClicked: parent.forceActiveFocus()
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

        //[[相机状态]]//
        Text {
            id: textCameraState
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            text: qsTr("请稍候")
            color: "#555555"
            font.pixelSize: 18
        }
        //[[相机状态]]//

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
            onSwitchLight: {
                if (isOpen)
                    fluorescence.closeAll()
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
                console.debug("switchPreview.onCheckedChanged, planName:", capturePlan.planName, ", checked:", checked)
                if (capturePlan.planName === "chemi") {
                    if (checked) {
                        previewExposure.exposureTime.forceActiveFocus()
                        captureService.previewExposureMs = previewExposure.exposureTime.value
                    }
                } else {
                    if (checked)
                        captureService.previewExposureMs = exposureTime.exposureMs
                }

                if (checked) {
                    captureService.enableCameraPreview(true, buttonBinning.binning)
                } else {
                    if (!noStopPreview)
                        captureService.enableCameraPreview(false, buttonBinning.binning)
                }
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
            }

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
            property bool noSaveExposureMs: false
            onExposureMsChanged: {
                if (noSaveExposureMs)
                    return
                if (capturePlan.planName === "chemi") {
                    adminParams.chemi.exposureMs = exposureTime.exposureMs
                    return
                } else if (capturePlan.planName === "rna")
                    adminParams.rna.exposureMs = exposureTime.exposureMs
                else if (capturePlan.planName === "protein")
                    adminParams.protein.exposureMs = exposureTime.exposureMs
                else if (capturePlan.planName === "fluor") {
                    if (mcu.latestLightType === "red")
                        adminParams.red.exposureMs = exposureTime.exposureMs
                    else if (mcu.latestLightType === "green")
                        adminParams.green.exposureMs = exposureTime.exposureMs
                    else if (mcu.latestLightType === "blue")
                        adminParams.blue.exposureMs = exposureTime.exposureMs
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
        WzButton {
            id: buttonFocusNear
            enabled: mcuConnected
            opacity: enabled ? 1 : 0.4
            width: {
                switch(WzI18N.language) {
                case "zh": return 126
                case "en": return 134
                }
            }
            height: 48
            radius: 4
            text: qsTr("聚焦近")
            anchors.bottom: buttonFocusFar.bottom
            anchors.left: buttonFocusFar.right
            anchors.leftMargin: 5
            normalColor: "#141414"
            label.font.pixelSize: {
                switch(WzI18N.language) {
                case "en": return 15
                case "zh": return 18
                default: return 18
                }
            }
            label.horizontalAlignment: Text.AlignRight
            label.anchors.rightMargin: {
                switch(WzI18N.language) {
                case "zh": return 22
                case "en": return 15
                }
            }
            label.anchors.fill: label.parent
            imageVisible: true
            imageAlign: Qt.AlignLeft | Qt.AlignVCenter
            imageSourceNormal: "qrc:/images/button_minus.png"
            image.anchors.leftMargin: {
                switch(WzI18N.language) {
                case "zh": return 20
                case "en": return 15
                }
            }
            image.anchors.verticalCenterOffset: 0
            onPressed: mcu.focusStartNear()
            onReleased: mcu.focusStop()
        }

        WzButton {
            id: buttonFocusFar
            enabled: mcuConnected
            opacity: enabled ? 1 : 0.4
            width: {
                switch(WzI18N.language) {
                case "zh": return 126
                case "en": return 128
                }
            }
            height: 48
            radius: 3
            text: qsTr("聚焦远")
            anchors.bottom: lineCapture.top
            anchors.bottomMargin: 18
            anchors.left: lineCapture.left
            anchors.leftMargin: 0
            normalColor: "#141414"
            label.font.pixelSize: buttonFocusNear.label.font.pixelSize
            label.horizontalAlignment: Text.AlignRight
            label.anchors.rightMargin: {
                switch(WzI18N.language) {
                case "zh": return 22
                case "en": return 15
                }
            }
            label.anchors.fill: label.parent
            imageVisible: true
            imageAlign: Qt.AlignLeft | Qt.AlignVCenter
            imageSourceNormal: "qrc:/images/button_plus.png"
            image.anchors.leftMargin: {
                switch(WzI18N.language) {
                case "zh": return 20
                case "en": return 15
                }
            }
            image.anchors.verticalCenterOffset: 0
            onPressed: mcu.focusStartFar()
            onReleased: mcu.focusStop()
        }

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
                    lightControl.closeAll()
                    mcu.switchLight(lightType, true)
                    capturePlan.switchFluorescence(lightType)
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

    PreviewExposure {
        id: previewExposure
        anchors.right: rectangleRight.left
        anchors.rightMargin: capturePlan.planName === "chemi" && switchPreview.checked ? 5 : -120
        anchors.top: rectangleRight.top
        anchors.topMargin: textPreviewSwitch.y - 10
        opacity: capturePlan.planName === "chemi" && switchPreview.checked ? 0.9 : 0
        z:2
        exposureTime.onValueChanged: {
            if (capturePlan.planName === "chemi") {
                adminParams.chemi.previewExposureMs = previewExposure.exposureTime.value
                captureService.previewExposureMs = previewExposure.exposureTime.value
                changePreviewExposureMs.restart()
            }
        }
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

    function captureWithoutPreviewingClicked(buttonIndex) {
        if (buttonIndex === 2) {
            confirmCaptureWithoutPreviewing = true
            capture()
        } else {
            switchPreview.checked = true
        }
        msgBox.buttonClicked.disconnect(captureWithoutPreviewingClicked)
    }

    function capture() {

        // 没有预览而且没有确认过却是不用预览则提示用户先预览
        if (capturePlan.planName === "chemi" && !isPreviewing && !chemiHasWhiteImage && !confirmCaptureWithoutPreviewing) {
            msgBox.buttonCount = 2
            msgBox.buttonClicked.connect(captureWithoutPreviewingClicked)
            msgBox.show(qsTr("当前没有开启预览, 这样在拍摄后无法叠加 Marker 图片, 请在下面作出选择:"),
                        qsTr("开启预览"), qsTr("确定拍摄"))
            return
        }

        // 需要在关闭光源之前把预览图备份出来, 否则可能在关灯后仍然进行短时间的预览并使其覆盖掉正常预览图
        captureService.getMarkerImage();

        confirmCaptureWithoutPreviewing = false
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
            params.sampleName = textInputSampleName.text
            params.exposureMs = exposureTime.getExposureMs()
            params.binning = buttonBinning.binning
            params.grayAccumulateAddExposure = adminParams.grayAccumulateAddExposure
            captureService.captureMulti(params)
        } else {
            var params1 = {
                sampleName: textInputSampleName.text,
                binning: buttonBinning.binning,
                exposureMs: exposureTime.getExposureMs(),
                openedLightType: mcu.latestLightType,
                isLightOpened: mcu.latestLightOpened,
                grayAccumulate: false
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
