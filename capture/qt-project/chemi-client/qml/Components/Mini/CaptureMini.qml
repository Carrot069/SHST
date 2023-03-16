import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

import WzCapture 1.0
import WzUtils 1.0
import WzI18N 1.0
import "../../WzControls"
import "../../Controller/PageCaptureController.js" as PageCaptureController

Item {
    id: root

    Rectangle {
        color: "black"
        anchors.fill: parent
    }
    Image {
        source: "qrc:/images/background.png"
        width: 764
        height: 641
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        visible: !WzUtils.isNoLogo()
    }

    Rectangle {
        id: rectLeftMini
        color: "transparent"
        border.color: "#6d6f6f"
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 20
        width: parent.width - rectRightMini.width - 60
        height: parent.height - 40
        radius: 10
    }

    Rectangle {
        id: rectRightMini
        color: "transparent"
        border.color: "#6d6f6f"
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        width: 330
        height: parent.height - 40
        radius: 10

        MouseArea {
            anchors.fill: parent
            onClicked: {
                forceActiveFocus()
            }
        }

        Image {
            id: imageLogo
            source: {
                if (WzUtils.isNoLogo())
                    return ""
                switch(WzI18N.language) {
                case "en":
                    return "qrc:/images/logo_text3.png"
                default:
                    return "qrc:/images/logo_text2.png"
                }
            }
            anchors.horizontalCenter: parent.horizontalCenter
            y: 20            
        }

        PageNav {
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.top: imageLogo.bottom
            anchors.topMargin: 15
            onClicked: pageChange(buttonIndex)
        }

        // 拍摄选项
        CaptureOption {
            id: captureOption
            width: 300
            height: 230
            anchors.bottom: captureMode.top
            anchors.horizontalCenter: parent.horizontalCenter
        }

        // 拍摄模式选择
        CaptureMode {
            id: captureMode
            anchors.bottom: captureButton.top
            anchors.bottomMargin: 50
            anchors.horizontalCenter: parent.horizontalCenter
            onActiveIndexChanged: {
                dbService.saveIntOption("miniCaptureMode", activeIndex)
                captureOption.currentIndex = activeIndex
            }
            Component.onCompleted: {
                changeIndex(dbService.readIntOption("miniCaptureMode", 1))
            }
        }

        // 拍摄按钮
        CaptureButton {
            id: captureButton
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottom: textInputSampleName.top
            anchors.bottomMargin: 30

            width: 160
            height: 160
            circle: true
            opacity: 0.8
            normalColor: "white"
            hotColor: "white"
            downColor: "white"

            onClicked: {
                console.info("mini capture")
                var captureParams = {
                    multiCapture: false
                }
                if (captureMode.isManual) {
                    console.info("\tmanual")
                    captureParams.singleCaptureParams = {
                        sampleName: textInputSampleName.text,
                        binning: buttonBinning.binning,
                        exposureMs: captureOption.params.manualExposureMs,
                        openedLightType: mcu.latestLightType,
                        isLightOpened: mcu.latestLightOpened,
                        grayAccumulate: false,
                        removeFluorCircle: isRemoveFluorCircle,
                        multiCapture: false
                    }
                    captureService.isAutoExposure = false
                } else if (captureMode.isAuto) {
                    console.info("\tauto")
                    captureParams.singleCaptureParams = {
                        sampleName: textInputSampleName.text,
                        binning: buttonBinning.binning,
                        openedLightType: mcu.latestLightType,
                        isLightOpened: mcu.latestLightOpened,
                        grayAccumulate: false,
                        removeFluorCircle: isRemoveFluorCircle,
                        multiCapture: false
                    }
                    captureService.isAutoExposure = true
                } else if (captureMode.isMulti) {
                    console.info("\tmulti")
                    captureParams.multiCapture = true
                    captureParams.multiCaptureParams = captureOption.params.multiCaptureParams
                    captureParams.multiCaptureParams.sampleName = textInputSampleName.text
                    captureParams.multiCaptureParams.binning = buttonBinning.binning
                    captureParams.multiCaptureParams.openedLightType = mcu.latestLightType
                    captureParams.multiCaptureParams.isLightOpened = mcu.latestLightOpened
                    captureParams.multiCaptureParams.grayAccumulate = false
                    captureParams.multiCaptureParams.removeFluorCircle = isRemoveFluorCircle
                    captureService.isAutoExposure = false
                }

                console.debug("\tcaptureParams:", captureParams)

                capture(captureParams)
            }
        }

        // 样品输入框
        Rectangle {
            id: rectSampleName
            anchors.left: parent.left
            anchors.leftMargin: 50
            anchors.right: parent.right
            anchors.rightMargin: 50
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 40
            height: 2
            color: textInputSampleName.focus ? "#00ccff" : (textInputSampleName.hovered ? "#dddddd" : "#404040")
        }
        TextInput {
            id: textInputSampleName
            clip: true
            width: rectSampleName.width
            anchors.bottom: rectSampleName.top
            anchors.bottomMargin: 2
            anchors.horizontalCenter: parent.horizontalCenter
            color: "#505050"
            font.pixelSize: 18
            selectByMouse: true
            horizontalAlignment: TextInput.AlignHCenter
            property string placeholderText: qsTr("此处可输入样品名")
            property var hovered
            Text {
                text: parent.placeholderText
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                visible: parent.text === "" && !parent.focus
                color: "#404040"
                font: parent.font
            }
        }
        MouseArea {
            anchors.fill: textInputSampleName
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

    Component.onCompleted: {
        captureProgress.anchors.verticalCenter = parent.verticalCenter
        captureProgress.anchors.horizontalCenter = parent.horizontalCenter
        captureProgress.anchors.horizontalCenterOffset = -(parent.width - rectLeftMini.width - rectLeftMini.x * 2) / 2
    }
}


/*##^##
Designer {
    D{i:0;autoSize:true;height:800;width:1280}D{i:7;anchors_width:497}
}
##^##*/
