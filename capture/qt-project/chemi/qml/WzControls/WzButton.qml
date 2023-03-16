import QtQuick 2.12
// 2019-8-18 09:49:39

Rectangle {
    id: root
    state: "exited"
    color: "transparent"

    signal clicked()
    signal pressed(var mouse)
    signal released()

    property bool checkable: false
    property bool checked: false
    property color normalColor: "#1e1e1e"
    property color hotColor: "#0f6add"
    property color downColor: "#0f25c9"
    property color normalFontColor: "#b4b4b4"
    property color hotFontColor: normalFontColor
    property color downFontColor: normalFontColor
    property color checkedFontColor: "white"
    property alias box: buttonBox
    property alias label: buttonLabel
    property alias text: buttonLabel.text
    property alias label2: buttonLabel2
    property bool circle: false
    property alias image: buttonImage
    property alias imageHot: buttonImageHot
    property alias imageDown: buttonImageDown
    property alias imageVisible: buttonImage.visible
    property alias imageSourceNormal: buttonImage.source
    property alias imageSourceHot: buttonImageHot.source
    property alias imageSourceDown: buttonImageDown.source
    property alias imageSourceChecked: buttonImageChecked.source
    property int imageAlign: Qt.AlignCenter

    radius: circle ? Math.max(root.height, root.width) : root.radius

    Rectangle {
        id: buttonBox
        color: checkable ? "transparent" : "#0fa5dd"
        anchors.fill: parent
        anchors.margins: 0
        radius: root.radius
    }

    Image {
        id: buttonImage
        visible: imageVisible
        antialiasing: true
        anchors.verticalCenter: root.verticalCenter
        anchors.horizontalCenter: root.horizontalCenter
    }
    Image {
        id: buttonImageHot
        visible: imageVisible
        antialiasing: true
        opacity: 0
        anchors.verticalCenter: root.verticalCenter
        anchors.horizontalCenter: root.horizontalCenter
    }
    Image {
        id: buttonImageDown
        visible: imageVisible
        antialiasing: true
        opacity: 0
        anchors.verticalCenter: root.verticalCenter
        anchors.horizontalCenter: root.horizontalCenter
    }
    Image {
        id: buttonImageChecked
        antialiasing: true
        opacity: 0
        anchors.verticalCenter: root.verticalCenter
        anchors.horizontalCenter: root.horizontalCenter
    }

    Text {
        id: buttonLabel
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        color: normalFontColor
    }

    Text {
        id: buttonLabel2
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        verticalAlignment: Text.AlignVCenter
        horizontalAlignment: Text.AlignHCenter
        color: normalFontColor
    }

    MouseArea {
        anchors.fill: parent
        hoverEnabled: true

        onEntered: {
            if (checkable) return
            if (!root.enabled) return
            root.state = "entered"
        }
        onExited: {
            if (checkable) return
            if (!root.enabled) return
            root.state = "exited"
        }
        onPressed: {
            root.pressed(mouse)
            if (checkable) return
            if (!root.enabled) return
            root.state = "pressed"
        }
        onReleased: {
            root.released()
            if (checkable) return
            if (!root.enabled) return
            if (root.state === "pressed")
                root.state = "released"
        }

        onClicked: {
            if (checkable) {
                checked = !checked
            }
            root.clicked(mouse)
        }
    }

    onFocusChanged: {
        if (checkable) return
        if (!enabled) return
        if(focus)
            root.state = "entered"
    }

    onEnabledChanged: {
        if (!enabled) {
            state = "disabled"
        } else {
            state = "exited"
        }
    }

    states: [
        State {
            name: "entered"
            PropertyChanges {
                target: buttonBox
                color: hotColor
                opacity: 1
            }
            PropertyChanges {
                target: label
                color: hotFontColor
                opacity: 1
            }
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
                target: buttonBox
                color: normalColor
                opacity: 1
            }
            PropertyChanges {
                target: label
                color: normalFontColor
                opacity: 1
            }
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
                target: buttonBox
                color: downColor
                opacity: 1
            }
            PropertyChanges {
                target: label
                color: downFontColor
                opacity: 1
            }
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
                target: buttonBox
                color: hotColor
                opacity: 1
            }
            PropertyChanges {
                target: buttonLabel
                opacity: 1
            }
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
        },
        State {
            name: "checked"
            PropertyChanges {
                target: buttonImage
                opacity: 0
            }
            PropertyChanges {
                target: buttonImageChecked
                opacity: 1
            }
            PropertyChanges {
                target: buttonLabel
                color: checkedFontColor
                opacity: 1
            }
            PropertyChanges {
                target: buttonLabel2
                color: checkedFontColor
                opacity: 1
            }
        },
        State {
            name: "unchecked"
            PropertyChanges {
                target: buttonImageChecked
                opacity: 0
            }
            PropertyChanges {
                target: buttonImage
                opacity: 1
            }
            PropertyChanges {
                target: buttonLabel
                color: normalFontColor
                opacity: 1
            }
            PropertyChanges {
                target: buttonLabel2
                color: normalFontColor
                opacity: 1
            }
        },
        State {
            name: "disabled"
            PropertyChanges {
                target: buttonBox
                color: normalColor
                opacity: 0.4
            }
            PropertyChanges {
                target: label
                color: normalFontColor
                opacity: 0.4
            }
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
                opacity: 0.4
            }
        }
    ]

    transitions: [

        Transition {
            from: "*"
            to: "*"

            ColorAnimation {
                target: buttonBox
                duration: 200
            }
            NumberAnimation {
                target: buttonImage
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: 200
            }
            NumberAnimation {
                target: buttonImageHot
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: 200
            }
            NumberAnimation {
                target: buttonImageDown
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: 200
            }
            ColorAnimation {
                target: label
                duration: 200
            }
        },
        Transition {
            from: "unchecked"
            to: "checked"

            ColorAnimation {
                target: buttonLabel
                duration: 500
            }
            ColorAnimation {
                target: buttonLabel2
                duration: 500
            }
            NumberAnimation {
                target: buttonImage
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: 300
            }
            NumberAnimation {
                target: buttonImageChecked
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: 1000
            }
        },
        Transition {
            from: "checked"
            to: "unchecked"

            ColorAnimation {
                target: buttonLabel
                duration: 500
            }
            ColorAnimation {
                target: buttonLabel2
                duration: 500
            }
            NumberAnimation {
                target: buttonImage
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: 1000
            }
            NumberAnimation {
                target: buttonImageChecked
                properties: "opacity"
                easing.type: Easing.InOutQuad
                duration: 300
            }
        }
    ]

    onImageAlignChanged: {
        if ((imageAlign & Qt.AlignCenter) === Qt.AlignCenter) {
            buttonImage.anchors.verticalCenter = root.verticalCenter
            buttonImage.anchors.horizontalCenter = root.horizontalCenter
            buttonImageChecked.anchors.verticalCenter = root.verticalCenter
            buttonImageChecked.anchors.horizontalCenter = root.horizontalCenter
            buttonImageHot.anchors.verticalCenter = root.verticalCenter
            buttonImageHot.anchors.horizontalCenter = root.horizontalCenter
        } else {
            if ((imageAlign & Qt.AlignRight) === Qt.AlignRight) {
                buttonImage.anchors.horizontalCenter = undefined
                buttonImage.anchors.left = undefined
                buttonImage.anchors.right = root.right
                buttonImageChecked.anchors.horizontalCenter = undefined
                buttonImageChecked.anchors.left = undefined
                buttonImageChecked.anchors.right = root.right
                buttonImageHot.anchors.horizontalCenter = undefined
                buttonImageHot.anchors.left = undefined
                buttonImageHot.anchors.right = root.right
            } else if ((imageAlign & Qt.AlignLeft) === Qt.AlignLeft) {
                buttonImage.anchors.horizontalCenter = undefined
                buttonImage.anchors.right = undefined
                buttonImage.anchors.left = root.left
                buttonImageChecked.anchors.horizontalCenter = undefined
                buttonImageChecked.anchors.right = undefined
                buttonImageChecked.anchors.left = root.left
                buttonImageHot.anchors.horizontalCenter = undefined
                buttonImageHot.anchors.right = undefined
                buttonImageHot.anchors.left = root.left
            } else if ((imageAlign & Qt.AlignHCenter) === Qt.AlignHCenter) {
                buttonImage.anchors.horizontalCenter = root.horizontalCenter
                buttonImageChecked.anchors.horizontalCenter = root.horizontalCenter
                buttonImageHot.anchors.horizontalCenter = root.horizontalCenter
            }

            if ((imageAlign & Qt.AlignTop) === Qt.AlignTop) {
                buttonImage.anchors.verticalCenter = undefined
                buttonImage.anchors.bottom = undefined
                buttonImage.anchors.top = root.top
                buttonImageChecked.anchors.verticalCenter = undefined
                buttonImageChecked.anchors.bottom = undefined
                buttonImageChecked.anchors.top = root.top
                buttonImageHot.anchors.verticalCenter = undefined
                buttonImageHot.anchors.bottom = undefined
                buttonImageHot.anchors.top = root.top
            } else if ((imageAlign & Qt.AlignBottom) === Qt.AlignBottom) {
                buttonImage.anchors.bottom = root.bottom
                buttonImageChecked.anchors.bottom = root.bottom
                buttonImageHot.anchors.bottom = root.bottom
            } else if ((imageAlign & Qt.AlignVCenter) === Qt.AlignVCenter) {
                buttonImage.anchors.verticalCenter = root.verticalCenter
                buttonImageChecked.anchors.verticalCenter = root.verticalCenter
                buttonImageHot.anchors.verticalCenter = root.verticalCenter
            }
        }
    }

    onCheckedChanged: {
        if (checkable && checked) {
            root.state = "checked"               
        } else if (checkable && !checked) {
            root.state = "unchecked"
        }
    }

    onCheckableChanged: {
        if (checkable && checked) {
            root.state = "checked"
        } else if (checkable && !checked) {
            root.state = "unchecked"
            if (buttonImage.source !== imageSourceNormal)
                buttonImage.source = imageSourceNormal
        }
    }
}
