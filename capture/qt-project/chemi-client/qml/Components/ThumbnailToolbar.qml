import QtQuick 2.12
import QtQuick.Controls 2.12

import WzI18N 1.0

import "../WzControls"

Rectangle {
    height: deleteButton.height + row.padding * 2
    width: row.implicitWidth
    radius: 5
    color: "black"
    opacity: 0.9

    Row {
        id: row
        padding: 15
        spacing: 5
        WzButton {
            id: exportButton
            width: 90
            height: 44
            radius: 4
            text: qsTr("分享")
            label.anchors.horizontalCenterOffset: 14
            label.anchors.verticalCenterOffset: 1
            label.font.pixelSize: {
                switch(WzI18N.language) {
                case "zh": return 17
                case "en": return 15
                default: return 17
                }
            }
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_share.svg"
            image.sourceSize.width: 32
            image.sourceSize.height: 32
            image.anchors.horizontalCenterOffset: -20

            onClicked: {
                var images = []
                for (var i = 0; i < thumbnail.listModel.count; i++) {
                    if (thumbnail.listModel.get(i).checked) {
                        images.push(thumbnail.listModel.get(i))
                    }
                }
                if (images.length === 0)
                    return

                imageFormatSelectII.images = images
                imageFormatSelectII.show()
                rectangleShade.opacity = 0.7

                thumbnail.isChecked = false
                uncheckAll()
            }
        }

        WzButton {
            id: deleteButton
            width: 90
            height: 44
            radius: 4
            text: qsTr("删除")
            label.anchors.horizontalCenterOffset: 14
            label.anchors.verticalCenterOffset: 0
            label.font.pixelSize: {
                switch(WzI18N.language) {
                case "zh": return 17
                case "en": return 15
                default: return 17
                }
            }
            imageVisible: true
            imageSourceNormal: "qrc:/images/button_cancel.svg"
            image.sourceSize.width: 16
            image.sourceSize.height: 16
            image.anchors.horizontalCenterOffset: -20
            onClicked: {
                msgBox.buttonClicked.connect(deleteQuestion)
                msgBox.show(qsTr("确定删除吗？"), qsTr("确定"), qsTr("取消"))
            }
        }

        WzButton {
            id: selectAllButton
            width: 90
            height: 44
            radius: 4
            text: qsTr("全选")
            label.anchors.horizontalCenterOffset: 14
            label.anchors.verticalCenterOffset: 0
            label.font.pixelSize: {
                switch(WzI18N.language) {
                case "zh": return 17
                case "en": return 15
                default: return 17
                }
            }
            imageVisible: true
            imageSourceNormal: "qrc:/images/button-all.svg"
            image.sourceSize.width: 16
            image.sourceSize.height: 16
            image.anchors.horizontalCenterOffset: -20

            onClicked: {
                for (var i = 0; i < thumbnail.listModel.count; i++) {
                    thumbnail.listModel.get(i).checked = true
                }
            }
        }

        WzButton {
            id: selectInvertButton
            width: 70
            height: 44
            radius: 4
            text: qsTr("反选")

            onClicked: {
                for (var i = 0; i < thumbnail.listModel.count; i++) {
                    if (thumbnail.listModel.get(i).checked)
                        thumbnail.listModel.get(i).checked = false
                    else
                        thumbnail.listModel.get(i).checked = true
                }
            }
        }

        WzButton {
            id: exitCheckedModeButton
            width: 120
            height: 44
            radius: 4
            text: qsTr("退出选择模式")
            onClicked: {
                thumbnail.isChecked = false
                uncheckAll()
            }
        }
    }

    function deleteQuestion(buttonID) {
        msgBox.buttonClicked.disconnect(deleteQuestion)
        if (buttonID !== 1)
            return
        for (var i = thumbnail.listModel.count - 1; i > -1; i--) {
            if (thumbnail.listModel.get(i).checked) {
                imageService.deleteImage(JSON.stringify(thumbnail.listModel.get(i)))
                thumbnail.listModel.remove(i)
            }
        }
        thumbnail.isChecked = false
        uncheckAll()
    }

    function uncheckAll() {
        for (var i = 0; i < thumbnail.listModel.count; i++) {
            if (thumbnail.listModel.get(i).checked)
                thumbnail.listModel.get(i).checked = false
        }
    }
}
