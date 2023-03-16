import QtQuick 2.12

Rectangle {
    id: root
    state: "exited"
    color: "transparent"

    signal clicked()
    signal pressed()
    signal released()

    property alias image: buttonImage
    property alias imageHot: buttonImageHot
    property alias imageDown: buttonImageDown
    property alias imageSourceNormal: buttonImage.source
    property alias imageSourceHot: buttonImageHot.source
    property alias imageSourceDown: buttonImageDown.source
    property int animationDuration: 200

    Image {
        id: buttonImage
        antialiasing: true
        anchors.fill: parent
    }
    Image {
        id: buttonImageHot
        antialiasing: true
        opacity: 0
        anchors.fill: parent
    }
    Image {
        id: buttonImageDown
        antialiasing: true
        opacity: 0
        anchors.fill: parent
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onEntered: {
            root.state = "entered"
        }
        onExited: {
            root.state = "exited"
        }
        onPressed: {
            root.pressed()
            root.state = "pressed"
        }
        onReleased: {
            root.released()
            if (root.state === "pressed")
                root.state = "released"
        }

        onClicked: {
            root.clicked(mouse)
        }
    }

    onFocusChanged: {
        if(focus)
            root.state = "entered"
    }

    states: [
        State {
            name: "entered"
            PropertyChanges {
                target: buttonImageHot
                opacity: imageHot.source.toString() === "" ? 0 : 1
            }
            PropertyChanges {
                target: buttonImage
                opacity: imageHot.source.toString() === "" ? 1 : 0
            }
        },
        State {
            name: "exited"
            PropertyChanges {
                target: buttonImageHot
                opacity: 0
            }
            PropertyChanges {
                target: buttonImageDown
                opacity: 0
            }
            PropertyChanges {
                target: buttonImage
                opacity: 1
            }
        },
        State {
            name: "pressed"
            PropertyChanges {
                target: buttonImageHot
                opacity: 0
            }
            PropertyChanges {
                target: buttonImageDown
                opacity: buttonImageDown.source.toString() === "" ? 0 : 1
            }
            PropertyChanges {
                target: buttonImage
                opacity: buttonImageDown.source.toString() === "" ? 1 : 0
            }
        },
        State {
            name: "released"
            PropertyChanges {
                target: buttonImageHot
                opacity: buttonImageHot.source.toString() === "" ? 0 : 1
            }
            PropertyChanges {
                target: buttonImageDown
                opacity: 0
            }
            PropertyChanges {
                target: buttonImage
                opacity: buttonImageHot.source.toString() === "" ? 1 : 0
            }
        }
    ]

    transitions: [

        Transition {
            from: "*"
            to: "*"

            NumberAnimation {
                target: buttonImage
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: animationDuration
            }
            NumberAnimation {
                target: buttonImageHot
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: animationDuration
            }
            NumberAnimation {
                target: buttonImageDown
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: animationDuration
            }
        }
    ]
}
