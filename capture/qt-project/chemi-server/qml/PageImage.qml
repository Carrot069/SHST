import QtQuick 2.12
import QtQuick.Controls 2.12
import Qt.labs.platform 1.1
import QtQuick.Window 2.12

import WzImage 1.0
import WzUtils 1.0
import WzEnum 1.0
import WzI18N 1.0

import "WzControls"
import "Components"

import "Controller/PageImageController.js" as PageImageController

Item {
    id: root
    signal pageChange(int pageIndex)

    // 这些属性都是当前正在查看的图片的信息
    // 逐步弃用这些属性, 转为从 thumbnail 列表中获取当前图片属性
    property string imageFile: ""
    property string imageWhiteFile: ""
    property string sampleName: ""
    property int exposureMs: 0
    property string captureDate
    property var imageShowOptions
    property string paletteName: "Gray"
    property int imageInvertFlag: 0
    property string lightType: ""
    property int imageColorChannel: 0

    // 控制图片叠加控件的隐藏和显示
    property bool imageMergeVisible: true
    property bool chemiWhiteVisible: true
    property bool fluorRGBVisible: false

    property alias captureProgress: captureProgressMini

    function closeActiveImage() {
        root.imageFile = ""
        imageService.closeActiveImage()
    }

    function openImage(imageFile, showOptions) {
        imageService.lowMarker = -1
        imageService.highMarker = -1
        imageShowOptions = showOptions
        imageService.openImage(imageFile)
    }

    function loadThumbnials() {
        var images = dbService.getImages(0, 100)
        for(var n = 0; n < images.length; n++) {
            thumbnail.add(images[n])
        }
    }

    function updateView() {
        var activeImageInfo = imageService.getActiveImageInfo()
        var imageInfo = dbService.getImageByFileName(activeImageInfo.imageFile)
        if (imageInfo.imageFile === undefined) {
            dbService.saveImage(activeImageInfo)
            imageInfo = activeImageInfo
        }
        if (!thumbnail.existsImageFile(imageInfo.imageFile))
            thumbnail.add(imageInfo)

        root.imageFile = imageInfo.imageFile === undefined ? "" : imageInfo.imageFile
        root.imageWhiteFile = imageInfo.imageWhiteFile === undefined ? "" : imageInfo.imageWhiteFile
        root.sampleName = imageInfo.sampleName === undefined ? "" : imageInfo.sampleName
        root.exposureMs = imageInfo.exposureMs === undefined ? 0 : imageInfo.exposureMs
        if (imageInfo.captureDate !== undefined) root.captureDate = Qt.formatDateTime(imageInfo.captureDate, "yyyy-MM-dd hh:mm:ss")
        root.lightType = imageInfo.openedLight

        imageService.markerImageName = root.imageWhiteFile

        if (imageInfo.imageInvert === undefined)
            imageInvertFlag = 0
        else
            imageInvertFlag = imageInfo.imageInvert

        var imageInvert = true
        if (imageInfo.imageInvert === undefined) {
            imageInvert = true
        } else if (imageInfo.imageInvert === 1) {
            imageInvert = true
        } else if (imageInfo.imageInvert === 2 || imageInfo.imageInvert === 4) {
            imageInvert = false
        } else {
            imageInvert = true
        }        

        if (imageInfo.palette === undefined || imageInfo.palette === "")
            imageInfo.palette = "Gray";
        if (imageInfo.palette !== root.paletteName) {
            var palette = imageService.getPaletteByName(imageInfo.palette)
            if (palette !== undefined) {
                root.paletteName = palette.name
                imageView.colorTable = palette.rgbList
                pseudoColor.setActive(root.paletteName)
                pseudoBar.rgbList = palette.rgbList
                pseudoBar.update()
            }
        }

        var colorTableInvert = isColorTableInvert()
        imageService.isColorChannel = false

        if (lightType === "uv_penetrate" || lightType === "white_down") {
            imageMergeVisible = false
        } else if (lightType === "red" || lightType === "green" || lightType === "blue") {
            imageMergeVisible = true
            chemiWhiteVisible = false
            fluorRGBVisible = true

            root.imageColorChannel = imageInfo.colorChannel

            // 在 onImageOpened 中也有这样一段类似的代码, 在自动计算灰阶时需要在那个地方设置好颜色通道文件
            // 此处的代码是在手动调整灰阶时用到的, 即使用灰阶调整滑竿的当前值作为Low/High
            if (imageShowOptions !== undefined && imageShowOptions.isNewImage !== undefined) {
                if (imageShowOptions.isNewImage) {
                    imageShowOptions.isNewImage = false
                    if (!checkboxAutoLowHigh.checked) {
                        imageInfo.grayLow = grayRange.first.value
                        imageInfo.grayHigh = grayRange.second.value
                    }
                }
            }

            allColorChannelUnchecked()
            if (root.imageColorChannel !== undefined &&
                    imageService.getColorChannelFile(root.imageColorChannel) === imageFile) {
                imageService.setColorChannelLowHigh(root.imageColorChannel,
                                                    imageInfo.grayLow,
                                                    imageInfo.grayHigh)
                grayRange.first.value = imageInfo.grayLow
                grayRange.second.value = imageInfo.grayHigh
                imageService.isColorChannel = true
                imageService.updateView()
                switch(root.imageColorChannel) {
                case WzEnum.Red:
                    switchFluorRed.isIgnoreCheckedChanged = true
                    switchFluorRed.checked = true
                    rectFluorRed.active = true
                    break
                case WzEnum.Green:
                    switchFluorGreen.isIgnoreCheckedChanged = true
                    switchFluorGreen.checked = true
                    rectFluorGreen.active = true
                    break
                case WzEnum.Blue:
                    switchFluorBlue.isIgnoreCheckedChanged = true
                    switchFluorBlue.checked = true
                    rectFluorBlue.active = true
                    break
                default:
                    allColorChannelUnchecked()
                }
            } else {
                imageService.invert = true
                imageService.showMarker = false
                imageService.showChemi = true
                imageView.colorTableInvert = colorTableInvert
                imageService.isColorChannel = false
                imageService.low = grayRange.first.value
                imageService.high = grayRange.second.value
                imageService.updateLowHigh(grayRange.first.value, grayRange.second.value)
                imageService.updateView()
            }
            return
        } else {
            imageMergeVisible = true
            chemiWhiteVisible = true
            fluorRGBVisible = false
        }

        if (!imageMergeVisible || (!switchChemiImage.checked && !switchWhiteImage.checked)) {
            imageService.invert = imageInvert
            imageService.showMarker = false
            imageService.showChemi = true
            imageView.colorTableInvert = colorTableInvert
        } else if (switchChemiImage.checked && switchWhiteImage.checked) {
            imageService.invert = false
            imageService.showMarker = true
            imageService.showChemi = true
            imageView.colorTableInvert = true
        } else if (switchChemiImage.checked && !switchWhiteImage.checked) {
            imageService.invert = imageInvert
            imageService.showMarker = false
            imageService.showChemi = true
            imageView.colorTableInvert = colorTableInvert
        } else if (isOnlyShowWhiteImage()) {
            imageService.invert = false
            imageService.showMarker = true
            imageService.showChemi = false
            imageView.colorTableInvert = true
        }

        if (imageService.lowMarker === -1) {
            // 以前生成的记录中没有记录白光图的low/high, 所以此处设置一个默认值
            if (imageInfo.grayLowMarker === "")
                imageInfo.grayLowMarker = 0
            if (imageInfo.grayHighMarker === 0 || imageInfo.grayHighMarker === "")
                imageInfo.grayHighMarker = 20000
            imageService.lowMarker = imageInfo.grayLowMarker
            imageService.highMarker = imageInfo.grayHighMarker
            if (isOnlyShowWhiteImage()) {
                grayRange.first.value = imageService.lowMarker
                grayRange.second.value = imageService.highMarker
            }
        }

        imageService.updateLowHighMarker(imageService.lowMarker, imageService.highMarker)
        imageService.updateLowHigh(imageService.low, imageService.high)
        imageService.updateView()

        saveMarkerLowHigh()
    }

    function allColorChannelUnchecked() {
        if (switchFluorRed.checked) {
            switchFluorRed.isIgnoreCheckedChanged = true
            switchFluorRed.checked = false
        }
        if (switchFluorGreen.checked) {
            switchFluorGreen.isIgnoreCheckedChanged = true
            switchFluorGreen.checked = false
        }
        if (switchFluorBlue.checked) {
            switchFluorBlue.isIgnoreCheckedChanged = true
            switchFluorBlue.checked = false
        }
    }

    function isOnlyShowWhiteImage() {
        return switchWhiteImage.checked && !switchChemiImage.checked
    }

    function isColorTableInvert() {
        if (imageInvertFlag === 0 || imageInvertFlag === 1) {
            return true
        }

        if (imageInvertFlag === 2) {
            if (paletteName == "Gray")
                return true
            else
                return false
        }

        if (imageInvertFlag === 4) {
            return true
        }
    }

    Component.onCompleted: {
        checkboxAutoLowHigh.checked = dbService.readIntOption("auto_low_high", 0) === 1
        grayRange.first.value = dbService.readIntOption("gray_low", 0)
        grayRange.second.value = dbService.readIntOption("gray_high", 20000)
        imageService.low = grayRange.first.value
        imageService.high = grayRange.second.value
        checkBoxColorBar.checked = dbService.readIntOption("color_bar_in_image", 0) === 1

        root.imageColorChannel = dbService.readIntOption("activeImageColorChannel", 0)
        imageService.isColorChannel = false
        imageService.setColorChannelFile(dbService.readStrOption("redImageFile", ""), WzEnum.Red)
        imageService.setColorChannelFile(dbService.readStrOption("greenImageFile", ""), WzEnum.Green)
        imageService.setColorChannelFile(dbService.readStrOption("blueImageFile", ""), WzEnum.Blue)

        if (imageService.getColorChannelFile(WzEnum.Red) !== "") {
            rectFluorRed.active = true
            var redImageInfo = dbService.getImageByFileName(imageService.getColorChannelFile(WzEnum.Red))
            imageService.setColorChannelLowHigh(WzEnum.Red, redImageInfo.grayLow, redImageInfo.grayHigh)
        }
        if (imageService.getColorChannelFile(WzEnum.Green) !== "") {
            rectFluorGreen.active = true
            var greenImageInfo = dbService.getImageByFileName(imageService.getColorChannelFile(WzEnum.Green))
            imageService.setColorChannelLowHigh(WzEnum.Green, greenImageInfo.grayLow, greenImageInfo.grayHigh)
        }
        if (imageService.getColorChannelFile(WzEnum.Blue) !== "") {
            rectFluorBlue.active = true
            var blueImageInfo = dbService.getImageByFileName(imageService.getColorChannelFile(WzEnum.Blue))
            imageService.setColorChannelLowHigh(WzEnum.Blue, blueImageInfo.grayLow, blueImageInfo.grayHigh)
        }
    }
    Component.onDestruction: {
        dbService.saveIntOption("auto_low_high", checkboxAutoLowHigh.checked ? 1 : 0)
        dbService.saveIntOption("color_bar_in_image", checkBoxColorBar.checked ? 1 : 0)
        dbService.saveIntOption("gray_low", grayRange.first.value)
        dbService.saveIntOption("gray_high", grayRange.second.value)
    }

    WzImageService {
        id: imageService
        onImageOpened: {
            if (errorCode === WzEnum.UnsupportedFormat) {
                msgBox.show(qsTr("不支持的文件格式, 请打开16位TIFF图片"), qsTr("确定"))
                return
            }

            if (errorCode !== WzEnum.Success) {
                // TODO 显示打开失败
                return
            }

            // 此处代码与 updateView 函数中的代码有部分重叠, 后期以清理后者的代码为主, 2020-6-9 by wangzhe
            var activeImageInfo = imageService.getActiveImageInfo()
            var imageInfo
            if (thumbnail.existsImageFile(activeImageInfo.imageFile)) {
                imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)
            } else {
                imageInfo = dbService.getImageByFileName(activeImageInfo.imageFile)
                if (imageInfo.imageFile === undefined) {
                    imageInfo = activeImageInfo
                    dbService.saveImage(imageInfo)
                }
                thumbnail.add(imageInfo)
                thumbnail.activeIndex = 0
            }

            if (imageInfo.openedLight === "uv_penetrate" || imageInfo.openedLight === "white_down") {
                root.imageMergeVisible = false
            } else if (imageInfo.openedLight === "red" || imageInfo.openedLight === "green" || imageInfo.openedLight === "blue") {
                root.imageMergeVisible = true
                root.chemiWhiteVisible = false
                root.fluorRGBVisible = true

                if (imageShowOptions !== undefined && imageShowOptions.isNewImage !== undefined) {
                    if (imageShowOptions.isNewImage) {
                        clearImageColorChannel(imageInfo.colorChannel, imageInfo.imageFile)
                        imageService.setColorChannelFile(imageInfo.imageFile, imageInfo.colorChannel)
                        saveFluorParams()
                    }
                }
            } else {
                root.imageMergeVisible = true
                root.chemiWhiteVisible = true
                root.fluorRGBVisible = false
            }

            if (checkboxAutoLowHigh.checked) {
                if (root.chemiWhiteVisible && switchWhiteImage.checked && !switchChemiImage.checked)
                    imageService.calculateLowHighMarker()
                else if (root.fluorRGBVisible && imageInfo.colorChannel > 0)
                    imageService.calculateLowHighRGB(imageInfo.colorChannel)
                else
                    imageService.calculateLowHigh()
            } else {
                root.updateView()
            }
            updatePseudoColorBarInImage()
            if (imageShowOptions !== undefined) {
                if (imageShowOptions.autoFitOnce !== undefined && imageShowOptions.autoFitOnce) {
                    imageView.fit(imageViewWrapper.width, imageViewWrapper.height)
                }
            }
            dbService.saveStrOption("open_image_path", openImageDialog.folder)
        }
        onLowHighCalculated: {
            if (errorCode === WzEnum.Success) {
                grayRange.first.value = low
                grayRange.second.value = high
                root.updateView()
            } else {
                // TODO 显示错误内容
            }
        }
        onLowHighMarkerCalculated: {
            if (errorCode === WzEnum.Success) {
                grayRange.first.value = low
                grayRange.second.value = high
                root.updateView()
            } else {
                // TODO 显示错误内容
            }
        }
        onLowHighRGBCalculated: {
            if (errorCode === WzEnum.Success) {
                var imageInfo = {
                    imageFile: thumbnail.getImageInfo(thumbnail.activeIndex).imageFile,
                    grayLow: low,
                    grayHigh: high
                }
                dbService.saveImage(imageInfo)
                root.updateView()
            } else {
                // TODO 显示错误内容
            }
        }
    }

    Rectangle {
        id: rectLeft
        anchors.left: parent.left
        anchors.right: rectRight.left
        height: parent.height
        color: "#969696"

        Flickable {
            id: imageViewWrapper
            anchors.top: parent.top
            anchors.right: parent.right
            anchors.bottom: thumbnail.top
            anchors.left: parent.left
            clip: true
            contentWidth: imageView.width < width ? width : imageView.width
            contentHeight: imageView.height < height ? height : imageView.height
            property real previousWidthRatio
            property real previousHeightRatio
            property real previousXPos
            property real previousYPos
            property bool isIgnorePosChanged
            visibleArea.onWidthRatioChanged: {
                if (previousWidthRatio === 1) {
                    imageViewHorizontalScrollBar.position = (1 - visibleArea.widthRatio) / 2
                } else {
                    imageViewHorizontalScrollBar.position = (previousXPos / (1 - previousWidthRatio)) * (1 - visibleArea.widthRatio)
                }
                previousWidthRatio = visibleArea.widthRatio
                previousXPos = visibleArea.xPosition
            }
            visibleArea.onHeightRatioChanged: {
                if (previousHeightRatio === 1) {
                    imageViewVerticalScrollBar.position = (1 - visibleArea.heightRatio) / 2
                } else {
                    imageViewVerticalScrollBar.position = (previousYPos / (1 - previousHeightRatio)) * (1 - visibleArea.heightRatio)
                }
                previousHeightRatio = visibleArea.heightRatio
                previousYPos = visibleArea.yPosition
            }
            visibleArea.onXPositionChanged: {
                if (isIgnorePosChanged) {
                    return
                }
                previousXPos = visibleArea.xPosition
            }
            visibleArea.onYPositionChanged: {
                if (isIgnorePosChanged) {
                    return
                }
                previousYPos = visibleArea.yPosition
            }
            Component.onCompleted: {
                previousWidthRatio = visibleArea.widthRatio
                previousHeightRatio = visibleArea.heightRatio
                previousXPos = visibleArea.xPosition
                previousYPos = visibleArea.yPosition
            }

            ScrollBar.horizontal: ScrollBar {
                id: imageViewHorizontalScrollBar
                policy: imageViewWrapper.contentWidth > imageViewWrapper.width ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                opacity: 0.5

                background: Rectangle {
                    color: "transparent"
                }
                contentItem: Rectangle {
                    implicitHeight: 15
                    color: "black"
                    radius: 15
                }
            }
            ScrollBar.vertical: ScrollBar {
                id: imageViewVerticalScrollBar
                policy: imageViewWrapper.contentHeight > imageViewWrapper.height ? ScrollBar.AlwaysOn : ScrollBar.AlwaysOff
                opacity: 0.5

                background: Rectangle {
                    color: "transparent"
                }

                contentItem: Rectangle {
                    implicitWidth: 15
                    color: "black"
                    radius: 15
                }
            }

            Rectangle {
                color: "#7f7f7f"
                anchors.fill: parent
            }

            WzImageView {
                id: imageView
                anchors.centerIn: parent
                showColorBar: false//checkBoxColorBar.checked &&
                              //width < imageViewWrapper.width && height < imageViewWrapper.height
                onZoomChanging: imageViewWrapper.isIgnorePosChanged = true
                onZoomChanged: imageViewWrapper.isIgnorePosChanged = false
                Component.onCompleted: {
                    /** Demo code **
                    var arr = []
                    for (var n = 0; n < 256; n++) {
                        var rgb = {R: 255-n, G: 0, B: 0}
                        arr.push(rgb)
                    }
                    imageView.colorTable = arr
                    /** Demo code **/
                    imageService.updateView();
                }
            }
        }

        /************* 图片右下角的缩放 *************/
        Rectangle {
            id: rectZoom
            visible: imageFile !== ""
            color: "white"
            opacity: 0.5
            radius: 5
            height: 30
            width: 145
            anchors.right: imageViewWrapper.right
            anchors.rightMargin: imageViewWrapper.height > imageView.height ? 10 : 25
            anchors.bottom: imageViewWrapper.bottom
            anchors.bottomMargin: imageViewWrapper.width > imageView.width ? 10 : 25

            Behavior on anchors.rightMargin { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
            Behavior on anchors.bottomMargin { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
        }
        WzButton {
            id: buttonZoomOriginal
            visible: rectZoom.visible
            text: "1:1"
            label.font.family: "Arial"
            label.font.pixelSize: 10
            anchors.right: buttonZoomAuto.left
            anchors.rightMargin: 5
            anchors.verticalCenter: textInputZoom.verticalCenter
            width: 17
            height: 17
            radius: 17
            border.color: "#333333"
            normalColor: "#dddddd"
            hotColor: "#ffffff"
            downColor: "#cccccc"
            normalFontColor: "#333333"
            onClicked: {
                forceActiveFocus()
                imageView.zoom = 100
            }
        }
        WzButton {
            id: buttonZoomAuto
            visible: rectZoom.visible
            text: "A"
            label.font.family: "Arial"
            label.font.pixelSize: 11
            anchors.right: buttonZoomIn.left
            anchors.rightMargin: 5
            anchors.verticalCenter: textInputZoom.verticalCenter
            width: 17
            height: 17
            radius: 17
            border.color: "#333333"
            normalColor: "#dddddd"
            hotColor: "#ffffff"
            downColor: "#cccccc"
            normalFontColor: "#333333"
            onClicked: {
                forceActiveFocus()
                imageView.fit(imageViewWrapper.width, imageViewWrapper.height)
            }
        }
        WzButton {
            id: buttonZoomIn
            visible: rectZoom.visible
            label.font.family: "Arial"
            anchors.right: buttonZoomOut.left
            anchors.rightMargin: 5
            anchors.verticalCenter: textInputZoom.verticalCenter
            width: 17
            height: 17
            radius: 17
            border.color: "#333333"
            normalColor: "#dddddd"
            hotColor: "#ffffff"
            downColor: "#cccccc"
            onClicked: {
                forceActiveFocus()
                imageView.zoom = imageView.zoom + 10
            }
            Rectangle {
                height: 1
                width: parent.width - 6
                color: "#333333"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                width: 1
                height: parent.height - 6
                color: "#333333"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        WzButton {
            id: buttonZoomOut
            visible: rectZoom.visible
            label.font.family: "Arial"
            anchors.right: textInputZoom.left
            anchors.rightMargin: 7
            anchors.verticalCenter: textInputZoom.verticalCenter
            width: 17
            height: 17
            radius: 17
            border.color: "#333333"
            normalColor: "#dddddd"
            hotColor: "#ffffff"
            downColor: "#cccccc"
            onClicked: {
                forceActiveFocus()
                imageView.zoom = imageView.zoom - 10
            }
            Rectangle {
                height: 1
                width: parent.width - 6
                color: "#333333"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
        }
        TextInput {
            id: textInputZoom
            visible: rectZoom.visible
            text: imageView.zoom + "%"
            clip: true
            width: 40
            anchors.right: rectZoom.right
            anchors.rightMargin: 7
            anchors.verticalCenter: rectZoom.verticalCenter
            anchors.verticalCenterOffset: 1
            font.pixelSize: 13
            font.family: "Arial"
            color: "#333333"
            horizontalAlignment: TextInput.AlignRight
            selectByMouse: true
            onEditingFinished: {
                var i = parseInt(text.replace("%", ""))
                imageView.zoom = i
            }
        }
        /************* 图片右下角的缩放 *************/

        /************* 图片右下角的伪彩 *************/
        WzPseudoColor {
            id: pseudoColorInImage
            anchors.right: rectZoom.right
            anchors.bottom: rectZoom.top
            anchors.bottomMargin: 10
            height: 256
            width: 20
            vertical: true
            visible: checkBoxColorBar.checked && imageFile !== ""/* && !imageView.showColorBar*/
        }
        Rectangle {
            anchors.fill: textPseudoColorGrayHigh
            anchors.margins: -3
            color: "white"
            opacity: 0.5
            radius: 5
            visible: textPseudoColorGrayHigh.visible && textPseudoColorGrayHigh.text !== ""
        }
        Text {
            id: textPseudoColorGrayHigh
            anchors.right: pseudoColorInImage.left
            anchors.rightMargin: 10
            anchors.top: pseudoColorInImage.top
            anchors.topMargin: 5
            horizontalAlignment: Text.AlignRight
            visible: pseudoColorInImage.visible
        }
        Rectangle {
            anchors.fill: textPseudoColorGrayMiddle
            anchors.margins: -3
            color: "white"
            opacity: 0.5
            radius: 5
            visible: textPseudoColorGrayMiddle.visible && textPseudoColorGrayMiddle.text !== ""
        }
        Text {
            id: textPseudoColorGrayMiddle
            anchors.right: pseudoColorInImage.left
            anchors.rightMargin: 10
            anchors.verticalCenter: pseudoColorInImage.verticalCenter
            horizontalAlignment: Text.AlignRight
            visible: pseudoColorInImage.visible
        }
        Rectangle {
            anchors.fill: textPseudoColorGrayLow
            anchors.margins: -3
            color: "white"
            opacity: 0.5
            radius: 5
            visible: textPseudoColorGrayLow.visible && textPseudoColorGrayLow.text !== ""
        }
        Text {
            id: textPseudoColorGrayLow
            anchors.right: pseudoColorInImage.left
            anchors.rightMargin: 10
            anchors.bottom: pseudoColorInImage.bottom
            anchors.bottomMargin: 5
            horizontalAlignment: Text.AlignRight
            visible: pseudoColorInImage.visible
        }
        /************* 图片右下角的伪彩 *************/

        Rectangle {
            id: captureProgressMiniWrapper
            anchors.left: parent.left
            anchors.leftMargin: 0
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            width: 141
            height: 150
            color: "black"
            visible: pageCapture.captureProgress.visible
            Rectangle {
                radius: 5
                color: "#aaaaaa"
                width: 130
                height: 130
                anchors.top: parent.top
                anchors.topMargin: 10
                anchors.left: parent.left
                anchors.leftMargin: 10
                CaptureProgressMini {
                    id: captureProgressMini
                    anchors.fill: parent
                    anchors.margins: 11
                    progress: pageCapture.captureProgress.progress
                    text: pageCapture.captureProgress.text
                    isProgress: pageCapture.captureProgress.isProgress
                    buttonCancel2.visible: pageCapture.captureProgress.buttonCancel2.visible
                    countProgress: pageCapture.captureProgress.countProgress
                    leftTime: pageCapture.captureProgress.leftTime
                    elapsedTime: pageCapture.captureProgress.elapsedTime
                    onCancel: pageCapture.cancelCapture()
                    onBack: pageChange(0)
                }
            }
            Behavior on opacity {
                NumberAnimation {
                    duration: 200
                }
            }
        }


        Thumbnail {
            id: thumbnail
            state: "unfold"
            anchors.left: captureProgressMiniWrapper.visible ? captureProgressMiniWrapper.right : parent.left
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            listView.anchors.leftMargin: pageCapture.captureProgress.visible ? 0 : 10
            width: 38
            height: 32
            //Behavior on anchors.left { NumberAnimation {duration: 200}}
            Component.onCompleted: {
                loadThumbnials()
            }
            onSelectImage: {
                imageService.lowMarker = -1
                imageService.highMarker = -1
                imageService.openImage(imageInfo.imageFile)
            }
            onDeleteImage: {
                var imageInfo = getImageInfo(index)
                if (isDeleteFile)
                    imageService.deleteImage(imageInfo.imageFile)
                dbService.deleteImageByFileName(imageInfo.imageFile)
                if (imageInfo.colorChannel > 0) {
                    switch(imageInfo.colorChannel) {
                    case WzEnum.Red:
                        if (switchFluorRed.checked) {
                            switchFluorRed.isIgnoreCheckedChanged = true
                            switchFluorRed.checked = false
                        }
                        rectFluorRed.active = false
                        imageService.setColorChannelFile("", WzEnum.Red)
                        saveFluorParams()
                        updateView()
                        break
                    case WzEnum.Green:
                        if (switchFluorGreen.checked) {
                            switchFluorGreen.isIgnoreCheckedChanged = true
                            switchFluorGreen.checked = false
                        }
                        rectFluorGreen.active = false
                        imageService.setColorChannelFile("", WzEnum.Green)
                        saveFluorParams()
                        updateView()
                        break
                    case WzEnum.Blue:
                        if (switchFluorBlue.checked) {
                            switchFluorBlue.isIgnoreCheckedChanged = true
                            switchFluorBlue.checked = false
                        }
                        rectFluorBlue.active = false
                        imageService.setColorChannelFile("", WzEnum.Blue)
                        saveFluorParams()
                        updateView()
                        break
                    }
                }
                if (imageInfo.imageFile === root.imageFile)
                    root.closeActiveImage()
            }
        }
    }
    Rectangle {
        id: rectRight
        width: 500
        height: parent.height
        anchors.right: parent.right
        color: "black"

        Item {
            id: rectRightContent
            anchors.topMargin: 27
            anchors.bottomMargin: 9
            anchors.rightMargin: 20
            anchors.leftMargin: 18
            anchors.fill: parent
            anchors.margins: 15

            /********************** Logo **********************/
            LogoNav {
                id: imageLogoFlower
                anchors.left: parent.left
                anchors.leftMargin: 2
                anchors.top: parent.top
                anchors.topMargin: -12
                anchors.right: parent.right
                anchors.rightMargin: -20
            }            

            /********************** 顶部按钮 **********************/
            WzButton {
                id: buttonSaveAs
                width: 109
                height: 44
                radius: 4
                text: qsTr("另存为")
                anchors.top: WzUtils.isGelCapture() ? imageLogoFlower.bottom : undefined
                anchors.topMargin: 20
                anchors.bottom: WzUtils.isGelCapture() ? undefined : textImageInformation.top
                anchors.bottomMargin: 10
                label2.font.pixelSize: 30
                label.anchors.leftMargin: 5
                label2.font.family: "Wingdings"
                label2.text: "<"
                label.font.pixelSize: {
                    switch(WzI18N.language) {
                    case "zh": return 18
                    case "en": return 15
                    }
                }
                label.anchors.left: label2.right
                label.anchors.horizontalCenter: undefined
                label2.anchors.horizontalCenterOffset: {
                    switch(WzI18N.language) {
                    case "zh": return -(label.width+5) / 2
                    case "en": return -(label.width+10) / 2
                    }
                }
                onClicked: {
                    if (!imageService.canSaveAsImage())
                        return
                    saveImageDialog.folder = dbService.readStrOption("save_image_path", WzUtils.desktopPath())
                    saveImageDialog.currentFile = "file:///" + sampleName
                    saveImageDialog.selectedNameFilter.index = dbService.readIntOption("save_image_filter", 0)
                    saveImageDialog.open()
                }
            }

            WzButton {
                id: buttonPrint
                width: 95
                height: 44
                radius: 4
                text:  qsTr("打印")
                anchors.leftMargin: 18
                anchors.left: buttonOpen.right
                anchors.top: buttonSaveAs.top
                label.font: buttonSaveAs.label.font
                label.anchors.horizontalCenter: undefined
                label.anchors.left: label2.right
                label.anchors.leftMargin: 5
                label2.text: "6"
                label2.font.family: "Wingdings 2"
                label2.font.pixelSize: 30
                label2.anchors.horizontalCenterOffset: -(label.width+5) / 2
                //label2.anchors.leftMargin: (label.implicitWidth + label2.implicitWidth) / 2
                onClicked: {
                    imageService.printImage(WzI18N.language)
                }
            }

            WzButton {
                id: buttonOpen
                width: 95
                height: 44
                radius: 4
                text: qsTr("打开")
                anchors.left: buttonSaveAs.right
                anchors.leftMargin: 18
                anchors.top: buttonSaveAs.top
                label2.font.pixelSize: 26
                label.anchors.leftMargin: 5
                label2.font.family: "Wingdings"
                label2.text: "1"
                label.font: buttonSaveAs.label.font
                label.anchors.left: label2.right
                label.anchors.horizontalCenter: undefined
                label2.anchors.horizontalCenterOffset: -(label.width+5) / 2
                onClicked: {
                    openImageDialog.folder = dbService.readStrOption("open_image_path", "")
                    openImageDialog.open()
                }
            }

            /************************** 图片信息 **************************/
            Text {
                id: textImageInformation
                color: "#323232"
                text: qsTr("图片信息")
                anchors.bottom: textSampleNameTitle.top
                anchors.bottomMargin: 8
                font.pixelSize: 18
            }

            Text {
                id: textSampleNameTitle
                color: "#838383"
                text: qsTr("样品名:")
                anchors.bottomMargin: 4
                anchors.bottom: textImageExposureTimeTitle.top
                elide: Text.ElideRight
                font.pixelSize: 18
                horizontalAlignment: {
                    switch(WzI18N.language) {
                    case "zh": return Text.AlignLeft
                    case "en": return Text.AlignRight
                    }
                }
                width: {
                    switch(WzI18N.language) {
                    case "zh": return 80
                    case "en": return 90
                    }
                }
            }
            Text {
                id: textSampleName
                anchors.left: textSampleNameTitle.right
                anchors.leftMargin: {
                    switch(WzI18N.language) {
                    case "zh": return 5
                    case "en": return 10
                    }
                }
                anchors.verticalCenter: textSampleNameTitle.verticalCenter
                anchors.verticalCenterOffset: 2
                anchors.right: parent.right
                color: "#838383"
                font.pixelSize: 18
                text: {
                    if (root.imageFile === "")
                        return ""
                    else if (root.sampleName === "")
                        return ""
                    else
                        return root.sampleName
                }
            }

            Text {
                id: textImageExposureTimeTitle
                color: "#838383"
                text: qsTr("曝光时间:")
                anchors.bottom: textImageCaptureTimeTitle.top
                anchors.bottomMargin: 4
                elide: Text.ElideRight
                font.pixelSize: 18
                horizontalAlignment: textSampleNameTitle.horizontalAlignment
                width: textSampleNameTitle.width
            }
            Text {
                id: textImageExposureTime
                anchors.left: textSampleName.left
                anchors.right: parent.right
                color: "#838383"
                text: root.imageFile === "" ? "" : WzUtils.getTimeStr(root.exposureMs, true, true)
                anchors.verticalCenter: textImageExposureTimeTitle.verticalCenter
                elide: Text.ElideRight
                font.pixelSize: 18
            }

            Text {
                id: textImageCaptureTimeTitle
                color: "#838383"
                text: qsTr("拍摄时间:")
                anchors.bottom: textImageFileNameTitle.top
                anchors.bottomMargin: 4
                elide: Text.ElideRight
                font.pixelSize: 18
                horizontalAlignment: textSampleNameTitle.horizontalAlignment
                width: textSampleNameTitle.width
            }
            Text {
                id: textImageCaptureTime
                color: "#838383"
                text: root.imageFile === undefined ? "" : captureDate
                anchors.left: textSampleName.left
                anchors.right: parent.right
                anchors.verticalCenter: textImageCaptureTimeTitle.verticalCenter
                elide: Text.ElideRight
                font.pixelSize: 18
            }

            Text {
                id: textImageFileNameTitle
                color: "#838383"
                text: qsTr("存储位置:")
                elide: Text.ElideRight
                anchors.bottomMargin: 20
                anchors.bottom: rectangleLine1.top
                font.pixelSize: 18
                horizontalAlignment: textSampleNameTitle.horizontalAlignment
                width: textSampleNameTitle.width
            }
            Text {
                id: textImageFileName
                color: "#838383"
                text: root.imageFile
                elide: Text.ElideLeft
                anchors.left: textSampleName.left
                anchors.right: parent.right
                anchors.verticalCenter: textImageFileNameTitle.verticalCenter
                height: textImageFileNameTitle.height
                font.pixelSize: 18
                property var hovered
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    onClicked: {
                        WzUtils.copyToClipboard(root.imageFile)
                        tooltipImageFileName.delay = 1
                        tooltipImageFileName.show(qsTr("已复制到剪贴板"), 1000)
                    }
                    onEntered: {
                        tooltipImageFileName.delay = 1000
                        tooltipImageFileName.show(parent.text, 5000)
                    }
                    onExited: {
                        tooltipImageFileName.close()
                    }
                }
                WzToolTip {
                    id: tooltipImageFileName
                }
            }

            Rectangle {
                id: rectangleLine1
                width: parent.width
                height: 1
                color: "#161616"
                anchors.bottomMargin: textImageMerge.visible ? 150 : 20
                anchors.bottom: textColorBar.top
                Behavior on anchors.bottomMargin {
                    NumberAnimation {
                        duration: 500
                        easing.type: Easing.InOutQuad
                    }
                }
            }

            /********************* 图片叠加 *********************/
            Text {
                id: textImageMerge
                visible: opacity > 0
                opacity: imageMergeVisible ? 1 : 0
                x: -1
                y: 0
                color: "#323232"
                text: qsTr("图片叠加")
                anchors.bottomMargin: 15
                anchors.bottom: textWhiteImage.top
                font.pixelSize: 18
                Behavior on opacity {NumberAnimation{duration: 500}}
            }

            Text {
                id: textChemiImage
                visible: opacity > 0
                opacity: imageMergeVisible && chemiWhiteVisible ? 1 : 0
                anchors.left: textImageMerge.left
                anchors.leftMargin: {
                    switch(WzI18N.language) {
                    case "zh": return 0
                    case "en": return 2;
                    }
                }
                color: "#838383"
                text: qsTr("化学发光")
                anchors.bottom: textWhiteImage.bottom
                font.pixelSize: {
                    switch (WzI18N.language) {
                    case "zh": return 18
                    case "en": return 15
                    }
                }
                Behavior on opacity {NumberAnimation{duration: 500}}
            }

            WzSwitch {
                id: switchChemiImage
                visible: opacity > 0
                opacity: imageMergeVisible && chemiWhiteVisible ? 1 : 0
                width: 50
                anchors.horizontalCenter: textChemiImage.horizontalCenter
                anchors.bottom: switchWhiteImage.bottom
                indicator.height: 22
                textControl.font.pixelSize: 10
                textControl.anchors.verticalCenterOffset: 1
                textControl.x: checked ? 7 : 23
                slideBar.height: 16
                slideBar.width: 16
                Behavior on opacity {NumberAnimation{duration: 500}}
                onCheckedChanged: {
                    if (checkboxAutoLowHigh.checked) {
                        if (switchWhiteImage.checked && !switchChemiImage.checked)
                            imageService.calculateLowHighMarker()
                        else
                            imageService.calculateLowHigh()
                    } else {
                        if (switchWhiteImage.checked && !switchChemiImage.checked) {
                            grayRange.first.value = imageService.lowMarker
                            grayRange.second.value = imageService.highMarker
                        } else {
                            grayRange.first.value = imageService.low
                            grayRange.second.value = imageService.high
                        }
                        updateView()
                    }
                }
            }

            Text {
                id: textWhiteImage
                visible: opacity > 0
                opacity: imageMergeVisible && chemiWhiteVisible ? 1 : 0
                anchors.left: textChemiImage.right
                anchors.leftMargin: 28
                color: "#838383"
                text: qsTr("白光图")
                anchors.bottom: switchWhiteImage.top
                anchors.bottomMargin: 5
                font.pixelSize: {
                    switch (WzI18N.language) {
                    case "zh": return 18
                    case "en": return 15
                    }
                }
                Behavior on opacity {NumberAnimation{duration: 500}}
            }

            WzSwitch {
                id: switchWhiteImage
                visible: opacity > 0
                opacity: imageMergeVisible && chemiWhiteVisible ? 1 : 0
                anchors.horizontalCenter: textWhiteImage.horizontalCenter
                anchors.horizontalCenterOffset: 1
                width: 50
                anchors.bottomMargin: 10
                anchors.bottom: rectangleLine2.top
                indicator.height: 22
                textControl.font.pixelSize: 10
                textControl.anchors.verticalCenterOffset: 1
                textControl.x: checked ? 7 : 23
                slideBar.height: 16
                slideBar.width: 16
                Behavior on opacity {NumberAnimation{duration: 500}}
                onCheckedChanged: {
                    if (imageFile === "") {
                        return
                    }
                    if (checked && imageWhiteFile === "") {
                        msgBox.show(qsTr("没有指定白光图片, 所以无法显示"), qsTr("确定"))
                        checked = false
                        return
                    }
                    if (checkboxAutoLowHigh.checked) {
                        if (switchWhiteImage.checked && !switchChemiImage.checked)
                            imageService.calculateLowHighMarker()
                        else
                            imageService.calculateLowHigh()
                    } else {
                        if (switchWhiteImage.checked && !switchChemiImage.checked) {
                            grayRange.first.value = imageService.lowMarker
                            grayRange.second.value = imageService.highMarker
                        } else {
                            grayRange.first.value = imageService.low
                            grayRange.second.value = imageService.high
                        }
                        updateView()
                    }
                }
            }

            Text {
                id: textFluorRed
                visible: opacity > 0
                opacity: imageMergeVisible && fluorRGBVisible ? 1 : 0
                x: textImageMerge.x
                color: "#838383"
                text: qsTr("红色通道")
                anchors.bottom: switchFluorRed.top
                anchors.bottomMargin: 5
                font.pixelSize: 18
                Behavior on opacity {NumberAnimation{duration: 500}}
            }

            WzSwitch {
                id: switchFluorRed
                visible: opacity > 0
                opacity: imageMergeVisible && fluorRGBVisible ? 1 : 0
                anchors.horizontalCenter: textFluorRed.horizontalCenter
                width: 50
                anchors.bottomMargin: 10
                anchors.bottom: rectangleLine2.top
                indicator.height: 22
                textControl.font.pixelSize: 10
                textControl.anchors.verticalCenterOffset: 1
                textControl.x: checked ? 7 : 23
                slideBar.height: 16
                slideBar.width: 16
                Behavior on opacity {NumberAnimation{duration: 500}}
                property bool isIgnoreCheckedChanged: false
                onCheckedChanged: {
                    if (isIgnoreCheckedChanged) {
                        isIgnoreCheckedChanged = false
                        return
                    }
                    if (checked && imageFile === "") {
                        msgBox.show(qsTr("请先选择一张图片"), qsTr("确定"))
                        checked = false
                        return
                    }
                    var imageInfo = {
                        imageFile: root.imageFile,
                        colorChannel: 0
                    }
                    if (checked) {                        
                        imageInfo.colorChannel = WzEnum.Red
                        clearImageColorChannel(imageInfo.colorChannel, imageFile)
                        dbService.saveImage(imageInfo)
                        root.imageColorChannel = imageInfo.colorChannel
                        imageService.isColorChannel = true
                        imageService.setColorChannelFile(imageFile, imageInfo.colorChannel)
                        imageService.setColorChannelLowHigh(imageInfo.colorChannel, grayRange.first.value, grayRange.second.value)
                        imageService.updateView()
                        rectFluorRed.active = true
                        thumbnail.setActiveColorChannel(imageInfo.colorChannel)
                    } else {
                        imageService.setColorChannelFile("", WzEnum.Red)
                        dbService.saveImage(imageInfo)
                        root.imageColorChannel = WzEnum.Null
                        updateView()
                        rectFluorRed.active = false
                        thumbnail.setActiveColorChannel(WzEnum.Null)
                    }
                    saveFluorParams()
                }
            }
            Rectangle {
                id: rectFluorRed
                width: 6
                height: 6
                radius: 6
                anchors.horizontalCenter: switchFluorRed.horizontalCenter
                anchors.top: switchFluorRed.bottom
                anchors.topMargin: -8
                color: "#dd0000"
                property bool active
                opacity: imageMergeVisible && fluorRGBVisible && active ? 1 : 0
                visible: opacity > 0
                Behavior on opacity { NumberAnimation {duration: 500} }
            }

            Text {
                id: textFluorGreen
                visible: opacity > 0
                opacity: imageMergeVisible && fluorRGBVisible ? 1 : 0
                anchors.left: textFluorRed.right
                anchors.leftMargin: 20
                color: "#838383"
                text: qsTr("绿色通道")
                anchors.bottom: switchFluorGreen.top
                anchors.bottomMargin: 5
                font.pixelSize: 18
                Behavior on opacity {NumberAnimation{duration: 500}}
            }

            WzSwitch {
                id: switchFluorGreen
                visible: opacity > 0
                opacity: imageMergeVisible && fluorRGBVisible ? 1 : 0
                anchors.horizontalCenter: textFluorGreen.horizontalCenter
                width: 50
                anchors.bottomMargin: 10
                anchors.bottom: rectangleLine2.top
                indicator.height: 22
                textControl.font.pixelSize: 10
                textControl.anchors.verticalCenterOffset: 1
                textControl.x: checked ? 7 : 23
                slideBar.height: 16
                slideBar.width: 16
                Behavior on opacity {NumberAnimation{duration: 500}}
                property bool isIgnoreCheckedChanged: false
                onCheckedChanged: {
                    if (isIgnoreCheckedChanged) {
                        isIgnoreCheckedChanged = false
                        return
                    }

                    if (checked && imageFile === "") {
                        msgBox.show(qsTr("请先选择一张图片"), qsTr("确定"))
                        checked = false
                        return
                    }

                    var imageInfo = {
                        imageFile: root.imageFile,
                        colorChannel: 0
                    }
                    if (checked) {
                        imageInfo.colorChannel = WzEnum.Green
                        clearImageColorChannel(imageInfo.colorChannel, imageFile)
                        dbService.saveImage(imageInfo)
                        root.imageColorChannel = imageInfo.colorChannel
                        imageService.isColorChannel = true
                        imageService.setColorChannelFile(imageFile, imageInfo.colorChannel)
                        imageService.setColorChannelLowHigh(imageInfo.colorChannel, grayRange.first.value, grayRange.second.value)
                        imageService.updateView()
                        rectFluorGreen.active = true
                        thumbnail.setActiveColorChannel(imageInfo.colorChannel)
                    } else {
                        imageService.setColorChannelFile("", WzEnum.Green)
                        dbService.saveImage(imageInfo)
                        root.imageColorChannel = WzEnum.Null
                        updateView()
                        rectFluorGreen.active = false
                        thumbnail.setActiveColorChannel(WzEnum.Null)
                    }
                    saveFluorParams()
                }
            }

            Rectangle {
                id: rectFluorGreen
                width: 6
                height: 6
                radius: 6
                anchors.horizontalCenter: switchFluorGreen.horizontalCenter
                anchors.top: switchFluorGreen.bottom
                anchors.topMargin: -8
                color: "#00dd00"
                property bool active
                opacity: imageMergeVisible && fluorRGBVisible && active ? 1 : 0
                visible: opacity > 0
                Behavior on opacity { NumberAnimation {duration: 500} }
            }

            Text {
                id: textFluorBlue
                visible: opacity > 0
                opacity: imageMergeVisible && fluorRGBVisible ? 1 : 0
                anchors.left: textFluorGreen.right
                anchors.leftMargin: 20
                color: "#838383"
                text: qsTr("蓝色通道")
                anchors.bottom: switchFluorBlue.top
                anchors.bottomMargin: 5
                font.pixelSize: 18
                Behavior on opacity {NumberAnimation{duration: 500}}
            }

            WzSwitch {
                id: switchFluorBlue
                visible: opacity > 0
                opacity: imageMergeVisible && fluorRGBVisible ? 1 : 0
                anchors.horizontalCenter: textFluorBlue.horizontalCenter
                width: 50
                anchors.bottomMargin: 10
                anchors.bottom: rectangleLine2.top
                indicator.height: 22
                textControl.font.pixelSize: 10
                textControl.anchors.verticalCenterOffset: 1
                textControl.x: checked ? 7 : 23
                slideBar.height: 16
                slideBar.width: 16
                Behavior on opacity {NumberAnimation{duration: 500}}
                property bool isIgnoreCheckedChanged: false
                onCheckedChanged: {
                    if (isIgnoreCheckedChanged) {
                        isIgnoreCheckedChanged = false
                        return
                    }

                    if (checked && imageFile === "") {
                        msgBox.show(qsTr("请先选择一张图片"), qsTr("确定"))
                        checked = false
                        return
                    }

                    var imageInfo = {
                        imageFile: root.imageFile,
                        colorChannel: 0
                    }
                    if (checked) {
                        imageInfo.colorChannel = WzEnum.Blue
                        clearImageColorChannel(imageInfo.colorChannel, imageFile)
                        dbService.saveImage(imageInfo)
                        root.imageColorChannel = imageInfo.colorChannel
                        imageService.isColorChannel = true
                        imageService.setColorChannelFile(imageFile, imageInfo.colorChannel)
                        imageService.setColorChannelLowHigh(imageInfo.colorChannel, grayRange.first.value, grayRange.second.value)
                        imageService.updateView()
                        rectFluorBlue.active = true
                        thumbnail.setActiveColorChannel(imageInfo.colorChannel)
                    } else {
                        imageService.setColorChannelFile("", WzEnum.Blue)
                        dbService.saveImage(imageInfo)
                        root.imageColorChannel = WzEnum.Null
                        updateView()
                        rectFluorBlue.active = false
                        thumbnail.setActiveColorChannel(WzEnum.Null)
                    }
                    saveFluorParams()
                }
            }
            Rectangle {
                id: rectFluorBlue
                width: 6
                height: 6
                radius: 6
                anchors.horizontalCenter: switchFluorBlue.horizontalCenter
                anchors.top: switchFluorBlue.bottom
                anchors.topMargin: -8
                color: "#4295f5"
                property bool active
                opacity: imageMergeVisible && fluorRGBVisible && active ? 1 : 0
                visible: opacity > 0
                Behavior on opacity { NumberAnimation {duration: 500} }
            }

            Text {
                id: textCancelColorChannel
                visible: opacity > 0
                opacity: imageMergeVisible && fluorRGBVisible ? 1 : 0
                anchors.left: textFluorBlue.right
                anchors.leftMargin: 32
                anchors.verticalCenter: textFluorBlue.verticalCenter
                color: "#838383"
                text: qsTr("取消叠加")
                font.pixelSize: 18
                Behavior on opacity {NumberAnimation{duration: 500}}
            }

            WzButton {
                id: buttonCancelColorChannel
                text: "r"
                label.font.family: "Webdings"
                label.font.pixelSize: 18
                normalColor: "#1e1e1e"
                normalFontColor: "#393939"
                hotFontColor: "#cccccc"
                downFontColor: "#dddddd"
                anchors.horizontalCenter: textCancelColorChannel.horizontalCenter
                anchors.top: textCancelColorChannel.bottom
                anchors.topMargin: 5
                width: 42
                height: 24
                radius: 4

                visible: opacity > 0
                opacity: imageMergeVisible && fluorRGBVisible ? 1 : 0
                Behavior on opacity {NumberAnimation{duration: 500}}

                onClicked: {
                    root.imageColorChannel = WzEnum.Null
                    rectFluorRed.active = false
                    rectFluorGreen.active = false
                    rectFluorBlue.active = false
                    allColorChannelUnchecked()
                    thumbnail.clearColorChannel()
                    imageService.isColorChannel = false
                    imageService.setColorChannelFile("", WzEnum.Red)
                    imageService.setColorChannelFile("", WzEnum.Green)
                    imageService.setColorChannelFile("", WzEnum.Blue)
                    updateView()
                    saveFluorParams()
                }
            }

            Rectangle {
                id: rectangleLine2
                visible: opacity > 0
                opacity: imageMergeVisible ? 1 : 0
                x: 1
                y: 0
                width: parent.width
                height: 1
                color: "#161616"
                anchors.bottom: textColorBar.top
                anchors.bottomMargin: 15
                Behavior on opacity {NumberAnimation{duration: 500}}
            }

            /******************************** 颜色调整 ********************************/
            Text {
                id: textColorBar
                color: "#323232"
                text: qsTr("伪彩")
                anchors.bottom: rectPseudoBar.top
                anchors.bottomMargin: 15
                font.pixelSize: 18
            }

            Rectangle {
                id: rectPseudoBar
                x: 2
                width: 400
                height: 22
                anchors.bottom: checkBoxColorBar.top
                anchors.bottomMargin: 5
                border.width: 1
                border.color: "#888888"
                color: "transparent"

                WzPseudoColor {
                    id: pseudoBar
                    anchors.fill: parent
                    anchors.margins: 1
                }

                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        pseudoColor.backupConfig()
                        pseudoColor.height = 420
                        pseudoColor.opacity = 1
                    }
                }
            }

            WzCheckBox {
                id: checkBoxColorBar
                x: 5
                y: 548
                width: 191
                height: 34
                text: qsTr("在图片中显示 ColorBar")
                anchors.bottomMargin: 10
                anchors.bottom: rectangleLine3.top
                anchors.leftMargin: -8
                anchors.left: parent.left
                onCheckedChanged: {
                    if (checked) {
                        updatePseudoColorBarInImage()
                    }
                }
            }

            Rectangle {
                id: rectangleLine3
                width: parent.width
                height: 1
                color: "#161616"
                anchors.bottom: element.top
                anchors.bottomMargin: 15
            }

            /********************* 显示灰阶 *********************/
            Text {
                id: element
                color: "#323232"
                text: qsTr("显示灰阶")
                anchors.bottom: textInputGrayLow.top
                anchors.bottomMargin: 15
                font.pixelSize: 18
            }

            TextInput {
                id: textInputGrayLow
                text: Math.round(grayRange.first.value)
                width: 120
                anchors.bottom: grayRange.top
                anchors.bottomMargin: 3
                font.pixelSize: 30
                font.family: "Digital-7"
                color: "#b4b4b4"
                selectByMouse: true
                selectionColor: "white"
                selectedTextColor: "black"
                wrapMode: TextEdit.NoWrap
                validator: IntValidator {
                    bottom: 0
                    top: 65535
                }
                onTextEdited: {
                    var i = parseInt(text)
                    if (i >= grayRange.second.value) {
                        if (grayRange.second.value === 0)
                            text = 0
                        else
                            text = Math.round(grayRange.second.value) - 1
                    } else if (i > 65535)
                        text  = 65535
                }
                onEditingFinished: {
                    var newGray = parseInt(text)
                    if (newGray >= grayRange.second.value) {
                        if (grayRange.second.value === 0)
                            newGray = 0
                        else
                            newGray = Math.round(grayRange.second.value) - 1
                    }
                    grayRange.first.value = newGray
                    if (isOnlyShowWhiteImage()) {
                        imageService.lowMarker = Math.round(grayRange.first.value)
                        imageService.highMarker = Math.round(grayRange.second.value)
                        imageService.updateLowHighMarker(imageService.lowMarker, imageService.highMarker)
                        saveMarkerLowHigh()
                    } else {
                        imageService.low = Math.round(grayRange.first.value)
                        imageService.high = Math.round(grayRange.second.value)
                        imageService.updateLowHigh(Math.round(grayRange.first.value),
                                                   Math.round(grayRange.second.value))
                    }
                    imageService.updateView()
                }
                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    cursorShape: Qt.IBeamCursor
                    onClicked: mouse.accepted = false
                    onPressed: mouse.accepted = false
                    onReleased: mouse.accepted = false
                    onDoubleClicked: mouse.accepted = false
                    onPositionChanged: mouse.accepted = false
                    onPressAndHold: mouse.accepted = false
                }
            }

            TextInput {
                id: textInputGrayHigh
                x: 9
                y: 3
                color: "#b4b4b4"
                width: 120
                text: Math.round(grayRange.second.value)
                anchors.right: parent.right
                anchors.bottom: textInputGrayLow.bottom
                font.family: "Digital-7"
                font.pixelSize: 30
                horizontalAlignment: TextInput.AlignRight
                selectByMouse: true
                selectionColor: "white"
                selectedTextColor: "black"
                wrapMode: TextEdit.NoWrap
                validator: IntValidator {
                    bottom: 0
                    top: 65535
                }
                onTextEdited: {
                    var i = parseInt(text)
                    if (i > 65535)
                        text = 65535
                }
                onEditingFinished: {
                    var newGray = parseInt(text)
                    if (newGray <= grayRange.first.value) {
                        if (grayRange.first === 65535)
                            newGray = 65535
                        else
                            newGray = Math.round(grayRange.first.value) + 1
                    }
                    grayRange.second.value = newGray
                    if (isOnlyShowWhiteImage()) {
                        imageService.lowMarker = Math.round(grayRange.first.value)
                        imageService.highMarker = Math.round(grayRange.second.value)
                        imageService.updateLowHighMarker(imageService.lowMarker, imageService.highMarker)
                        saveMarkerLowHigh()
                    } else {
                        imageService.low = Math.round(grayRange.first.value)
                        imageService.high = Math.round(grayRange.second.value)
                        imageService.updateLowHigh(Math.round(grayRange.first.value),
                                                   Math.round(grayRange.second.value))
                    }
                    imageService.updateView()
                }
                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    cursorShape: Qt.IBeamCursor
                    onClicked: mouse.accepted = false
                    onPressed: mouse.accepted = false
                    onReleased: mouse.accepted = false
                    onDoubleClicked: mouse.accepted = false
                    onPositionChanged: mouse.accepted = false
                    onPressAndHold: mouse.accepted = false
                }
            }

            WzRangeSlider {
                id: grayRange
                width: 460
                stepSize: 1
                first.value: 500
                second.value: 20000
                from: 0
                to: 65535                
                anchors.left: parent.left
                anchors.bottom: checkboxAutoLowHigh.top
                anchors.bottomMargin: 5
                first.onValueChanged: textInputGrayLow.text = Math.round(first.value)
                first.onPressedChanged: {
                    if (!first.pressed) {
                        grayLowHighChanged()
                    }
                }
                second.onValueChanged: textInputGrayHigh.text = Math.round(second.value)
                second.onPressedChanged: {
                    if (!second.pressed) {                        
                        grayLowHighChanged()
                    }
                }
            }

            CheckBox {
                id: checkboxAutoLowHigh
                text: qsTr("自动调整")
                anchors.leftMargin: -7
                anchors.bottom: parent.bottom
                anchors.left: parent.left

                onCheckedChanged: {
                    if (checked) {
                        if (fluorRGBVisible && imageColorChannel > 0)
                            imageService.calculateLowHighRGB(root.imageColorChannel)
                        else if (isOnlyShowWhiteImage())
                            imageService.calculateLowHighMarker()
                        else
                            imageService.calculateLowHigh()
                    }
                }                
            }
            WzButton {
                id: buttonAutoLowHigh
                width: 25
                height: 25
                anchors.verticalCenter: checkboxAutoLowHigh.verticalCenter
                anchors.verticalCenterOffset: 1
                anchors.left: checkboxAutoLowHigh.right
                radius: width
                normalColor: "transparent"
                label.font.family: "Webdings"
                text: "~"
                onClicked: {
                    if (fluorRGBVisible && imageColorChannel > 0)
                        imageService.calculateLowHighRGB(root.imageColorChannel)
                    else if (isOnlyShowWhiteImage())
                        imageService.calculateLowHighMarker()
                    else
                        imageService.calculateLowHigh()
                }
            }

            /*********************** 伪彩面板 ***********************/
            PseudoColor {
                id: pseudoColor
                anchors.horizontalCenter: rectPseudoBar.horizontalCenter
                anchors.bottom: rectPseudoBar.top
                anchors.bottomMargin: 5
                height: 1
                visible: height > 1
                pseudoList: imageService.pseudoList

                property string paletteName
                property var palette

                onPreview: {
                    root.paletteName = palette.name
                    // from QQmlListModel to QJSValue
                    var arr = []
                    for(var n = 0; n < palette.rgbList.count; n++) {
                        var rgb = {
                            R: palette.rgbList.get(n).R,
                            G: palette.rgbList.get(n).G,
                            B: palette.rgbList.get(n).B
                        }
                        arr.push(rgb)
                    }
                    imageView.colorTable = arr
                    imageView.colorTableInvert = isColorTableInvert()
                    imageView.update()
                    pseudoBar.rgbList = arr
                    pseudoBar.update()
                }
                onClose: {
                    if (!isOk)
                        pseudoColor.restoreConfig()
                    else {
                        var imageInfo = {
                            imageFile: root.imageFile,
                            palette: root.paletteName
                        }
                        dbService.saveImage(imageInfo)
                    }

                    pseudoColor.height = 1
                    pseudoColor.opacity = 0
                }

                Behavior on height {NumberAnimation {duration: 200}}
                Behavior on opacity {NumberAnimation {duration: 500}}
            }

        }
    }

    FileDialog {
        id: openImageDialog
        fileMode: FileDialog.OpenFile
        title: qsTr("打开图片文件")
        nameFilters: ["TIFF(*.tif *.tiff)"]//, "JPEG(*.jpg *.jpeg)", "PNG(*.png)", "Bitmap(*.bmp)"]
        onAccepted: {
            // 异步函数, 回调函数是 imageOpened, 定义在 imageService 声明的地方
            imageService.openImage(openImageDialog.currentFile)
        }
        onRejected: {

        }
    }

    FileDialog {
        id: saveImageDialog
        title: qsTr("另存图片")
        fileMode: FileDialog.SaveFile        
        nameFilters: [qsTr("16位TIFF(*.tif)"), qsTr("8位 TIFF(*.tif)"), "JPEG(*.jpg)", "PNG(*.png)"]
        onAccepted: {
            console.log(saveImageDialog.currentFile)
            dbService.saveStrOption("save_image_path", saveImageDialog.folder)
            var formats = ["tiff16", "tiff8", "jpeg", "png"]
            var ret = imageService.saveAsImage(WzUtils.toLocalFile(saveImageDialog.currentFile), formats[selectedNameFilter.index])
            if (ret) {
                msgBox.show(qsTr("保存成功"), qsTr("确定"))
            } else {
                msgBox.show(qsTr("保存失败"), qsTr("确定"))
            }
            dbService.saveIntOption("save_image_filter", selectedNameFilter.index)
        }
    }

    function grayLowHighChanged() {
        if (imageService.isColorChannel && undefined !== imageColorChannel && imageColorChannel > 0) {
            imageService.setColorChannelLowHigh(imageColorChannel,
                                                grayRange.first.value,
                                                grayRange.second.value)
        } else if (isOnlyShowWhiteImage()) {
            imageService.lowMarker = Math.round(grayRange.first.value)
            imageService.highMarker = Math.round(grayRange.second.value)
            imageService.updateLowHighMarker(imageService.lowMarker, imageService.highMarker)
            saveMarkerLowHigh()
        } else {
            imageService.low = Math.round(grayRange.first.value)
            imageService.high = Math.round(grayRange.second.value)
            imageService.updateLowHigh(Math.round(grayRange.first.value),
                                       Math.round(grayRange.second.value))
        }
        imageService.updateView()
        updatePseudoColorBarInImage()
        var imageInfo = {
            imageFile: root.imageFile,
            grayLow: grayRange.first.value,
            grayHigh: grayRange.second.value
        }
        dbService.saveImage(imageInfo)
    }

    function updatePseudoColorBarInImage() {
        textPseudoColorGrayHigh.text = Math.round(grayRange.second.value)
        textPseudoColorGrayLow.text = Math.round(grayRange.first.value)
        textPseudoColorGrayMiddle.text = Math.round((grayRange.second.value - grayRange.first.value) / 2 + grayRange.first.value)
        imageView.grayHigh = Math.round(grayRange.second.value)
        imageView.grayLow = Math.round(grayRange.first.value)
        imageView.update()
    }

    function saveMarkerLowHigh() {
        var imageInfoNew = {
            imageFile: root.imageFile,
            grayLowMarker: imageService.lowMarker,
            grayHighMarker: imageService.highMarker
        }
        dbService.saveImage(imageInfoNew)
    }

    function saveFluorParams() {
        dbService.saveIntOption("activeImageColorChannel", root.imageColorChannel)
        dbService.saveStrOption("redImageFile", imageService.getColorChannelFile(WzEnum.Red))
        dbService.saveStrOption("greenImageFile", imageService.getColorChannelFile(WzEnum.Green))
        dbService.saveStrOption("blueImageFile", imageService.getColorChannelFile(WzEnum.Blue))
    }

    function clearImageColorChannel(channel, activeImageFile) {
        // 清理掉其他通道相同的文件, 使同一个文件只存在于一个通道中
        for (var i = 1; i < 4; i++) { // Red = 1, Green = 2, Blue = 3
            if (channel !== i && activeImageFile !== "" && activeImageFile === imageService.getColorChannelFile(i)) {
                imageService.setColorChannelFile("", i)
                switch(i) {
                case WzEnum.Red:
                    switchFluorRed.isIgnoreCheckedChanged = true
                    switchFluorRed.checked = false
                    rectFluorRed.active = false
                    break
                case WzEnum.Green:
                    switchFluorGreen.isIgnoreCheckedChanged = true
                    switchFluorGreen.checked = false
                    rectFluorGreen.active = false
                    break
                case WzEnum.Blue:
                    switchFluorBlue.isIgnoreCheckedChanged = true
                    switchFluorBlue.checked = false
                    rectFluorBlue.active = false
                    break
                }
            }
        }

        var imageFile = imageService.getColorChannelFile(channel)
        if (imageFile === undefined)
            return
        if (imageFile === "")
            return
        if (imageFile === activeImageFile)
            return
        var imageInfo = {
            imageFile: imageFile,
            colorChannel: 0
        }
        dbService.saveImage(imageInfo)
        thumbnail.setColorChannel(imageFile, 0)

    }
}


/*##^##
Designer {
    D{i:0;autoSize:true;height:800;width:1280}D{i:26;anchors_y:99}D{i:27;anchors_width:95;anchors_x:2;anchors_y:99}
D{i:28;anchors_y:99}
}
##^##*/
