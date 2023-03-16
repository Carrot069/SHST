import QtQuick 2.12
import QtQuick.Controls 2.12

import WzI18N 1.0

import "../../WzControls"

Item {
    signal saveAs()
    signal open()
    signal printImage()

    WzButton {
        id: buttonSaveAs
        width: 109
        height: 44
        radius: 4
        text: qsTr("另存为")
        label.anchors.leftMargin: 5
        label.font.pixelSize: {
            switch(WzI18N.language) {
            case "zh": return 18
            case "en": return 15
            }
        }
        label.anchors.horizontalCenterOffset: 15
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_save_b4b4b4.svg"
        image.sourceSize.width: 18
        image.sourceSize.height: 18
        image.anchors.horizontalCenterOffset: -30
        onClicked: {
            saveAs()
        }
    }

    WzButton {
        id: buttonOpen
        width: 95
        height: 44
        radius: 4
        text: qsTr("打开")
        label2.color: "#ffffff"
        label.color: "#ffffff"
        box.color: "#ffffff"
        anchors {
            left: buttonSaveAs.right
            leftMargin: 18
        }
        label.anchors.horizontalCenterOffset: 13
        label.font: buttonSaveAs.label.font
        label.anchors.left: label2.right
        label.anchors.horizontalCenter: undefined
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_open.svg"
        image.sourceSize.width: 24
        image.sourceSize.height: 24
        image.anchors.horizontalCenterOffset: -22
        onClicked: {
            open()
        }
    }

    WzButton {
        id: buttonPrint
        width: 95
        height: 44
        radius: 4
        text:  qsTr("打印")
        anchors {
            left: buttonOpen.right
            leftMargin: 18
        }
        label.font: buttonSaveAs.label.font
        label.anchors.horizontalCenterOffset: 13
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_print.svg"
        image.sourceSize.width: 24
        image.sourceSize.height: 24
        image.anchors.horizontalCenterOffset: -21
        onClicked: {
            printImage()
        }
    }

    WzButton {
        id: buttonAnalysis
        width: 95
        height: 44
        radius: 4
        text: qsTr("分析")
        anchors.left: buttonPrint.right
        anchors.leftMargin: 18
        anchors.top: buttonSaveAs.top
        label.anchors.leftMargin: 5
        label.font: buttonSaveAs.label.font
        label.anchors.horizontalCenterOffset: 15
        imageVisible: true
        imageSourceNormal: "qrc:/images/button-chart.svg"
        image.sourceSize.width: 28
        image.sourceSize.height: 28
        image.anchors.horizontalCenterOffset: -22
        onClicked: {
            var imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)
            if (imageInfo) {
                var result = imageService.analysisImage(imageInfo)
                if (result.code !== 0) {
                    msgBox.show(result.msg, qsTr("确定"))
                } else {
                    msgBox.show(qsTr("请稍候, 正在打开分析软件"))
                }
            }
        }
    }
}
