import QtQuick 2.12
import WzI18N 1.0

import "../WzControls"

WzButton {
    id: buttonFocusFar
    enabled: mcuConnected
    opacity: enabled ? 1 : 0.4
    width: {
        switch(WzI18N.language) {
        case "zh": return 126
        case "en": return 128
        }
    }
    height: 48
    radius: 3
    text: qsTr("聚焦远")
    anchors.bottom: lineCapture.top
    anchors.bottomMargin: 18
    anchors.left: lineCapture.left
    anchors.leftMargin: 0
    normalColor: "#141414"
    label.font.pixelSize: buttonFocusNear.label.font.pixelSize
    label.horizontalAlignment: Text.AlignRight
    label.anchors.rightMargin: {
        switch(WzI18N.language) {
        case "zh": return 22
        case "en": return 15
        }
    }
    label.anchors.fill: label.parent
    imageVisible: true
    imageAlign: Qt.AlignLeft | Qt.AlignVCenter
    imageSourceNormal: "qrc:/images/button_plus.png"
    image.anchors.leftMargin: {
        switch(WzI18N.language) {
        case "zh": return 20
        case "en": return 15
        }
    }
    image.anchors.verticalCenterOffset: 0
    onPressed: {
        // 根据镜头固件版本使用不同的控制指令
        if (prop.lensFirmwareVersion !== "22.4.13") {
            mcu.focusStartFar()
            return
        }

        //prop.focusMode = readStrOption("focusMode", "high")
        if (mouse.x < buttonFocusFar.width * 0.33)
            prop.focusMode = "low"
        else if (mouse.x < buttonFocusFar.width * 0.66)
            prop.focusMode = "middle"
        else
            prop.focusMode = "high"

        console.info("[focus]FarButton,", prop.focusMode)

        if (prop.focusMode === "high")
            execCommand("focus,far,0,59,2,199,0")
        else if (prop.focusMode === "middle")
            execCommand("focus,far,0,199,2,199,0")
        else if (prop.focusMode === "low")
            execCommand("focus,far_repeat,300,100,199")
    }
    onReleased: {
        // 根据镜头固件版本使用不同的控制指令
        if (prop.lensFirmwareVersion !== "22.4.13") {
            mcu.focusStop()
            return
        }

        if (prop.focusMode === "high" || prop.focusMode === "middle")
            // TODO 2022-05-07 22:16:27 改成命令行形式
            focus2.ensureStopFocus()
        else if (prop.focusMode === "low")
            execCommand("focus,stop")
    }

    Connections {
        target: window
        onCommandResponse: {
            if (response.startsWith("lens,info,")) {
                var stringList = response.split(",")
                prop.lensFirmwareVersion = stringList[2]
            }
        }
    }

    QtObject {
        id: prop
        property string lensFirmwareVersion: ""
        property string focusMode: ""
    }
}

