import QtQuick 2.12
import QtQuick.Controls 2.12

import WzUtils 1.0

import "."

Item {
    id: root
    width: row.implicitWidth
    property bool showTumbler: !WzUtils.isPC()
    property int tumblerVisibleItemCount: 5
    property int tumblerHeight: 170
    property bool editable: WzUtils.isPC()
    property bool isShowMinute: true
    property bool isShowSecond: true
    property bool isShowButton: false
    property alias spinBoxSecond: second
    property alias spinBoxMillisecond: millisecond
    property Font font
    property color buttonColor: "#535353"
    property color fontColor: "#828282"
    property int fontPixelSize: 44
    property int exposureMs
    property int maxSecond: 59

    property alias millisecondControl: millisecond

    onExposureMsChanged: {
        setExposureMs(exposureMs)
    }

    function getExposureMs() {
        return (minute.visible ? minute.value * 60 * 1000 : 0) +
               (second.visible ? second.value * 1000 : 0) +
                millisecond.value;
    }
    function setExposureMs(value) {
        millisecond.value = value % 1000
        var v2 = value - millisecond.value
        second.value = (v2 % 60000) / 1000
        v2 = v2 - second.value
        minute.value = (v2 / 60000)
    }

    QtObject {
        id: privateObject
        property bool tumblerDisableEvent: false
    }

    height: 50
    onFontChanged: {
        minute.font = font
    }
    Row {
        id: row
        height: parent.height

        WzSpinBox {
            id: minute
            width: 50
            height: parent.height
            from: 0
            to: 99
            visible: isShowMinute
            font.family: "Digital Dismay"
            font.pixelSize: fontPixelSize
            fontColor: root.fontColor
            isShowButton: root.isShowButton
            buttonColor: root.buttonColor
            buttonFontColor: "white"
            editable: root.editable

            onValueChanged: {
                privateObject.tumblerDisableEvent = true
                minuteTumblersPopup.value = value
                privateObject.tumblerDisableEvent = false
                exposureMs = getExposureMs()
            }
        }
        Text {
            text: ":"
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            font.family: "Arial"
            font.pixelSize: fontPixelSize * 0.8
            color: root.fontColor
            visible: isShowMinute
        }
        WzSpinBox {
            id: second
            width: 50
            height: parent.height
            from: 0
            to: maxSecond
            visible: isShowSecond
            font.family: "Digital Dismay"
            font.pixelSize: fontPixelSize
            fontColor: root.fontColor
            isShowButton: root.isShowButton
            buttonColor: root.buttonColor
            buttonFontColor: "white"
            editable: root.editable

            onValueChanged: {
                privateObject.tumblerDisableEvent = true
                secondTumblersPopup.value = value
                privateObject.tumblerDisableEvent = false
                exposureMs = getExposureMs()
            }
        }
        Text {
            text: "."
            height: parent.height
            verticalAlignment: Text.AlignVCenter
            font.family: "Arial"
            font.pixelSize: fontPixelSize * 0.8
            color: root.fontColor
            visible: isShowSecond
        }
        WzSpinBox {
            id: millisecond
            width: 75
            height: parent.height
            from: 0
            to: 999
            font.family: "Digital Dismay"
            font.pixelSize: fontPixelSize
            fontColor: root.fontColor
            isShowButton: root.isShowButton
            buttonColor: root.buttonColor
            buttonFontColor: "white"
            editable: root.editable

            onValueChanged: {
                privateObject.tumblerDisableEvent = true
                millisecondTumblersPopup.value = value
                privateObject.tumblerDisableEvent = false
                exposureMs = getExposureMs()
            }
        }
    }

    MouseArea {
        anchors.fill: parent
        visible: root.showTumbler
        onClicked: {
            var p
            if (mouse.x < second.x) {
                if (minuteTumblersPopup.visible)
                    return
                p = minute.mapToItem(rootView, 0, 0)
                minuteTumblersPopup.parent = rootView
                minuteTumblersPopup.x = p.x + (minute.width - 64 / window.scale) / 2
                minuteTumblersPopup.y = p.y + -minuteTumblersPopup.implicitHeight / window.scale - 5
                minuteTumblersPopup.open()
            } else if (mouse.x < millisecond.x) {
                if (secondTumblersPopup.visible)
                    return                
                p = second.mapToItem(rootView, 0, 0)
                secondTumblersPopup.parent = rootView
                secondTumblersPopup.x = p.x + (second.width - 64 / window.scale) / 2
                secondTumblersPopup.y = p.y + -secondTumblersPopup.implicitHeight / window.scale - 5
                secondTumblersPopup.open()
            } else {
                if (millisecondTumblersPopup.visible)
                    return                
                p = millisecond.mapToItem(rootView, 0, 0)
                millisecondTumblersPopup.parent = rootView
                millisecondTumblersPopup.x = p.x + (millisecond.width - 96 / window.scale) / 2
                millisecondTumblersPopup.y = p.y + -millisecondTumblersPopup.implicitHeight / window.scale - 5
                millisecondTumblersPopup.open()
            }
        }
    }

    WzTumblers2Popup {
        id: minuteTumblersPopup
        fontPixelSize: 30
        tumblerHeight: root.tumblerHeight

        onValueChanged: {
            if (privateObject.tumblerDisableEvent)
                return
            minute.value = value
        }
    }

    WzTumblers2Popup {
        id: secondTumblersPopup
        count1: 6
        fontPixelSize: 30
        tumblerHeight: root.tumblerHeight

        onValueChanged: {
            if (privateObject.tumblerDisableEvent)
                return
            second.value = value
        }
    }

    WzTumblers3Popup {
        id: millisecondTumblersPopup
        fontPixelSize: 30
        tumblerHeight: root.tumblerHeight

        onValueChanged: {
            if (privateObject.tumblerDisableEvent)
                return
            millisecond.value = value
        }
    }
}
