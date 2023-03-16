import QtQuick 2.12
import QtQuick.Controls 2.12

import WzUtils 1.0

import "../WzControls"
import "../WzControls.2"

Item {
    id: root
    width: WzUtils.isPC() ? 122 : 180
    height: WzUtils.isPC() ? 90 : 110
    opacity: 0
    visible: opacity > 0
    onVisibleChanged: millisecondTumblersPopup.close()

    Behavior on opacity { NumberAnimation {duration: 500} }
    Behavior on anchors.rightMargin { NumberAnimation {duration: 500} }

    property alias exposureTime: exposureTime
    property alias textExposureTime: textExposureTime

    Rectangle {
        anchors.fill: parent
        radius: 5
        color: "black"
        border.color: "#777777"
    }

    Text {
        id: textExposureTime
        color: "#b4b4b4"
        text: qsTr("预览时间")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: parent.top
        anchors.topMargin: 10
        font.pixelSize: WzUtils.isPC() ? 18 : 22
    }

    WzSpinBox {
        id: exposureTime        
        isShowButton: true
        isAlwaysShowButton: true
        isHorizontalButton: true
        width: WzUtils.isPC() ? 110 : 170
        height: 45
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: textExposureTime.bottom
        anchors.topMargin: WzUtils.isPC() ? 2 : 10
        fontColor: "#b4b4b4"
        font.family: "Digital Dismay"
        font.pixelSize: WzUtils.isPC() ? 30 : 40
        from: 1
        to: 999
        value: 1
        z: 10
        buttonFontColor: "#eeeeee"
        buttonColor: "transparent"
        buttonColorHot: WzUtils.isPC() ? "#666666" : "transparent"
        buttonSize: WzUtils.isPC() ? 32 : 48
        buttonFont.pixelSize: WzUtils.isPC() ? 25 : 35
        onValueChanged: {
            millisecondTumblersPopup.disableEvent = true
            millisecondTumblersPopup.value = value
            millisecondTumblersPopup.disableEvent = false
        }

        MouseArea {
            anchors.fill: parent
            anchors.leftMargin: parent.buttonSize
            anchors.rightMargin: parent.buttonSize
            visible: !WzUtils.isPC()
            z: 9999
            onClicked: {
                if (millisecondTumblersPopup.visible)
                    return
                var p = exposureTime.mapToItem(rootView, 0, 0)
                millisecondTumblersPopup.parent = rootView
                millisecondTumblersPopup.x = p.x + (exposureTime.width - 96 / window.scale) / 2
                millisecondTumblersPopup.y = p.y + -millisecondTumblersPopup.implicitHeight / window.scale - (5 / window.scale)
                millisecondTumblersPopup.open()
            }
        }
    }

    Component {
        id: delegateComponent

        Label {
            text: modelData
            opacity: 1.0 - Math.abs(Tumbler.displacement) / (Tumbler.tumbler.visibleItemCount / 2)
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
            font.family: "Digital Dismay"
            font.pixelSize: exposureTime.font.pixelSize * 0.7
            color: "#dddddd"
        }
    }

    Timer {
        id: delayChangeExposureTimeTimer
        interval: 50
        repeat: false
        running: false
        onTriggered: {
            if (millisecondTumblersPopup.disableEvent)
                return
            exposureTime.value = millisecondTumblersPopup.value
        }
    }

    WzTumblers3Popup {
        id: millisecondTumblersPopup
        x: (root.width - implicitWidth) / 2 - 14
        y: -280
        padding: 5
        fontPixelSize: 35

        property bool disableEvent: false

        onValueChanged: {
            delayChangeExposureTimeTimer.restart()
        }
    }
}
