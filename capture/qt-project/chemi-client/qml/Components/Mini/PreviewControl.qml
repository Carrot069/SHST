import QtQuick 2.12
import "../../WzControls"
import "../../WzControls.2"

Item {
    id: root
    width: 240
    height: 120

    property alias exposureTime: exposureTime

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.forceActiveFocus()
        }
    }

    Rectangle {
        id: rectBackground
        width: 190
        height: 68
        radius: height
        color: "#ffffff"
        opacity: 0.1
        y: 30
        anchors.horizontalCenter: parent.horizontalCenter
    }

    Text {
        id: textAutoPreviewInstruction
        visible: !isPreviewing && cameraTemperature > -20
        color: "white"
        text: qsTr("相机温度降至-20以下时将自动开启预览")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 5
        font.pixelSize: 13
        minimumPixelSize: 12
    }

    WzSpinBox {
        id: exposureTime
        width: 160
        isShowButton: true
        isAlwaysShowButton: true
        isHorizontalButton: true
        anchors.verticalCenter: rectBackground.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        fontColor: "white"
        font.pixelSize: 40
        font.family: "Digital Dismay"
        from: 1
        to: 999
        value: 1
        z: 10
        buttonFontColor: "white"
        buttonColor: "transparent"
        buttonFont.pixelSize: 45
        buttonSize: 45
        textTopMargin: 5
        onValueChanged: {
            captureService.previewExposureMs = value
            adminParams.chemi.previewExposureMs = value
            changePreviewExposureMs.restart()
        }
    }

    Text {
        id: textMiniCameraState
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        color: "white"
        text: textCameraState.text
    }

    Component.onCompleted: {
        exposureTime.value = adminParams.chemi.previewExposureMs
    }

//    Connections {
//        target: textCameraState
//        onTextChanged: {
//            textMiniCameraState.text = text
//        }
//    }
}
