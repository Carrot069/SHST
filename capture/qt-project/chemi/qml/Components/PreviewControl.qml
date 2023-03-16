import QtQuick 2.12
import "../WzControls"

Rectangle {
    id: root
    width: 281
    height: 78
    radius: 38
    color: "#838383"

    property alias exposureTime: exposureTime
    property alias cameraTemperature: textCameraTemperature.text

    // cameraConnecting, cameraConnected, cameraDisconnected, cameraNotFound
    property string cameraState: "cameraDisconnected"

    // cameraPreviewStarting, cameraPreviewStarted, cameraPreviewStopping, cameraPreviewStopped
    property string previewState: "cameraPreviewStopped"

    signal previewStart()
    signal previewStop()

    function startPreview() {
        root.previewState = "cameraPreviewStarting"
        forceActiveFocus()
        previewStart()
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.forceActiveFocus()
        }
    }

    WzButton {
        id: buttonPause
        circle: true
        enabled: root.previewState === "cameraPreviewStarted"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 7
        height: 67
        width: 67
        normalColor: "transparent"
        hotColor: "transparent"
        downColor: "transparent"
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_pause_normal.png"
        imageSourceHot: "qrc:/images/button_pause_hot.png"
        imageSourceDown: "qrc:/images/button_pause_down.png"
        onClicked: {
            root.previewState = "cameraPreviewStopping"
            forceActiveFocus()
            previewStop()
        }
    }

    Text {
        id: textExposureTime
        color: "#383838"
        text: qsTr("预览时间")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 5
        font.pixelSize: 18
        visible: root.cameraState === "cameraConnected"
    }

    Text {
        id: textState
        color: "#383838"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter
        font.pixelSize: 18
        visible: root.cameraState != "cameraConnected"
        text: {
            if (root.cameraState === "cameraConnecting")
                return qsTr("正在连接相机")
            else if (root.cameraState === "cameraDisconnected")
                return qsTr("相机未连接")
            else if (root.cameraState === "cameraNotFound")
                return qsTr("没有发现相机")
            else
                return ""
        }
    }

    WzExposureTime {
        id: exposureTime
        isShowMinute: false
        isShowButton: true
        fontPixelSize: 38
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: textExposureTime.bottom
        anchors.topMargin: -4
        spinBoxSecond.width: 35
        spinBoxMillisecond.width: 60
        visible: textExposureTime.visible
        fontColor: "#383838"
        z: 10
    }

    WzButton {
        id: buttonPlay
        enabled: root.cameraState === "cameraConnected" && root.previewState === "cameraPreviewStopped"
        circle: true
        anchors.verticalCenter: parent.verticalCenter
        anchors.right: parent.right
        anchors.rightMargin: 7
        height: 67
        width: 67
        normalColor: "transparent"
        hotColor: "transparent"
        downColor: "transparent"
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_play_normal.png"
        imageSourceHot: "qrc:/images/button_play_hot.png"
        imageSourceDown: "qrc:/images/button_play_down.png"
        onClicked: {
            startPreview()
        }
    }

    Rectangle {
        id: rectangleCameraTemperature
        width: 50
        height: width
        radius: width / 2
        visible: cameraState === "cameraConnected"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: visible ? -15 : 0
        opacity: visible ? 1 : 0
        color: parent.color


        Behavior on anchors.bottomMargin {
            NumberAnimation {duration: 1500}
        }
        Behavior on opacity {
            NumberAnimation {duration: 1500}
        }

        Text {
            id: textCameraTemperature
            anchors.verticalCenter: parent.verticalCenter
            anchors.verticalCenterOffset: 13
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: text < 0 ? -2 : 0
            font.family: "Arial"
            font.pixelSize: 13
        }
    }
}
