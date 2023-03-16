import QtQuick 2.12
import QtQuick.Controls 2.12
import "../WzControls"

Rectangle {
    id: advOptions
    anchors.bottom: parent.bottom
    width: parent.width
    height: 150
    color: "black"
    border.color: "white"
    border.width: 2

    signal close()

    Item {
        anchors.fill: parent
        anchors.margins: 10

        Text {
            id: textPidVid
            text: qsTr("PID-VID:")
            color: "#dddddd"
            font.pixelSize: 20
            anchors.verticalCenter: textFieldPidVid.verticalCenter
        }
        TextField {
            id: textFieldPidVid
            height: 43
            anchors.left: textPidVid.right
            anchors.top: parent.top
            anchors.topMargin: 0
            anchors.leftMargin: 20
            anchors.right: parent.right
        }

        WzButton {
            id: buttonCancel
            text: qsTr("取消")
            anchors.right: buttonOk.left
            anchors.rightMargin: 10
            anchors.bottom: parent.bottom
            label.font.pixelSize: 20
            width: 95
            height: 43
            radius: 3
            onClicked: {
                close()
            }
        }

        WzButton {
            id: buttonOk
            text: qsTr("确定")
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            label.font.pixelSize: 20
            width: 95
            height: 43
            radius: 3
            onClicked: {
                dbService.saveStrOption("vid_pid", textFieldPidVid.text)
                close()
            }
        }
    }

    Component.onCompleted: {
        textFieldPidVid.text = dbService.readStrOption("vid_pid", "59A1,17D4")
    }
}
