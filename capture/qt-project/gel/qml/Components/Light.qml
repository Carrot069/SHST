import QtQuick 2.12
import WzUtils 1.0
import "../WzControls"

Item {
    signal switchLight(string lightType, bool isOpen)

    property alias buttonLightWhiteUp: buttonLightWhiteUp

    opacity: enabled ? 1 : 0.4

    function closeAll() {
        buttonLightWhiteUp.checked = false
        buttonLightWhiteDown.checked = false
        buttonLightBluePenetrate.checked = false
        buttonLightUVPenetrate.checked = false
        buttonLightUVPenetrateForce.checked = false
    }

    Text {
        id: textLight
        anchors.left: parent.left
        anchors.leftMargin: 19
        text: qsTr("辅助和透射光源")
        font.pixelSize: 18
        color: "#707070"
    }

    Row {
        anchors.top: textLight.top
        anchors.topMargin: 18
        spacing: -6
        WzButton {
            id: buttonLightWhiteUp
            checkable: true
            text: qsTr("白光反射")
            width: 101
            height: 110
            normalColor: "transparent"
            hotColor: "transparent"
            downColor: "transparent"
            downFontColor: "white"
            imageVisible: true
            imageSourceNormal: "qrc:/images/light_white_off.png"
            imageSourceChecked: "qrc:/images/light_white_on.png"
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
                switchLight("white_up", checked)
            }
            onCheckedChanged: {
                if (checked) {
                    //buttonLightWhiteUp.checked = false
                    buttonLightWhiteDown.checked = false
                    buttonLightBluePenetrate.checked = false
                    buttonLightUVPenetrate.checked = false
                    buttonLightUVPenetrateForce.checked = false
                }
            }
        }

        WzButton {
            id: buttonLightWhiteDown
            checkable: true
            text: qsTr("紫外反射")
            width: 101
            height: 110
            normalColor: "transparent"
            hotColor: "transparent"
            downColor: "transparent"
            downFontColor: "white"
            imageVisible: true
            imageSourceNormal: "qrc:/images/light_uv_penetrate_off.png"
            imageSourceChecked: "qrc:/images/light_uv_penetrate_on.png"
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

            }
            onCheckedChanged: {
                if (checked) {
                    buttonLightWhiteUp.checked = false
                    //buttonLightWhiteDown.checked = false
                    buttonLightBluePenetrate.checked = false
                    buttonLightUVPenetrate.checked = false
                    buttonLightUVPenetrateForce.checked = false
                }
            }
        }

        WzButton {
            id: buttonLightBluePenetrate
            checkable: true
            text: qsTr("蓝白光透射")
            width: 101
            height: 110
            normalColor: "transparent"
            hotColor: "transparent"
            downColor: "transparent"
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
                switchLight("white_down", checked)
            }
            onCheckedChanged: {
                if (checked) {
                    buttonLightWhiteUp.checked = false
                    buttonLightWhiteDown.checked = false
                    //buttonLightBluePenetrate.checked = false
                    buttonLightUVPenetrate.checked = false
                    buttonLightUVPenetrateForce.checked = false
                }
            }
        }
        WzButton {
            id: buttonLightUVPenetrate
            checkable: true
            text: qsTr("紫外透射")
            width: 101
            height: 110
            normalColor: "transparent"
            hotColor: "transparent"
            downColor: "transparent"
            checkedFontColor: "#5f2cee"
            imageVisible: true
            imageSourceNormal: "qrc:/images/light_uv_penetrate_off.png"
            imageSourceChecked: "qrc:/images/light_uv_penetrate_on.png"
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
                switchLight("uv_penetrate", checked)
            }
            onCheckedChanged: {
                if (checked) {
                    buttonLightWhiteUp.checked = false
                    buttonLightWhiteDown.checked = false
                    buttonLightBluePenetrate.checked = false
                    //buttonLightUVPenetrate.checked = false
                    buttonLightUVPenetrateForce.checked = false
                }
            }
        }
        WzButton {
            id: buttonLightUVPenetrateForce
            checkable: true
            text: qsTr("切胶")
            width: 101
            height: 110
            normalColor: "transparent"
            hotColor: "transparent"
            downColor: "transparent"
            checkedFontColor: "#5f2cee"
            imageVisible: true
            imageSourceNormal: "qrc:/images/light_uv_penetrate_off.png"
            imageSourceChecked: "qrc:/images/light_uv_penetrate_on.png"
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
                switchLight("uv_penetrate_force", checked)
            }
            onCheckedChanged: {
                if (checked) {
                    buttonLightWhiteUp.checked = false
                    buttonLightWhiteDown.checked = false
                    buttonLightBluePenetrate.checked = false
                    buttonLightUVPenetrate.checked = false
                    //buttonLightUVPenetrateForce.checked = false
                }
            }
        }
    }
}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
