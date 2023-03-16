import QtQuick 2.12
import "../WzControls"
import QtQuick.Controls 2.12

Item {
    id: root
    height: 120
    width: 406

    property int progress: 90
    property bool isProgress: true // 是否为进度条模式, 非进度条模式显示 Busy图标 和文本内容
    property alias text: textStatus.text
    property alias leftTime: textLeftTime.text
    property alias elapsedTime: textElapsedTime.text
    property color fontColor: "#111111"
    property alias buttonCancel2: buttonCancel2
    property alias countProgress: textProgress2.text // 数量进度, 多帧拍摄时用到, 单帧拍摄时设为空串
    signal cancel()

    onProgressChanged: {
        textProgress.text = root.progress + "%"
    }

    Rectangle {
        id: rectProgressBackground
        width: 372
        height: 34
        color: "#262626"
        visible: isProgress

        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -(textLeftTime.implicitHeight * 0.5)
    }

    Rectangle {
        id: rectProgressForeground
        anchors.top: rectProgressBackground.top
        width: Math.min(rectProgressBackground.width, rectProgressBackground.width * progress / 100)
        height: 34
        color: "#000000"
        visible: isProgress
        Behavior on width {
            NumberAnimation {
                duration: 200
            }
        }
    }

    Text {
        id: textElapsedTime
        anchors.left: rectProgressBackground.left
        anchors.top: rectProgressBackground.bottom
        font.family: "Digital Dismay"
        font.pixelSize: 30
        color: fontColor
        text: "01:00"
        visible: isProgress
    }

    Text {
        id: textLeftTime
        anchors.right: rectProgressBackground.right
        anchors.top: rectProgressBackground.bottom
        horizontalAlignment: Text.AlignRight
        font.family: "Digital Dismay"
        font.pixelSize: 30
        color: fontColor
        text: "01:00"
        visible: isProgress
    }

    Text {
        id: textProgress
        anchors.verticalCenter: rectProgressBackground.verticalCenter
        anchors.horizontalCenter: rectProgressBackground.horizontalCenter
        anchors.horizontalCenterOffset: 5
        color: "#7f7f7f"
        font.pixelSize: 20
        font.family: "Arial"
        text: root.progress + "%"
        visible: isProgress
    }

    Text {
        id: textProgress2
        anchors.top: rectProgressBackground.bottom
        anchors.topMargin: 5
        anchors.horizontalCenter: rectProgressBackground.horizontalCenter
        font.pixelSize: 20
        font.family: "Arial"
        color: fontColor
        text: "10/10"
        visible: isProgress
    }

    WzButton {
        id: buttonCancel1
        width: 34
        height: 34
        anchors.top: rectProgressBackground.top
        anchors.left: rectProgressBackground.right
        anchors.leftMargin: 1
        visible: isProgress
        label.font.family: "Webdings"
        label.font.pixelSize: 22
        text: "r"
        normalColor: rectProgressBackground.color
        label.color: "#8c8c8c"
        onClicked: cancel()
    }

    WzBusyIndicator {
        id: busyIndicator
        visible: !isProgress
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -textStatus.implicitHeight
        anchors.horizontalCenter: parent.horizontalCenter

        WzButton {
            id: buttonCancel2
            width: 34
            height: 34
            normalColor: "transparent"
            radius: 17
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            label.font.family: "Webdings"
            label.font.pixelSize: 25
            text: "r"
            onClicked: cancel()
        }
    }

    Text {
        id: textStatus
        anchors.top: busyIndicator.bottom
        anchors.topMargin: 10
        anchors.horizontalCenter: busyIndicator.horizontalCenter
        text: qsTr("正在初始化")
        font.pointSize: 13
        color: fontColor
        visible: !isProgress
    }
}
