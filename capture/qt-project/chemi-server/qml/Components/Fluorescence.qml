import QtQuick 2.12
import "../WzControls"
import "."

Item {
    id: root

    opacity: enabled ? 1 : 0.4

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
        switch (index) {
        case 0:
            filter1.active = true
            break
        case 1:
            filter2.active = true
            break
        case 2:
            filter3.active = true
            break
        case 3:
            filter4.active = true
            break
        case 4:
            filter5.active = true
        }
    }

    property alias filter1: filter1
    property alias filter2: filter2
    property alias filter3: filter3
    property alias filter4: filter4
    property alias filter5: filter5

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

    Rectangle {
        id: rectFilterWheel
        antialiasing: true
        width: 124
        height: 124
        anchors.right: parent.right
        radius: 62
        anchors.top: parent.top
        anchors.topMargin: 8
        border.width: 1
        border.color: "#666666"
        opacity: 1
        color: "#111111"
    }

    Filter {
        id: filter1
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 3
        isNull: true
        active: true
        border.color: "#eeeeee"
        onActiveChanged: {
            console.log("filter1.active: ", filter1.active)
            if (active) {
                filter1.active = true
                filter2.active = false
                filter3.active = false
                filter4.active = false
                filter5.active = false
            }
        }
        onClicked: {
            if (!active) {
                filter1.active = true
                switchFilter(0)
            }
        }
    }

    Filter {
        id: filter5
        text: "470"
        isNull: false
        active: false
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.horizontalCenterOffset: -35
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 30
        border.color: "#11b9ff"
        onActiveChanged: {
            if (active) {
                filter1.active = false
                filter2.active = false
                filter3.active = false
                filter4.active = false
            }
        }
        onClicked: {
            if (!active) {
                filter5.active = true
                switchFilter(4)
            }
        }
    }

    Filter {
        id: filter3
        text: "530"
        active: false
        isNull: false
        border.color: "green"
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.horizontalCenterOffset: 22
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 72
        onActiveChanged: {
            if (active) {
                filter1.active = false
                filter2.active = false
                filter4.active = false
                filter5.active = false
            }
        }
        onClicked: {
            if (!active) {
                filter3.active = true
                switchFilter(2)
            }
        }
    }

    Filter {
        id: filter4
        text: "590"
        active: false
        isNull: false
        border.color: "#F4CCB0"
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.horizontalCenterOffset: -22
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 72
        onActiveChanged: {
            if (active) {
                filter1.active = false
                filter2.active = false
                filter3.active = false
                filter5.active = false
            }
        }
        onClicked: {
            if (!active) {
                filter4.active = true
                switchFilter(3)
            }
        }
    }

    Filter {
        id: filter2
        isNull: false
        active: false
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.horizontalCenterOffset: 35
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 30
        border.color: "red"
        text: "690"
        onActiveChanged: {
            if (active) {
                filter1.active = false
                filter3.active = false
                filter4.active = false
                filter5.active = false
            }
        }
        onClicked: {
            if (!active) {
                filter2.active = true
                switchFilter(1)
            }
        }
    }
}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
