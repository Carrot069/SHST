import QtQuick 2.12
import QtQuick.Controls 2.12

ToolTip {
    id: control

    contentItem: Text {
        text: control.text
        font: control.font
        color: "#eeeeee"
    }

    background: Rectangle {
        radius: 5
        border.color: "gray"
        color: "black"
    }
}
