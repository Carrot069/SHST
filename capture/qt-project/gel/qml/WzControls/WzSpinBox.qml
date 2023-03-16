import QtQuick 2.12
import QtQuick.Controls 2.12

SpinBox {
    height: 45
    width: 80
    id: control
    editable: true
    font.pixelSize: 30

    property bool isShowButton: false
    property int buttonHeight: 32
    property color buttonColor: "#535353"
    property color buttonColorDown: "#777777"
    property color buttonFontColor: "#535353"
    property color fontColor: "#dddddd"
    property color backgroundColor: "transparent"
    property color selectionColor: "white"
    property color selectedTextColor: "black"
    property bool isPaddingZero: true

    // private
    property int originalHeight
    property int originalY

    Component.onCompleted: {
        originalHeight = control.height
        originalY = control.y
    }

    Behavior on height {
        NumberAnimation {
            duration: 300
        }
    }
    Behavior on y {
        NumberAnimation {
            duration: 300
        }
    }

    onFocusChanged: {
        if (isShowButton) {
            if (focus) {
                control.height = originalHeight + control.buttonHeight + control.buttonHeight
                control.y = originalY - buttonHeight
            } else {
                control.height = originalHeight
                control.y = originalY
            }
        }
    }

    contentItem: TextInput {
        id: textInput
        z: 2
        anchors.fill: parent
        anchors.topMargin: control.focus && isShowButton ? control.buttonHeight : 0
        anchors.bottomMargin: control.focus && isShowButton ? control.buttonHeight : 0
        text: {
            var topValue = Math.max(control.from, control.to)
            var topValueStr = "" + topValue
            var zeroStr = ""
            var valueStr = control.value + ""

            if (isPaddingZero) {
                if (valueStr.length < topValueStr.length) {
                    for(var i = 0; i < topValueStr.length - valueStr.length; i++)
                        zeroStr += "0"
                }
            }

            return zeroStr + valueStr;
        }
        color: control.fontColor
        font: control.font
        horizontalAlignment: Qt.AlignHCenter
        verticalAlignment: Qt.AlignVCenter
        readOnly: !control.editable
        validator: control.validator
        inputMethodHints: Qt.ImhFormattedNumbersOnly
        selectByMouse: true
        selectionColor: control.selectionColor
        selectedTextColor: control.selectedTextColor
        wrapMode: Text.NoWrap
        onAccepted: {
            control.focus = false
        }
        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true
            onClicked: {
                textInput.forceActiveFocus()
                mouse.accepted = false
            }
            onPressed: {
                textInput.forceActiveFocus()
                mouse.accepted = false
            }
            onReleased: mouse.accepted = false
            cursorShape: Qt.IBeamCursor
            onWheel: {
                if (wheel.angleDelta.y > 0)
                    control.value = control.value + 1
                else
                    control.value = control.value - 1
            }
        }
    }

    up.indicator: Rectangle {
        visible: isShowButton
        Behavior on opacity {
            NumberAnimation {
                duration: 300
            }
        }

        radius: control.buttonHeight / 2
        opacity: control.focus ? 0.7 : 0
        height: control.buttonHeight
        width: control.buttonHeight
        x: (control.width - width) / 2
        color: control.up.pressed ? control.buttonColorDown : control.buttonColor

        Text {
            text: "+"
            font.pixelSize: control.font.pixelSize * 1.5
            color: control.buttonFontColor
            anchors.fill: parent
            anchors.bottomMargin: 1
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter            
        }
    }

    down.indicator: Rectangle {
        Behavior on opacity {
            NumberAnimation {
                duration: 300
            }
        }

        visible: isShowButton
        radius: control.buttonHeight / 2
        opacity: control.focus ? 0.7 : 0
        height: control.buttonHeight
        width: control.buttonHeight
        x: (control.width - width) / 2
        anchors.bottom: parent.bottom
        color: control.down.pressed ? control.buttonColorDown : control.buttonColor

        Text {
            text: "-"
            font.pixelSize: control.font.pixelSize * 1.5
            color: control.buttonFontColor
            anchors.fill: parent
            anchors.bottomMargin: 2
            fontSizeMode: Text.Fit
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }

    background: Rectangle {
        anchors.fill: parent
        color: control.backgroundColor
        opacity: control.focus ? 1 : 0
        Behavior on opacity {
            NumberAnimation {
                duration: 300
            }
        }
    }
}
