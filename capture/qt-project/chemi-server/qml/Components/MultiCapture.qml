import QtQuick 2.12
import QtQuick.Controls 2.12
import WzUtils 1.0
import WzI18N 1.0
import "../WzControls"

Rectangle {
    id: root
    color: "black"
    border.color: "#666666"
    clip: true
    antialiasing: true
    width: 450
    height: 240

    signal multiCapture()

    function show() {
        height = getHeight()
        opacity = 1
    }
    function hide() {
        opacity = 0
        height = 0
    }

    function params() {
        var p = {}
        switch(contentView.currentIndex) {
        case 0:
            p.multiType = "gray"
            break
        case 1:
            p.multiType = "time"
            break
        default:
            p.multiType = "time_list"
        }
        p.grayAccumulate_frameCount = spinBoxGrayAccumulate.value
        p.timeAccumulate_frameCount = spinBoxTimeAccumulateCount.value
        p.timeAccumulate_exposureMs = spinBoxTimeAccumulateTime.getExposureMs()
        p.timeSequenceExposureMs = exposureTimeSequenceTime.getExposureMs()
        p.timeSequenceExposureMsList = []

        for (var i = 0; i < listModel.count; i++) {
            p.timeSequenceExposureMsList.push(listModel.get(i).exposureMs)
        }

        return p
    }

    function setParams(params) {
        if (params === undefined) return

        var tabIndex = 0
        if (params.multiType === "gray")
            tabIndex = 0
        else if (params.multiType === "time")
            tabIndex = 1
        else if (params.multiType === "time_list")
            tabIndex = 2

        tabBar.currentIndex = tabIndex
        tabBar.itemAt(tabIndex).clicked()

        if (undefined !== params.grayAccumulate_frameCount)
            spinBoxGrayAccumulate.value = params.grayAccumulate_frameCount

        if (undefined !== params.timeAccumulate_frameCount)
            spinBoxTimeAccumulateCount.value = params.timeAccumulate_frameCount

        if (undefined !== params.timeAccumulate_exposureMs)
            spinBoxTimeAccumulateTime.setExposureMs(params.timeAccumulate_exposureMs)

        if (undefined !== params.timeSequenceExposureMs)
            exposureTimeSequenceTime.setExposureMs(params.timeSequenceExposureMs)

        if (undefined !== params.timeSequenceExposureMsList) {
            for (var i = 0; i < params.timeSequenceExposureMsList.length; i++) {
                var item = {
                    time: WzUtils.getTimeStr(params.timeSequenceExposureMsList[i], true),
                    exposureMs: params.timeSequenceExposureMsList[i]
                }
                listModel.append(item)
            }
        }
    }

    function getHeight() {
        switch(tabBar.currentIndex) {
        case 2:
            return 373
        default:
            return 315
        }
    }

    Behavior on height {
        NumberAnimation {
            duration: 200
        }
    }
    Behavior on opacity {NumberAnimation{duration: 400}}

    Rectangle {
        anchors.top: parent.top
        anchors.topMargin: 1
        anchors.bottom: tabBar.bottom
        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.right: parent.right
        anchors.rightMargin: 1
        radius: parent.radius
        color: "#151515"
    }
    TabBar {
        id: tabBar
        clip: true
        anchors.left: parent.left
        anchors.leftMargin: 1
        anchors.right: parent.right
        anchors.rightMargin: 1
        anchors.top: parent.top
        anchors.topMargin: 1
        contentHeight: 50

        background: Rectangle {
            color: "#151515"
        }

        WzTabButton {
            id: buttonGrayAccumulation
            text: qsTr("灰度累积")
            font.capitalization: Font.MixedCase
            font.pixelSize: {
                switch(WzI18N.language) {
                case "zh": return 15
                case "en": return 12
                }
            }
            onClicked: {
                contentView.currentIndex = tabBar.currentIndex
                root.height = getHeight()
            }
        }
        WzTabButton {
            text: qsTr("时间累积")
            font: buttonGrayAccumulation.font
            onClicked: {
                contentView.currentIndex = tabBar.currentIndex
                root.height = getHeight()
            }
        }
        WzTabButton {
            text: qsTr("时间序列")
            font: buttonGrayAccumulation.font
            onClicked: {
                contentView.currentIndex = tabBar.currentIndex
                root.height = getHeight()
            }
        }
    }

    SwipeView {
        id: contentView
        currentIndex: 0
        anchors.top: tabBar.bottom
        anchors.bottom: rectBottom.top
        anchors.left: parent.left
        anchors.right: parent.right

        //[[灰度累积]]//
        Item {
            width: 450
            height: 200
            MouseArea {
                anchors.fill: parent
                onClicked: spinBoxGrayAccumulate.focus = false
            }
            Item {
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 20
                anchors.horizontalCenter: parent.horizontalCenter
                implicitWidth: textGrayAccumulate.implicitWidth
                implicitHeight: textGrayAccumulate.implicitHeight + spinBoxGrayAccumulate.implicitHeight
                Text {
                    id: textGrayAccumulate
                    text: qsTr("累积数量")
                    font.pixelSize: 18
                    color: "#b4b4b4"
                }
                Item {
                    height: 50
                    anchors.top: textGrayAccumulate.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: 200
                    WzSpinBox {
                        anchors.horizontalCenter: parent.horizontalCenter
                        id: spinBoxGrayAccumulate
                        isPaddingZero: false
                        font.pixelSize: 34
                        font.family: "Digital-7"
                        fontColor: "#b4b4b4"
                        from: 2
                        to: 99
                        isShowButton: true
                        buttonColor: "#535353"
                        buttonFontColor: "white"
                    }
                }
            }
        }
        //[[灰度累积]]//

        //[[时间累积]]//
        Item {
            width: 450
            height: 200
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    parent.forceActiveFocus()
                }
            }
            Item {
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: 15
                anchors.left: parent.left
                anchors.leftMargin: 60
                width: parent.width
                implicitHeight: textTimeAccumulateCount.implicitHeight + spinBoxTimeAccumulateCount.implicitHeight
                Text {
                    id: textTimeAccumulateCount
                    text: qsTr("累积数量")
                    font.pixelSize: 18
                    color: "#b4b4b4"
                }
                Item {
                    anchors.top: textTimeAccumulateCount.bottom
                    width: textTimeAccumulateCount.implicitWidth
                    height: 50
                    WzSpinBox {
                        id: spinBoxTimeAccumulateCount
                        width: textTimeAccumulateCount.implicitWidth
                        isPaddingZero: false
                        font.pixelSize: 34
                        font.family: "Digital-7"
                        fontColor: "#b4b4b4"
                        from: 1
                        to: 99
                        isShowButton: true
                        buttonColor: "#535353"
                        buttonFontColor: "white"
                        height: 50
                    }
                }

                Text {
                    id: textTimeAccumulateTime
                    anchors.left: textTimeAccumulateCount.right
                    anchors.leftMargin: 70
                    text: qsTr("累积时间")
                    font.pixelSize: 18
                    color: "#b4b4b4"
                }

                WzExposureTime {
                    id: spinBoxTimeAccumulateTime
                    anchors.top: textTimeAccumulateTime.bottom
                    anchors.left: textTimeAccumulateTime.left
                    anchors.leftMargin: -7
                    isShowButton: true
                    fontPixelSize: 36
                    buttonColor: "#404040"
                }
            }
        }

        //[[时间序列]]//
        Item {
            width: 450
            height: 400

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
                anchors.rightMargin: 15
                color: "#090909"
                radius: 3
            }

            ListView {
                id: listView
                currentIndex: -1
                interactive: true
                keyNavigationWraps: true
                focus: true
                highlightFollowsCurrentItem: true
                width: 193
                visible: model.count > 0 ? true : false
                anchors.top: parent.top
                anchors.topMargin: 10
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 10
                anchors.left: parent.left
                anchors.leftMargin: 10
                clip: true
                ScrollBar.vertical: ScrollBar{}

                model: listModel

                add: Transition {
                    ParallelAnimation {
                        NumberAnimation { properties: "x"; from: exposureTimeSequenceTime.x; duration: 300 }
                        NumberAnimation { properties: "y"; from: exposureTimeSequenceTime.y; duration: 300 }
                    }
                }
                addDisplaced: Transition {
                    NumberAnimation { properties: "x,y"; duration: 300 }
                }

                remove: Transition {
                    ParallelAnimation {
                        NumberAnimation { property: "opacity"; to: 0; duration: 300 }
                        NumberAnimation { properties: "x"; to: -200; duration: 300 }
                    }
                }
                removeDisplaced: Transition {
                    NumberAnimation { properties: "x,y"; duration: 300 }
                }

                displaced: Transition {
                    NumberAnimation { properties: "x,y"; duration: 1000 }
                }

                delegate: Item {
                    height: 45
                    width: 177

                    Rectangle {
                        radius: 3
                        height: 43
                        width: 177
                        color: listView.currentIndex == index ? "transparent" : "#111111"

                        Text {
                            id: textTime
                            text: model.time
                            color: "#b4b4b4"
                            anchors.verticalCenter: parent.verticalCenter
                            anchors.verticalCenterOffset: 2
                            anchors.left: parent.left
                            anchors.leftMargin: 12
                            font.pixelSize: 30
                            font.family: "Digital Dismay"
                        }

                        MouseArea {
                            anchors.left: parent.left
                            anchors.right: buttonDeleteItem.left
                            height: parent.height
                            onClicked: {
                                listView.currentIndex = index
                                parent.color = "transparent"
                                listView.focus = true
                            }
                            onDoubleClicked: {
                                exposureTimeSequenceTime.setExposureMs(model.exposureMs)
                            }
                            hoverEnabled: true
                            onEntered: {
                                if (index !== listView.currentIndex) {
                                    parent.color = "#222222"
                                } else {
                                    parent.color = "transparent"
                                }
                            }
                            onExited: {
                                if (index !== listView.currentIndex) {
                                    parent.color = "#111111"
                                }
                            }
                        }


                        WzButton {
                            id: buttonDeleteItem
                            height: 30
                            width: 30
                            anchors.right: parent.right
                            anchors.rightMargin: 8
                            anchors.verticalCenter: parent.verticalCenter
                            normalColor: "transparent"
                            text: "r"
                            label.font.family: "Webdings"
                            label.font.pixelSize: 25
                            onClicked: {
                                listModel.remove(model.index)
                                forceActiveFocus()
                            }
                        }
                    }
                }
                highlight: Item  {
                    width: 177
                    Rectangle {
                        height: 43
                        width: parent.width
                        radius: 3
                        color: "#373737"
                    }
                    Behavior on y { SpringAnimation { spring: 2; damping: 0.2 } }
                }
            }

            WzButton {
                id: buttonEditTime
                text: qsTr("修改")
                anchors.right: parent.right
                anchors.rightMargin: 40
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 12
                width: 83
                height: 43
                label.font.pixelSize: 18
                radius: 3
                onClicked: {
                    if (listView.currentIndex === -1)
                        return
                    var item = {
                        time: WzUtils.getTimeStr(exposureTimeSequenceTime.getExposureMs(), true),
                        exposureMs: exposureTimeSequenceTime.getExposureMs()
                    }
                    listModel.set(listView.currentIndex, item)
                    forceActiveFocus()
                }
            }

            WzButton {
                id: buttonAddTime
                text: qsTr("新增")
                anchors.right: buttonEditTime.left
                anchors.rightMargin: 15
                anchors.top: buttonEditTime.top
                width: 83
                height: 43
                label.font.pixelSize: 18
                radius: 3
                onClicked: {
                    if (exposureTimeSequenceTime.getExposureMs() === 0)
                        return
                    var item = {
                        time: WzUtils.getTimeStr(exposureTimeSequenceTime.getExposureMs(), true),
                        exposureMs: exposureTimeSequenceTime.getExposureMs()
                    }
                    listModel.append(item)
                    listView.currentIndex = listModel.count - 1
                    forceActiveFocus()
                }
            }

            Text {
                id: textTimeSequenceTime
                anchors.bottom: exposureTimeSequenceTime.top
                anchors.bottomMargin: 5
                anchors.left: exposureTimeSequenceTime.left
                anchors.leftMargin: {
                    switch(WzI18N.language) {
                    case "zh": return 7
                    case "en": return 8
                    }
                }
                text: qsTr("曝光时间")
                font.pixelSize: 18
                color: "#b4b4b4"
            }

            WzExposureTime {
                id: exposureTimeSequenceTime
                anchors.bottom: buttonEditTime.top
                anchors.bottomMargin: 5
                anchors.left: buttonAddTime.left
                anchors.leftMargin: -8
                isShowButton: true
                fontPixelSize: 36
                buttonColor: "#404040"
                exposureMs: 10000
            }
        }
    }

    Rectangle {
        id: rectBottom
        height: 75
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        anchors.margins: 1
        color: "#101010"

        WzButton {
            id: buttonMultiFrameCapture
            opacity: enabled ? 1 : 0.4
            width: 140
            height: 48
            radius: 3
            text: qsTr("开始拍摄")
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: ((buttonMultiFrameCapture.width + 5) / 2) * -1
            label.font.pixelSize: 18
            label.horizontalAlignment: Text.AlignRight
            label.anchors.rightMargin: {
                switch(WzI18N.language) {
                case "zh": return 14
                case "en": return 28
                }
            }
            label.anchors.fill: label.parent
            label.anchors.verticalCenterOffset: 1
            imageVisible: true
            imageAlign: Qt.AlignLeft | Qt.AlignVCenter
            imageSourceNormal: "qrc:/images/button_multi_capture_start.png"
            image.anchors.leftMargin: {
                switch(WzI18N.language) {
                case "zh": return 14
                case "en": return 28
                }
            }
            image.anchors.verticalCenterOffset: 0
            onClicked: {
                dbService.saveStrOption("multi_capture_params", JSON.stringify(root.params()))
                root.multiCapture()
            }
        }

        WzButton {
            id: buttonClose
            opacity: enabled ? 1 : 0.4
            width: 140
            height: 48
            radius: 3
            text: qsTr("关闭")
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: (buttonClose.width + 5) / 2
            label.font.pixelSize: 18
            onClicked: {
                root.hide()
                dbService.saveStrOption("multi_capture_params", JSON.stringify(root.params()))
            }
        }
    }
}


/*##^## Designer {
    D{i:0;autoSize:true;height:450;width:240}D{i:7;anchors_width:497}
}
 ##^##*/
