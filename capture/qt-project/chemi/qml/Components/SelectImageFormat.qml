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

    property string imageFormat: ""

    signal hide()
    signal confirm(string format)

    function show() {
        height = 240
        width = 480
        opacity = 1
    }

    Behavior on height {NumberAnimation {duration: 400}}
    Behavior on width {NumberAnimation {duration: 400}}
    Behavior on opacity {NumberAnimation{duration: 500}}

    Text {
        id: textImageFormat
        text: qsTr("请选择图片格式:")
        anchors.left: parent.left
        anchors.leftMargin: 40
        anchors.top: parent.top
        anchors.topMargin: 40
        color: "#dddddd"
        font.pixelSize: 15
    }

    WzRadioButton {
        id: rbTIFF16bit
        anchors.left: textImageFormat.left
        anchors.leftMargin: -10
        anchors.top: textImageFormat.bottom
        anchors.topMargin: 20
        text: qsTr("16bit TIFF")
        checked: true
    }
    WzRadioButton {
        id: rbTIFF8bit
        anchors.left: rbTIFF16bit.right
        anchors.leftMargin: 10
        anchors.bottom: rbTIFF16bit.bottom
        text: qsTr("8bit TIFF")
        checked: false
    }
    WzRadioButton {
        id: rbJPEG
        anchors.left: rbTIFF8bit.right
        anchors.leftMargin: 10
        anchors.bottom: rbTIFF16bit.bottom
        text: qsTr("JPEG")
        checked: false
    }
    WzRadioButton {
        id: rbPNG
        anchors.left: rbJPEG.right
        anchors.leftMargin: 10
        anchors.bottom: rbTIFF16bit.bottom
        text: qsTr("PNG")
        checked: false
    }

    WzButton {
        id: buttonCancel
        text: qsTr("取消")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -(width / 2 + 5)
        anchors.bottom: buttonOk.bottom
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
        text: qsTr("确定")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: width / 2 + 5
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 40
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
        label.anchors.horizontalCenterOffset: 13
        radius: 3
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_right_b4b4b4.svg"
        image.sourceSize.width: 24
        image.sourceSize.height: 24
        image.anchors.horizontalCenterOffset: -23
        onClicked: {
            if (rbTIFF16bit.checked)
                imageFormat = "tiff16"
            else if (rbTIFF8bit.checked)
                imageFormat = "tiff8"
            else if (rbJPEG.checked)
                imageFormat = "jpeg"
            else if (rbPNG.checked)
                imageFormat = "png"

            hide()
            root.opacity = 0
            root.height = 0
            root.width = 0

            confirm(imageFormat)
        }
    }

}


/*##^## Designer {
    D{i:0;autoSize:true;height:450;width:240}D{i:7;anchors_width:497}
}
 ##^##*/
