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
    property int binning: 1
    onBinningChanged: {
        if (buttonBinning.binning !== root.binning)
            buttonBinning.binning = root.binning
    }
    property alias isMedFilTiff8: medFilTiff8CheckBox.checked
    property int medFilSize: 9

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

        ScrollView {
            anchors.fill: parent
            anchors.rightMargin: 4
            anchors.topMargin: 4
            anchors.bottomMargin: 4
            contentHeight: textLanguage.visible ? 700 : 650
            clip: true

            MouseArea {
                anchors.fill: parent
                onClicked: {
                    // 单击空白处的时候，让其他的控件失去焦点，比如SpinBox
                    root.forceActiveFocus()
                }
            }

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
                anchors.topMargin: 30
                color: "#dddddd"
                font.pixelSize: 19
                visible: !WzUtils.isOnlyChinese()
            }
            WzComboBox {
                id: comboBoxLanguage
                property bool noSaveIndex: true
                anchors.left: textLanguage.left
                anchors.top: textLanguage.bottom
                anchors.topMargin: 2
                visible: textLanguage.visible
                popup.y: height
                model: ["简体中文", "English"]
                property var languages: ["zh", "en"]
                onCurrentIndexChanged: {
                    if (noSaveIndex) {
                        noSaveIndex = false
                        return
                    }
                    WzI18N.switchLanguage(languages[currentIndex], true)
                    dbService.saveStrOption("languageName", languages[currentIndex])
                }
            }

            WzCheckBox {
                id: autoSaveChemiMarkerCheckBox
                text: qsTr("另存叠加图时同时保存化学发光图和Marker图")
                anchors.left: textLanguage.left
                anchors.leftMargin: -7
                anchors.top: comboBoxLanguage.visible ? comboBoxLanguage.bottom : textFieldImagePath.bottom
                anchors.topMargin: 20
                onCheckedChanged: {
                    dbService.saveIntOption("autoSaveChemiMarker", checked ? 1 : 0)
                }
            }

            WzCheckBox {
                id: grayHighAutoMiddleCheckBox
                text: qsTr("自动调整使用中值")
                anchors.left: textLanguage.left
                anchors.leftMargin: -7
                anchors.top: autoSaveChemiMarkerCheckBox.bottom
                anchors.topMargin: -10
                onCheckedChanged: {
                    dbService.saveIntOption("grayHighAutoMiddle", checked ? 1 : 0)
                    pageImage.grayHighAutoMiddle = checked
                }
            }

            WzCheckBox {
                id: updateLowHighWhenValueChangedCheckBox
                text: qsTr("调整灰阶实时生效")
                anchors.left: textLanguage.left
                anchors.leftMargin: -7
                anchors.top: grayHighAutoMiddleCheckBox.bottom
                anchors.topMargin: -10
                onCheckedChanged: {
                    dbService.saveIntOption("updateLowHighWhenValueChanged", checked ? 1 : 0)
                    pageImage.updateLowHighWhenValueChanged = checked
                }
            }

            WzCheckBox {
                id: rememberLowHighCheckBox
                text: qsTr("记忆调整过的灰阶值")
                anchors.left: textLanguage.left
                anchors.leftMargin: -7
                anchors.top: updateLowHighWhenValueChangedCheckBox.bottom
                anchors.topMargin: -10
                onCheckedChanged: {
                    dbService.saveIntOption("rememberLowHigh", checked ? 1 : 0)
                    pageImage.rememberLowHigh = checked
                }
            }

            // 过爆提醒
            WzCheckBox {
                id: overExposureHintCheckBox
                text: qsTr("过爆提醒")
                anchors.left: rememberLowHighCheckBox.left
                anchors.top: rememberLowHighCheckBox.bottom
                anchors.topMargin: -10
                onCheckedChanged: {
                    dbService.saveBoolOption("overExposureHint", checked)
                    pageImage.overExposureHint = checked
                }
            }
            Text {
                id: overExposureHintValueText
                text: qsTr("过爆阈值:")
                anchors.left: overExposureHintCheckBox.right
                anchors.leftMargin: 30
                anchors.verticalCenter: overExposureHintCheckBox.verticalCenter
                color: "#dddddd"
                font.pixelSize: 15
            }
            WzSpinBox {
                id: overExposureHintValueSpinBox
                width: 60
                isPaddingZero: false
                font.pixelSize: 20
                font.family: "Digital Dismay"
                fontColor: "#b4b4b4"
                from: 0
                to: 65535
                isShowButton: true
                buttonColor: "#535353"
                buttonFontColor: "white"
                height: 90
                anchors.left: overExposureHintValueText.right
                anchors.verticalCenter: overExposureHintValueText.verticalCenter
                anchors.verticalCenterOffset: 2
                onValueChanged: {
                    dbService.saveIntOption("overExposureHintValue", value)
                    pageImage.overExposureHintValue = value
                }
            }
            Text {
                id: overExposureHintColorText
                text: qsTr("提示颜色:")
                anchors.left: overExposureHintValueSpinBox.right
                anchors.leftMargin: 30
                anchors.verticalCenter: overExposureHintCheckBox.verticalCenter
                color: "#dddddd"
                font.pixelSize: 15
            }
            Rectangle {
                id: overExposureHintColorRectangle
                anchors.left: overExposureHintColorText.right
                anchors.leftMargin: 5
                anchors.verticalCenter: overExposureHintCheckBox.verticalCenter
                anchors.verticalCenterOffset: 1
                width: 25
                height: 16
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        overExposureHintColorDialog.currentColor = parent.color
                        overExposureHintColorDialog.open()
                    }
                }
            }
            ColorDialog {
                id: overExposureHintColorDialog
                onAccepted: {
                    overExposureHintColorRectangle.color = currentColor
                    pageImage.overExposureHintColor = currentColor
                    dbService.saveStrOption("overExposureHintColor", currentColor)
                }
            }
            // 过爆提醒
            WzCheckBox {
                id: previewZoomCheckBox
                text: qsTr("预览启用缩放")
                anchors.left: rememberLowHighCheckBox.left
                anchors.top: overExposureHintCheckBox.bottom
                anchors.topMargin: -10
                onCheckedChanged: {
                    dbService.saveBoolOption("previewZoomEnabled", checked)
                    pageCapture.previewImageControlEnabled = checked
                }
            }
            // 启用日志文件输出
            WzCheckBox {
                id: logFileCheckBox
                text: qsTr("启用日志")
                anchors.top: previewZoomCheckBox.bottom
                anchors.topMargin: -10
                anchors.left: previewZoomCheckBox.left
                onCheckedChanged: {
                    iniSetting.saveBool("Normal", "LogFile", checked)
                }
            }

            // 另存Tiff(8bit)时优化
            WzCheckBox {
                id: medFilTiff8CheckBox
                text: qsTr("另存Tiff(8bit)时优化")
                anchors.top: visible ? logFileCheckBox.bottom : logFileCheckBox.top
                anchors.topMargin: visible ? -10 : 0
                anchors.left: logFileCheckBox.left
                visible: WzUtils.isMedFilTiff8bit()
                onCheckedChanged: {
                    iniSetting.saveBool("Normal", "MedFilTiff8", checked)
                }
            }

            // Mini Binning
            Text {
                id: textBinning
                font: textImagePath.font
                text: qsTr("像素合并")
                color: "#dddddd"
                visible: isMini
                anchors.top: medFilTiff8CheckBox.visible ? medFilTiff8CheckBox.bottom : logFileCheckBox.bottom
                anchors.left: textImagePath.left
            }

            WzButton {
                id: buttonBinning
                visible: textBinning.visible
                anchors.left: textBinning.right
                anchors.leftMargin: 20
                anchors.verticalCenter: textBinning.verticalCenter
                width: 63
                height: 32
                radius: 3
                text: "1x1"
                label.font.pixelSize: 18
                label.font.family: "Arial"
                label.horizontalAlignment: Text.AlignLeft
                label.anchors.leftMargin: 5
                label.anchors.fill: label.parent
                imageVisible: true
                imageAlign: Qt.AlignRight | Qt.AlignVCenter
                imageSourceNormal: "qrc:/images/combox_arrow_down_gray.png"
                image.sourceSize.height: 9
                image.sourceSize.width: 15
                image.anchors.rightMargin: 7
                image.anchors.verticalCenterOffset: 2
                property int binning: 1
                onBinningChanged: {
                    text = binning + "x" + binning
                    root.binning = buttonBinning.binning
                }

                onClicked: {
                    menuBinning.popup(0, -menuBinning.height - 10)
                }

                WzMenu {
                    id: menuBinning
                    width: parent.width * 1.3
                    indicatorWidth: 8
                    font.pixelSize: 18
                    font.family: "Arial"

                    Action {text: "1x1"; onTriggered: {buttonBinning.binning = 1}}
                    WzMenuSeparator {width: parent.width; color: "#525252"}
                    Action {text: "2x2"; onTriggered: {buttonBinning.binning = 2}}
                    WzMenuSeparator {width: parent.width; color: "#525252"}
                    Action {text: "3x3"; onTriggered: {buttonBinning.binning = 3}}
                    WzMenuSeparator {width: parent.width; color: "#525252"}
                    Action {text: "4x4"; onTriggered: {buttonBinning.binning = 4}}
                }
            }

            WzCheckBox {
                id: autoStartPreviewAfterCapturedCheckBox
                anchors {
                    top: medFilTiff8CheckBox.bottom
                    topMargin: -10
                    left: medFilTiff8CheckBox.left
                }
                text: qsTr("拍摄后自动开始预览")
                visible: !isMini
                onCheckedChanged: {
                    dbService.saveBoolOption("autoStartPreviewAfterCaptured", checked)
                }
            }

            WzCheckBox {
                id: checkMarkerDarkCheckBox
                anchors {
                    top: autoStartPreviewAfterCapturedCheckBox.bottom
                    topMargin: -10
                    left: autoStartPreviewAfterCapturedCheckBox.left
                }
                text: qsTr("检查Marker是否过暗")
                visible: !isMini
                onCheckedChanged: {
                    dbService.saveStrOption("isCheckMarkerDark", checked ? "1" : "0")
                }
            }
            Text {
                id: checkMarkerDarkThresholdText
                text: qsTr("阈值:")
                anchors.left: checkMarkerDarkCheckBox.right
                anchors.leftMargin: 30
                anchors.verticalCenter: checkMarkerDarkCheckBox.verticalCenter
                color: "#dddddd"
                font.pixelSize: 15
            }
            WzSpinBox {
                id: checkMarkerDarkThresholdSpinBox
                width: 60
                isPaddingZero: false
                font.pixelSize: 20
                font.family: "Digital Dismay"
                fontColor: "#b4b4b4"
                from: 0
                to: 65535
                isShowButton: true
                buttonColor: "#535353"
                buttonFontColor: "white"
                height: 90
                anchors.left: checkMarkerDarkThresholdText.right
                anchors.verticalCenter: checkMarkerDarkThresholdText.verticalCenter
                onValueChanged: {
                    dbService.saveStrOption("checkMarkerDarkThreshold", value)
                }
            }

            WzButton {
                id: buttonClearHistoryMachineSN
                anchors.left: textLanguage.left
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                visible: false
                width: {
                    switch(WzI18N.language) {
                    case "en": return 280
                    default: return 180
                    }
                }
                height: 45
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

            WzCheckBox {
                id: checkBoxRemoveFluorCircle
                visible: false
                text: qsTr("去除荧光样品中培养皿的边框")
                anchors.left: textLanguage.left
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 20
                onCheckedChanged: {
                    dbService.saveIntOption("removeFluorCircle", checked ? 1 : 0)
                }
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
        autoSaveChemiMarkerCheckBox.checked = 1 === dbService.readIntOption("autoSaveChemiMarker", 0)
        grayHighAutoMiddleCheckBox.checked = 1 === dbService.readIntOption("grayHighAutoMiddle", 0)
        updateLowHighWhenValueChangedCheckBox.checked = 1 === dbService.readIntOption("updateLowHighWhenValueChanged", 1)
        rememberLowHighCheckBox.checked = 1 === dbService.readIntOption("rememberLowHigh", 0)
        overExposureHintCheckBox.checked = dbService.readBoolOption("overExposureHint", false)
        overExposureHintValueSpinBox.value = dbService.readIntOption("overExposureHintValue", "60000")
        overExposureHintColorRectangle.color = dbService.readStrOption("overExposureHintColor", "red")
        checkBoxRemoveFluorCircle.checked = 1 === dbService.readIntOption("removeFluorCircle", 0)        
        previewZoomCheckBox.checked = dbService.readBoolOption("previewZoomEnabled", false)
        logFileCheckBox.checked = iniSetting.readBool("Normal", "LogFile", false)
        medFilTiff8CheckBox.checked = iniSetting.readBool("Normal", "MedFilTiff8", false)
        medFilSize = iniSetting.readInt("Normal", "MedFilSize", 9)
        autoStartPreviewAfterCapturedCheckBox.checked = dbService.readBoolOption("autoStartPreviewAfterCaptured", false)
        checkMarkerDarkCheckBox.checked = ("1" === dbService.readStrOption("isCheckMarkerDark", "0"))
        checkMarkerDarkThresholdSpinBox.value = parseInt(dbService.readStrOption("checkMarkerDarkThreshold", "100"))

        pageImage.grayHighAutoMiddle = grayHighAutoMiddleCheckBox.checked
        pageImage.updateLowHighWhenValueChanged = updateLowHighWhenValueChangedCheckBox.checked
        pageImage.rememberLowHigh = rememberLowHighCheckBox.checked
        pageImage.overExposureHint = overExposureHintCheckBox.checked
        pageImage.overExposureHintValue = overExposureHintValueSpinBox.value
        pageImage.overExposureHintColor = overExposureHintColorRectangle.color
        pageCapture.previewImageControlEnabled = previewZoomCheckBox.checked
    }

    Component {
        id: adminSettingComponent
        AdminSetting {            
            anchors.fill: parent
            anchors.margins: 0
            onConfirm: {
                dbService.saveAdminSetting(params)
                dbService.saveAdminSettingToDisk(params)
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
