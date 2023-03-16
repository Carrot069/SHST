import QtQuick 2.12
import "../WzControls.2"

Item {
    id: root
    width: 122
    height: 90
    opacity: 0
    visible: opacity > 0

    Behavior on opacity { NumberAnimation {duration: 500} }
    Behavior on anchors.rightMargin { NumberAnimation {duration: 500} }

    property alias exposureTime: exposureTime

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
        font.pixelSize: 18
    }

    WzSpinBox {
        id: exposureTime        
        isShowButton: true
        isAlwaysShowButton: true
        isHorizontalButton: true
        width: 110
        height: 45
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.top: textExposureTime.bottom
        anchors.topMargin: 2        
        fontColor: "#b4b4b4"
        font.family: "Digital Dismay"
        from: 1
        to: 999
        value: 1
        z: 10
        buttonFontColor: "#eeeeee"
        buttonColor: "transparent"
    }    
}
