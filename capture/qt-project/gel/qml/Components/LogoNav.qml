import QtQuick 2.12
import "../WzControls"
import WzUtils 1.0

Item {
    id: root
    height: 96

    Image {
        id: imageLogoFlower
        width: 110
        visible: WzUtils.isNoLogo() ? false : true
        source: "qrc:/images/logo_flower.png"
        y: 0
        MouseArea {
            anchors.fill: parent
            onClicked: showAbout()
        }
        WzJumpAnimation {
            target: imageLogoFlower
            jumpHeight: 10
            cursorShape: Qt.PointingHandCursor
        }
    }
    Image {
        id: imageLogoText
        visible: WzUtils.isNoLogo() ? false : true
        source: "qrc:/images/logo_text.png"
        anchors.left: imageLogoFlower.right
        anchors.leftMargin: 5
        anchors.verticalCenter: parent.verticalCenter
    }

    PageNav {
        id: pageNav
        anchors.verticalCenterOffset: 2
        anchors.right: parent.right
        anchors.rightMargin: 20
        anchors.verticalCenter: parent.verticalCenter
        onClicked: pageChange(buttonIndex)
    }
}
