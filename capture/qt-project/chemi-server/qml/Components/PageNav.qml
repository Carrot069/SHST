import QtQuick 2.0
import "../WzControls"

Item {
    height: 32
    width: 100

    signal clicked(int buttonIndex)

    WzImageButton {
        id: imageCamera
        y: 1
        width: 32
        height: 32
        image.source: "qrc:/images/camera.svg"
        image.sourceSize.height: 24
        image.sourceSize.width: 24
        imageHot.source: "qrc:/images/camera_cccccc.svg"
        imageHot.sourceSize.height: 24
        imageHot.sourceSize.width: 24
        onClicked: parent.clicked(0)
    }
    WzImageButton {
        id: imageImage
        width: 32
        height: 32
        image.source: "qrc:/images/image.svg"
        image.sourceSize.height: 24
        image.sourceSize.width: 24
        imageHot.source: "qrc:/images/image_cccccc.svg"
        imageHot.sourceSize.height: 24
        imageHot.sourceSize.width: 24
        anchors.left: imageCamera.right
        anchors.leftMargin: 2
        onClicked: parent.clicked(1)
    }
    WzButton {
        id: buttonOption
        width: 32
        height: 32
        text: "@"
        label.font.family: "Webdings"
        label.font.pixelSize: 30
        label.anchors.fill: label.parent
        anchors.left: imageImage.right
        anchors.leftMargin: 1
        normalColor: "transparent"
        downColor: "transparent"
        hotColor: "transparent"
        normalFontColor: "#555555"
        hotFontColor: "#cccccc"
        downFontColor: "#555555"
        onClicked: parent.clicked(2)
    }
}
