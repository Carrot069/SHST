import QtQuick 2.12
import WzUtils 1.0
import WzI18N 1.0
import "../WzControls"

Rectangle {
    id: root

    width: {
        switch(WzI18N.language) {
        case "zh": return 523
        case "en": return 580
        }
    }
    height: 256
    anchors.verticalCenter: parent.verticalCenter
    anchors.horizontalCenter: parent.horizontalCenter
    color: "black"
    border.color: "#707070"
    border.width: 3
    radius: 5
    antialiasing: true

    Behavior on opacity {NumberAnimation {duration: 500}}

    WzButton {
        id: buttonClose
        height: 27
        width: 27
        anchors.right: parent.right
        anchors.rightMargin: 5
        anchors.top: parent.top
        anchors.topMargin: 5
        normalColor: "transparent"
        normalFontColor: "#606060"
        hotFontColor: "white"
        text: "r"
        label.font.family: "Webdings"
        radius: 15
        onClicked: {
            root.opacity = 0
        }
    }

    Image {
        id: imageLogoFlower
        source: "qrc:/images/logo_flower.png"
        anchors.verticalCenter: parent.verticalCenter
        anchors.left: parent.left
        anchors.leftMargin: 30
    }

    Text {
        id: textCompanyName
        text: qsTr("杭州申花科技有限公司")
        font.pixelSize: 20
        color: "#555555"
        anchors.right: parent.right
        anchors.rightMargin: {
            switch(WzI18N.language) {
            case "zh": return 80
            case "en": return 40
            }
        }
        anchors.top: imageLogoFlower.top
        anchors.topMargin: -5
    }

    Text {
        id: textSoftwareName
        text: WzUtils.appName() + " " + WzUtils.appVersion()
        font.pixelSize: 20
        color: "#555555"
        anchors.horizontalCenter: textCompanyName.horizontalCenter
        anchors.verticalCenter: imageLogoFlower.verticalCenter
        anchors.verticalCenterOffset: -2
    }

    Text {
        id: textWebsite
        text: qsTr("http://www.shenhua.bio")
        font.pixelSize: 20
        color: "#555555"
        anchors.horizontalCenter: textCompanyName.horizontalCenter
        anchors.bottom: imageLogoFlower.bottom
        MouseArea {
            anchors.fill: parent
            cursorShape: Qt.PointingHandCursor
            onClicked: Qt.openUrlExternally(parent.text)
        }
    }
}
