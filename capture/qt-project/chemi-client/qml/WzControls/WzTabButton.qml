import QtQuick 2.12
import QtQuick.Controls 2.12

TabButton {
    id: control
    font.pixelSize: 15
    height: 50

    property color backgroundColor: "#313131"
    property color fontColor: "#b4b4b4"
    property color fontColorChecked: "#b4b4b4"

    contentItem: Text {
        text: control.text
        font: control.font
        anchors.fill: parent
        color: control.checked ? control.fontColorChecked : control.fontColor
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        elide: Text.ElideRight
    }

    background: Rectangle {
        anchors.bottom: control.bottom
        color: control.backgroundColor
        height: control.checked ? 50 : 0
        Behavior on height {
            NumberAnimation {
                duration: 300
            }
        }
    }
}
