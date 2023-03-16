import QtQuick 2.12
import QtQuick.Controls 2.12

RangeSlider {
    id: control
    property int activePoint: 1
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

        Rectangle {
            color: "#008ce3"
            width: 10
            height: 10
            radius: 10
            anchors.centerIn: parent
            visible: control.activePoint === 0
        }
    }

    first.onPressedChanged: {
        if (first.pressed)
            activePoint = 0
    }

    second.handle: Rectangle {
        x: control.leftPadding + second.visualPosition * (control.availableWidth - width)
        y: control.topPadding + control.availableHeight / 2 - height / 2
        implicitWidth: 20
        implicitHeight: 20
        radius: 10
        color: second.pressed ? "#0085cb" : "#009be3"

        Rectangle {
            color: "#008ce3"
            width: 10
            height: 10
            radius: 10
            anchors.centerIn: parent
            visible: control.activePoint === 1
        }
    }

    second.onPressedChanged: {
        if (second.pressed)
            activePoint = 1
    }

    MouseArea {
        anchors.fill: parent
        propagateComposedEvents: true
        onClicked: mouse.accepted = false
        onDoubleClicked: mouse.accepted = false
        onPositionChanged: mouse.accepted = false
        onPressAndHold: mouse.accepted = false
        onPressed: mouse.accepted = false
        onReleased: mouse.accepted = false
        onWheel: {
            if (control.activePoint === 0)
                control.first.value += wheel.angleDelta.y
            else
                control.second.value += wheel.angleDelta.y
        }
    }
}
