import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
    id: root
    height: 100
    width: 500

    function append(response){func.append(response)}
    signal close()
    signal show()

    Item {
        anchors {
            fill: parent
            margins: 5
        }
        Rectangle {
            anchors.fill: parent
            radius: 5
            color: "black"
            opacity: 0.8
        }

        ScrollView {
            id: view
            anchors {
                fill: parent
                margins: 5
            }

            TextArea {
                background: Item{ }
                id: textAreaResponse
            }
        }

        Text {
            color: "white"
            anchors {
                right: parent.right
                rightMargin: 3
                top: parent.top
                topMargin: 3
            }
            text: "[X]"
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    root.visible = false
                    root.close()
                }
            }
        }
    }

    QtObject {
        id: func
        function append(response) {
            if (!root.visible) {
                root.show()
                root.visible = true
            }
            textAreaResponse.append(response)
        }
    }
}
