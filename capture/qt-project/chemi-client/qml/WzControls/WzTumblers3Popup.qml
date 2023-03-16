import QtQuick 2.12
import QtQuick.Controls 2.12

Popup {
    id: root

    padding: 5

    property int value: 0
    property int count1: 10
    property int count2: 10
    property int tumblerWidth: 30
    property int tumblerHeight: 170
    property int fontPixelSize: 30
    property int visibleItemCount: 5

    onValueChanged: {
        tumbler1.currentIndex = value / 100
        tumbler2.currentIndex = (value - (tumbler1.currentIndex * 100)) / 10
        tumbler3.currentIndex = value % 10
    }

    enter: Transition {
        NumberAnimation { property: "opacity"; from: 0.0; to: 1.0 }
    }
    exit: Transition {
        NumberAnimation { property: "opacity"; from: 1.0; to: 0 }
    }
    background: Rectangle {
        border.width: 1
        border.color: "#222222"
        radius: 5
        color: "black"
    }
    contentItem: Item {
        width: tumblersRow.implicitWidth
        height: tumblersRow.implicitHeight
        clip: true

        Row {
            id: tumblersRow
            Tumbler {
                id: tumbler1
                model: 10
                delegate: delegateComponent
                width: tumblerWidth
                height: tumblerHeight
                visibleItemCount: root.visibleItemCount
                onMovingChanged: {
                    if (!moving)
                        value = tumbler1.currentIndex * 100 + tumbler2.currentIndex * 10 + tumbler3.currentIndex
                }
            }
            Tumbler {
                id: tumbler2
                model: 10
                delegate: delegateComponent
                width: tumblerWidth
                height: tumblerHeight
                visibleItemCount: root.visibleItemCount
                onMovingChanged: {
                    if (!moving)
                        value = tumbler1.currentIndex * 100 + tumbler2.currentIndex * 10 + tumbler3.currentIndex
                }
            }
            Tumbler {
                id: tumbler3
                model: 10
                delegate: delegateComponent
                width: tumblerWidth
                height: tumblerHeight
                visibleItemCount: root.visibleItemCount
                onMovingChanged: {
                    if (!moving)
                        value = tumbler1.currentIndex * 100 + tumbler2.currentIndex * 10 + tumbler3.currentIndex
                }
            }
        }
    }

    Component {
        id: delegateComponent

        Label {
            text: modelData
            opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2.2)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: "Digital Dismay"
            font.pixelSize: root.fontPixelSize
            color: "#dddddd"
        }
    }
}
