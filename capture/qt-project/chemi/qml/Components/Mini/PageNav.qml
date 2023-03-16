import QtQuick 2.0
import "../../WzControls"

Item {
    height: 40
    width: 163

    signal clicked(int buttonIndex)

    WzImageButton {
        id: imageCamera
        y: 1
        width: 44
        height: 44
        image.source: "qrc:/images/camera_white.svg"
        image.sourceSize.width: width
        image.sourceSize.height: height
        imageHot.source: "qrc:/images/camera_cccccc.svg"
        imageHot.sourceSize.width: width
        imageHot.sourceSize.height: height
        anchors.verticalCenter: parent.verticalCenter
        onClicked: parent.clicked(0)
        animationDuration: 500
    }
    WzImageButton {
        id: imageImage
        width: 45
        height: 42
        image.source: "qrc:/images/image_ffffff.svg"
        image.sourceSize.width: width
        image.sourceSize.height: height
        imageHot.source: "qrc:/images/image_cccccc.svg"
        imageHot.sourceSize.width: width
        imageHot.sourceSize.height: height
        anchors.left: imageCamera.right
        anchors.leftMargin: 15
        anchors.verticalCenter: parent.verticalCenter
        onClicked: parent.clicked(1)
        animationDuration: 500
    }
    WzImageButton {
        id: buttonOption
        width: 39
        height: 36
        image.source: "qrc:/images/button_option_ffffff.svg"
        image.sourceSize.width: width
        image.sourceSize.height: height
        imageHot.source: "qrc:/images/button_option_hot.svg"
        imageHot.sourceSize.width: width
        imageHot.sourceSize.height: height
        anchors.left: imageImage.right
        anchors.leftMargin: 15
        anchors.verticalCenter: parent.verticalCenter
        onClicked: parent.clicked(2)
        animationDuration: 500
    }
}
