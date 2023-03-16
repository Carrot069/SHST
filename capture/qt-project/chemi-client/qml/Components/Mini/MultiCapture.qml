import QtQuick 2.12
import QtQuick.Controls 2.12
import WzUtils 1.0
import WzI18N 1.0
import "../../WzControls"

Rectangle {
    id: root
    color: "transparent"
    clip: true
    antialiasing: true
    width: 300
    height: 300

    function params() {
        var p = {}
        switch(contentView.currentIndex) {
        case 0:
            p.multiType = "gray"
            p.exposureMs = exposureTimeGrayAccumulate.getExposureMs()
            break
        case 1:
            p.multiType = "time"
            p.exposureMs = timeAccumulateExposureTimeBase.getExposureMs()
            break
        default:
            p.multiType = "time_list"
        }
        p.grayAccumulate_frameCount = spinBoxGrayAccumulate.value
        p.grayAccumulate_exposureMs = exposureTimeGrayAccumulate.getExposureMs()
        p.timeAccumulate_frameCount = spinBoxTimeAccumulateCount.value
        p.timeAccumulate_exposureMs = timeAccumulateExposureTimeInc.getExposureMs()
        p.timeAccumulate_exposureMsBase = timeAccumulateExposureTimeBase.getExposureMs()
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

        if (undefined !== params.grayAccumulate_exposureMs)
            exposureTimeGrayAccumulate.setExposureMs(params.grayAccumulate_exposureMs)

        if (undefined !== params.timeAccumulate_frameCount)
            spinBoxTimeAccumulateCount.value = params.timeAccumulate_frameCount

        if (undefined !== params.timeAccumulate_exposureMs)
            timeAccumulateExposureTimeInc.setExposureMs(params.timeAccumulate_exposureMs)

        if (undefined !== params.timeAccumulate_exposureMsBase)
            timeAccumulateExposureTimeBase.setExposureMs(params.timeAccumulate_exposureMsBase)

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
        return 240
    }

    Behavior on height {
        NumberAnimation {
            duration: 200
        }
    }
    Behavior on opacity {NumberAnimation{duration: 400}}

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
            color: "transparent"
        }

        WzTabButton {
            id: buttonGrayAccumulation
            text: qsTr("灰度累积")
            backgroundColor: "transparent"
            font.family: WzI18N.font.family
            font.capitalization: Font.MixedCase
            font.pixelSize: {
                switch(WzI18N.language) {
                case "zh": return 21
                case "en": return 10
                }
            }
            onClicked: {
                contentView.currentIndex = tabBar.currentIndex
                root.height = getHeight()
            }
        }
        WzTabButton {
            text: qsTr("时间累积")
            backgroundColor: "transparent"
            font: buttonGrayAccumulation.font
            onClicked: {
                contentView.currentIndex = tabBar.currentIndex
                root.height = getHeight()
            }
        }
        WzTabButton {
            text: qsTr("时间序列")
            backgroundColor: "transparent"
            font: buttonGrayAccumulation.font
            onClicked: {
                contentView.currentIndex = tabBar.currentIndex
                root.height = getHeight()
            }
        }
    }

    SwipeView {
        id: contentView
        currentIndex: 1
        anchors.top: tabBar.bottom
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right

        //[[灰度累积]]//
        Item {
            width: root.width
            height: parent.height

            MouseArea {
                anchors.fill: parent
                onClicked: forceActiveFocus()
            }

            Item {
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.horizontalCenterOffset: 2
                implicitWidth: textGrayAccumulate.implicitWidth + spinBoxGrayAccumulate.width
                implicitHeight: parent.height

                Text {
                    id: textGrayAccumulate
                    text: qsTr("累积数量:")
                    font.pixelSize: {
                        switch(WzI18N.language) {
                        case "zh": return 25
                        case "en": return 20
                        }
                    }
                    color: "white"
                    y: 30
                }

                WzSpinBox {
                    id: spinBoxGrayAccumulate
                    height: 110
                    width: {
                        switch(WzI18N.language) {
                        case "zh": return 80
                        case "en": return 65
                        }
                    }
                    anchors.verticalCenter: textGrayAccumulate.verticalCenter
                    anchors.verticalCenterOffset: 2
                    anchors.left: textGrayAccumulate.right
                    isPaddingZero: false
                    font.pixelSize: {
                        switch(WzI18N.language) {
                        case "zh": return 37
                        case "en": return 30
                        }
                    }
                    font.family: "Digital Dismay"
                    fontColor: "white"
                    from: 2
                    to: 999
                    buttonColor: "#404040"
                    buttonFontColor: "white"
                }

                WzExposureTime {
                    id: exposureTimeGrayAccumulate
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 0
                    fontColor: "white"
                    fontPixelSize: 48
                }
            }
        }
        //[[灰度累积]]//

        //[[时间累积]]//
        Item {
            width: 450
            height: 230
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    parent.forceActiveFocus()
                }
            }
            Item {
                id: item1
                anchors.verticalCenter: parent.verticalCenter
                anchors.verticalCenterOffset: -20
                anchors.left: parent.left
                anchors.leftMargin: {
                    switch(WzI18N.language) {
                    case "zh": return 30
                    case "en": return 20
                    }
                }
                width: parent.width - anchors.leftMargin
                implicitHeight: textTimeAccumulateCount.implicitHeight + spinBoxTimeAccumulateCount.implicitHeight
                Text {
                    id: textTimeAccumulateCount
                    text: qsTr("累积数量")
                    font.pixelSize: {
                        switch(WzI18N.language) {
                        case "zh": return 20
                        case "en": return 17
                        }
                    }
                    color: "white"
                }
                WzSpinBox {
                    id: spinBoxTimeAccumulateCount
                    anchors.top: textTimeAccumulateCount.bottom
                    anchors.horizontalCenter: textTimeAccumulateCount.horizontalCenter
                    width: 80
                    isPaddingZero: false
                    font.pixelSize: 32
                    font.family: "Digital Dismay"
                    fontColor: "white"
                    from: 1
                    to: 99
                    buttonColor: "#535353"
                    buttonFontColor: "white"
                    isShowButton: false
                }

                Text {
                    id: textTimeAccumulateTime
                    anchors.left: textTimeAccumulateCount.right
                    anchors.leftMargin: {
                        switch(WzI18N.language) {
                        case "zh": return 55
                        case "en": return 25
                        }
                    }
                    text: qsTr("累积时间")
                    font: textTimeAccumulateCount.font
                    color: "white"
                }

                WzExposureTime {
                    id: timeAccumulateExposureTimeInc
                    anchors.verticalCenter: spinBoxTimeAccumulateCount.verticalCenter
                    anchors.horizontalCenter: textTimeAccumulateTime.horizontalCenter
                    fontColor: "white"
                    fontPixelSize: 32
                    spinBoxMinute.width: 31
                    spinBoxSecond.width: 31
                    spinBoxMillisecond.width: 46
                }
            }

            WzExposureTime {
                id: timeAccumulateExposureTimeBase
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: 0
                fontColor: "white"
                fontPixelSize: 48
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

            ListView {
                id: listView
                currentIndex: -1
                property int hoveredIndex: -1
                interactive: true
                keyNavigationWraps: true
                focus: true
                highlightFollowsCurrentItem: true
                width: 135
                visible: model.count > 0 ? true : false
                anchors.top: parent.top
                anchors.topMargin: 8
                anchors.bottom: parent.bottom
                anchors.left: parent.left
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
                    height: 34
                    width: listView.width

                    Rectangle {
                        radius: 3
                        height: 34
                        width: parent.width - 5
                        color: listView.hoveredIndex === model.index ? "white" : (listView.currentIndex !== model.index ? "transparent" : "#404040")
                        opacity: 0.1
                    }
                    Row {
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.verticalCenterOffset: 2
                        anchors.left: buttonDeleteItem.right
                        anchors.leftMargin: 2
                        height: parent.height
                        width: parent.width
                        Text {
                            text: model.time.substring(0, 2)
                            color: "white"
                            font.pixelSize: 25
                            font.family: "Digital Dismay"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: ":"
                            color: "white"
                            font.pixelSize: 20
                            font.family: "Arial"
                            horizontalAlignment: Text.horizontalAlignment
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: model.time.substring(3, 5)
                            color: "white"
                            font.pixelSize: 25
                            font.family: "Digital Dismay"
                            width: 20
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: "."
                            color: "white"
                            font.pixelSize: 20
                            font.family: "Arial"
                            horizontalAlignment: Text.horizontalAlignment
                            anchors.verticalCenter: parent.verticalCenter
                        }
                        Text {
                            text: model.time.substring(6, 9)
                            color: "white"
                            font.pixelSize: 25
                            font.family: "Digital Dismay"
                            anchors.verticalCenter: parent.verticalCenter
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        onClicked: {
                            listView.currentIndex = model.index
                            listView.focus = true
                        }
                        onDoubleClicked: {
                            exposureTimeSequenceTime.setExposureMs(model.exposureMs)
                        }
                        hoverEnabled: true
                        onEntered: {
                            listView.hoveredIndex = model.index
                        }
                        onExited: {
                            listView.hoveredIndex = -1
                        }
                    }

                    WzButton {
                        id: buttonDeleteItem
                        scale: 0.7
                        height: 30
                        width: 30
                        radius: 4
                        anchors.verticalCenter: parent.verticalCenter
                        normalColor: "transparent"
                        imageVisible: true
                        imageSourceNormal: "qrc:/images/button_cancel_b4b4b4.svg"
                        image.sourceSize.width: 18
                        image.sourceSize.height: 18
                        onClicked: {
                            listModel.remove(model.index)
                            forceActiveFocus()
                        }
                        onStateChanged: {
                            if (state === "entered")
                                listView.hoveredIndex = model.index
                        }
                    }
                }
                highlight: Item  {
                    width: listView.width
                    Rectangle {
                        height: 34
                        width: parent.width - 5
                        radius: 3
                        color: "white"
                        opacity: 0.2
                    }
                    Behavior on y { SpringAnimation { spring: 2; damping: 0.2 } }
                }
            }

            WzButton {
                id: buttonEditTime
                text: qsTr("修改")
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                normalColor: "#707070"
                normalFontColor: "white"
                opacity: 0.4
                width: 70
                height: 34
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
                normalColor: "#707070"
                normalFontColor: "white"
                opacity: 0.4
                width: buttonEditTime.width
                height: buttonEditTime.height
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
                anchors.left: buttonAddTime.left
                text: qsTr("曝光时间")
                font.pixelSize: 18
                color: "white"
            }

            WzExposureTime {
                id: exposureTimeSequenceTime
                anchors.bottom: buttonEditTime.top
                anchors.bottomMargin: 5
                anchors.left: buttonAddTime.left
                fontPixelSize: 30
                spinBoxMinute.width: 31
                spinBoxSecond.width: 31
                spinBoxMillisecond.width: 46
                fontColor: "white"
            }
        }
    }

}
