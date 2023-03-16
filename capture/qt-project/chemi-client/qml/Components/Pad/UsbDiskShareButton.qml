import QtQuick 2.12
import QtWebSockets 1.1

import WzI18N 1.0

import "../../WzControls"

WzButton {
    id: buttonUsbDiskShare
    signal share(var options)
    text: qsTr("保存")
    width: {
        switch(WzI18N.language) {
        case "en": return 125
        default: return 105
        }
    }
    height: 43
    label.font.pixelSize: {
        switch(WzI18N.language) {
        case "en": return 15
        default: return 18
        }
    }
    label.anchors.horizontalCenterOffset: 13
    radius: 3
    imageVisible: true
    imageSourceNormal: "qrc:/images/button_udisk.svg"
    image.sourceSize.width: 24
    image.sourceSize.height: 24
    image.anchors.horizontalCenterOffset: -23

    Timer {
        id: wsTimeoutTimer
        interval: 5000
        repeat: false
        onTriggered: {
            ws.active = false
            msgBox.show(qsTr("和服务器通信超时"), qsTr("确定"))
        }
    }

    WebSocket {
        id: ws
        onStatusChanged: {
            if (status === WebSocket.Open) {
                var msg = {action: "getUsbDiskCount"}
                ws.sendTextMessage(JSON.stringify(msg))
            } else if (status == WebSocket.Error) {
                msgBox.show(ws.errorString, qsTr("确定"))
            }
        }
        onTextMessageReceived: {
            ws.active = false
            wsTimeoutTimer.stop()
            var msg = JSON.parse(message)
            if (msg) {
                if (msg.action === "getUsbDiskCount") {
                    if (msg.count === 0)
                        msgBox.show(qsTr("没有检测到U盘"), qsTr("确定"))
                    else
                        var options = {
                            usbDisks: msg.drives,
                            shareToUsbDisk: true
                        }
                        share(options)
                }
            }
        }
    }

    onClicked: {
        if (serverInfo.serverHost === "") {
            msgBox.show(qsTr("没有服务器主机信息"), qsTr("确定"))
            return
        }
        if (serverInfo.serverPort === 0) {
            msgBox.show(qsTr("没有服务器端口信息"), qsTr("确定"))
            return
        }
        if (ws.status === WebSocket.Closed || ws.status === WebSocket.Error) {
            ws.url = serverInfo.getFullWsUrl()
            ws.active = true
            wsTimeoutTimer.start()
            return
        } else {
            return
        }
    }
}
