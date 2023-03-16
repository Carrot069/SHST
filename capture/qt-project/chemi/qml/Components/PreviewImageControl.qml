import QtQuick 2.12

import "../WzControls"

Rectangle {
    id: root

    opacity: 0.99
    radius: 5
    height: 70
    width: 122
    color: "black"
    border.color: "#777777"

    Behavior on anchors.rightMargin { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
    Behavior on anchors.bottomMargin { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }

    WzButton {
        id: buttonZoomOriginal
        text: "1:1"
        label.font.family: "Arial"
        label.font.pixelSize: 13
        anchors.right: buttonZoomAuto.left
        anchors.rightMargin: 5
        anchors.top: buttonZoomOut.top
        width: buttonZoomOut.width
        height: width
        radius: width
        border.color: buttonZoomOut.border.color
        onClicked: {
            forceActiveFocus()
            liveImageView.zoom = 100
        }
    }
    WzButton {
        id: buttonZoomAuto
        anchors.right: buttonZoomIn.left
        anchors.rightMargin: 5
        anchors.top: buttonZoomOut.top
        width: buttonZoomOut.width
        height: width
        radius: width
        border.color: buttonZoomOut.border.color
        imageVisible: true
        image.source: "qrc:/images/fullscreen_b4b4b4.svg"
        image.sourceSize.width: 16
        image.sourceSize.height: 16
        onClicked: {
            forceActiveFocus()
            liveImageView.fit(imageViewWrapper.width, imageViewWrapper.height)
        }
    }
    WzButton {
        id: buttonZoomIn
        label.font.family: "Arial"
        anchors.right: buttonZoomOut.left
        anchors.rightMargin: 5
        anchors.top: buttonZoomOut.top
        width: buttonZoomOut.width
        height: width
        radius: width
        border.color: buttonZoomOut.border.color
        onClicked: {
            forceActiveFocus()
            liveImageView.zoom = liveImageView.zoom + 10
        }
        Rectangle {
            height: 1
            width: parent.width - 8
            color: "#b4b4b4"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
        Rectangle {
            width: 1
            height: parent.height - 8
            color: "#b4b4b4"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    WzButton {
        id: buttonZoomOut
        label.font.family: "Arial"
        anchors.right: textInputZoom.right
        anchors.rightMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        width: 22
        height: width
        radius: width
        border.color: "#333333"
        onClicked: {
            forceActiveFocus()
            liveImageView.zoom = liveImageView.zoom - 10
        }
        Rectangle {
            height: 1
            width: parent.width - 8
            color: "#b4b4b4"
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.verticalCenter: parent.verticalCenter
        }
    }
    TextInput {
        id: textInputZoom
        text: liveImageView.zoom + "%"
        //text: "10000%"
        clip: true
        width: parent.width - 20
        anchors.right: parent.right
        anchors.rightMargin: 10
        anchors.top: parent.top
        anchors.topMargin: 10
        font.pixelSize: 18
        font.family: "Arial"
        color: "#b4b4b4"
        horizontalAlignment: TextInput.AlignHCenter
        verticalAlignment: TextInput.AlignVCenter
        selectByMouse: true
        onEditingFinished: {
            var i = parseInt(text.replace("%", ""))
            liveImageView.zoom = i
        }        
    }

}
