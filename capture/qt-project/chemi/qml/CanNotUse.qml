import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12

import WzI18N 1.0

import "WzControls"

ApplicationWindow {
    id: window
    visible: false
    width: 800
    height: 600

    Rectangle {
        anchors.fill: parent
        color: "black"
    }
    Rectangle {
        anchors.centerIn: parent
        width: 480
        height: 280
        color: "transparent"
        border.color: "#aaaaaa"
        border.width: 3
        radius: 5

        Text {
            id: explanationText
            anchors.centerIn: parent
            anchors.verticalCenterOffset: -height
            text: qsTr("本软件已经运行了一个，不能重复打开")
            font.pointSize: 15
            color: "#dddddd"
            horizontalAlignment: Text.AlignHCenter
            width: parent.width - parent.border.width * 2
            wrapMode: Text.WordWrap
        }

        WzButton {
            width: 120
            height: 40
            label.font.pointSize: 13
            text: qsTr("退出")
            anchors.top: explanationText.bottom
            anchors.topMargin: 30
            anchors.horizontalCenter: parent.horizontalCenter
            radius: 3
            onClicked: window.close()
        }
    }
    onClosing: {
        launcherHelper.exiting()
    }

    Component.onCompleted: {
        var wx = (Screen.desktopAvailableWidth - width) / 2
        var wy = (Screen.desktopAvailableHeight - height) / 2

        window.x = wx
        window.y = wy
        visible = true

        launcherHelper.started()
    }
}
