import QtQuick 2.12
import "../WzControls"
import "."

Item {
    id: root

    opacity: enabled ? 1 : 0.4
    property bool isFilterWheel8: false

    signal switchLight(string lightType, bool isOpen)
    signal switchFilter(int filterIndex)

    function closeAllLight() {
        buttonRed.checked = false
        buttonGreen.checked = false
        buttonBlue.checked = false
    }

    function setLightChecked(lightName) {
        if (lightName === "red")
            buttonRed.checked = true
        else if (lightName === "green")
            buttonGreen.checked = true
        else if (lightName === "blue")
            buttonBlue.checked = true
    }

    function setActiveFilter(index) {
        if (isFilterWheel8)
            filterWheel8.setActiveFilter(index)
        else
            filterWheel5.setActiveFilter(index)
    }

    function setFilterOptions(options) {
        filterWheel5.setOptions(options)
        filterWheel8.setOptions(options)
    }

    function closeAll() {
        buttonRed.checked = false
        buttonGreen.checked = false
        buttonBlue.checked = false
    }

    Text {
        id: textLight
        anchors.left: parent.left
        anchors.leftMargin: 19
        text: qsTr("荧光光源和滤镜轮")
        font.pixelSize: 18
        color: "#707070"
    }

    WzButton {
        id: buttonRed
        anchors.top: textLight.top
        anchors.topMargin: 22
        checkable: true
        text: qsTr("红色荧光")
        width: 101
        height: 110
        normalColor: "transparent"
        hotColor: "transparent"
        downColor: "transparent"
        normalFontColor: "gray"
        checkedFontColor: "red"
        imageVisible: true
        imageSourceNormal: "qrc:/images/light_fr_off.png"
        imageSourceChecked: "qrc:/images/light_fr_on.png"
        imageAlign: Qt.AlignHCenter | Qt.AlignTop
        label.anchors.verticalCenter: undefined
        label.anchors.bottom: label.parent.bottom
        label.font.pixelSize: 14
        label2.text: checked ? "ON" : "OFF"
        label2.font.pixelSize: 9
        label2.font.family: "Arial"
        label2.anchors.verticalCenter: label2.parent.verticalCenter
        label2.anchors.verticalCenterOffset: -8
        label2.anchors.horizontalCenter: label2.parent.horizontalCenter
        onClicked: {
            switchLight("red", checked)
        }
        onCheckedChanged: {
            if (checked) {
                //buttonRed.checked = false
                buttonGreen.checked = false
                buttonBlue.checked = false
            }
        }
    }

    WzButton {
        id: buttonGreen
        anchors.left: buttonRed.right
        anchors.leftMargin: -5
        anchors.top: buttonRed.top
        checkable: true
        text: qsTr("绿色荧光")
        width: 101
        height: 110
        normalColor: "transparent"
        hotColor: "transparent"
        downColor: "transparent"
        normalFontColor: "gray"
        checkedFontColor: "green"
        imageVisible: true
        imageSourceNormal: "qrc:/images/light_fg_off.png"
        imageSourceChecked: "qrc:/images/light_fg_on.png"
        imageAlign: Qt.AlignHCenter | Qt.AlignTop
        label.anchors.verticalCenter: undefined
        label.anchors.bottom: label.parent.bottom
        label.font.pixelSize: 14
        label2.text: checked ? "ON" : "OFF"
        label2.font.pixelSize: 9
        label2.font.family: "Arial"
        label2.anchors.verticalCenter: label2.parent.verticalCenter
        label2.anchors.verticalCenterOffset: -8
        label2.anchors.horizontalCenter: label2.parent.horizontalCenter
        onClicked: {
            switchLight("green", checked)
        }
        onCheckedChanged: {
            if (checked) {
                buttonRed.checked = false
                //buttonGreen.checked = false
                buttonBlue.checked = false
            }
        }
    }

    WzButton {
        id: buttonBlue
        anchors.left: buttonGreen.right
        anchors.leftMargin: -5
        anchors.top: buttonRed.top
        checkable: true
        text: qsTr("蓝色荧光")
        width: 101
        height: 110
        normalColor: "transparent"
        hotColor: "transparent"
        downColor: "transparent"
        normalFontColor: "gray"
        checkedFontColor: "#11b9ff"
        imageVisible: true
        imageSourceNormal: "qrc:/images/light_fb_off.png"
        imageSourceChecked: "qrc:/images/light_fb_on.png"
        imageAlign: Qt.AlignHCenter | Qt.AlignTop
        label.anchors.verticalCenter: undefined
        label.anchors.bottom: label.parent.bottom
        label.font.pixelSize: 14
        label2.text: checked ? "ON" : "OFF"
        label2.font.pixelSize: 9
        label2.font.family: "Arial"
        label2.anchors.verticalCenter: label2.parent.verticalCenter
        label2.anchors.verticalCenterOffset: -8
        label2.anchors.horizontalCenter: label2.parent.horizontalCenter
        onClicked: {
            switchLight("blue", checked)
        }
        onCheckedChanged: {
            if (checked) {
                buttonRed.checked = false
                buttonGreen.checked = false
                //buttonBlue.checked = false
            }
        }
    }

    FilterWheel5 {
        id: filterWheel5
        anchors.right: parent.right
        anchors.top: parent.top
        anchors.topMargin: 8
        visible: !root.isFilterWheel8
        onSwitchFilter: root.switchFilter(filterIndex)
    }
    FilterWheel8 {
        id: filterWheel8
        anchors.fill: filterWheel5
        onSwitchFilter: switchFilter(filterIndex)
        visible: root.isFilterWheel8
    }
}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
