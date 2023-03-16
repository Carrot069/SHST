import QtQuick 2.12
import QtQuick.Window 2.12
import QtQuick.Controls 2.12
import "qrc:/../chemi/qml/WzControls"

Window {
    id: window
    width: 480
    height: 220
    visible: true
    title: qsTr("软件异常终止")
    flags: Qt.Dialog

    Rectangle {
        color: "black"
        anchors.fill: parent
    }

    Text {
        id: textInstruction
        anchors.verticalCenter: parent.verticalCenter
        anchors.verticalCenterOffset: -btnReboot.height
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.right: parent.right
        anchors.rightMargin: 20
        wrapMode: Text.Wrap
        text: qsTr("软件异常终止, 相关错误信息已保存到桌面.\n您可以将其发送给我们，以便我们解决此问题.")
        font.pointSize: 14
        color: "white"
    }

    WzButton {
        id: btnExit
        width: 90
        height: 35
        text: qsTr("退出")
        label.font.pointSize: 13
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 5
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -width
        radius: 3
        onClicked: window.close()
    }

    WzButton {
        id: btnReboot
        width: 90
        height: 35
        text: qsTr("重启")
        label.font.pointSize: 13
        anchors.bottom: parent.bottom
        anchors.bottomMargin: parent.height / 5
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: width
        radius: 3
        onClicked: {
            btnReboot.enabled = false
            btnReboot.text = qsTr("请稍等")
            startProcess.startProcess()
            tmrCloseWindow.running = true
        }
    }

    Timer {
        id: tmrCloseWindow
        repeat: false
        interval: 5000
        running: false
        onTriggered: {
            window.close()
        }
    }
}
