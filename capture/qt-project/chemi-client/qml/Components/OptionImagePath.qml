import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2

import WzUtils 1.0
import WzI18N 1.0

import "../WzControls"

//Rectangle {
//    color: "black"
Item {
    property alias imagePath: textFieldImagePath.text

    Component.onCompleted: {
        imagePath = dbService.readStrOption("image_path", "")
    }

    Text {
        id: textImagePath
        text: qsTr("图片存储目录:")
        anchors.left: parent.left
        anchors.top: parent.top
        color: "#dddddd"
        font.pixelSize: 19
    }

    TextField {
        id: textFieldImagePath
        anchors.top: textImagePath.bottom
        anchors.topMargin: 2
        anchors.left: textImagePath.left
        anchors.right: buttonSelect.left
        anchors.rightMargin: 10
        selectByMouse: true
        onTextChanged: {
            imagePathHint.visible = false
        }
        onEditingFinished: {
            dbService.saveStrOption("image_path", textFieldImagePath.text)
        }
    }

    Text {
        id: imagePathHint
        anchors.left: textImagePath.left
        anchors.top: textFieldImagePath.bottom
        anchors.topMargin: 3
        color: "#dddddd"
    }

    WzButton {
        id: buttonSelect
        text: qsTr("选择...")
        anchors.right: parent.right
        anchors.bottom: textFieldImagePath.bottom
        anchors.bottomMargin: 6
        width: {
            switch (WzI18N.language) {
            case "en": return 120
            default: return 80
            }
        }
        height: 37
        label.font.pixelSize: 18
        radius: 3

        onClicked: {
            if (textFieldImagePath.text === "")
                fileDialog.folder = fileDialog.shortcuts.desktop
            else
                fileDialog.folder = "file:/" + textFieldImagePath.text
            fileDialog.open()
        }
    }

    FileDialog {
        id: fileDialog
        selectFolder: true

        onAccepted: {
            textFieldImagePath.text = WzUtils.toLocalPath(fileDialog.folder)
            dbService.saveStrOption("image_path", textFieldImagePath.text)
        }
    }
}
