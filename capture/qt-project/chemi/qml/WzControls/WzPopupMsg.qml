import QtQuick 2.12
import QtQuick.Controls 2.12

Popup {
    function showMsg(msg) {
        textMsg.text = msg
        timerAutoClose.tick = 100
        timerAutoClose.start()
        open()
    }

    visible: false

    background: Rectangle {
        radius: 5
        color: "black"
        opacity: 0.9
    }

    contentItem: Item {
        anchors.margins: 20
        Text {
            id: textMsg
            color: "white"
        }
    }

    Timer {
        id: timerAutoClose
        property int tick: 0
        running: false
        interval: 20
        repeat: true
        onTriggered: {
            if (tick <= 0) {
                running = false
                close()
            } else {
                tick = tick - 1
            }
        }
    }
}
