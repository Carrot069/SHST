import QtQuick 2.12
import QtQuick.Controls 2.12

CheckBox {
    id: control
    text: qsTr("CheckBox")
    spacing: 5

    property int checkBoxFontPixelSize: 24

    indicator: Item {
        implicitWidth: 26
        implicitHeight: 26
        x: control.leftPadding
        y: parent.height / 2 - height / 2 + 2

        Text {
            anchors.fill: parent
            color: "#828282"
            font.family: "Wingdings"
            font.weight: Font.Light
            text: control.checked ? qsTr("þ") : qsTr("o")
            font.pixelSize: checkBoxFontPixelSize
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
        }
    }

    contentItem: Text {
        text: control.text
        font: control.font
        opacity: enabled ? 1.0 : 0.3
        color: "#828282"
        verticalAlignment: Text.AlignVCenter
        leftPadding: control.indicator.width + control.spacing
    }
}
