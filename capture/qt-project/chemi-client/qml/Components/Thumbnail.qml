import QtQuick 2.12
import QtQuick.Controls 2.12
import WzEnum 1.0
import WzUtils 1.0
import "../WzControls"

Rectangle {
    id: root
    clip: true
    state: "fold"
    signal selectImage(var imageInfo)
    signal deleteImage(int index, bool isDeleteFile)
    //visible: listModel.count > 0
    property int activeIndex: -1
    property string activeSampleName
    property bool isChecked: false
    property alias listView: listView
    property alias listModel: listModel
    property alias imagesCount: listModel.count

    function cleanImages(keepImagesCount) {
        while (listModel.count > keepImagesCount) {
            deleteImage(listModel.count - 1, true)
            listModel.remove(listModel.count - 1)
        }
    }

    function getImageInfo(index) {
        return listModel.get(index)
    }
    function setImageInfo(index, imageInfo) {
        listModel.set(index, imageInfo)
    }

    function setActiveColorChannel(colorChannel) {
        listModel.get(activeIndex).colorChannel = colorChannel
    }
    function setColorChannel(fileName, colorChannel) {
        for (var i = 0; i < listModel.count; i++)
            if (listModel.get(i).imageFile === fileName) {
                listModel.get(i).colorChannel = colorChannel
                return
            }
    }
    function clearColorChannel() {
        for (var i = 0; i < listModel.count; i++)
            if (listModel.get(i).colorChannel !== 0) {
                listModel.get(i).colorChannel = 0
                var imageInfo = {
                    imageFile: listModel.get(i).imageFile,
                    colorChannel: 0
                }
                dbService.saveImage(imageInfo)
            }
    }

    // private property
    property var groupColorMap
    property int groupColorIndex
    property var groupColorEnum: ["#A65959", "#58FAF4", "#F8E0F1", "#D0F5A9", "#292A0A", "#240B3B", "#0B3B0B"]

    function add(imageInfo) {
        if (existsImageFile(imageInfo.imageFile))
            return

        imageInfo.groupColor = getGroupColor(imageInfo.groupId)
        imageInfo.checked = false
        listModel.insert(0, imageInfo)
    }
    function existsImageFile(fileName) {
        for (var i = 0; i < listModel.count; i++)
            if (listModel.get(i).imageFile === fileName)
                return true
        return false
    }
    function getIndexByImageFile(fileName) {
        for (var i = 0; i < listModel.count; i++)
            if (listModel.get(i).imageFile === fileName)
                return i
        return -1
    }

    // private function
    function getGroupColor(groupId) {
        if (!(groupId > 0))
            return "transparent"
        if (undefined === groupColorMap)
            groupColorMap = {}
        if (undefined === groupColorMap[groupId]) {
            groupColorIndex++
            if (groupColorIndex > 6)
                groupColorIndex = 0
            groupColorMap[groupId] = groupColorEnum[groupColorIndex]
        }
        return groupColorMap[groupId]
    }

    onActiveSampleNameChanged: {
        if (activeIndex === -1) return
        if (activeIndex >= listModel.count) return
        listModel.get(activeIndex).sampleName = activeSampleName
    }

    transitions: [
        Transition {
            from: "*"
            to: "*"
            NumberAnimation {
                target: root
                properties: "height,width,anchors.leftMargin,anchors.bottomMargin"
                easing.type: Easing.InOutQuad
                duration: 300
            }

            NumberAnimation {
                targets: [buttonUnfoldFull, buttonFold, buttonUnfold, listView]
                properties: "opacity"
                duration: 300
            }
        }
    ]

    states: [
        State {
            name: "unfold"
            PropertyChanges {
                target: root
                color: "black"
                height: 150
                //width: parent.width
                //anchors.leftMargin: 0
                //anchors.bottomMargin: 0
            }
            /*PropertyChanges {
                target: buttonFold
                opacity: 1
            }
            PropertyChanges {
                target: buttonUnfoldFull
                opacity: 1
            }
            PropertyChanges {
                target: buttonUnfold
                opacity: 0
            }*/
            PropertyChanges {
                target: listView
                opacity: 1
            }
        },
        State {
            name: "fold"
            PropertyChanges {
                target: root
                color: "transparent"
                height: 40
                width: 40
                //anchors.leftMargin: 10
                //anchors.bottomMargin: 10
            }
            /*PropertyChanges {
                target: buttonFold
                opacity: 0
            }
            PropertyChanges {
                target: buttonUnfoldFull
                opacity: 0
            }
            PropertyChanges {
                target: buttonUnfold
                opacity: 1
            }*/
            PropertyChanges {
                target: listView
                opacity: 0
            }
        }
    ]

    /*WzButton {
        id: buttonUnfoldFull
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.bottom: buttonFold.top
        anchors.bottomMargin: 10
        normalColor: "transparent"
        hotColor: "#676767"
        downColor: "#474747"
        width: 40
        height: 40
        imageVisible: true
        imageSourceNormal: "qrc:/images/thumbnail_unfold.png"
        opacity: 0
    }

    WzButton {
        id: buttonFold
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 10
        normalColor: "transparent"
        hotColor: "#676767"
        downColor: "#474747"
        width: 40
        height: 40
        imageVisible: true
        imageSourceNormal: "qrc:/images/thumbnail_fold.png"
        opacity: 0
        visible: opacity > 0 ? true : false
        onClicked: {
            root.state = "fold"
        }
    }

    WzButton {
        id: buttonUnfold
        anchors.left: parent.left
        anchors.leftMargin: 0
        anchors.bottom: parent.bottom
        anchors.bottomMargin: 0
        normalColor: "transparent"
        hotColor: "transparent"
        downColor: "transparent"
        width: 38
        height: 32
        imageVisible: true
        imageSourceNormal: "qrc:/images/thumbnail_image.png"
        imageSourceHot: "qrc:/images/thumbnail_image_hot.png"
        opacity: 1
        visible: opacity > 0 ? true : false
        onClicked: {
            root.state = "unfold"
        }
    }*/

    Row {
        x: listView.anchors.leftMargin + 1
        y: 10
        height: parent.height
        Repeater {
            model: ((root.width - parent.x) / 131).toFixed(0) + 1
            Rectangle {
                color: "black"
                height: parent.height
                width: 131
                Rectangle {
                    radius: 5
                    width: 130
                    height: 130
                    color: "#aaaaaa"
                }
            }
        }
    }

    ListView {
        id: listView
        anchors.left: parent.left
        anchors.leftMargin: 10
        anchors.right: parent.right
        height: parent.height
        opacity: 0
        clip: true

        orientation: ListView.Horizontal

        ScrollBar.horizontal: ScrollBar {
            height: 15
            opacity: 0.5
            hoverEnabled: true
            active: hovered || pressed
            parent: listView.parent
            anchors.bottomMargin: 10
            anchors.bottom: listView.bottom
            anchors.left: listView.left
            anchors.leftMargin: 1
            anchors.right: listView.right

            background: Rectangle {
                color: "transparent"
            }

            contentItem: Rectangle {
                implicitHeight: 15
                color: "black"
                radius: 15
            }
        }

        delegate: Rectangle {
            id: itemWrapper

            height: listView.height
            width: 131

            color: root.color

            Rectangle {
                id: content
                anchors.verticalCenter: parent.verticalCenter
                anchors.horizontalCenter: parent.horizontalCenter                
                radius: 5
                width: 130
                height: 130
                property bool isHovered
                color: {
                    if (root.activeIndex === model.index)
                        return "#e2c495"
                    else if (isHovered && !WzUtils.isAndroid()) // issue5-1
                        return "#d1c0a5"
                    else
                        return "#aaaaaa"
                }
                Image {
                    id: imageThumb
                    source: model.imageThumbFile === "" || (model.loading !== undefined && model.loading) ? "" : "file:///" + model.imageThumbFile
                    fillMode: Image.PreserveAspectCrop
                    height: 100
                    width: 120
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.horizontalCenter: parent.horizontalCenter
                }

                WzBusyIndicator {
                    id: loading
                    width: 30
                    height: 30
                    circleSize: 6
                    color: "#555555"
                    anchors.centerIn: parent
                    visible: model.loading !== undefined && model.loading
                }

                Rectangle {
                    id: rectGroupColor
                    width: 10
                    height: 10
                    radius: 5
                    color: model.groupColor
                    anchors.left: parent.left
                    anchors.top: parent.top
                    anchors.leftMargin: WzUtils.isPC() ? 4 : (parent.width - width) / 2
                    anchors.topMargin: 2
                    opacity: 0.5
                }

                Rectangle {
                    id: rectangleSampleName
                    anchors.fill: parent
                    color: "white"
                    opacity: 0
                    Behavior on opacity {
                        OpacityAnimator {
                            duration: 300
                        }
                    }
                }

                Text {
                    anchors.bottom: parent.bottom
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: parent.width
                    text: model.sampleName === undefined ? "" : model.sampleName
                    horizontalAlignment: Text.Center
                    clip: true
                    elide: Text.ElideLeft
                    color: "black"
                    visible: !(model.sampleName === undefined || model.sampleName === "")

                    Behavior on height {NumberAnimation {duration: 200}}
                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: false//true
                        propagateComposedEvents: true
                        onEntered: {
                            parent.height = parent.parent.height
                            parent.wrapMode = Text.WordWrap
                            rectangleSampleName.opacity = 1
                        }
                        onExited: {
                            parent.wrapMode = Text.NoWrap
                            parent.height = parent.implicitHeight
                            rectangleSampleName.opacity = 0
                        }
                    }
                }

                Rectangle {
                    id: rectColorChannelPoint
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.bottom: parent.bottom
                    anchors.bottomMargin: 4
                    width: 6
                    height: 6
                    radius: 6
                    color: {
                        switch(model.colorChannel) {
                        case WzEnum.Null:
                            return "transparent"
                        case WzEnum.Red:
                            return "#dd0000"
                        case WzEnum.Green:
                            return "#00dd00"
                        case WzEnum.Blue:
                            return "#4295f5"
                        }
                    }
                }

                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onEntered: content.isHovered = true
                    onExited: content.isHovered = false
                    onClicked: {
                        root.activeIndex = model.index
                        selectImage(listModel.get(model.index))
                    }
                    onPressAndHold: {
                        if (!isChecked)
                            isChecked = true
                    }
                }
                MultiPointTouchArea {
                    visible: !WzUtils.isPC()
                    anchors.fill: parent
                    onPressed: {
                        pageSwipeView.interactive = false
                        root.activeIndex = model.index
                        selectImage(listModel.get(model.index))
                        thumbnailPressAndHoldTimer.running = true
                        console.debug("onPressed")
                    }
                    onTouchUpdated: {
                        if (touchPoints.length > 0) {
                            var offsetPointX = touchPoints[0].x - touchPoints[0].startX
                            var offsetPointY = touchPoints[0].y - touchPoints[0].startY
                            var offsetThreshold = 20
                            if (Math.abs(offsetPointX) > offsetThreshold ||
                                    Math.abs(offsetPointY) > offsetThreshold) {
                                thumbnailPressAndHoldTimer.running = false
                            }
                            console.debug("offsetPointX:", offsetPointX,
                                          "offsetPointY:", offsetPointY)
                            // 上滑分享图片
                            if (offsetPointY < offsetThreshold * -1) {
                                if (!imageService.canSaveAsImage())
                                    return
                                var images = [thumbnail.listModel.get(thumbnail.activeIndex)]
                                imageFormatSelectII.images = images
                                imageFormatSelectII.show()
                                rectangleShade.opacity = 0.7
                            // 下滑删除图片
                            } else if (offsetPointY > offsetThreshold) {
                                buttonDeleteImage.clicked()
                            }
                        }
                    }
                    onReleased: {
                        thumbnailPressAndHoldTimer.running = false
                        pageSwipeView.interactive = true
                    }
                    onCanceled: {
                        thumbnailPressAndHoldTimer.running = false
                        pageSwipeView.interactive = true
                    }
                    Timer {
                        id: thumbnailPressAndHoldTimer
                        running: false
                        interval: 1000
                        repeat: false
                        onTriggered: {
                            if (!isChecked) {
                                isChecked = true
                                listModel.get(model.index).checked = true
                            }
                        }
                    }
                }

                WzButton {
                    id: buttonDeleteImage
                    width: 15
                    height: 15
                    visible: WzUtils.isPC()
                    anchors.right: parent.right
                    anchors.rightMargin: 3
                    normalColor: "transparent"
                    hotColor: "transparent"
                    downColor: "transparent"
                    imageVisible: true
                    imageSourceNormal: "qrc:/images/button_cancel_6a6a6a.svg"
                    image.sourceSize.width: 11
                    image.sourceSize.height: 11
                    imageSourceHot: "qrc:/images/button_cancel_red.svg"
                    imageHot.sourceSize.width: 11
                    imageHot.sourceSize.height: 11
                    imageSourceDown: "qrc:/images/button_cancel_maroon.svg"
                    imageDown.sourceSize.width: 11
                    imageDown.sourceSize.height: 11
                    onClicked: {
                        rectangleDeleteConfrim.opacity = 0.8
                        textDeleteConfrim.opacity = 1
                        buttonDeleteImageOk.opacity = 1
                        buttonDeleteImageCancel.opacity = 1
                    }
                }

                Rectangle {
                    id: rectangleDeleteConfrim
                    anchors.fill: parent
                    color: "black"
                    opacity: 0
                    visible: opacity > 0
                    Behavior on opacity {NumberAnimation{duration: 200}}
                    MouseArea {anchors.fill: parent; hoverEnabled: true}
                }
                Text {
                    id: textDeleteConfrim
                    text: qsTr("确定删除吗？")
                    anchors.horizontalCenter: rectangleDeleteConfrim.horizontalCenter
                    anchors.verticalCenter: rectangleDeleteConfrim.verticalCenter
                    color: "white"
                    opacity: 0
                    visible: opacity > 0
                    Behavior on opacity {NumberAnimation{duration: 200}}
                    anchors.verticalCenterOffset: -15
                    wrapMode: Text.WordWrap
                    width: parent.width
                    horizontalAlignment: Text.Center
                }

                WzButton {
                    id: buttonDeleteImageOk
                    radius: 3
                    text: qsTr("确定")
                    width: 40
                    height: 25
                    anchors.top: textDeleteConfrim.bottom
                    anchors.topMargin: 5
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.horizontalCenterOffset: -(width/2) - 2
                    opacity: 0
                    visible: opacity > 0
                    Behavior on opacity {NumberAnimation{duration: 200}}
                    onClicked: {
                        rectangleDeleteConfrim.opacity = 0
                        textDeleteConfrim.opacity = 0
                        buttonDeleteImageOk.opacity = 0
                        buttonDeleteImageCancel.opacity = 0
                        deleteImage(model.index, true)
                        listModel.remove(model.index)
                    }
                }
                WzButton {
                    id: buttonDeleteImageCancel
                    radius: 3
                    text: qsTr("取消")
                    width: 40
                    height: 25
                    anchors.top: textDeleteConfrim.bottom
                    anchors.topMargin: 5
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.horizontalCenterOffset: (width/2) + 2
                    opacity: 0
                    visible: opacity > 0
                    Behavior on opacity {NumberAnimation{duration: 200}}
                    onClicked: {
                        rectangleDeleteConfrim.opacity = 0
                        textDeleteConfrim.opacity = 0
                        buttonDeleteImageOk.opacity = 0
                        buttonDeleteImageCancel.opacity = 0
                    }
                }

                Rectangle {
                    id: checkboxShadeRectangle
                    visible: isChecked
                    color: "black"
                    opacity: 0.5
                    anchors.fill: parent
                    radius: parent.radius

                    MouseArea {
                        anchors.fill: parent
                        onClicked: checkBox.checked = !checkBox.checked
                    }
                }

                CheckBox {
                    id: checkBox
                    visible: checkboxShadeRectangle.visible
                    anchors.centerIn: parent
                    checked: model.checked
                    onCheckedChanged: model.checked = checked
                    scale: 1.5
                }
            }

        }

        model: listModel

        add: Transition {
            NumberAnimation { properties: "opacity"; from: 0; to: 1; duration: 500 }
        }
        displaced: Transition {
            NumberAnimation { properties: "x,y"; duration: 300 }
        }
    }

    ListModel {
        id: listModel
    }
}


/*##^## Designer {
    D{i:0;height:150;width:800}
}
 ##^##*/
