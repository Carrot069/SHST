import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

import WzCapture 1.0
import WzUtils 1.0
import WzI18N 1.0
import "WzControls"
import "Components"

import "Controller/PageCaptureController.js" as PCC

Item {
    id: root

    signal imageLoaded(string imageFile)
    signal newImage(string imageFile)
    signal pageChange(int pageIndex)

    property bool mcuConnected: mcu.connected || WzUtils.isDemo()
    property bool cameraConnected: WzUtils.isDemo() ? true : captureService.cameraConnected
    property bool isPreviewing: false
    property bool confirmCaptureWithoutPreviewing: false
    property bool chemiHasWhiteImage: false // 化学发光模式下预览过此变量为 true, 切换模式后为 false, 需要再次预览
    property bool isAutoFocusSelectRect: false
    property bool isFluorGetMarker: false
    property bool isFluorGetMarkerSuccess: false

    property alias captureProgress: captureProgress
    property alias mcu: mcu
    property var fluorescence

    property var adminParams

    onCameraConnectedChanged: {
        if (cameraConnected) {
            previewControl.cameraState = "cameraConnected"
            if (WzUtils.isChemiCapture()) {
                textCameraState.text = qsTr("相机已连接")
                getTemperatureTimer.running = true
            } else if (WzUtils.isGelCapture()) {
                textCameraState.text = ""
                getTemperatureTimer.running = false
            }
        }
    }

    WzMcu {
        id: mcu
        onLightSwitched: {
            if (!isOpened) {
                lightControl.closeAll()
                if (fluorescenceLoader.active)
                    fluorescence.closeAllLight()
            } else {
                lightControl.setLightChecked(lightType, true)
                if (fluorescenceLoader.active) {
                    if (lightType === "red" || lightType === "green" || lightType === "blue") {
                        lightControl.closeAll()
                        fluorescence.setLightChecked(lightType)
                    } else {
                        fluorescence.closeAllLight()
                        // 荧光方案下, 切换到白光时自动把滤镜轮转到空位
                        if (PCC.capturePlanIsFluor() && (lightType === "white_up")) {
                            switchToFilter0Timer.running = true
                        }
                    }
                }
            }
        }
        onConnectedChanged: {
            capturePlan.comboBoxCapturePlan.enabled = connected
        }
    }
    Timer {
        id: switchToFilter0Timer
        repeat: false
        running: false
        interval: 800
        onTriggered: {
            mcu.switchFilterWheel(0)
            if (fluorescenceLoader.active)
                fluorescence.filter1.active = true
        }
    }
    Timer {
        id: getTemperatureTimer
        interval: 1000
        repeat: true
        onTriggered: {
            var temperature = captureService.getCameraTemperature()
            previewControl.cameraTemperature = temperature
            textCameraState.text = qsTr("相机温度: ") + temperature
        }
    }

    Timer {
        id: changePreviewExposureMs
        interval: 500
        repeat: false
        running: false
        onTriggered: {
            if (PCC.capturePlanIsChemiFluor())
                captureService.previewExposureMs = previewExposure.exposureTime.value
            else
                captureService.previewExposureMs = exposureTime.exposureMs
            captureService.resetPreview()
        }
    }

    Timer {
        id: delaySaveAdminSettingTimer
        interval: 1000
        repeat: false
        running: false
        onTriggered: {
            saveAdminSetting(adminParams)
        }
    }

    WzCaptureService {
        id: captureService
        onCaptureStateChanged: {
            console.info("CaptureService.onCaptureStateChanged:", state)
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
                // 如果在曝光时客户端程序异常退出, 服务端的曝光还会继续
                // 客户端程序重新启动后读取到正在曝光的状态，这时候需要恢复到进行的状态
                if (!captureProgress.visible) {
                    PCC.showCaptureProgress()
                }

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
                PCC.hideCaptureProgress()
                break
            case WzCaptureService.Image:
                PCC.showCaptureProgressTextWithoutPercent(qsTr("正在处理图片"))
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
                    lightControl.closeAll()
                }
                break
            }
        }
        onCameraStateChanged: {
            switch(state) {
            case WzCameraState.ConnectingServer:
                if (connectFailedCount > 0)
                    textCameraState.text = qsTr("连接服务器超时, 正在进行第%1次连接").arg(connectFailedCount+1)
                else
                    textCameraState.text = qsTr("正在连接服务器")
                break
            case WzCameraState.Connecting:
                previewControl.cameraState = "cameraConnecting"
                textCameraState.text = qsTr("正在连接相机")
                break;
            case WzCameraState.Disconnected:
                switchPreview.checked = false
                getTemperatureTimer.running = false
                previewControl.cameraState = "cameraDisconnected"
                textCameraState.text = qsTr("相机已断开")
                break;
            case WzCameraState.PreviewStarted:
                isPreviewing = true
                previewControl.previewState = "cameraPreviewStarted"
                //textCameraState.text = qsTr("正在预览")
                if (!mcu.latestLightOpened) {
                    // 相机已开始预览, 此处的业务逻辑需要打开对应光源, 但此时有可能还没获取到拍摄方案,
                    // 如果是则延迟这项动作
                    // TODO 延迟打开对应光源
                    if (undefined === capturePlan.params) {
                        return
                    }

                    var lightType = capturePlan.getLightNameFromIndex(capturePlan.params.light)
                    mcu.switchLight(lightType, true)
                }                
                break
            case WzCameraState.PreviewStopped:
                isPreviewing = false
                previewControl.previewState = "cameraPreviewStopped"
                //textCameraState.text = qsTr("预览已停止")
                liveImageView.clearImage()
                switchPreview.checked = false
                if (mcu.latestLightOpened && mcu.latestLightType === "white_up" && WzUtils.isChemiCapture()) {
                    mcu.closeAllLight()
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
        onImageLoaded: {
            console.debug("CaptureService.onImageLoaded:", fileName)
            root.imageLoaded(fileName)
        }
        onAdminParamsReceived: {
            root.adminParams = adminParams
            capturePlan.init()
        }
        onServerAddressChanged: {
            //captureService.connectCamera()
            //mcu.serverAddress = serverAddress + ":" + captureService.mcuPort
            //mcu.init()
            // TODO 暂时放在此处, by wz 2022-03-03 19:54:21
            heart.connectToServer(serverInfo.getFullWsUrl())
        }
        function connectCameraAndMcu() {
            captureService.connectCamera()
            mcu.serverAddress = captureService.serverAddress + ":" + captureService.mcuPort
            mcu.init()
        }

        onServerMessage: {
            if (message.action !== undefined) {
                if (message.action === "getMarkerImage") {
                    if (message.code > 0) {
                        PCC.getMarkerImageSuccess()
                        if (isFluorGetMarker) {
                            PCC.hideCaptureProgress()
                            isFluorGetMarker = false
                            isFluorGetMarkerSuccess = true
                            fluorescenceLoader.continueSwitch()
                            return
                        }
                        root.capture({getMarkerImageSuccess: true})
                    } else if (message.code === -3) {
                        PCC.abortWaitingMarkerImage()
                        msgBox.show(qsTr("保存Marker图失败, 因为检测到画面太暗.\n请检查光源是否打开或延长曝光时间."), qsTr("确定"))
                    } else {
                        PCC.abortWaitingMarkerImage()
                        msgBox.show(qsTr("保存Marker图失败, 错误代码:" + message.code), qsTr("确定"))
                    }
                }
            }
        }
    }

    Component.onCompleted: {        
        // 安卓版不再从硬件读取, 而是通过网络从服务端获取, 代码在 CaptureService 的 onAdminParamsReceived
        //adminParams = dbService.readAdminSetting()
        var udpPort = dbService.readIntOption("udpPort", 30030)
        captureService.openUdp(udpPort)
    }
    Connections {
        target: window
        onClosing: function onClosing() {
            mcu.uninit()
        }
    }

    Connections {
        target: Qt.application
        onStateChanged: function onApplicationStateChanged(state) {
            console.info("onApplicationStateChanged:", state)
            if (state === Qt.ApplicationInactive) {
                switchPreview.checked = false
            }
        }
    }

    Connections {
        target: heart
        onAliveCountResult: {
            console.info("heart alive count:", heart.aliveCount)
            if (heart.aliveCount === 0) {
                cannotConnectServer.state = "hide"
                heart.start()
                captureService.connectCameraAndMcu()
            } else {
                textCameraState.text = qsTr("仪器忙")
                cannotConnectServer.state = "deviceBusy"
            }
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
                visible: isAutoFocusSelectRect || captureService.isAutoFocusing
                Text {
                    anchors.centerIn: parent
                    color: "#dddddd"
                    font.pixelSize: 17
                    text: {
                        if (isAutoFocusSelectRect)
                            return qsTr("请划选一个聚焦区域")
                        else if (captureService.isAutoFocusing)
                            return qsTr("正在自动聚焦")
                        else
                            return ""
                    }
                }
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        parent.forceActiveFocus()
                        captureService.stopAutoFocus()
                    }
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
                onDoubleClicked: {
                    mcu.switchDoor()
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
                        selectRect.visible = false
                        liveImageView.selectRect(selectRect.x, selectRect.x + selectRect.width,
                                                 selectRect.y, selectRect.y + selectRect.height)
                        captureService.startAutoFocus(liveImageView.getSelectedRect())
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
            text: qsTr("等待服务器广播")
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
            visible: fluorescenceLoader.active
            color: lineCapture.color
            anchors.left: lineCapture.left
            anchors.leftMargin: lineCapture.anchors.leftMargin
            anchors.right: buttonCaptureAE.right
            anchors.bottomMargin: 9
            anchors.bottom: fluorescenceLoader.top
        }

        // 拍摄方案
        CapturePlan {
            id: capturePlan
            visible: WzUtils.isChemiCapture()
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
            visible: WzUtils.isChemiCapture()
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
            anchors.bottom: fluorescenceLoader.active ? lineFluorescence.top : lineApertureSlider.top
            onSwitchLight: {
                if (isOpen && fluorescenceLoader.active)
                    fluorescence.closeAll()
                mcu.switchLight(lightType, isOpen)
                if (WzUtils.isChemiCapture())
                    capturePlan.switchLight(lightType)
            }
        }

        //[[凝胶拍摄按钮]]//
        Loader {
            id: captureGelLoader
            anchors.right: parent.right
            anchors.rightMargin: 20
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 20
            width: 92
            height: 99
            sourceComponent: captureGelComponent
            active: WzUtils.isGelCapture()
        }
        Component {
            id: captureGelComponent
            WzButton {
                anchors.fill: parent
                label.font.pixelSize: 20
                label.anchors.verticalCenterOffset: 26
                text: qsTr("拍摄")
                imageVisible: true
                imageSourceNormal: "qrc:/images/button_capture.png"
                image.anchors.verticalCenterOffset: -16
                radius: 5
                enabled: root.cameraConnected;
                onClicked: {
                    forceActiveFocus()
                    capture()
                }
            }
        }
        //[[凝胶拍摄按钮]]//

        WzButton {
            id: buttonCapture
            visible: !textBinning.visible && WzUtils.isChemiCapture()
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
            visible: !textBinning.visible && WzUtils.isChemiCapture()
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
            enabled: root.cameraConnected
            anchors.horizontalCenter: textPreviewSwitch.horizontalCenter
            width: 66
            anchors.verticalCenter: switchMultiFrameCapture.verticalCenter
            anchors.verticalCenterOffset: -1
            property bool noStopPreview: false
            onCheckedChanged: {
                console.debug("switchPreview.onCheckedChanged, planName:", capturePlan.planName, ", checked:", checked)
                if (capturePlan.planName === "chemi") {
                    if (checked) {
                        //previewExposure.exposureTime.forceActiveFocus()
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
            visible: {
                if (WzUtils.isDemo())
                    return true
                if (WzUtils.isGelCapture()) {
                    return false
                } else if (WzUtils.isChemiCapture()) {
                    if (adminParams === undefined)
                        return false
                    else
                        return adminParams.binningVisible
                }
            }
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
            enabled: root.cameraConnected
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
                var p = mapToItem(rootView, 0, 0)
                menuBinning.parent = rootView
                menuBinning.popup(p.x - (104 - width) / 2 / window.scale,
                                  p.y + -menuBinning.implicitHeight / window.scale - 5)
            }

            Component.onCompleted: {binning = dbService.readIntOption("binning", 4)}
            Component.onDestruction: {dbService.saveIntOption("binning", binning)}

            WzMenu {
                id: menuBinning
                implicitWidth: 80
                indicatorWidth: 8
                font.pixelSize: 18
                font.family: "Arial"
                background: Rectangle {color: "#222222"; radius: 5}
                enter: Transition {
                    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
                }
                exit: Transition {
                    NumberAnimation { property: "opacity"; from: 1.0; to: 0 }
                }
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
                onClicked: {
                    var p = mapToItem(rootView, 0, 0)
                    menuExposureTimePlans.parent = rootView
                    menuExposureTimePlans.popup(p.x,
                                      p.y + -menuExposureTimePlans.implicitHeight / window.scale - 5)
                }
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
                background: Rectangle {color: "#222222"; radius: 5}
                enter: Transition {
                    NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
                }
                exit: Transition {
                    NumberAnimation { property: "opacity"; from: 1.0; to: 0 }
                }

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
        // 凝胶采集软件才显示的自动曝光复选框
        Text {
            id: textExposureTimeAuto
            visible: WzUtils.isGelCapture()
            font.pixelSize: 14
            text: qsTr("[自动计算")
            color: "#828282"
            anchors.leftMargin: 4
            anchors.left: textExposureTime.right
            anchors.verticalCenter: textExposureTime.verticalCenter
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    exposureTimeAutoCheckBox.checked = !exposureTimeAutoCheckBox.checked
                }
            }
        }
        CheckBox {
            id: exposureTimeAutoCheckBox
            visible: textExposureTimeAuto.visible
            width: 30
            height: 30
            scale: 0.7
            anchors.leftMargin: -1
            anchors.left: textExposureTimeAuto.right
            anchors.verticalCenter: textExposureTimeAuto.verticalCenter
            onCheckedChanged: {
                // 自动曝光和多帧拍摄逻辑上有冲突, 所以开启了自动曝光后自动关闭多帧拍摄
                if (checked && switchMultiFrameCapture.checked)
                    switchMultiFrameCapture.checked = false
            }
            Component.onCompleted: checked = dbService.readIntOption("auto_exposure", 0)
            Component.onDestruction: dbService.saveIntOption("auto_exposure", checked)
        }
        Text {
            id: textExposureTimeAuto2
            visible: WzUtils.isGelCapture()
            font.pixelSize: 14
            text: qsTr("]")
            color: "#828282"
            anchors.leftMargin: -1
            anchors.left: exposureTimeAutoCheckBox.right
            anchors.verticalCenter: textExposureTime.verticalCenter
        }
        // --- //

        WzExposureTime {
            id: exposureTime
            enabled: root.cameraConnected            
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
                    delaySaveAdminSettingTimer.restart()
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
                delaySaveAdminSettingTimer.restart()
            }
        }

        Rectangle {
            id: lineCapture
            anchors.left: textMultiFrameCapture.left
            anchors.leftMargin: 2
            anchors.right: buttonCaptureAE.right
            anchors.bottom: {
                if (WzUtils.isChemiCapture())
                    return buttonCapture.top
                else if (WzUtils.isGelCapture())
                    return captureGelLoader.top
                else
                    return undefined
            }
            anchors.bottomMargin: 18
            height: 1
            color: "#161616"
        }

        //[[镜头]]//
        //[[凝胶用自动聚焦按钮]]//
        Loader {
            id: buttonFocusAutoLoader
            active: WzUtils.isGelCapture()
            anchors.bottom: buttonFocusFar.bottom
            anchors.right: buttonFocusFar.left
            anchors.rightMargin: 10
            sourceComponent: buttonFocusAutoComponent
        }
        Component {
            id: buttonFocusAutoComponent

            WzButton {
                id: buttonFocusAuto
                enabled: mcuConnected
                opacity: enabled ? 1 : 0.4
                width: {
                    switch(WzI18N.language) {
                    case "zh": return 140
                    case "en": return 145
                    }
                }
                height: 48
                radius: 4
                text: qsTr("自动聚焦")
                normalColor: "#141414"
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
                    if (captureService.isAutoFocusing) return
                    if (!switchPreview.checked) return
                    isAutoFocusSelectRect = !isAutoFocusSelectRect
                }
            }
        }
        // --- //

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
            anchors.left: WzUtils.isChemiCapture() ? buttonFocusFar.right : undefined
            anchors.leftMargin: 5
            anchors.right: WzUtils.isGelCapture() ? captureGelLoader.right : undefined
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
            anchors.left: WzUtils.isChemiCapture() ? lineCapture.left : undefined
            anchors.right: WzUtils.isGelCapture() ? buttonFocusNear.left : undefined
            anchors.rightMargin: 10
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

        //[[凝胶用的焦距滑竿]]//
        Loader {
            id: focusSliderGelLoader
            active: WzUtils.isGelCapture()
            height: 70
            anchors.bottom: apertureSliderGelLoader.top
            anchors.bottomMargin: 15
            anchors.left: lineApertureSlider.left
            anchors.right: buttonFocusNear.right
            anchors.rightMargin: 0
            sourceComponent: focusSliderGelComponent
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
        //[[凝胶用的焦距滑竿]]//

        //[[凝胶用的光圈滑竿]]//
        Loader {
            id: apertureSliderGelLoader
            active: WzUtils.isGelCapture()
            sourceComponent: apertureSliderGelComponent
            anchors.bottom: buttonFocusNear.top
            anchors.bottomMargin: 15
            anchors.left: lineApertureSlider.left
            anchors.right: buttonFocusNear.right
            anchors.rightMargin: 0
            height: 70
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
        //[[凝胶用的光圈滑竿]]//

        Component {
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
        }

        Text {
            id: textLens
            text: qsTr("电动镜头")
            font.pixelSize: 18
            color: "#707070"
            anchors.left: lineApertureSlider.left
            anchors.leftMargin: 0
            anchors.bottom: {
                if (WzUtils.isChemiCapture())
                    return buttonFocusFar.top
                else if (WzUtils.isGelCapture())
                    return focusSliderGelLoader.top
                else
                    return undefined
            }
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
            visible: WzUtils.isChemiCapture()
        }
        WzButton {
            id: buttonMultiFrameCaptureSetting
            enabled: mcuConnected
            opacity: enabled ? 1 : 0.4
            width: {
                switch(WzI18N.language) {
                case "zh": return 190
                case "en": return 150
                }
            }
            height: 48
            radius: 3
            visible: textMultiFrameCapture2.visible
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
        Loader {
            id: fluorescenceLoader
            active: WzUtils.isChemiCapture()
            sourceComponent: fluorescenceComponent
            anchors.left: textMultiFrameCapture.left
            anchors.right: parent.right
            anchors.bottom: lineApertureSlider.top
            anchors.bottomMargin: 0
            height: 152
            anchors.leftMargin: -15
            anchors.rightMargin: 20
            property int activeFilter: 0
            property bool isLoaded: false

            property var lightType

            function continueSwitch() {
                lightControl.closeAll()
                mcu.switchLight(fluorescenceLoader.lightType, true)
                capturePlan.switchFluorescence(fluorescenceLoader.lightType)
                capturePlan.noClearMarker = false
            }
        }
        Component {
            id: fluorescenceComponent

            Fluorescence {
                id: fluorescence_inner
                enabled: mcuConnected

                onSwitchLight: {
                    console.info("Fluorescence.onSwitchLight, ", isOpen)
                    if (isOpen) {
                        // 如果之前开了白光而且正在预览, 当切换成荧光光源的时候就需要保留一张白光图
                        if (mcu.latestLightOpened && mcu.latestLightType === "white_up" && isPreviewing) {
                            isFluorGetMarker = true
                            fluorescenceLoader.lightType = lightType
                            captureService.setCameraParam("markerAvgGrayThreshold", dbService.readStrOption("markerAvgGrayThreshold", 500))
                            captureService.getMarkerImage()
                            PCC.waitMarkerImage()
                            if (0) {
                            //if (!captureService.hasMarkerImage()) {
                                msgBox.buttonCount = 1
                                msgBox.show(qsTr("自动保存Marker图时发生了意外, 请点击“确定”按钮后重新拍摄。"),
                                            qsTr("确定"))
                                return
                            } else {
                                capturePlan.noClearMarker = true
                            }
                        } else {
                            lightControl.closeAll()
                            mcu.switchLight(lightType, true)
                            capturePlan.switchFluorescence(lightType)
                            capturePlan.noClearMarker = false
                        }
                    } else {
                        mcu.closeAllLight()
                    }
                    console.info("Fluorescence.onSwitchLight, end")
                }
                onSwitchFilter: {
                    mcu.switchFilterWheel(filterIndex)
                }

                Component.onCompleted: {
                    root.fluorescence = fluorescence_inner
                }
            }
        }

        Rectangle {
            id: lineLight
            visible: fluorescenceLoader.active
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
            visible: mcuConnected && opacity > 0 && WzUtils.isChemiCapture()
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
        anchors.rightMargin: PCC.previewExposureIsVisible() ? 5 : -120
        anchors.bottom: rectangleRight.bottom
        anchors.bottomMargin: 5
        opacity: PCC.previewExposureIsVisible() ? 0.9 : 0
        z:2
        exposureTime.onValueChanged: {
            if (PCC.capturePlanIsChemiFluor()) {
                if (PCC.capturePlanIsChemi())
                    adminParams.chemi.previewExposureMs = previewExposure.exposureTime.value
                else if (PCC.capturePlanIsFluor())
                    adminParams.fluorPreviewExposureMs = previewExposure.exposureTime.value
                captureService.previewExposureMs = previewExposure.exposureTime.value
                changePreviewExposureMs.restart()
                delaySaveAdminSettingTimer.restart()
                console.info("exposureTime.onValueChanged")
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

    Rectangle {
        id: rectShade
        anchors.fill: parent
        color: "#000000"
        visible: opacity > 0
        opacity: 0
        z: 10
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
            PCC.cancelCapture()
        }
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

    CannotConnectServer {
        id: cannotConnectServer
        onDeviceBusyRetry: {
            heart.getAliveCount()
            cannotConnectServer.state = "getAliveCount"
        }
        onDeviceBusyExit: window.close()
    }

    Timer {
        id: waitMarkerImageTimer
        repeat: false
        interval: 3000
        running: false
        onTriggered: {
            PCC.waitMarkerImageTimeout()
        }
    }

    function captureWithoutPreviewingClicked(buttonIndex) {
        if (buttonIndex === 2) {
            confirmCaptureWithoutPreviewing = true
            capture()
        } else {
            switchPreview.checked = true
        }
        msgBox.buttonClicked.disconnect(captureWithoutPreviewingClicked)
    }


    function capture(captureParams) {
        if (undefined === captureParams)
            captureParams = {}

        if (isFluorGetMarkerSuccess) {
            captureParams.getMarkerImageSuccess = true
        }

        if (captureParams.getMarkerImageSuccess) {

        } else {
            // 没有预览而且没有确认过确实不用预览则提示用户先预览
            if (capturePlan.planName === "chemi" && !isPreviewing && !chemiHasWhiteImage && !confirmCaptureWithoutPreviewing) {
                msgBox.buttonCount = 2
                msgBox.buttonClicked.connect(captureWithoutPreviewingClicked)
                msgBox.show(qsTr("当前没有开启预览, 这样在拍摄后无法叠加 Marker 图片, 请在下面作出选择:"),
                            qsTr("开启预览"), qsTr("确定拍摄"))
                return
            }

            // 需要在关闭光源之前把预览图备份出来, 否则可能在关灯后仍然进行短时间的预览并使其覆盖掉正常预览图
            if (WzUtils.isChemiCapture()) {
                console.info("It's chemi, wait the marker.")
                PCC.waitMarkerImage()
                captureService.setCameraParam("markerAvgGrayThreshold", dbService.readStrOption("markerAvgGrayThreshold", 500))
                captureService.getMarkerImage()
                return
            }
        }

        confirmCaptureWithoutPreviewing = false
        if (mcu.latestLightOpened) {
            if (mcu.latestLightType === "white_up" && WzUtils.isChemiCapture()) {
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
        captureProgress.progress = 0
        PCC.showCaptureProgress()
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
            if (WzUtils.isGelCapture()) {
                params1.isSaveMarker = false
                if (mcu.latestLightType === "white_up") {
                    params1.isThumbNegative = false
                    params1.imageInvert = 0x04 // 像白光透射图片那样显示白光反射的图片
                }
            }
            if (PCC.capturePlanIsFluor()) {
                params1.isThumbNegative = true
                params1.readoutFrequency = 25
            }
            captureService.capture(params1)
        }
    }

    function saveAdminSetting(params) {
        captureService.saveAdminSetting(JSON.stringify(params))
    }


}


/*##^##
Designer {
    D{i:0;autoSize:true;height:800;width:1280}D{i:7;anchors_width:497}
}
##^##*/
