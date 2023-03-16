import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import QtQuick.Window 2.12

import WzUtils 1.0
import WzEnum 1.0
import WzI18N 1.0

import "WzControls"
import "Components"

Rectangle {
    id: root
    color: "black"

    signal pageChange(int pageIndex)
    signal adminParamsChanged(var params)
    signal testDrawer()

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

    Text {
        id: textOptionIcon
        text: "@"
        font.family: "Webdings"
        font.pixelSize: 40
        anchors.left: rectWrapper.left
        anchors.leftMargin: -4
        anchors.bottom: rectWrapper.top
        anchors.bottomMargin: 10
        color: "#cccccc"

        property bool adminSettingLoading: false
        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                console.log(textOptionIcon.adminSettingLoading)
                if (textOptionIcon.adminSettingLoading)
                    return
                textOptionIcon.adminSettingLoading = true
                adminSettingComponent.createObject(root)
            }
        }
    }

    Text {
        id: textOption
        text: qsTr("选项")
        anchors.left: textOptionIcon.right
        anchors.leftMargin: 10
        anchors.bottom: textOptionIcon.bottom
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
            anchors.topMargin: 40
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
            anchors.topMargin: 30
            color: "#dddddd"
            font.pixelSize: 19
        }
        ComboBox {
            id: comboBoxLanguage
            property bool noSaveIndex: true
            anchors.left: textLanguage.left
            anchors.top: textLanguage.bottom
            anchors.topMargin: 2
            textRole: "text"
            popup.y: height
            model: ListModel {
                ListElement {text: "简体中文"; language: "zh"}
                ListElement {text: "English"; language: "en"}
            }
            onCurrentIndexChanged: {
                if (noSaveIndex) {
                    noSaveIndex = false
                    return
                }
                WzI18N.switchLanguage(model.get(currentIndex).language)
                dbService.saveStrOption("languageName", model.get(currentIndex).language)
            }
        }

        Text {
            id: textDiskFreeSpaceLimit
            text: qsTr("可用存储空间小于x时清理图片使可用空间增加到y(MB):")
            anchors.left: textLanguage.left
            anchors.top: comboBoxLanguage.bottom
            anchors.topMargin: 20
            color: "#dddddd"
            font.pixelSize: 19
            visible: false
        }

        TextField {
            id: textFieldDiskFreeSpaceLimit
            visible: textDiskFreeSpaceLimit.visible
            width: 70
            horizontalAlignment: Text.AlignHCenter
            anchors.left: textDiskFreeSpaceLimit.right
            anchors.leftMargin: 20
            anchors.verticalCenter: textDiskFreeSpaceLimit.verticalCenter
            validator: IntValidator {
                top: 99999
                bottom: 100
            }
            onTextChanged: {
                dbService.saveIntOption("diskFreeSpaceLimit", textFieldDiskFreeSpaceLimit.text)
            }
        }
        TextField {
            id: textFieldDiskFreeSpaceTarget
            visible: textDiskFreeSpaceLimit.visible
            width: 70
            horizontalAlignment: Text.AlignHCenter
            anchors.left: textFieldDiskFreeSpaceLimit.right
            anchors.leftMargin: 10
            anchors.verticalCenter: textDiskFreeSpaceLimit.verticalCenter
            validator: IntValidator {
                top: 99999
                bottom: 100
            }
            onTextChanged: {
                dbService.saveIntOption("diskFreeSpaceTarget", textFieldDiskFreeSpaceTarget.text)
            }
        }

        Row {
            anchors.left: textLanguage.left
            anchors.top: comboBoxLanguage.bottom
            anchors.topMargin: 20
            spacing: 10

            Button {
                text: qsTr("打开配置目录")
                onClicked: {
                    WzUtils.openConfigPath()
                }
            }

            Button {
                text: qsTr("测试抽屉")
                onClicked: testDrawerTimer.running = !testDrawerTimer.running

                Timer {
                    id: testDrawerTimer
                    interval: 7000
                    repeat: true
                    onTriggered: testDrawer()
                }
            }

            Button {
                text: qsTr("Crash")
                onClicked: WzUtils.takeCrash()
            }
        }
    }

    Component.onCompleted: {
        imagePath = dbService.readStrOption("image_path", "")
        var languageName = dbService.readStrOption("languageName", "zh")
        for (var i = 0; i < comboBoxLanguage.model.count; i++)
            if (comboBoxLanguage.model.get(i).language === languageName) {
                comboBoxLanguage.currentIndex = i
                break
            }

        textFieldDiskFreeSpaceLimit.text = dbService.readIntOption("diskFreeSpaceLimit", "500")
        textFieldDiskFreeSpaceTarget.text = dbService.readIntOption("diskFreeSpaceTarget", "1000")
    }

    Component {
        id: adminSettingComponent
        AdminSetting {            
            anchors.fill: parent
            anchors.margins: 0
            onConfirm: {
                dbService.saveAdminSetting(params)
                adminParamsChanged(params)
            }

            Component.onCompleted: {
                textOptionIcon.adminSettingLoading = false
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
