import QtQuick 2.12

MouseArea {
    id: root

    property var target
    property int targetY
    property int jumpHeight
    property bool repeat: false
    property bool stop: true

    anchors.fill: parent
    hoverEnabled: true
    propagateComposedEvents: true

    onEntered: {
        if (timer1.running) return
        if (timer2.running) return
        root.stop = false
        n1.start()
        timer1.start()
        if (repeat)
            timer2.running = true
    }
    onExited: {
        root.stop = true
    }

    onClicked: mouse.accepted = false
    onDoubleClicked: mouse.accepted = false
    onPositionChanged: mouse.accepted = false
    onPressAndHold: mouse.accepted = false
    onPressed: mouse.accepted = false
    onReleased: mouse.accepted = false

    NumberAnimation {
        id: n1
        target: root.target
        properties: "y"
        duration: 400
        from: targetY
        to: targetY - jumpHeight
        easing.type: Easing.OutQuad
    }

    NumberAnimation {
        id: n2
        target: root.target
        properties: "y"
        duration: 1500
        from: targetY - jumpHeight
        to: targetY
        easing.type: Easing.OutBounce
        easing.amplitude: 3
        easing.overshoot: 5
    }

    Timer {
        id: timer1
        repeat: false
        running: false
        interval: 400
        onTriggered: {
            timer1.running = false
            n2.start()
        }
    }

    Timer {
        id: timer2
        repeat: root.repeat
        running: false
        interval: 2500
        onTriggered: {
            if (root.stop) {
                timer2.running = false
                return
            }
            n1.start()
            timer1.start()
        }
    }
}
