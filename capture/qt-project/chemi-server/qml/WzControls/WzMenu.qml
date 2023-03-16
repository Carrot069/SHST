import QtQuick 2.12
import QtQuick.Controls 2.12

Menu {
    id: root
    opacity: 0.98

    property int itemHeight: 45
    property color itemBackground: "#1e1e1e"
    property color itemBackgroundHighlighted: "#525252"
    property color fontColor: "#b4b4b4"
    property color fontColorHighlighted: "#b4b4b4"
    property bool isSeparator: true
    property color separatorColor: "#525252"
    property int indicatorWidth: itemHeight

    function add(caption, isSeparator) {
        if (isSeparator && root.count > 0) {
            var newObject = Qt.createQmlObject('import QtQuick 2.12; import QtQuick.Controls 2.12; MenuSeparator {contentItem: Rectangle {implicitWidth: root.width; implicitHeight: 1; color: root.separatorColor}}', root)
            newObject.topPadding = 0
            newObject.bottomPadding = 0
            newObject.leftPadding = 0
            newObject.rightPadding = 0
            root.addItem(newObject)
        }

        var obj = newAction.createObject(root, {text: caption})
        root.addAction(obj)
    }

    Component {
        id: newAction
        Action {}
    }
    Component {
        id: menuSeparator
        MenuSeparator {
            contentItem: Rectangle {
                implicitWidth: root.width
                implicitHeight: 1
                color: root.separatorColor
            }
        }
    }

    delegate: MenuItem {
        id: menuItem
        implicitWidth: root.width
        implicitHeight: root.itemHeight
        autoExclusive: true

        indicator: Item {
            implicitWidth: root.indicatorWidth
            implicitHeight: root.itemHeight

            Text {
                anchors.centerIn: parent
                visible: menuItem.checked
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                text: ">"
                color: menuItem.highlighted ? root.fontColorHighlighted : root.fontColor
            }
        }

        contentItem: Text {
            leftPadding: root.indicatorWidth
            rightPadding: menuItem.arrow.width
            text: menuItem.text
            font: root.font
            opacity: enabled ? 1.0 : 0.3
            color: menuItem.highlighted ? root.fontColorHighlighted : root.fontColor
            horizontalAlignment: Text.AlignLeft
            verticalAlignment: Text.AlignVCenter
            elide: Text.ElideRight
        }

        background: Rectangle {
            implicitWidth: root.width
            implicitHeight: root.itemHeight
            opacity: enabled ? 1 : 0.3
            color: menuItem.highlighted ? root.itemBackgroundHighlighted : "transparent"
        }
    }

    background: Rectangle {
        implicitWidth: root.width
        implicitHeight: root.itemHeight
        color: root.itemBackground
    }
}
