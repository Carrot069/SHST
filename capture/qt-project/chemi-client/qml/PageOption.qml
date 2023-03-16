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
    property string imagePath: ""
    onImagePathChanged: {
        if (loaderImagePath.active)
            loaderImagePath.item.imagePath = imagePath
    }

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

        property bool adminSettingLoading: false
        MouseArea {
            anchors.fill: parent
            onDoubleClicked: {
                console.log(imageOptionIcon.adminSettingLoading)
                if (imageOptionIcon.adminSettingLoading)
                    return
                imageOptionIcon.adminSettingLoading = true
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
        anchors.verticalCenterOffset: -1
        color: "#cccccc"
        font.pixelSize: 31
    }

    Text {
        id: textVersion
        visible: WzUtils.isMobile()
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 20
        anchors.horizontalCenter: parent.horizontalCenter
        text: WzUtils.appVersion()
        color: "#202020"
    }

    Rectangle {
        id: rectWrapper
        anchors.margins: 150
        anchors.fill: parent
        color: "transparent"
        border.color: "#555555"
        border.width: 3
        radius: 5

        ScrollView {
            anchors.fill: parent
            anchors.rightMargin: 4
            anchors.topMargin: 4
            anchors.bottomMargin: 4
            contentHeight: 500
            clip: true

            WzLoader {
                id: loaderWifiOption
                anchors.left: parent.left
                anchors.leftMargin: 40
                anchors.top: parent.top
                anchors.topMargin: 40
                anchors.right: parent.right
                anchors.rightMargin: 40
                height: 80
                active: WzUtils.isMobile()
                source: "Components/OptionWifi.qml"
            }
            WzLoader {
                id: loaderImagePath
                anchors.left: parent.left
                anchors.leftMargin: 40
                anchors.top: parent.top
                anchors.topMargin: 40
                anchors.right: parent.right
                anchors.rightMargin: 40
                height: 70
                active: WzUtils.isPC()
                source: "Components/OptionImagePath.qml"
            }

            Text {
                id: textLanguage
                text: qsTr("界面语言:")
                anchors.left: parent.left
                anchors.leftMargin: 40
                anchors.top: loaderWifiOption.active ? loaderWifiOption.bottom : loaderImagePath.bottom
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
                id: imagesCountTitleText
                text: qsTr("当前图片数量:")
                anchors.top: comboBoxLanguage.bottom
                anchors.topMargin: 40
                anchors.left: textLanguage.left
                color: "#dddddd"
                font.pixelSize: 19
            }
            Text {
                id: imagesCountText
                anchors.verticalCenter: imagesCountTitleText.verticalCenter
                anchors.left: imagesCountTitleText.right
                anchors.leftMargin: 20
                text: pageImage.imagesCount
                font.family: "Arial"
                font.pixelSize: 20
                color: imagesCountTitleText.color
            }

            Text {
                id: cleanImageText1
                anchors.top: imagesCountTitleText.bottom
                anchors.topMargin: 10
                anchors.left: imagesCountTitleText.left
                color: "#dddddd"
                font.pixelSize: 19
                text: qsTr("只保留最新的")
            }
            TextField {
                id: cleanImageKeepCountTextField
                anchors.left: cleanImageText1.right
                horizontalAlignment: Text.AlignHCenter
                anchors.leftMargin: 10
                anchors.verticalCenter: cleanImageText1.verticalCenter
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                width: 50
                text: "50"
                validator: IntValidator {
                    top: pageImage.imagesCount
                }
                onTextChanged: {
                    dbService.saveStrOption("cleanImageKeepCount", cleanImageKeepCountTextField.text)
                }
                Component.onCompleted: {
                    cleanImageKeepCountTextField.text = dbService.readStrOption("cleanImageKeepCount", "50")
                }
            }
            Text {
                id: cleanImageText2
                anchors.top: imagesCountTitleText.bottom
                anchors.topMargin: 10
                anchors.left: cleanImageKeepCountTextField.right
                anchors.leftMargin: 10
                color: "#dddddd"
                font.pixelSize: 19
                text: qsTr("张图片，")
            }
            WzButton {
                id: cleanImageButton
                anchors.left: cleanImageText2.right
                anchors.leftMargin: 10
                anchors.verticalCenter: cleanImageText1.verticalCenter
                width: 80
                height: 30
                radius: 5
                text: qsTr("删除")
                onClicked: {
                    msgBox.buttonClicked.connect(confirmCleanImage)
                    msgBox.show(qsTr("您确定清理吗？"), "确定", "取消")
                }
            }
            Text {
                id: cleanImageText3
                anchors.top: imagesCountTitleText.bottom
                anchors.topMargin: 10
                anchors.left: cleanImageButton.right
                anchors.leftMargin: 10
                color: "#dddddd"
                font.pixelSize: 19
                text: qsTr("其他图片")
            }

            Text {
                id: markerAvgGrayThresholdText
                anchors.top: cleanImageText1.bottom
                anchors.topMargin: 10
                anchors.left: cleanImageText1.left
                color: "#dddddd"
                font.pixelSize: 19
                text: qsTr("白光图提醒阈值")
            }
            TextField {
                id: markerAvgGrayThresholdTextField
                anchors.left: markerAvgGrayThresholdText.right
                horizontalAlignment: Text.AlignHCenter
                anchors.leftMargin: 10
                anchors.verticalCenter: markerAvgGrayThresholdText.verticalCenter
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                width: 50
                text: "50"
                validator: IntValidator {
                    top: 65535
                }
                onTextChanged: {
                    dbService.saveStrOption("markerAvgGrayThreshold", markerAvgGrayThresholdTextField.text)
                }
                Component.onCompleted: {
                    markerAvgGrayThresholdTextField.text = dbService.readStrOption("markerAvgGrayThreshold", "500")
                }
            }

            Text {
                id: udpPortText
                anchors.top: markerAvgGrayThresholdText.bottom
                anchors.topMargin: 10
                anchors.left: markerAvgGrayThresholdText.left
                color: "#dddddd"
                font.pixelSize: 19
                text: qsTr("UDP 端口")
            }
            TextField {
                id: udpPortTextField
                anchors.left: udpPortText.right
                horizontalAlignment: Text.AlignHCenter
                anchors.leftMargin: 10
                anchors.verticalCenter: udpPortText.verticalCenter
                inputMethodHints: Qt.ImhFormattedNumbersOnly
                width: 50
                text: "50"
                validator: IntValidator {
                    top: 65535
                    bottom: 0
                }
                onTextChanged: {
                    dbService.saveIntOption("udpPort", parseInt(text))
                }
                Component.onCompleted: {
                    text = dbService.readIntOption("udpPort", "30030")
                }
            }

            WzButton {
                id: unpackDemoTiffButton
                width: 220
                height: 40
                anchors.top: udpPortText.bottom
                anchors.topMargin: 30
                anchors.left: comboBoxLanguage.left
                radius: 5
                text: qsTr("生成化学发光Demo图")
                onClicked: {
                    var imagePath = dbService.readStrOption("image_path", "")
                    var imageFile = dbService.unpackDemoTiff(imagePath)
                    var showOptions = {
                        autoFitOnce: true,
                        isNewImage: true,
                        isPreload: false
                    }
                    pageImage.openImage(imageFile, showOptions)
                }
            }

            WzButton {
                id: unpackDemoTiffFluorButton
                width: 220
                height: 40
                anchors.top: unpackDemoTiffButton.top
                anchors.left: unpackDemoTiffButton.right
                anchors.leftMargin: 10
                radius: 5
                text: qsTr("生成荧光Demo图")
                onClicked: {
                    var imagePath = dbService.readStrOption("image_path", "")
                    var imageFiles = dbService.unpackDemoTiffFluor(imagePath)
                    console.info(imageFiles)
                    for (var k in imageFiles) {
                        var showOptions = {
                            autoFitOnce: true,
                            isNewImage: true,
                            isPreload: false
                        }
                        var imageFile = imageFiles[k]
                        pageImage.openImage(imageFile, showOptions)
                    }
                }
            }
        }
    }

    function confirmCleanImage(buttonID) {
        msgBox.buttonClicked.disconnect(confirmCleanImage)
        if (buttonID === 1) {
            var keepImagesCount = parseInt(cleanImageKeepCountTextField.text)
            if (keepImagesCount !== undefined) {
                pageImage.cleanImages(keepImagesCount)
            }
        }
    }

    WzButton {
        text: qsTr("退出软件")
        width: 120
        height: 40
        anchors.left: rectWrapper.left
        anchors.top: rectWrapper.bottom
        anchors.topMargin: 20
        radius: 5
        onClicked: {
            window.close()
        }
    }

    WzButton {
        text: qsTr("保存选项")
        width: 120
        height: 40
        anchors.right: rectWrapper.right
        anchors.top: rectWrapper.bottom
        anchors.topMargin: 20
        radius: 5
        onClicked: {
            if (loaderWifiOption.active) {
                dbService.saveStrOption("wifiName", loaderWifiOption.item.wifiName)
                dbService.saveStrOptionAes("wifiPassword", loaderWifiOption.item.wifiPassword)
            }
        }
    }

    Component.onCompleted: {
        var languageName = dbService.readStrOption("languageName", "zh")
        for (var i = 0; i < comboBoxLanguage.model.count; i++)
            if (comboBoxLanguage.model.get(i).language === languageName) {
                comboBoxLanguage.currentIndex = i
                break
            }
    }

    Component {
        id: adminSettingComponent
        AdminSetting {            
            anchors.fill: parent
            anchors.margins: 0
            onConfirm: {
                saveAdminSetting(params)
                adminParamsChanged(params)
            }

            Component.onCompleted: {
                imageOptionIcon.adminSettingLoading = false
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
