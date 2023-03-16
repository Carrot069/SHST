import QtQuick 2.12
import QtQuick.Controls 2.12

import WzI18N 1.0

import "../WzControls"

Item {
    property alias autoConnectWifi: autoConnectWiFiCheckBox.checked
    property alias wifiName: textFieldWiFiName.text
    property alias wifiPassword: textFieldWiFiPassword.text

    Component.onCompleted: {
        autoConnectWiFiCheckBox.checked = dbService.readIntOption("autoConnectWiFi", 1) === 1
        textFieldWiFiName.text = dbService.readStrOption("wifiName", "SH-Infinite523")
        textFieldWiFiPassword.text = dbService.readStrOptionAes("wifiPassword", "YFHapQXGv7")
    }

    Text {
        id: textWiFiName
        text: qsTr("无线名称和密码:")
        anchors.left: parent.left
        anchors.top: parent.top
        color: "#dddddd"
        font.pixelSize: 19
    }

    CheckBox {
        id: autoConnectWiFiCheckBox
        text: qsTr("启动时自动连接")
        anchors.left: textWiFiName.right
        anchors.leftMargin: 10
        anchors.verticalCenter: textWiFiName.verticalCenter
        font.family: WzI18N.font.family
        onCheckedChanged: {
            dbService.saveIntOption("autoConnectWiFi", autoConnectWiFiCheckBox.checked ? 1 : 0)
        }
    }

    TextField {
        id: textFieldWiFiName
        anchors.top: textWiFiName.bottom
        anchors.topMargin: 2
        anchors.left: textWiFiName.left
        width: (parent.width - x * 2) * 0.5 - 10
        selectByMouse: true
        onEditingFinished: {
            dbService.saveStrOption("wifiName", textFieldWiFiName.text)
        }
    }
    TextField {
        id: textFieldWiFiPassword
        anchors.top: textFieldWiFiName.top
        anchors.left: textFieldWiFiName.right
        anchors.leftMargin: 10
        width: textFieldWiFiName.width
        height: textFieldWiFiName.height
        selectByMouse: true
        echoMode: TextInput.PasswordEchoOnEdit
        passwordCharacter: "●"
        onEditingFinished: {
            dbService.saveStrOptionAes("wifiPassword", textFieldWiFiPassword.text)
        }
    }
}
