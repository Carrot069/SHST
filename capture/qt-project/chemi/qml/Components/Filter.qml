import QtQuick 2.12

Rectangle {
    property bool isNull: true
    property bool active: false
    property string text
    property alias font: caption.font
    signal clicked()    

    id: root
    antialiasing: true
    width: 42
    height: 42
    radius: 21
    border.width: active ? 3 : 1
    opacity: active ? 1 : 0.5
    color: "transparent"
    Text {
        id: caption
        anchors.verticalCenter: parent.verticalCenter
        anchors.horizontalCenter: parent.horizontalCenter
        text: isNull ? qsTr("<空>") : root.text
        color: root.border.color
    }
    MouseArea {
        anchors.fill: parent
        onClicked: {
            root.clicked()
        }
    }

    Behavior on opacity {
        NumberAnimation { duration: 500 }
    }
}
