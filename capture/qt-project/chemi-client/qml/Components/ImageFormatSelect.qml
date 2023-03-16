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
    signal selectFormat(int imageFormat)

    function show() {
        var imageFormat = dbService.readIntOption("share_image_format", 0)
        setImageFormat(imageFormat)

        height = 180
        width = 510
        opacity = 1
    }

    function getImageFormat() {
        if (rbTIFF16bit.checked)
            return 0
        else if (rbTIFF8bit.checked)
            return 1
        else if (rbJPEG.checked)
            return 2
        else if (rbPNG.checked)
            return 3
        else
            return -1
    }

    function setImageFormat(imageFormat) {
        switch (imageFormat) {
        case 0:
            rbTIFF16bit.checked = true
            return
        case 1:
            rbTIFF8bit.checked = true
            return
        case 2:
            rbJPEG.checked = true
            return
        case 3:
            rbPNG.checked = true
            return
        }
    }

    Behavior on height {NumberAnimation {duration: 400}}
    Behavior on width {NumberAnimation {duration: 400}}
    Behavior on opacity {NumberAnimation{duration: 500}}

    Text {
        id: textImageFormat
        text: qsTr("请选择要分享图片的格式:")
        anchors.left: parent.left
        anchors.leftMargin: 20
        anchors.top: parent.top
        anchors.topMargin: 20
        color: "#dddddd"
        font.pixelSize: 15
    }

    WzRadioButton {
        id: rbTIFF16bit
        text: "16bit TIFF"
        checked: false
        anchors.top: textImageFormat.bottom
        anchors.topMargin: 10
        anchors.left: textImageFormat.left
        anchors.leftMargin: -8
    }

    WzRadioButton {
        id: rbTIFF8bit
        text: "8bit TIFF"
        checked: false
        anchors.top: rbTIFF16bit.top
        anchors.left: rbTIFF16bit.right
        anchors.leftMargin: 20
    }

    WzRadioButton {
        id: rbJPEG
        text: "JPEG"
        checked: false
        anchors.top: rbTIFF16bit.top
        anchors.left: rbTIFF8bit.right
        anchors.leftMargin: 20
    }

    WzRadioButton {
        id: rbPNG
        text: "PNG"
        checked: false
        anchors.top: rbTIFF16bit.top
        anchors.left: rbJPEG.right
        anchors.leftMargin: 20
    }

    WzButton {
        id: buttonOk
        text: qsTr("确定")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: width / 2 + 10
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
        label.anchors.horizontalCenterOffset: 13
        radius: 3
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_ok.svg"
        image.anchors.horizontalCenterOffset: -24
        image.sourceSize.width: 22
        image.sourceSize.height: 22
        onClicked: {
            dbService.saveIntOption("share_image_format", getImageFormat())
            selectFormat(getImageFormat())

            hide()
            root.opacity = 0
            root.height = 0
            root.width = 0
        }
    }

    WzButton {
        id: buttonCancel
        text: qsTr("取消")
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.horizontalCenterOffset: -(width / 2) - 10
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
        label.anchors.horizontalCenterOffset: 11
        radius: 3
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_cancel.svg"
        image.anchors.horizontalCenterOffset: -23
        image.sourceSize.width: 16
        image.sourceSize.height: 16
        onClicked: {
            hide()
            root.opacity = 0
            root.height = 0
            root.width = 0
        }
    }

}


/*##^## Designer {
    D{i:0;autoSize:true;height:450;width:240}D{i:7;anchors_width:497}
}
 ##^##*/
