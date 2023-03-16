// author wz, date: 2020-07-27 16:53:35
import QtQuick 2.12
import QtQuick.Controls 2.12

SpinBox {
    height: 45
    width: 80
    id: control
    editable: true
    font.pixelSize: 30

    property bool isShowButton: false
    property bool isAlwaysShowButton: false
    property bool isHorizontalButton: false
    property int buttonSize: 32
    property color buttonColor: "#535353"
    property color buttonColorHot: "#666666"
    property color buttonColorDown: "#777777"
    property color buttonFontColor: "#535353"
    property color fontColor: "#dddddd"
    property alias buttonFont: buttonDefaultFont.font
    property color backgroundColor: "transparent"
    property color selectionColor: "white"
    property color selectedTextColor: "black"
    property bool isPaddingZero: true

    // private
    property int originalHeight
    property int originalY
    property int originalWidth
    property int originalX

    Text {
        id: buttonDefaultFont
        font.pixelSize: 25
        font.family: "Arial"
    }

    Component.onCompleted: {
        originalHeight = control.height
        originalY = control.y
        originalWidth = control.width
        originalX = control.x
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

    Behavior on width {
        NumberAnimation {
            duration: 300
        }
    }
    Behavior on x {
        NumberAnimation {
            duration: 300
        }
    }

    contentItem: TextInput {
        z: 2
        anchors.fill: parent
        anchors.topMargin: up.indicator.opacity === 0 ? 0 : (isHorizontalButton ? 0 : control.buttonSize)
        anchors.bottomMargin: down.indicator.opacity === 0 ? 0 : (isHorizontalButton ? 0 : control.buttonSize)
        anchors.leftMargin: down.indicator.opacity === 0 ? 0 : (isHorizontalButton ? control.buttonSize : 0)
        anchors.rightMargin: up.indicator.opacity === 0 ? 0 : (isHorizontalButton ? control.buttonSize : 0)
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
            //control.focus = false
        }
        MouseArea {
            anchors.fill: parent
            propagateComposedEvents: true
            onClicked: mouse.accepted = false
            onPressed: mouse.accepted = false
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
        visible: isShowButton || isAlwaysShowButton
        Behavior on opacity {NumberAnimation {duration: 300}}
        Behavior on y {NumberAnimation {duration: 300}}
        Behavior on x {NumberAnimation {duration: 300}}

        radius: control.buttonSize / 2
        opacity: control.focus || isAlwaysShowButton ? 0.7 : 0
        height: control.buttonSize
        width: control.buttonSize
        x: isAlwaysShowButton || (isShowButton && control.focus) ?
               (isHorizontalButton ? control.width - width : (control.width - width) / 2) :
               (control.width - width) / 2
        y: isAlwaysShowButton || (isShowButton && control.focus) ?
               (isHorizontalButton ? (control.height - height) / 2 : 0) :
               (control.height - height) / 2
        color: control.up.pressed ? control.buttonColorDown : (control.up.hovered ? control.buttonColorHot : control.buttonColor)

        Text {
            text: "+"
            font: control.buttonFont
            color: control.buttonFontColor
            anchors.fill: parent
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter            
        }
    }

    down.indicator: Rectangle {
        visible: isShowButton || isAlwaysShowButton
        Behavior on opacity {NumberAnimation {duration: 300}}
        Behavior on y {NumberAnimation {duration: 300}}
        Behavior on x {NumberAnimation {duration: 300}}

        radius: control.buttonSize / 2
        opacity: control.focus || isAlwaysShowButton ? 0.7 : 0
        height: control.buttonSize
        width: control.buttonSize
        x: isAlwaysShowButton || (isShowButton && control.focus) ?
               (isHorizontalButton ? 0 : (control.width - width) / 2) :
               (control.width - width) / 2
        y: isAlwaysShowButton || (isShowButton && control.focus) ?
               (isHorizontalButton ? (control.height - height) / 2 : control.height - height) :
               (control.height - height) / 2
        color: control.down.pressed ? control.buttonColorDown : (control.down.hovered ? control.buttonColorHot : control.buttonColor)

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
