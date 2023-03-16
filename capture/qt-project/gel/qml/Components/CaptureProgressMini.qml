import QtQuick 2.12
import "../WzControls"
import QtQuick.Controls 2.12

Item {
    id: root
    height: 120
    width: 120

    property int progress: 90
    property bool isProgress: true // 是否为进度条模式, 非进度条模式显示 Busy图标 和文本内容
    property alias text: textStatus.text
    property alias leftTime: textLeftTime.text
    property alias elapsedTime: textElapsedTime.text
    property color fontColor: "#303030"
    property alias buttonCancel2: buttonCancel2
    property alias countProgress: textProgress2.text // 数量进度, 多帧拍摄时用到, 单帧拍摄时设为空串
    signal cancel()
    signal back()

    onIsProgressChanged: {
        console.log("CaptureProgressMini,isProgress = ", isProgress)
    }

    onProgressChanged: {
        textProgress.text = root.progress + "%"
    }

    Rectangle {
        id: rectProgressBackground
        width: parent.width
        height: 25
        color: "#262626"
        anchors.topMargin: 10
        anchors.top: textStatus.bottom
        visible: isProgress
    }

    Rectangle {
        id: rectProgressForeground
        anchors.top: rectProgressBackground.top
        width: Math.min(rectProgressBackground.width, rectProgressBackground.width * progress / 100)
        color: "#000000"
        visible: isProgress
        Behavior on width {
            NumberAnimation {
                duration: 200
            }
        }
        height: rectProgressBackground.height
    }

    Text {
        id: textElapsedTime
        anchors.left: rectProgressBackground.left
        anchors.top: rectProgressBackground.bottom
        font.family: "Digital Dismay"
        font.pixelSize: 17
        color: root.fontColor
        text: "01:00"
        anchors.topMargin: 7
        visible: isProgress
    }

    Text {
        id: textLeftTime
        anchors.right: rectProgressBackground.right
        anchors.top: rectProgressBackground.bottom
        horizontalAlignment: Text.AlignRight
        font.family: "Digital Dismay"
        font.pixelSize: 17
        color: fontColor
        text: "01:00"
        anchors.topMargin: 7
        visible: isProgress
    }

    Text {
        id: textProgress
        anchors.verticalCenter: rectProgressBackground.verticalCenter
        anchors.horizontalCenter: rectProgressBackground.horizontalCenter
        anchors.horizontalCenterOffset: 5
        color: "#7f7f7f"
        font.pixelSize: 17
        font.family: "Arial"
        text: root.progress + "%"
        visible: isProgress
    }

    Text {
        id: textProgress2
        font.pixelSize: 13
        font.family: "Arial"
        color: root.fontColor
        text: "10/10"
        anchors.verticalCenterOffset: 0
        anchors.verticalCenter: textStatus.verticalCenter
        anchors.right: parent.right
        visible: isProgress
        horizontalAlignment: Text.AlignRight
    }

    WzButton {
        id: buttonReturn
        width: 20
        height: 20
        visible: isProgress
        normalColor: "transparent"
        hotColor: "transparent"
        downColor: "transparent"
        anchors.left: parent.left
        anchors.leftMargin: -3
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -5
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_back_7d7d7d.svg"
        image.sourceSize.width: 13
        image.sourceSize.height: 13
        imageSourceHot: "qrc:/images/button_back_555555.svg"
        imageHot.sourceSize.width: 13
        imageHot.sourceSize.height: 13
        imageSourceDown: "qrc:/images/button_back_222222.svg"
        imageDown.sourceSize.width: 13
        imageDown.sourceSize.height: 13
        onClicked: back()
    }

    WzButton {
        id: buttonCancel1
        width: 20
        height: 20
        visible: isProgress
        normalColor: "transparent"
        hotColor: "transparent"
        downColor: "transparent"
        anchors.right: parent.right
        anchors.rightMargin: -3
        anchors.bottom: parent.bottom
        anchors.bottomMargin: -5
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_cancel_7d7d7d.svg"
        image.sourceSize.width: 12
        image.sourceSize.height: 12
        imageSourceHot: "qrc:/images/button_cancel_555555.svg"
        imageHot.sourceSize.width: 12
        imageHot.sourceSize.height: 12
        imageSourceDown: "qrc:/images/button_cancel_222222.svg"
        imageDown.sourceSize.width: 12
        imageDown.sourceSize.height: 12
        onClicked: cancel()
    }

    WzBusyIndicator {
        id: busyIndicator
        width: 60
        height: 60
        color: "#7d7d7d"
        visible: !isProgress
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: 5
        anchors.horizontalCenter: parent.horizontalCenter

        WzButton {
            id: buttonCancel2
            width: 24
            height: 24
            normalColor: "transparent"
            hotColor: "transparent"
            downColor: "transparent"
            radius: 12
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_cancel_7d7d7d.svg"
            image.sourceSize.width: 12
            image.sourceSize.height: 12
            imageSourceHot: "qrc:/images/button_cancel_555555.svg"
            imageHot.sourceSize.width: 12
            imageHot.sourceSize.height: 12
            imageSourceDown: "qrc:/images/button_cancel_222222.svg"
            imageDown.sourceSize.width: 12
            imageDown.sourceSize.height: 12
            onClicked: cancel()
        }
    }

    Text {
        id: textStatus
        anchors.top: parent.top
        width: parent.width
        horizontalAlignment: textProgress2.visible && textProgress2.text !== "" ? Text.AlignLeft : Text.AlignHCenter
        text: qsTr("正在初始化")
        visible: !isProgress
        font.pointSize: textProgress2.visible && textProgress2.text !== "" ? 9 : 11
        color: root.fontColor
    }
}
