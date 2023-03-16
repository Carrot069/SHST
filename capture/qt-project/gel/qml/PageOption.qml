import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.12

import WzUtils 1.0
import WzEnum 1.0
import WzI18N 1.0

import "WzControls"
import "WzControls.2"
import "Components"

Rectangle {
    id: root
    color: "black"

    signal pageChange(int pageIndex)
    signal adminParamsChanged(var params)

    property string imagePath: ""
    onImagePathChanged: textFieldImagePath.text = imagePath

    PageNav {
        id: pageNav
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 49
        onClicked: pageChange(buttonIndex)
    }

    Image {
        id: imageOptionIcon

        source: "qrc:/images/button_option_hot.svg"
        sourceSize.height: 28
        sourceSize.width: 30
        anchors.left: rectWrapper.left
        anchors.leftMargin: -4
        anchors.bottom: rectWrapper.top
        anchors.bottomMargin: 10
        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                adminSettingComponent.createObject(root)
            }
        }
    }
    Text {
        id: textOption
        text: qsTr("选项")
        anchors.left: imageOptionIcon.right
        anchors.leftMargin: 10
        anchors.verticalCenter: imageOptionIcon.verticalCenter
        color: "#cccccc"
        font.pixelSize: 30
    }

    Rectangle {
        id: rectWrapper
        anchors.margins: 150
        anchors.fill: parent
        color: "transparent"
        border.color: "#555555"
        border.width: 3
        radius: 5

        Text {
            id: textImagePath
            text: qsTr("图片存储目录:")
            anchors.left: parent.left
            anchors.leftMargin: 40
            anchors.top: parent.top
            anchors.topMargin: 20
            color: "#dddddd"
            font.pixelSize: 19
        }

        TextField {
            id: textFieldImagePath
            anchors.top: textImagePath.bottom
            anchors.topMargin: 5
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
            text: imagePath
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
            anchors.rightMargin: textImagePath.anchors.leftMargin
            anchors.bottom: textFieldImagePath.bottom
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

        Text {
            id: textLanguage
            text: qsTr("界面语言:")
            anchors.left: parent.left
            anchors.leftMargin: 40
            anchors.top: textFieldImagePath.bottom
            anchors.topMargin: 20
            color: "#dddddd"
            font.pixelSize: 19
        }
        WzComboBox {
            id: comboBoxLanguage
            property bool noSaveIndex: true
            anchors.left: textLanguage.left
            anchors.top: textLanguage.bottom
            anchors.topMargin: 5
            popup.y: height
            model: ["简体中文", "English"]
            property var languages: ["zh", "en"]
            onCurrentIndexChanged: {
                if (noSaveIndex) {
                    noSaveIndex = false
                    return
                }
                WzI18N.switchLanguage(languages[currentIndex])
                dbService.saveStrOption("languageName", languages[currentIndex])
            }
        }

        // 启用日志文件输出
        WzCheckBox {
            id: logFileCheckBox
            text: qsTr("启用日志")
            anchors.top: comboBoxLanguage.bottom
            //anchors.topMargin: -10
            anchors.left: textLanguage.left
            anchors.leftMargin: -7
            onCheckedChanged: {
                iniSetting.saveBool("Normal", "LogFile", checked)
            }
        }

        WzCheckBox {
            id: isCaptureSamePreviewCheckBox
            text: qsTr("拍摄与预览同步")
            anchors.top: logFileCheckBox.bottom
            //anchors.topMargin: -10
            anchors.left: logFileCheckBox.left
            onCheckedChanged: {
                dbService.saveIntOption("isCaptureSamePreview", isCaptureSamePreviewCheckBox.checked ? 1 : 0)
            }
        }

        WzCheckBox {
            id: noCloseWhiteUpWhenCaptureCheckBox
            text: qsTr("拍摄时不自动关闭白光反射")
            anchors.top: isCaptureSamePreviewCheckBox.bottom
            //anchors.topMargin: -10
            anchors.left: isCaptureSamePreviewCheckBox.left
            onCheckedChanged: {
                dbService.saveIntOption("noCloseWhiteUpWhenCapture", noCloseWhiteUpWhenCaptureCheckBox.checked ? 1 : 0)
            }
        }

        WzButton {
            id: buttonClearHistoryMachineSN
            anchors.left: textLanguage.left
            anchors.top: logFileCheckBox.bottom
            anchors.topMargin: 50
            width: 180
            height: 45
            visible: false
            label.font.pixelSize: 18
            radius: 3
            text: qsTr("清理历史连接信息")
            onClicked: {
                dbService.saveStrOption("opened_usb_sn", "")
                dbService.saveStrOption("ry3_sn", "")
                enabled = false
                text = qsTr("已清理")
            }
        }

        MouseArea {
            property int clickCount: 0
            anchors.bottom: parent.bottom
            anchors.right: parent.right
            width: 20
            height: 20
            onClicked: {
                clickCount++
                if (clickCount > 9) {
                    clickCount = 0
                    advOptionsLoader.sourceComponent = advOptionsComponent
                    advOptionsLoader.active = true
                }
            }
        }

        Loader {
            id: advOptionsLoader
            active: false
            sourceComponent: advOptionsComponent
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            anchors.leftMargin: 40
            anchors.rightMargin: 40
            anchors.bottomMargin: 20
        }
        Component {
            id: advOptionsComponent
            AdvOptions {
                id: advOptions
                onClose: {
                    advOptionsLoader.sourceComponent = undefined
                }
            }
        }

    }

    Component.onCompleted: {
        imagePath = dbService.readStrOption("image_path", "")
        var languageName = dbService.readStrOption("languageName", "zh")
        for (var i = 0; i < comboBoxLanguage.languages.length; i++)
            if (comboBoxLanguage.languages[i] === languageName) {
                comboBoxLanguage.currentIndex = i
                break
            }
        logFileCheckBox.checked = iniSetting.readBool("Normal", "LogFile", false)
        isCaptureSamePreviewCheckBox.checked = 1 === dbService.readIntOption("isCaptureSamePreview", 1)
        noCloseWhiteUpWhenCaptureCheckBox.checked = 1 === dbService.readIntOption("noCloseWhiteUpWhenCapture", 0)
    }

    Component {
        id: adminSettingComponent
        AdminSetting {
            anchors.fill: parent
            anchors.margins: 0
            onConfirm: {
                adminParamsChanged(params)
            }
        }
    }
}


/*##^##
Designer {
    D{i:0;autoSize:true;height:800;width:1280}D{i:26;anchors_y:99}D{i:27;anchors_width:95;anchors_x:2;anchors_y:99}
D{i:28;anchors_y:99}
}
##^##*/
