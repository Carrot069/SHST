import QtQuick 2.12
import QtQuick.Controls 2.12

MenuSeparator {
    id: menuSeparator
    property alias color: rect.color
    topPadding: 0
    bottomPadding: 0
    leftPadding: 0
    rightPadding: 0
    contentItem: Rectangle {
        id: rect
        implicitWidth: menuSeparator.width
        implicitHeight: 1
        color: menuSeparator.color
    }
}
