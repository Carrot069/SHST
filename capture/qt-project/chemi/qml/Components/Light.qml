import QtQuick 2.12
import QtQuick.Controls 2.12

import WzUtils 1.0
import WzEnum 1.0
import "../WzControls"

Item {
    id: root
    signal switchLight(string lightType, bool isOpen)
    function setLightChecked(lightType, isOpen) {
        if (!isOpen) {
            closeAll()
            return
        }
        if (lightType === "white_up")
            buttonLightWhiteUp.checked = true
        else if (lightType === "white_down")
            buttonLightBluePenetrate.checked = true
        else if (lightType === "uv_penetrate")
            buttonLightUVPenetrate.checked = true
        else if (lightType === "uv_reflex1") {
            buttonLightUvReflex.checked = true
            actionUvReflex1.checked = true
        } else if (lightType === "uv_reflex2") {
            buttonLightUvReflex.checked = true
            actionUvReflex2.checked = true
        } else {
            var lightTypeInt = WzUtils.lightTypeFromStr(lightType)
            switch(lightTypeInt) {
            case WzEnum.Light_BluePenetrate:
                buttonLightBluePenetrate2.checked = true
            }
        }
    }

    property alias buttonLightWhiteUp: buttonLightWhiteUp
    property alias buttonLightBluePenetrate: buttonLightBluePenetrate
    property alias buttonLightBluePenetrate2: buttonLightBluePenetrate2
    property alias buttonLightUVPenetrate: buttonLightUVPenetrate
    property alias buttonLightUVPenetrateForce: buttonLightUVPenetrateForce
    property int uvReflexIndex: -1 // -1 代表最后按下的是紫外透射
    onUvReflexIndexChanged: {
        console.info("Light.onUvReflexIndexChanged:", uvReflexIndex)
    }

    opacity: enabled ? 1 : 0.4

    function closeAll() {
        buttonLightWhiteUp.checked = false
        buttonLightUvReflex.checked = false
        buttonLightBluePenetrate.checked = false
        buttonLightBluePenetrate2.checked = false
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
                    buttonLightUvReflex.checked = false
                    buttonLightBluePenetrate.checked = false
                    buttonLightBluePenetrate2.checked = false
                    buttonLightUVPenetrate.checked = false
                    buttonLightUVPenetrateForce.checked = false
                }
            }
        }

        WzButton {
            id: buttonLightUvReflex
            checkable: true
            text: qsTr("紫外反射")
            width: 101
            height: 110
            normalColor: "transparent"
            hotColor: "transparent"
            downColor: "transparent"
            downFontColor: "transparent"
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
                var lightType
                if (actionUvReflex1.checked) {
                    lightType = "uv_reflex1"
                    root.uvReflexIndex = 0
                } else if (actionUvReflex2.checked) {
                    lightType = "uv_reflex2"
                    root.uvReflexIndex = 1
                } else {
                    lightType = "uv_reflex1"
                    root.uvReflexIndex = -1
                }
                console.info("buttonLightUvReflex.onClicked, uvReflexIndex:", root.uvReflexIndex)
                switchLight(lightType, checked)
            }
            onCheckedChanged: {
                if (checked) {
                    buttonLightWhiteUp.checked = false
                    //buttonLightWhiteDown.checked = false
                    buttonLightBluePenetrate.checked = false
                    buttonLightBluePenetrate2.checked = false
                    buttonLightUVPenetrate.checked = false
                    buttonLightUVPenetrateForce.checked = false
                }
            }

            Image {
                id: imageUvReflexDrop
                source: "qrc:/images/uv_drop.svg"
                anchors.right: parent.right
                anchors.rightMargin: 23
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 31
            }
            MouseArea {
                anchors.fill: imageUvReflexDrop
                anchors.margins: -3
                onClicked: {
                    var p = mapToItem(parent, mouse.x, mouse.y)
                    console.debug(p)
                    menuUvReflex.popup(p)
                }
            }
            WzMenu {
                id: menuUvReflex
                width: 100
                indicatorWidth: 8
                font.pixelSize: 18
                font.family: "Arial"

                Action {
                    id: actionUvReflex1
                    text: "254nm"
                    checked: true
                    onTriggered: {
                        checked = true
                        actionUvReflex2.checked = false
                        dbService.saveIntOption("uv_reflex", 0)
                        uvReflexIndex = 0
                        if (buttonLightUvReflex.checked)
                            switchLight("uv_reflex1", checked)
                    }
                }
                WzMenuSeparator {
                    width: parent.width
                    color: "#525252"
                }
                Action {
                    id: actionUvReflex2
                    text: "365nm"
                    onTriggered: {
                        checked = true
                        actionUvReflex1.checked = false
                        dbService.saveIntOption("uv_reflex", 1)
                        uvReflexIndex = 1
                        if (buttonLightUvReflex.checked)
                            switchLight("uv_reflex2", checked)
                    }
                }
            }
        }

        // 默认情况下蓝光透射和白光透射两种光源使用同一个按钮, 如果选择了显示单独的蓝光透射按钮,
        // 那么此处的按钮就需要显示为单独的白光透射按钮
        WzButton {
            id: buttonLightBluePenetrate
            checkable: true
            text: buttonLightBluePenetrate2.visible ? qsTr("白光透射") : qsTr("蓝白光透射")
            width: 101
            height: 110
            normalColor: "transparent"
            hotColor: "transparent"
            downColor: "transparent"
            checkedFontColor: buttonLightBluePenetrate2.visible ? "white" : "#11b9ff"
            imageVisible: true
            imageSourceNormal: buttonLightBluePenetrate2.visible ? "qrc:/images/light_white_off.png" : "qrc:/images/light_fb_off.png"
            imageSourceChecked: buttonLightBluePenetrate2.visible ? "qrc:/images/light_white_on.png" : "qrc:/images/light_fb_on.png"
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
                    buttonLightUvReflex.checked = false
                    buttonLightBluePenetrate2.checked = false
                    buttonLightUVPenetrate.checked = false
                    buttonLightUVPenetrateForce.checked = false
                }
            }
        }

        // 默认不显示, 此按钮为单独的蓝光透射按钮, 当设置为显示的时候, 蓝白光透射按钮就会变成
        // 单纯的白光透射按钮
        WzButton {
            id: buttonLightBluePenetrate2
            visible: false
            checkable: true
            text: qsTr("蓝光透射")
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
                switchLight("blue_penetrate", checked)
            }
            onCheckedChanged: {
                if (checked) {
                    buttonLightWhiteUp.checked = false
                    buttonLightUvReflex.checked = false
                    buttonLightBluePenetrate.checked = false
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
                    buttonLightUvReflex.checked = false
                    buttonLightBluePenetrate.checked = false
                    buttonLightBluePenetrate2.checked = false
                    //buttonLightUVPenetrate.checked = false
                    buttonLightUVPenetrateForce.checked = false
                    uvReflexIndex = -1
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
            checkedFontColor: buttonLightBluePenetrate2.visible ? buttonLightBluePenetrate2.checkedFontColor : "#5f2cee"
            imageVisible: true
            imageSourceNormal: buttonLightBluePenetrate2.visible ? "qrc:/images/light_fb_off.png" : "qrc:/images/light_uv_penetrate_off.png"
            imageSourceChecked: buttonLightBluePenetrate2.visible ? "qrc:/images/light_fb_on.png" : "qrc:/images/light_uv_penetrate_on.png"
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
                // 紫外透射按钮隐藏时意味着用蓝光透射切胶
                if (buttonLightUVPenetrate.visible)
                    switchLight("uv_penetrate_force", checked)
                else
                    switchLight("blue_penetrate_force", checked)
            }
            onCheckedChanged: {
                if (checked) {
                    buttonLightWhiteUp.checked = false
                    buttonLightUvReflex.checked = false
                    buttonLightBluePenetrate.checked = false
                    buttonLightBluePenetrate2.checked = false
                    buttonLightUVPenetrate.checked = false
                    //buttonLightUVPenetrateForce.checked = false
                }
            }
        }
    }

    Component.onCompleted: {
        var uv_reflex = dbService.readIntOption("uv_reflex", 0)
        if (uv_reflex === 0)
            actionUvReflex1.checked = true
        else if (uv_reflex === 1)
            actionUvReflex2.checked = true
        else
            actionUvReflex1.checked = true

        actionUvReflex1.text = iniSetting.readStr("UI", "UvReflex1", actionUvReflex1.text)
        actionUvReflex2.text = iniSetting.readStr("UI", "UvReflex2", actionUvReflex2.text)
    }
}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
