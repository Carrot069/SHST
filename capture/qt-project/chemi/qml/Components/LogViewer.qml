import QtQuick 2.12
import QtQuick.Controls 2.12

import Shst 1.0

Item {
    id: root

    property string position: "bottom"
    property bool datetime: true
    property string filter: ""

    Item {
        anchors {
            fill: parent
            margins: 10
        }
        Rectangle {
            anchors.fill: parent
            radius: 10
            color: "black"
            opacity: 0.8
        }

        ScrollView {
            id: view
            anchors {
                fill: parent
                margins: 20
            }

            TextArea {
                background: Item{ }
                id: textAreaLog
            }
        }
    }

    Connections {
        target: window
        onDebugCommand: {
            if (cmd === "log")
                root.visible = !root.visible
            else if (cmd === "log,top")
                root.position = "top"
            else if (cmd === "log,bottom")
                root.position = "bottom"
            else if (cmd === "log,datetime")
                root.datetime = !root.datetime
            else if (cmd.startsWith("log,filter,")) {
                root.filter = cmd.substring(11)
            }
            else {

            }
        }
    }

    ShstLogToQml {
        id: logToQml
        onMessage: {
            if (!root.visible)
                return
            if (root.filter != "" && !msg.startsWith(root.filter))
                return
            var msg1 = msg
            if (root.datetime) {
                var now = new Date()
                msg1 = "[" + nowStr() + "]" + msg1
            }
            textAreaLog.append(msg1)
        }
    }
}
