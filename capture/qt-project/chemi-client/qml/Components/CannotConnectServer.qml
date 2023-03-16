import QtQuick 2.12

import "../WzControls"

Item {
    id: root
    anchors.fill: parent
    state: "hide"

    signal deviceBusyRetry()
    signal deviceBusyExit()

    states: [
        State {
            name: "hide"
            PropertyChanges {
                target: root
                visible: false
            }
            PropertyChanges {
                target: itemDeviceBusy
                visible: false
            }
        },
        State {
            name: "deviceBusy"
            PropertyChanges {
                target: root
                visible: true
            }
            PropertyChanges {
                target: itemDeviceBusy
                visible: true
            }
            PropertyChanges {
                target: buttonDeviceBusyRetry
                enabled: true
            }
            StateChangeScript {
                script: {
                    textDeviceBusyAnimation.start(qsTr("已有人在使用仪器, 请等待其他人用完后再重试"))
                }
            }
        },
        State {
            name: "getAliveCount"
            PropertyChanges {
                target: root
                visible: true
            }
            PropertyChanges {
                target: itemDeviceBusy
                visible: true
            }
            PropertyChanges {
                target: buttonDeviceBusyRetry
                enabled: false
            }
            StateChangeScript {
                script: {
                    textDeviceBusyAnimation.start(qsTr("正在重试"))
                }
            }
        }
    ]

    Rectangle {
        anchors.fill: parent
        color: "black"
        opacity: 0.9
    }

    Rectangle {
        id: rectBorder
        anchors {
            fill: parent
            margins: 50
        }
        color: "transparent"
        border {
            color: "#bbbbbb"
            width: 5
        }
        radius: 5
    }

    Item {
        id: itemDeviceBusy
        anchors {
            fill: rectBorder
            margins: 10
        }
        Text {
            id: textDeviceBusy
            wrapMode: Text.WordWrap
            styleColor: "#ffffff"
            clip: true
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
                verticalCenterOffset: -(buttonDeviceBusyRetry.height * 2)
            }
            horizontalAlignment: Text.Center
            width: parent.width
            color: "#eeeeee"
            font {
                pointSize: 25
            }
        }
        WzTextAnimation {
            id: textDeviceBusyAnimation
            target: textDeviceBusy
        }

        Row {
            id: rowDeviceBusyButtons
            anchors {
                horizontalCenter: parent.horizontalCenter
                verticalCenter: parent.verticalCenter
                verticalCenterOffset: height * 2
            }
            spacing: 5

            WzButton {
                id: buttonDeviceBusyRetry
                width: 120
                height: 45
                radius: 5
                text: qsTr("重试")
                label.font.pointSize: 25
                onClicked: {
                    deviceBusyRetry()
                }
            }

            WzButton {
                id: buttonDeviceBusyExit
                width: buttonDeviceBusyRetry.width
                height: buttonDeviceBusyRetry.height
                radius: 5
                text: qsTr("退出")
                label.font.pointSize: 25
                onClicked: deviceBusyExit()
            }
        }
    }
}
