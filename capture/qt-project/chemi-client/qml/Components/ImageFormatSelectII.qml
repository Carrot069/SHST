import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2
import WzUtils 1.0
import WzI18N 1.0
import "../WzControls"
import "../WzControls.2" as C2
import "Pad"

Rectangle {
    id: root
    color: "black"
    border.color: "#666666"
    border.width: 3
    radius: 5
    clip: true
    antialiasing: true
    opacity: 0
    width: 0
    height: 0
    property real expandedWidth: 640
    property real expandedHeight: 350
    property bool usbDiskShareButtonVisible: WzUtils.isMobile()

    signal hide()
    signal confirm(var options)

    function show() {
        width = expandedWidth
        height = expandedHeight
        opacity = 1
    }

    Behavior on height {NumberAnimation {duration: 400}}
    Behavior on width {NumberAnimation {duration: 400}}
    Behavior on opacity {NumberAnimation{duration: 500}}

    Text {
        id: textImageFormat
        text: qsTr("请选择图片内容和格式:")
        anchors.left: parent.left
        anchors.leftMargin: 40
        anchors.top: parent.top
        anchors.topMargin: 40
        color: "#dddddd"
        font.pixelSize: 15
    }

    ListView {
        id: listView
        anchors.top: textImageFormat.bottom
        anchors.topMargin: 10
        anchors.left: textImageFormat.left
        anchors.leftMargin: 25
        anchors.right: parent.right
        anchors.rightMargin: 20
        height: 110
        model: listModel

        delegate: delegateComponent
    }

    ListModel {
        id: listModel
    }

    C2.WzCheckBox {
        id: zipCheckBox
        text: WzUtils.isMobile() ? qsTr("打包成Zip文件再分享") : qsTr("保存为Zip文件")
        anchors.left: textImageFormat.left
        anchors.leftMargin: -10
        anchors.top: listView.bottom
        anchors.topMargin: 15
        onCheckedChanged: {
            dbService.saveBoolOption("shareImagesToZip", checked)
        }
    }

    Component {
        id: delegateComponent
        Row {
            spacing: 10
            height: 35

            Text {
                id: imageContentText
                width: 100
                text: model.imageTitle
                color: "#dddddd"
                font.pixelSize: 15
                anchors.verticalCenter: parent.verticalCenter
            }

            C2.WzCheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("TIFF(8位)")
                checked: model.tiff8bit
                onCheckedChanged: {
                    model.tiff8bit = checked
                    dbService.saveBoolOption(model.imageName + ".tiff8bit", checked)
                }
            }
            C2.WzCheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("TIFF(24位)")
                checked: model.tiff24bit
                onCheckedChanged: {
                    model.tiff24bit = checked
                    dbService.saveBoolOption(model.imageName + ".tiff24bit", checked)
                }
            }
            C2.WzCheckBox {
                id: jpegCheckBox
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("JPEG")
                checked: model.jpeg
                onCheckedChanged: {
                    model.jpeg = checked
                    dbService.saveBoolOption(model.imageName + ".jpeg", checked)
                }
            }
            /*
            CheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("PNG")
                checked: model.png
                onCheckedChanged: {
                    model.png = checked
                    dbService.saveBoolOption(model.imageName + ".png", checked)
                }
            }
            */
            C2.WzCheckBox {
                anchors.verticalCenter: parent.verticalCenter
                text: qsTr("TIFF(16位)")
                checked: model.tiff16bit
                visible: !model.hideTiff16bit
                onCheckedChanged: {
                    model.tiff16bit = checked
                    dbService.saveBoolOption(model.imageName + ".tiff16bit", checked)
                }
            }
        }
    }

    Row {
        id: buttonsRow
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
        spacing: 30

        WzButton {
            id: buttonCancel
            text: qsTr("取消")
            onClicked: {
                root.opacity = 0
                root.height = 0
                root.width = 0
                hide()
            }
            width: buttonOk.width
            height: buttonOk.height
            label.font: buttonOk.label.font
            label.anchors.horizontalCenterOffset: 13
            radius: 3
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_cancel_b4b4b4.svg"
            image.sourceSize.width: 16
            image.sourceSize.height: 16
            image.anchors.horizontalCenterOffset: -23

        }

        WzButton {
            id: buttonOk
            text: WzUtils.isMobile() ? qsTr("分享") : qsTr("保存")
            width: {
                switch(WzI18N.language) {
                case "en": return 125
                default: return 105
                }
            }
            height: 43
            label.font.pixelSize: {
                switch(WzI18N.language) {
                case "en": return 15
                default: return 18
                }
            }
            label.anchors.horizontalCenterOffset: 13
            radius: 3
            imageVisible: true
            imageSourceNormal: WzUtils.isMobile() ? "qrc:/images/button_share2.svg" : "qrc:/images/button_save.svg"
            image.sourceSize.width: 24
            image.sourceSize.height: 24
            image.anchors.horizontalCenterOffset: -23
            onClicked: {
                func.confirmShare({})
            }
        }

        UsbDiskShareButton {
            visible: root.usbDiskShareButtonVisible
            onShare: func.confirmShare(options)
        }
    }

    Component.onCompleted: {
        var options =
            [{
               imageName: "overlapImage",
               imageTitle: qsTr("叠加图"),
               tiff8bit: dbService.readBoolOption('overlapImage.tiff8bit', true),
               tiff24bit: dbService.readBoolOption('overlapImage.tiff24bit', true),
               tiff16bit: false,
               jpeg: dbService.readBoolOption('overlapImage.jpeg', true),
               png: dbService.readBoolOption('overlapImage.png', false),
               hideTiff16bit: true
           },
           {
               imageName: "darkImage",
               tiff8bit: dbService.readBoolOption('darkImage.tiff8bit', true),
               tiff24bit: dbService.readBoolOption('overlapImage.tiff24bit', true),
               tiff16bit: dbService.readBoolOption('darkImage.tiff16bit', true),
               jpeg: dbService.readBoolOption('darkImage.jpeg', true),
               png: dbService.readBoolOption('darkImage.png', false),
               imageTitle: qsTr("化学发光图")
           },
           {
               imageName: "lightImage",
               imageTitle: qsTr("白光图"),
               tiff8bit: dbService.readBoolOption('lightImage.tiff8bit', true),
               tiff24bit: dbService.readBoolOption('overlapImage.tiff24bit', true),
               tiff16bit: dbService.readBoolOption('lightImage.tiff16bit', true),
               jpeg: dbService.readBoolOption('lightImage.jpeg', true),
               png: dbService.readBoolOption('lightImage.png', false)
           }]
        listModel.append(options[0])
        listModel.append(options[1])
        listModel.append(options[2])
        zipCheckBox.checked = dbService.readBoolOption("shareImagesToZip", true)
    }

    QtObject {
        id: func
        function confirmShare(options) {
            options.overlapImage = {
                tiff8bit: listView.model.get(0).tiff8bit,
                tiff24bit: listView.model.get(0).tiff24bit,
                jpeg: listView.model.get(0).jpeg,
                png: false,
                tiff16bit: listView.model.get(0).tiff16bit
            }
            options.darkImage = {
                tiff8bit: listView.model.get(1).tiff8bit,
                tiff24bit: listView.model.get(0).tiff24bit,
                jpeg: listView.model.get(1).jpeg,
                png: false,
                tiff16bit: listView.model.get(1).tiff16bit
            }
            options.lightImage = {
                tiff8bit: listView.model.get(2).tiff8bit,
                tiff24bit: listView.model.get(0).tiff24bit,
                jpeg: listView.model.get(2).jpeg,
                png: false,
                tiff16bit: listView.model.get(2).tiff16bit
            }
            options.zip = zipCheckBox.checked

            root.hide()
            root.opacity = 0
            root.height = 0
            root.width = 0

            root.confirm(options)
        }
    }
}


/*##^## Designer {
    D{i:0;autoSize:true;height:450;width:240}D{i:7;anchors_width:497}
}
 ##^##*/
