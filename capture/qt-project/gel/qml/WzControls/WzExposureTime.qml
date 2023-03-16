import QtQuick 2.12
import "."

Row {
    id: root
    property bool isShowMinute: true
    property bool isShowButton: false
    property alias spinBoxMinute: minute
    property alias spinBoxSecond: second
    property alias spinBoxMillisecond: millisecond
    property Font font
    property color buttonColor: "#535353"
    property color fontColor: "#828282"
    property int fontPixelSize: 44
    property int exposureMs
    property int maxSecond: 59

    onExposureMsChanged: {
        setExposureMs(exposureMs)
    }

    function getExposureMs() {
        return (minute.visible ? minute.value * 60 * 1000 : 0) +
                second.value * 1000 +
                millisecond.value;
    }
    function setExposureMs(value) {
        millisecond.value = value % 1000
        var v2 = value - millisecond.value
        second.value = (v2 % 60000) / 1000
        v2 = v2 - second.value
        minute.value = (v2 / 60000)
    }

    height: 50
    onFontChanged: {
        minute.font = font
    }    
    WzSpinBox {
        id: minute
        width: 50
        height: parent.height
        from: 0
        to: 99
        visible: isShowMinute
        font.family: "Digital Dismay"
        font.pixelSize: fontPixelSize
        fontColor: parent.fontColor
        isShowButton: root.isShowButton
        buttonColor: root.buttonColor
        buttonFontColor: "white"

        onValueChanged: {
            exposureMs = getExposureMs()
        }
    }
    Text {
        text: ":"
        height: parent.height
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: fontPixelSize * 0.5
        color: parent.fontColor
        visible: isShowMinute
    }
    WzSpinBox {
        id: second
        width: 50
        height: parent.height
        from: 0
        to: maxSecond
        font.family: "Digital Dismay"
        font.pixelSize: fontPixelSize
        fontColor: parent.fontColor
        isShowButton: root.isShowButton
        buttonColor: root.buttonColor
        buttonFontColor: "white"

        onValueChanged: {
            exposureMs = getExposureMs()
        }
    }
    Text {
        text: "."
        height: parent.height
        verticalAlignment: Text.AlignBottom
        font.pixelSize: fontPixelSize * 0.5
        color: parent.fontColor
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 6
    }
    WzSpinBox {
        id: millisecond
        width: 75
        height: parent.height
        from: 0
        to: 999
        font.family: "Digital Dismay"
        font.pixelSize: fontPixelSize
        fontColor: parent.fontColor
        isShowButton: root.isShowButton
        buttonColor: root.buttonColor
        buttonFontColor: "white"

        onValueChanged: {
            exposureMs = getExposureMs()
        }
    }
}
