import QtQuick 2.12

import WzI18N 1.0

Rectangle {
    id: root
    width: 180
    height: 50
    color: "transparent"
    clip: true
    property int textWidth: 60
    property int activeIndex: -1
    property var texts: [qsTr("自动"), qsTr("手动"), qsTr("累积")]
    property bool isManual: activeIndex === 1
    property bool isAuto: activeIndex === 0
    property bool isMulti: activeIndex === 2
    property int fontPixelSize: WzI18N.language === "zh" ? 22 : 15

    function changeIndex(actIndex) {
        if (actIndex === activeIndex)
            return
        if (activeIndex === -1)
            activeIndex = 0
        switch(activeIndex) {
        case 0:
            switch(actIndex) {
            case 0:
                break

            case 1:
                row1Animation.from = textWidth * 1
                row1Animation.to = textWidth * 0
                row2Animation.from = textWidth * -2
                row2Animation.to = textWidth * -3
                animation.start()
                break

            case 2:
                row1Animation.from = textWidth
                row1Animation.to = textWidth * 2
                row2Animation.from = textWidth * 2 * -1
                row2Animation.to = textWidth * 1 * -1
                animation.start()
                break
            }
            break

        case 1:
            switch(actIndex) {
            case 0:
                row1Animation.from = 0
                row1Animation.to = textWidth
                row2Animation.from = textWidth * 3 * -1
                row2Animation.to = textWidth * 2 * -1
                animation.start()
                break
            case 1:
                break
            case 2:
                row1Animation.from = 0
                row1Animation.to = textWidth * -1
                row2Animation.from = textWidth * 3
                row2Animation.to = textWidth * 2
                animation.start()
                break
            }
            break

        case 2:
            switch(actIndex) {
            case 0:
                row1Animation.from = textWidth * -1
                row1Animation.to = textWidth * -2
                row2Animation.from = textWidth * 2
                row2Animation.to = textWidth * 1
                animation.start()
                break

            case 1:
                row1Animation.from = textWidth * 1 * -1
                row1Animation.to = textWidth * 0
                row2Animation.from = textWidth * 2
                row2Animation.to = textWidth * 3
                animation.start()
                break

            case 2:
                break
            }
            break
        }

        activeIndex = actIndex
        console.log("CaptureMode, index = ", activeIndex)
    }

    Row {
        id: row1
        x: 0
        height: 30
        width: textWidth * itemCount
        spacing: 0
        property int itemCount: 3

        Repeater {
            model: parent.itemCount
            Text {
                text: root.texts[model.index]
                font.pixelSize: root.fontPixelSize
                color: "white"
                height: root.height
                width: root.textWidth
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        changeIndex(model.index)
                    }
                }
            }
        }
    }

    Row {
        id: row2
        x: row1.width
        height: parent.height
        width: textWidth * itemCount
        spacing: 0
        property int itemCount: 3

        Repeater {
            model: parent.itemCount
            Text {
                text: root.texts[model.index]
                font.pixelSize: fontPixelSize
                color: "white"
                height: root.height
                width: root.textWidth
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        changeIndex(model.index)
                    }
                }
            }
        }
    }

    Rectangle {
        width: 6
        height: 6
        radius: width
        color: "#32C5FF"
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
    }

    ParallelAnimation {
        id: animation

        NumberAnimation {
            id: row1Animation
            target: row1
            property: "x"
            duration: 500
            easing.type: Easing.OutBack
        }
        NumberAnimation {
            id: row2Animation
            target: row2
            property: "x"
            duration: 500
            easing.type: Easing.OutBack
        }
    }
}
