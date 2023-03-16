import QtQuick 2.12
import QtQuick.Controls 2.12
import WzImage 1.0
import WzI18N 1.0
import "../WzControls"

Rectangle {
    id: root
    color: "black"
    border.color: "#aaaaaa"
    border.width: 3
    radius: 5
    clip: true
    antialiasing: true
    width: 400
    height: 320

    property int paletteIndex: -1
    property var pseudoList: []
    onPseudoListChanged: {
        listModel.clear()
        for (var i = 0; i < pseudoList.length; i++)
            listModel.append(pseudoList[i])
    }

    signal preview(var palette)
    signal close(bool isOk)

    function setActive(paletteName) {
        for (var i = 0; i < pseudoList.length; i++)
            if (listModel.get(i).name === paletteName) {
                listView.currentIndex = i
                return
            }
    }

    function backupConfig() {
        paletteIndex = listView.currentIndex
    }

    function restoreConfig() {
        if (paletteIndex !== listView.currentIndex) {
            listView.currentIndex = paletteIndex
            preview(listModel.get(paletteIndex))
        }
    }

    ListModel {
        id: listModel
    }

    MouseArea {
        anchors.fill: parent
        onClicked: forceActiveFocus()
    }

    Rectangle {
        id: listViewBackground
        anchors.fill: listView
        anchors.rightMargin: 10
        color: "#090909"
        radius: 3
    }

    ListView {
        id: listView
        currentIndex: 0
        focus: true
        anchors.top: parent.top
        anchors.topMargin: 10
        anchors.bottom: buttonOk.top
        anchors.bottomMargin: 10
        anchors.left: parent.left
        anchors.leftMargin: 5
        anchors.right: parent.right
        anchors.rightMargin: 10
        clip: true
        ScrollBar.vertical: ScrollBar{}

        model: listModel

        delegate: Item {
            height: 28
            width: parent.width


            WzRadioButton {
                id: rb
                anchors.verticalCenter: parent.verticalCenter
                width: 100
                text: model.name

                font.family: "Arial"
                checked: listView.currentIndex === model.index
                onClicked: listView.currentIndex = model.index
                onCheckedChanged: if (checked) preview(model)
            }

            Rectangle {
                anchors.verticalCenter: parent.verticalCenter
                anchors.right: parent.right
                width: 258
                height: 19
                border.width: 1
                border.color: "#bbbbbb"
                color: "transparent"
                clip: true
                WzPseudoColor {
                    id: pc
                    anchors.fill: parent
                    anchors.margins: 1
                    rgbList: {
                        // from QQmlListModel to QJSValue
                        var arr = []
                        for(var n = 0; n < model.rgbList.count; n++) {
                            var rgb = {
                                R: model.rgbList.get(n).R,
                                G: model.rgbList.get(n).G,
                                B: model.rgbList.get(n).B
                            }
                            arr.push(rgb)
                        }
                        return arr
                   }
                }
            }

            MouseArea {
                anchors.left: rb.right
                anchors.right: parent.right
                height: parent.height
                onClicked: {
                    preview(model)
                    listView.currentIndex = model.index
                }
            }
        }
    }

    WzButton {
        id: buttonCancel
        text: qsTr("取消")
        anchors.right: buttonOk.left
        anchors.rightMargin: 10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15
        width: 95
        height: 43
        label.font.pixelSize: {
            switch(WzI18N.language) {
            case "zh": return 18
            case "en": return 15
            }
        }
        label.anchors.horizontalCenterOffset: 13
        radius: 3
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_cancel_b4b4b4.svg"
        image.sourceSize.width: 16
        image.sourceSize.height: 16
        image.anchors.horizontalCenterOffset: -23

        onClicked: {
            close(false)
        }
    }

    WzButton {
        id: buttonOk
        text: qsTr("确定")
        anchors.right: parent.right
        anchors.rightMargin: 15
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 15
        width: 95
        height: 43
        label.font.pixelSize: {
            switch(WzI18N.language) {
            case "zh": return 18
            case "en": return 15
            }
        }
        label.anchors.leftMargin: {
            switch(WzI18N.language) {
            case "zh": return 5
            case "en": return -2
            }
        }
        label.anchors.horizontalCenterOffset: 13
        radius: 3
        imageVisible: true
        imageSourceNormal: "qrc:/images/button_right_b4b4b4.svg"
        image.sourceSize.width: 24
        image.sourceSize.height: 24
        image.anchors.horizontalCenterOffset: -23
        onClicked: {
            close(true)
        }
    }

}


/*##^## Designer {
    D{i:0;autoSize:true;height:450;width:240}D{i:7;anchors_width:497}
}
 ##^##*/
