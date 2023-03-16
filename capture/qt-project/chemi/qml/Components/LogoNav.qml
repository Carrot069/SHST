import QtQuick 2.12
import WzUtils 1.0
import WzI18N 1.0
import "../WzControls"

Item {
    id: root
    height: 96

    Image {
        id: imageLogoFlower
        visible: !WzUtils.isNoLogo()
        width: 110
        source: WzUtils.isNoLogo() ? "" : "qrc:/images/logo_flower.png"
        y: 0
        MouseArea {
            anchors.fill: parent
            onClicked: if (!WzUtils.isOEM()) showAbout()
        }
        WzJumpAnimation {
            target: imageLogoFlower
            jumpHeight: 10
            cursorShape: Qt.PointingHandCursor
        }
    }
    Image {
        id: imageLogoText
        visible: !WzUtils.isNoLogo()
        source: {
            if (WzUtils.isNoLogo())
                return ""
            switch(WzI18N.language) {
            case "en":
                return "qrc:/images/logo_text4.png"
            default:
                return "qrc:/images/logo_text.png"
            }
        }
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
