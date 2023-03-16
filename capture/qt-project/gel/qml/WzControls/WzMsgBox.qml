import QtQuick 2.12
import QtQuick.Controls 2.12

import WzUtils 1.0
import WzI18N 1.0

Item {

    id: root

    property int buttonCount: 3
    property int buttonClickedAt: -1

    signal buttonClicked(int buttonID)

    anchors.fill: parent
    visible: opacity > 0
    focus: true
    opacity: 0

    Behavior on opacity {
        NumberAnimation {
            duration: 300
        }
    }

    Keys.onPressed: {
        if (event.key === Qt.Key_Enter || event.key === Qt.Key_Return) {
            root.buttonClickedAt = 1
            root.opacity = 0
            buttonClicked(1)

        } else if (event.key === Qt.Key_Escape) {
            root.buttonClickedAt = 0
            root.opacity = 0
            buttonClicked(0)
        }
    }

    MouseArea {
        anchors.fill: parent
        onClicked: {
            if (root.buttonCount===0) {
                root.buttonClickedAt = 0
                root.opacity = 0
                buttonClicked(0)
            }
        }
    }

    Item {
        width: 640
        height: 320
        anchors.centerIn: parent

        Rectangle {
            anchors.fill: parent
            color: "black"
            opacity: 0.90
            border.color: "white"
            border.width: 3
            radius: 8
        }

        Label {
            id: labelMessage
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.bottom: rowButtons.top
            anchors.margins: 40
            font.bold: true
            font.pixelSize: 22
            font.family: WzI18N.font.family
            lineHeight: 1.5
            text: ""
            wrapMode: Text.WordWrap
            horizontalAlignment: Label.AlignHCenter
            verticalAlignment: Label.AlignVCenter
        }

        Row {
            id: rowButtons
            anchors.bottom: parent.bottom
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.bottomMargin: 50
            spacing: 20

            WzButton {
                id: button1
                width: 150
                height: 50
                label.text: "Button 1"
                label.font.pixelSize: 20
                radius: 3
                onClicked: {
                    root.buttonClickedAt = 1
                    root.opacity = 0
                    buttonClicked(1)
                }
            }

            WzButton {
                id: button2
                width: 150
                height: 50
                label.text: "Button 2"
                label.font.pixelSize: 20
                radius: 3
                onClicked: {
                    root.buttonClickedAt = 2
                    root.opacity = 0
                    buttonClicked(2)
                }
            }

            WzButton {
                id: button3
                width: 150
                height: 50
                label.text: "Button 3"
                label.font.pixelSize: 20
                radius: 3
                onClicked: {
                    root.buttonClickedAt = 3
                    root.opacity = 0
                    buttonClicked(3)
                }
            }
        }
    }


    function show(message, buttonText1, buttonText2, buttonText3) {

        root.buttonCount = 0

        if (buttonText1 !== undefined) {
            button1.visible = true
            button1.label.text = buttonText1
            root.buttonCount++
        } else {
            button1.visible = false
        }

        if (buttonText2 !== undefined) {
            button2.visible = true
            button2.label.text = buttonText2
            root.buttonCount++
        } else {
            button2.visible = false
        }

        if (buttonText3 !== undefined) {
            button3.visible = true
            button3.label.text = buttonText3
            root.buttonCount++
        } else {
            button3.visible = false
        }

        if (root.buttonCount > 0) {
            rowButtons.anchors.bottomMargin = 50
        } else {
            rowButtons.anchors.bottomMargin = 0
        }

        labelMessage.text = message

        root.buttonClickedAt = -1
        root.opacity = 0.9

        forceActiveFocus()
    }
}
