import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import WzUtils 1.0
import WzI18N 1.0
import "../WzControls"

Rectangle {
    id: root
    color: "black"
    border.color: "#666666"
    border.width: 3
    radius: 5
    clip: true
    antialiasing: true
    width: 0
    height: 0

    signal hide()
    signal imagePathChanged(string imagePath)

    function show() {
        height = 200
        width = 650
        opacity = 1
    }

    Behavior on height {NumberAnimation {duration: 400}}
    Behavior on width {NumberAnimation {duration: 400}}
    Behavior on opacity {NumberAnimation{duration: 500}}

    Text {
        id: textImagePath
        text: qsTr("请选择一个图片存储目录:")
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 20
        color: "#dddddd"
        font.pixelSize: 15
    }

    TextField {
        id: textFieldImagePath
        anchors.top: textImagePath.bottom
        anchors.topMargin: 15
        anchors.left: textImagePath.left
        anchors.right: buttonSelect.left
        anchors.rightMargin: 10
        selectByMouse: true
        onTextChanged: {
            imagePathHint.visible = false
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
        anchors.rightMargin: 15
        anchors.bottom: textFieldImagePath.bottom
        anchors.bottomMargin: 6
        width: 80
        height: 37
        label.font.pixelSize: {
            switch(WzI18N.language) {
            case "zh": return 18
            case "en": return 15
            }
        }
        radius: 3

        onClicked: {
            fileDialog.open()
        }
    }

    WzButton {
        id: buttonOk
        text: qsTr("确定")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15
        width: {
            switch(WzI18N.language) {
            case "zh": return 105
            case "en": return 125
            }
        }
        height: 43
        label.font.pixelSize: {
            switch(WzI18N.language) {
            case "zh": return 18
            case "en": return 15
            }
        }
        label.anchors.leftMargin: 5
        label.anchors.left: label2.right
        label.anchors.horizontalCenter: undefined
        radius: 3
        label2.font.pixelSize: {
            switch(WzI18N.language) {
            case "zh": return 35
            case "en": return 30
            }
        }
        label2.font.family: "Webdings"
        label2.text: "a"
        label2.anchors.horizontalCenterOffset: -(label.width+20) / 2
        onClicked: {
            if (textFieldImagePath.text === "" || !WzUtils.validatePath(textFieldImagePath.text)) {
                imagePathHint.text = qsTr("无效目录，请重新选择")
                imagePathHint.visible = true
                textFieldImagePath.forceActiveFocus()
                return
            }

            dbService.saveStrOption("image_path", textFieldImagePath.text)
            imagePathChanged(textFieldImagePath.text)

            hide()
            root.opacity = 0
            root.height = 0
            root.width = 0
        }
    }

    FileDialog {
        id: fileDialog
        selectFolder: true

        onAccepted: {
            textFieldImagePath.text = WzUtils.toLocalPath(fileDialog.folder)
        }
    }
}


/*##^## Designer {
    D{i:0;autoSize:true;height:450;width:240}D{i:7;anchors_width:497}
}
 ##^##*/
