import QtQuick 2.12
import QtQuick.Controls 2.12

Switch {
    id: control
    checked: false

    property alias textControl: textControl
    property alias slideBar: slideBar

    indicator: Rectangle {
        width: control.width
        height: 29
        radius: 15
        color: "white"

        Text {
            id: textControl
            text: control.checked ? qsTr("ON") : qsTr("OFF")
            font.family: "Arial"
            font.pixelSize: 14
            color: control.checked ? "#00b7ee" : "#bebebe"
            anchors.verticalCenter: parent.verticalCenter
            x: control.checked ? 12 : 29

            Behavior on x {
                NumberAnimation {
                    duration: 300
                    easing.type: Easing.InOutQuad
                }
            }
            Behavior on color {
                ColorAnimation {
                    duration: 300
                    easing.type: Easing.InOutQuad
                }
            }
        }

        Rectangle {
            id: slideBar
            x: control.checked ? parent.width - width - 4 : 4
            y: 3
            width: 23
            height: 23
            radius: 12
            color: control.checked ? "#00b7ee" : "#bebebe"

            Behavior on x {
                NumberAnimation {
                    duration: 300
                }
            }
            Behavior on color {
                ColorAnimation {
                    duration: 300
                    easing.type: Easing.InOutQuad
                }
            }
        }
    }
}
