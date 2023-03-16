import QtQuick 2.12
import QtQuick.Controls 2.12

Popup {
    property int buttonWidth: 50
    property int buttonHeight: 40
    contentWidth: grid.implicitWidth
    contentHeight: grid.implicitHeight

    signal keyNumber(string number)
    signal ok()
    signal backspace()

    background: Rectangle {
        color: "#333333"
        radius: 10
    }

    Grid {
        id: grid
        columns: 3
        rows: 4
        spacing: 3

        WzButton {
            text: "1"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            text: "2"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            text: "3"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            text: "4"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            text: "5"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            text: "6"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            text: "7"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            text: "8"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            text: "9"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            text: "0"
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            onClicked: keyNumber(text)
        }
        WzButton {
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_ok.svg"
            onClicked: ok()
        }
        WzButton {
            width: buttonWidth
            height: buttonHeight
            radius: 5
            label.font.pixelSize: 20
            imageVisible: true
            imageSourceNormal: "qrc:/images/button-backspace.svg"
            onClicked: backspace()
        }
    }
}
