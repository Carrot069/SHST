import QtQuick 2.12
import QtQuick.Controls 2.12

import WzI18N 1.0

Rectangle {
    property alias value: control.value

    signal switchFocus(int focusIndex)

    opacity: enabled ? 1 : 0.4
    height: 70
    width: 459
    color: "black"

    Text {
        id: title
        text: qsTr("焦距")
        anchors.verticalCenterOffset: {
            switch(WzI18N.language) {
            case "zh": return 0
            case "en": return -2
            }
        }
        anchors.left: parent.left
        anchors.leftMargin: {
            switch(WzI18N.language) {
            case "zh": return 0
            case "en": return 30
            }
        }
        font.pixelSize: 19
        color: "#b4b4b4"
        anchors.verticalCenter: parent.verticalCenter
    }

    Item {
        id: rowLabel
        height: 25
        width: control.width
        anchors.right: parent.right

        Text {
            color: "#bebebe"
            text: qsTr("小")
            anchors.left: parent.left
            anchors.leftMargin: 4            
        }
        Text {
            id: textLight
            text: qsTr("大")
            anchors.rightMargin: 4
            anchors.right: parent.right
            color: "#bebebe"
        }
    }

    Slider {
        id: control
        font.family: "Times New Roman"
        from: 0
        to: 6
        stepSize: 1
        value: 5

        anchors.left: rowLabel.left
        anchors.leftMargin: 0
        anchors.top: rowLabel.bottom
        anchors.topMargin: -3
        padding: 0

        background: Rectangle {
            x: control.leftPadding + 6
            y: control.topPadding + control.availableHeight / 2 - height / 2
            implicitWidth: {
                switch(WzI18N.language) {
                case "zh": return 400
                case "en": return 370
                }
            }
            implicitHeight: 4
            width: control.availableWidth - 12
            height: implicitHeight
            radius: 2
            color: "#b4b4b4"

            Rectangle {
                width: control.visualPosition * parent.width
                height: parent.height
                color: "#33acf5"
                radius: 2
            }
        }

        handle: Rectangle {
            x: control.leftPadding + control.visualPosition * (control.availableWidth - width)
            y: control.topPadding + control.availableHeight / 2 - height / 2
            implicitWidth: 22
            implicitHeight: 22
            radius: 11
            color: control.pressed ? "#009be3" : "#33acf5"
        }

        onValueChanged: {
            switchFocus(value)
        }
    }

    Row {
        id: rowNumber
        anchors.horizontalCenter: control.horizontalCenter
        anchors.top: control.bottom
        anchors.topMargin: 6
        height: 19
        anchors.horizontalCenterOffset: 8
        width: control.implicitWidth
        Repeater {
            model: 7
            Text {
                text: index + 1
                width: {
                    switch(WzI18N.language) {
                    case "zh": return 63
                    case "en": return 57
                    }
                }
                font.pointSize: 12
                color: "#bebebe"
            }
        }
    }


}


