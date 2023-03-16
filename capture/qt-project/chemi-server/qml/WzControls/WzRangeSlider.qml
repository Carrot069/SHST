import QtQuick 2.12
import QtQuick.Controls 2.12

RangeSlider {
    id: control
    padding: 0

    background: Rectangle {
        x: control.leftPadding
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 200
        implicitHeight: 3
        width: control.availableWidth
        height: implicitHeight
        color: "#b4b4b4"

        Rectangle {
            x: control.first.visualPosition * parent.width
            width: control.second.visualPosition * parent.width - x
            height: parent.height
            color: "#009be3"
        }
    }

    first.handle: Rectangle {
        x: control.leftPadding + first.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 20
        implicitHeight: 20
        radius: 10
        color: first.pressed ? "#0085cb" : "#009be3"
    }

    second.handle: Rectangle {
        x: control.leftPadding + second.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 20
        implicitHeight: 20
        radius: 10
        color: second.pressed ? "#0085cb" : "#009be3"
    }
}
