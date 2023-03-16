import QtQuick 2.12
import QtWebSockets 1.1

Item {
    id: root
    property int aliveCount: -1
    signal aliveCountResult()
    function connectToServer(url) {
        console.info("Heart.connectToServer", url)
        ws.url = url
        ws.active = true
    }
    function getAliveCount() {
        var msg = {
            getAliveCount: true
        }
        ws.sendTextMessage(JSON.stringify(msg))
    }
    function start() {
        timerHeart.running = true
    }    

    WebSocket {
        id: ws
        active: false
        onStatusChanged: {
            console.info("Heart.WebSocket.onStatusChanged", status)
            if (status === WebSocket.Open) {
                getAliveCount()
            }
        }
        onTextMessageReceived: {
            console.info("Heart.WebSocket.onTextMessageReceived,", message)
            var response = JSON.parse(message)
            if (response) {
                if (response.getAliveCount) {
                    console.info("Heart.getAliveCount result:", response.aliveCount)
                    root.aliveCount = response.aliveCount
                    aliveCountResult()
                }
            }
        }
    }

    Timer {
        id: timerHeart
        running: false
        interval: 3000
        repeat: true
        onTriggered: {
            if (ws.status === WebSocket.Open) {
                var msg = {
                    heart: true
                }
                ws.sendTextMessage(JSON.stringify(msg))
            }
        }
    }
}
