import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
    property alias value: control.value

    signal switchAperture(int apertureIndex)

    opacity: enabled ? 1 : 0.4

    Text {
        id: title
        text: qsTr("光圈")
        font.pixelSize: 19
        color: "#b4b4b4"
        x: 4
    }

    Slider {
        id: control
        font.family: "Times New Roman"
        from: 0
        to: 6
        stepSize: 1
        anchors.top: title.bottom
        anchors.topMargin: 5
        padding: 0

        background: Rectangle {
            x: control.leftPadding + 6
            y: control.topPadding + control.availableHeight / 2 - height / 2
            implicitWidth: 305
            implicitHeight: 4
            width: control.availableWidth - 12
            height: implicitHeight
            radius: 2
            color: "#b4b4b4"

            Rectangle {
                width: control.visualPosition * parent.width
                height: parent.height
                color: "#33acf5"
                radius: 2
            }
        }

        handle: Rectangle {
            x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
            y: control.topPadding + control.availableHeight / 2 - height / 2
            implicitWidth: 22
            implicitHeight: 22
            radius: 11
            color: control.pressed ? "#009be3" : "#33acf5"
        }

        onValueChanged: {
            switchAperture(value)
        }
    }

    Row {
        id: row
        x: 6
        anchors.top: control.bottom
        anchors.topMargin: 6
        height: 19
        width: control.implicitWidth
        Repeater {
            model: 7
            Text {
                text: index + 1
                width: 47
                font.pointSize: 12
                color: "#bebebe"
            }
        }
    }

    Item {
        id: row1
        height: 25
        width: control.width
        anchors.top: row.bottom
        anchors.topMargin: 8
        anchors.horizontalCenter: control.horizontalCenter

        Text {
            color: "#bebebe"
            text: qsTr("暗")
            anchors.left: parent.left
            anchors.leftMargin: 4
        }
        Text {
            id: textLight
            text: qsTr("亮")
            anchors.rightMargin: 3
            anchors.right: parent.right
            color: "#bebebe"
        }
    }

}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
