import QtQuick 2.12
import QtQuick.Controls 2.12

BusyIndicator {
    id: control
    width: 80
    height: 80
    padding: 0
    property int circleSize: 10
    property color color: "#000000"

    contentItem: Item {
        implicitWidth: control.width
        implicitHeight: control.height

        Item {
            id: item1
            width: control.width
            height: control.height
            opacity: control.running ? 1 : 0

            Behavior on opacity {
                OpacityAnimator {
                    duration: 250
                }
            }

            RotationAnimator {
                target: item1
                running: control.visible && control.running
                from: 0
                to: 360
                loops: Animation.Infinite
                duration: 7000
            }

            Repeater {
                id: repeater
                model: 10
                anchors.fill: parent

                Rectangle {
                    x: item1.width / 2 - width / 2
                    y: item1.height / 2 - height / 2
                    implicitWidth: circleSize
                    implicitHeight: circleSize
                    radius: circleSize / 2
                    color: control.color
                    transform: [
                        Translate {
                            y: -Math.min(item1.width, item1.height) * 0.5 + 5
                        },
                        Rotation {
                            angle: index / repeater.count * 360
                            origin.x: 5
                            origin.y: 5
                        }
                    ]
                }
            }
        }
    }
}
