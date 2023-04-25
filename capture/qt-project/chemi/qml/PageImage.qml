import QtQuick 2.12
import QtQuick.Controls 2.12
import Qt.labs.platform 1.1 as Platform
import QtQuick.Window 2.12
import QtQuick.Dialogs 1.2 as Dialogs

import WzImage 1.0
import WzUtils 1.0
import WzEnum 1.0
import WzI18N 1.0

import "WzControls"
import "WzControls.2"
import "Components"

import "Controller/PageImageController.js" as PageImageController

Item {
    id: root
    signal pageChange(int pageIndex)
    signal captureWhiteImage(string imageFile)

    // 这些属性都是当前正在查看的图片的信息
    property string imageFile: ""
    property string imageWhiteFile: ""
    property string sampleName: ""
    property int exposureMs: 0
    property string captureDate
    property var imageShowOptions
    property string paletteName: "Gray"
    onPaletteNameChanged: imageService.isPseudoColor = paletteName !== "Gray"
    property int imageInvertFlag: 0
    property string lightType: ""
    property int imageColorChannel: 0
    property bool grayHighAutoMiddle: false
    property bool updateLowHighWhenValueChanged: true
    property bool rememberLowHigh: false
    property bool overExposureHint: false
    property int overExposureHintValue: 60000
    property color overExposureHintColor: "red"
    property string planName: ""

    // 控制图片叠加控件的隐藏和显示
    property bool imageMergeVisible: true
    property bool chemiWhiteVisible: true
    property bool fluorRGBVisible: false

    property alias captureProgress: captureProgressMini

    function updateMarkerImage(fileName) {
        PageImageController.updateMarkerImage2(fileName)
    }

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
        console.info("updateView")
        var activeImageInfo = imageService.getActiveImageInfo()
        var imageInfo = dbService.getImageByFileName(activeImageInfo.imageFile)
        if (imageInfo.imageFile === undefined) {
            dbService.saveImage(activeImageInfo)
            imageInfo = activeImageInfo
        }
        if (!thumbnail.existsImageFile(imageInfo.imageFile))
            thumbnail.add(imageInfo)
        console.info("\t", "imageInfo:", JSON.stringify(imageInfo))

        root.imageFile = imageInfo.imageFile === undefined ? "" : imageInfo.imageFile
        root.imageWhiteFile = imageInfo.imageWhiteFile === undefined ? "" : imageInfo.imageWhiteFile
        root.sampleName = imageInfo.sampleName === undefined ? "" : imageInfo.sampleName
        root.exposureMs = imageInfo.exposureMs === undefined ? 0 : imageInfo.exposureMs
        if (imageInfo.captureDate !== undefined) root.captureDate = Qt.formatDateTime(imageInfo.captureDate, "yyyy-MM-dd hh:mm:ss")
        root.lightType = imageInfo.openedLight

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
        console.info("\t", "imageInvert:", imageInvert)

        if (imageInfo.palette === undefined || imageInfo.palette === "")
            imageInfo.palette = "Gray";
        if (imageInfo.palette !== root.paletteName) {
            var palette = imageService.getPaletteByName(imageInfo.palette)
            if (palette !== undefined) {
                root.paletteName = palette.name
                imageView.colorTable = palette.rgbList
                pseudoColorInImage.rgbList = palette.rgbList
                pseudoColor.setActive(root.paletteName)
                pseudoBar.rgbList = palette.rgbList
                pseudoBar.update()
            }
        }
        console.info("\t", "paletteName:", paletteName)

        var colorTableInvert = isColorTableInvert()
        console.info("\t", "colorTableInvert:", colorTableInvert)
        imageService.isColorChannel = false

        if (lightType === "uv_penetrate" || lightType === "white_down") {
            imageMergeVisible = false
        // 荧光
        } else if (lightType === "red" || lightType === "green" || lightType === "blue" ||
                   lightType === "uv_reflex1" || lightType === "uv_reflex2") {
            console.info("\t", "branch 1")
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
            console.info("\t", "imageColorChannel:", root.imageColorChannel)
            if (root.imageColorChannel &&
                    imageService.getColorChannelFile(root.imageColorChannel) === imageFile) {
                console.info("\t", "branch 1.1")
                imageService.setColorChannelLowHigh(root.imageColorChannel,
                                                    imageInfo.grayLow,
                                                    imageInfo.grayHigh)
                //grayRange.first.value = imageInfo.grayLow
                //grayRange.second.value = imageInfo.grayHigh
                imageService.isColorChannel = true
                imageService.showMarker = switchWhiteImage.checked
                imageService.low = imageInfo.grayLow
                imageService.high = imageInfo.grayHigh
                imageService.lowMarker = imageInfo.grayLowMarker
                imageService.highMarker = imageInfo.grayHighMarker
                imageService.updateLowHigh(imageInfo.grayLow, imageInfo.grayHigh)
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
                console.info("\t", "branch 2")
                imageService.invert = imageInvert
                imageService.showMarker = switchWhiteImage.checked
                imageService.showChemi = true
                imageView.colorTableInvert = colorTableInvert
                imageService.isColorChannel = false
                if (switchWhiteImage.checked)
                    imageService.updateLowHighMarker(grayRange.first.value, grayRange.second.value)
                else {
                    imageService.lowMarker = imageInfo.grayLowMarker
                    imageService.highMarker = imageInfo.grayHighMarker
                    imageService.low = grayRange.first.value
                    imageService.high = grayRange.second.value
                    imageService.updateLowHigh(grayRange.first.value, grayRange.second.value)
                }
                imageService.updateView()
            }
            return
        } else {
            console.info("\t", "branch 3")
            imageMergeVisible = true
            chemiWhiteVisible = true
            fluorRGBVisible = false
        }

        if (!imageMergeVisible || (!switchChemiImage.checked && !switchWhiteImage.checked)) {
            console.info("\t", "branch 4")
            imageService.invert = imageInvert
            imageService.showMarker = false
            imageService.showChemi = true
            imageService.low = grayRange.first.value
            imageService.high = grayRange.second.value
            imageView.colorTableInvert = colorTableInvert
        } else if (switchChemiImage.checked && switchWhiteImage.checked) {
            console.info("\t", "branch 5")
            imageService.invert = false
            imageService.showMarker = true
            imageService.showChemi = true
            imageService.low = grayRange.first.value
            imageService.high = grayRange.second.value
            imageService.lowMarker = imageInfo.grayLowMarker
            imageService.highMarker = imageInfo.grayHighMarker
            imageView.colorTableInvert = true
        } else if (switchChemiImage.checked && !switchWhiteImage.checked) {
            console.info("\t", "branch 6")
            imageService.invert = imageInvert
            imageService.showMarker = false
            imageService.showChemi = true
            imageService.low = grayRange.first.value
            imageService.high = grayRange.second.value
            imageView.colorTableInvert = colorTableInvert
        } else if (isOnlyShowWhiteImage()) {
            console.info("\t", "branch 7")
            imageService.invert = false
            imageService.showMarker = true
            imageService.showChemi = false
            imageService.lowMarker = grayRange.first.value
            imageService.highMarker = grayRange.second.value
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

        PageImageController.saveMarkerLowHigh()

        if (imageShowOptions !== undefined) {
            imageShowOptions.isNewImage = false
        }
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
        return PageImageController.isOnlyShowWhiteImage()
    }

    function isOverlapShowMode() {
        return switchWhiteImage.checked && switchChemiImage.checked &&
                switchWhiteImage.visible && switchChemiImage.visible
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
        overExposureHint: root.overExposureHint && planName === "chemi"         //过爆提示
        overExposureHintValue: root.overExposureHintValue
        overExposureHintColor: root.overExposureHintColor
        onImageOpened: {
            if (errorCode === WzEnum.UnsupportedFormat) {
                msgBox.show(qsTr("不支持的文件格式, 请打开16位TIFF图片"), qsTr("确定"))
                return
            }

            if (errorCode !== WzEnum.Success) {
                // TODO 显示打开失败
                return
            }

            var isNewImage = false
            if (imageShowOptions !== undefined && imageShowOptions.isNewImage !== undefined) {
                if (imageShowOptions.isNewImage) {
                    isNewImage = true
                }
            }

            // 此处代码与 updateView 函数中的代码有部分重叠, 后期以清理后者的代码为主, 2020-6-9 by wangzhe
            var activeImageInfo = imageService.getActiveImageInfo()
            var imageInfo
            if (thumbnail.existsImageFile(activeImageInfo.imageFile)) {
                imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)
            } else {
                imageInfo = dbService.getImageByFileName(activeImageInfo.imageFile)
                if (imageInfo.imageFile === undefined) {
                    activeImageInfo = imageService.getActiveImageInfo({generateImageInfo: 1})
                    imageInfo = activeImageInfo
                    dbService.saveImage(imageInfo)
                } else if (imageInfo.isDeleted) {
                    imageInfo.isDeleted = 0
                    var newImageInfo = {
                        imageFile: imageInfo.imageFile,
                        isDeleted: 0
                    }
                    dbService.saveImage(newImageInfo)
                }
                thumbnail.add(imageInfo)
                thumbnail.activeIndex = 0
            }

            if (imageInfo.openedLight === "uv_penetrate") {
                planName = "rna"
                root.imageMergeVisible = false
            } else if (imageInfo.openedLight === "white_down") {
                planName = "protein"
                root.imageMergeVisible = false
            } else if (imageInfo.openedLight === "red" || imageInfo.openedLight === "green" || imageInfo.openedLight === "blue" ||
                       imageInfo.openedLight === "uv_reflex1" ||
                       imageInfo.openedLight === "uv_reflex2") {
                planName = "fluor"
                root.imageMergeVisible = true
                root.chemiWhiteVisible = false
                root.fluorRGBVisible = true

                if (isNewImage) {
                    clearImageColorChannel(imageInfo.colorChannel, imageInfo.imageFile)
                    imageService.setColorChannelFile(imageInfo.imageFile, imageInfo.colorChannel)
                    saveFluorParams()
                }
            } else {
                planName = "chemi"
                root.imageMergeVisible = true
                root.chemiWhiteVisible = true
                root.fluorRGBVisible = false
            }

            if (undefined !== imageInfo.imageWhiteFile &&
                    "" !== imageInfo.imageWhiteFile)
                imageService.markerImageName = imageInfo.imageWhiteFile
            else
                imageService.showMarker = false

            if (!isNewImage)
                PageImageController.restoreLowHigh(imageInfo)

            if (checkboxAutoLowHigh.checked) {
                if (root.chemiWhiteVisible && switchWhiteImage.checked && !switchChemiImage.checked)
                    imageService.calculateLowHighMarker()
                else if (root.fluorRGBVisible && imageInfo.colorChannel > 0)
                    imageService.calculateLowHighRGB(imageInfo.colorChannel)
                else
                    imageService.calculateLowHigh()
            } else {
                if (isNewImage && root.rememberLowHigh) {
                    var imageInfoNew = thumbnail.getImageInfo(thumbnail.activeIndex)
                    imageInfoNew.grayLow = grayRange.first.value
                    imageInfoNew.grayHigh = grayRange.second.value
                    dbService.saveImage(imageInfoNew)
                    thumbnail.setImageInfo(thumbnail.activeIndex, imageInfoNew)
                }
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
                if (grayHighAutoMiddle)
                    grayRange.second.value = (high - low) / 2 + low
                else
                    grayRange.second.value = high
                root.updateView()
            } else {
                // TODO 显示错误内容
            }
        }
        onLowHighMarkerCalculated: {
            if (errorCode === WzEnum.Success) {
                grayRange.first.value = low
                if (grayHighAutoMiddle)
                    grayRange.second.value = (high - low) / 2 + low
                else
                    grayRange.second.value = high
                root.updateView()
            } else {
                // TODO 显示错误内容
            }
        }
        onLowHighRGBCalculated: {
            if (errorCode === WzEnum.Success) {
                var highAdjusted = high
                if (grayHighAutoMiddle)
                    highAdjusted = (high - low) / 2 + low
                var imageInfo = {
                    imageFile: thumbnail.getImageInfo(thumbnail.activeIndex).imageFile,
                    grayLow: low,
                    grayHigh: highAdjusted
                }
                dbService.saveImage(imageInfo)
                root.updateView()
            } else {
                // TODO 显示错误内容
            }
        }
        onAnalysisImageOpened: {
            msgBox.opacity = 0
        }
    }

    Rectangle {
        visible: isMini
        anchors.fill: parent
        color: "black"
    }
    Image {
        visible: isMini && !WzUtils.isNoLogo()
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        source: "qrc:/images/background2.png"
        width: 760
        height: 652
    }
    Rectangle {
        visible: isMini
        anchors.fill: parent
        color: "black"
        opacity: 0.85
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
            width: 155
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
            label.font.pixelSize: 12
            anchors.right: buttonZoomAuto.left
            anchors.rightMargin: 5
            anchors.verticalCenter: textInputZoom.verticalCenter
            width: 20
            height: width
            radius: width
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
            anchors.right: buttonZoomIn.left
            anchors.rightMargin: 5
            anchors.verticalCenter: textInputZoom.verticalCenter
            width: 20
            height: width
            radius: width
            border.color: "#333333"
            normalColor: "#dddddd"
            hotColor: "#ffffff"
            downColor: "#cccccc"
            normalFontColor: "#333333"
            imageVisible: true
            image.source: "qrc:/images/fullscreen_515151.svg"
            image.sourceSize.width: 16
            image.sourceSize.height: 16
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
            width: 20
            height: width
            radius: width
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
                width: parent.width - 8
                color: "#333333"
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.verticalCenter: parent.verticalCenter
            }
            Rectangle {
                width: 1
                height: parent.height - 8
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
            width: 20
            height: width
            radius: width
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
                width: parent.width - 8
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
        color: isMini ? "transparent" : "black"

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
                label.anchors.leftMargin: 5
                label.font.pixelSize: {
                    switch(WzI18N.language) {
                    case "zh": return 18
                    case "en": return 15
                    }
                }
                label.anchors.horizontalCenterOffset: 15
                imageVisible: true
                imageSourceNormal: "qrc:/images/button_save_b4b4b4.svg"
                image.sourceSize.width: 18
                image.sourceSize.height: 18
                image.anchors.horizontalCenterOffset: -30
                onClicked: {
                    // batch export
                    if (thumbnail.listModelSelected.count > 1) {     //图片数量
                        rectShade.opacity = 0.7
                        selectImageFormat.show()
                        return
                    }

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
                label.anchors.horizontalCenterOffset: 17
                imageVisible: true
                imageSourceNormal: "qrc:/images/button_print_b4b4b4.svg"
                image.sourceSize.width: 21
                image.sourceSize.height: 21
                image.anchors.horizontalCenterOffset: -22
                onClicked: {
                    imageService.printImage(WzI18N.language)
                }
            }

            WzButton {
                id: buttonAnalysis
                width: {
                    switch(WzI18N.language) {
                    case "zh": return 95
                    case "en": return 105
                    }
                }
                height: 44
                radius: 4
                text: qsTr("分析")
                anchors.left: buttonPrint.right
                anchors.leftMargin: 18
                anchors.top: buttonSaveAs.top
                label.anchors.leftMargin: 5
                label.font: buttonSaveAs.label.font
                label.anchors.horizontalCenterOffset: 15
                imageVisible: true
                imageSourceNormal: "qrc:/images/button-chart.svg"
                image.sourceSize.width: 28
                image.sourceSize.height: 28
                image.anchors.horizontalCenterOffset: {
                    switch(WzI18N.language) {
                    case "zh": return -22
                    case "en": return -28
                    }
                }
                onClicked: {
                    var imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)
                    if (imageInfo) {
                        var result = imageService.analysisImage(imageInfo)
                        if (result.code !== 0) {
                            msgBox.show(result.msg, qsTr("确定"))
                        } else {
                            msgBox.show(qsTr("请稍候, 正在打开分析软件"))
                        }
                    }
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
                label.anchors.leftMargin: 5
                label.font: buttonSaveAs.label.font
                label.anchors.horizontalCenterOffset: 15
                imageVisible: true
                imageSourceNormal: "qrc:/images/button_open_b4b4b4.svg"
                image.sourceSize.width: 26
                image.sourceSize.height: 18
                image.anchors.horizontalCenterOffset: -22
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
                    case "en": return 2
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
                    if (thumbnail.activeIndex === -1)
                        return
                    if (checkboxAutoLowHigh.checked) {
                        if (switchWhiteImage.checked && !switchChemiImage.checked)
                            imageService.calculateLowHighMarker()
                        else
                            imageService.calculateLowHigh()
                    } else {
                        grayRange.disableValueChanged = true
                        if (switchWhiteImage.checked && !switchChemiImage.checked) {
                            grayRange.first.value = imageService.lowMarker
                            grayRange.second.value = imageService.highMarker
                        } else {
                            grayRange.first.value = imageService.low
                            grayRange.second.value = imageService.high
                        }
                        grayRange.disableValueChanged = false
                        updateView()
                    }
                }
            }

            Text {
                id: textWhiteImage
                visible: opacity > 0
                opacity: imageMergeVisible && (chemiWhiteVisible || fluorRGBVisible) ? 1 : 0
                anchors.left: textChemiImage.visible ? textChemiImage.right : textChemiImage.left
                anchors.leftMargin: textChemiImage.visible ? 28 : 0
                color: "#838383"
                text: qsTr("白光图")
                anchors.bottom: switchWhiteImage.top
                anchors.bottomMargin: 5
                font.pixelSize: textChemiImage.font.pixelSize
                Behavior on opacity {NumberAnimation{duration: 500}}
                MouseArea {
                    anchors.fill: parent
                    onClicked: {
                        if (imageFile === "")
                            return
                        if (WzUtils.isMini())
                            return
                        menuWhiteImage.popup(textWhiteImage.left, textWhiteImage.y + textWhiteImage.height + 2)
                    }
                }
                WzMenu {
                    id: menuWhiteImage
                    width: 170
                    indicatorWidth: 8
                    font.pixelSize: 15

                    Action {
                        text: qsTr("手动设置白光图")
                        onTriggered: {
                            openImageDialog.isOpenMarker = true
                            openImageDialog.open()
                        }
                    }

                    Action {
                        text: qsTr("重新拍摄白光图")
                        onTriggered: {
                            captureWhiteImage(imageFile)
                        }
                    }
                }
            }

            WzSwitch {
                id: switchWhiteImage
                visible: opacity > 0
                opacity: imageMergeVisible && (chemiWhiteVisible || fluorRGBVisible) ? 1 : 0
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
                        msgBox.buttonClicked.connect(selectWhiteImage)
                        msgBox.show(qsTr("没有指定白光图片, 所以无法显示"), qsTr("知道了"), qsTr("现在指定"))
                        checked = false
                        return
                    }
                    if (checkboxAutoLowHigh.checked) {
                        if (switchWhiteImage.checked && !switchChemiImage.checked)
                            imageService.calculateLowHighMarker()
                        else
                            imageService.calculateLowHigh()
                    } else {
                        // 荧光
                        if (fluorRGBVisible) {
                            grayRange.disableValueChanged = true
                            var imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)
                            if (switchWhiteImage.checked) {
                                grayRange.first.value = imageInfo.grayLowMarker
                                grayRange.second.value = imageInfo.grayHighMarker
                            } else {
                                grayRange.first.value = imageInfo.grayLow
                                grayRange.second.value = imageInfo.grayHigh
                            }
                            grayRange.disableValueChanged = false
                            updateView()
                            return
                        }

                        // 化学发光
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
            Text{
                id:textFlipBlackWhite
                opacity: imageMergeVisible &&chemiWhiteVisible ? 1 : 0
                anchors.left: textWhiteImage.visible ? textWhiteImage.right : textWhiteImage.left
                anchors.leftMargin: textWhiteImage.visible ? 28 : 0
                color: "#838383"
                text: qsTr("另存为16位白底图")
                anchors.bottom: textWhiteImage.bottom
                font.pixelSize: textChemiImage.font.pixelSize
                Behavior on opacity {NumberAnimation{duration: 500}}

            }
            WzSwitch{
                id:switchFlipBlackWhite
                opacity: imageMergeVisible &&chemiWhiteVisible ? 1 : 0
                anchors.horizontalCenter: textFlipBlackWhite.horizontalCenter
                anchors.horizontalCenterOffset: 1
                width: 50
                anchors.bottom: switchWhiteImage.bottom
                indicator.height: 22
                textControl.font.pixelSize: 10
                textControl.anchors.verticalCenterOffset: 1
                textControl.x: checked ? 7 : 23
                slideBar.height: 16
                slideBar.width: 16
                Behavior on opacity {NumberAnimation{duration: 500}}
                onCheckedChanged:{
                    imageService.ChangedWhite=checked;
                    imageExporter.ImagesWhite=checked;
                }

            }
            Text {
                id: textFluorRed
                visible: opacity > 0
                opacity: imageMergeVisible && fluorRGBVisible ? 1 : 0
                anchors.left: textWhiteImage.right
                anchors.leftMargin: 20
                color: "#838383"
                text: qsTr("红色通道")
                anchors.bottom: switchFluorRed.top
                anchors.bottomMargin: 5
                font.pixelSize: textChemiImage.font.pixelSize
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
                font.pixelSize: textChemiImage.font.pixelSize
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
                font.pixelSize: textChemiImage.font.pixelSize
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
                font.pixelSize: textChemiImage.font.pixelSize
                Behavior on opacity {NumberAnimation{duration: 500}}
            }

            WzButton {
                id: buttonCancelColorChannel
                normalColor: "#1e1e1e"
                anchors.horizontalCenter: textCancelColorChannel.horizontalCenter
                anchors.top: textCancelColorChannel.bottom
                anchors.topMargin: 5
                width: 42
                height: 24
                radius: 4

                imageVisible: true
                imageSourceNormal: "qrc:/images/button_cancel_393939.svg"
                image.sourceSize.width: 12
                image.sourceSize.height: 12
                imageSourceHot: "qrc:/images/button_cancel_cccccc.svg"
                imageHot.sourceSize.width: 12
                imageHot.sourceSize.height: 12
                imageSourceDown: "qrc:/images/button_cancel_dddddd.svg"
                imageDown.sourceSize.width: 12
                imageDown.sourceSize.height: 12

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
                width: 230
                height: 34
                text: qsTr("在图片中显示 ColorBar")
                anchors.bottomMargin: 10
                anchors.bottom: rectangleLine3.top
                anchors.leftMargin: -6
                anchors.left: parent.left
                onCheckedChanged: {
                    if (checked) {
                        updatePseudoColorBarInImage()
                    }
                    imageService.isPseudoColorBarInImage = checked
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
                font.pixelSize: 29
                font.family: "Digital Dismay"
                color: "#b4b4b4"
                selectByMouse: true
                selectionColor: "white"
                selectedTextColor: "black"
                wrapMode: TextEdit.NoWrap
                validator: IntValidator {
                    bottom: 0
                    top: 65535
                }
                onFocusChanged: {
                    if (focus) {
                        buttonLowSub.opacity = 0.9
                        buttonLowAdd.opacity = 0.9
                    } else {
                        buttonLowSub.opacity = 0
                        buttonLowAdd.opacity = 0
                    }
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
                        PageImageController.saveMarkerLowHigh()
                    } else {
                        imageService.low = Math.round(grayRange.first.value)
                        imageService.high = Math.round(grayRange.second.value)
                        imageService.updateLowHigh(imageService.low, imageService.high)
                        PageImageController.saveLowHigh()
                    }
                    imageService.updateView()
                    buttonLowSub.opacity = 0
                    buttonLowAdd.opacity = 0
                }
                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    cursorShape: Qt.IBeamCursor
                    onClicked: mouse.accepted = false
                    onPressed: {
                        buttonLowSub.opacity = 0.9
                        buttonLowAdd.opacity = 0.9
                        mouse.accepted = false
                    }
                    onReleased: mouse.accepted = false
                    onDoubleClicked: mouse.accepted = false
                    onPositionChanged: mouse.accepted = false
                    onPressAndHold: mouse.accepted = false
                }
            }

            WzButton {
                id: buttonLowSub
                width: 18
                height: 18
                radius: 18
                text: "-"
                anchors.left: textInputGrayLow.left
                anchors.bottom: textInputGrayLow.top
                visible: opacity > 0
                opacity: 0
                Behavior on opacity {NumberAnimation {duration: 500}}
                onClicked: {
                    var newLow = grayRange.first.value - 1
                    if (newLow > -1) {
                        grayRange.first.value = newLow
                        imageService.updateView()
                    }
                }
                onPressed: {
                    changeGrayLowHighTimer.isChangeLow = true
                    changeGrayLowHighTimer.isSub = true
                    changeGrayLowHighTimer.interval = 500
                    changeGrayLowHighTimer.start()
                }
                onReleased: {
                    changeGrayLowHighTimer.stop()
                }
            }
            WzButton {
                id: buttonLowAdd
                width: 18
                height: 18
                radius: 18
                text: "+"
                anchors.left: buttonLowSub.right
                anchors.leftMargin: 1
                anchors.bottom: textInputGrayLow.top
                visible: opacity > 0
                opacity: 0
                Behavior on opacity {NumberAnimation {duration: 500}}
                onClicked: {
                    var newHigh = grayRange.first.value + 1
                    if (newHigh < grayRange.second.value) {
                        grayRange.first.value = newHigh
                    }
                }
                onPressed: {
                    changeGrayLowHighTimer.isChangeLow = true
                    changeGrayLowHighTimer.isSub = false
                    changeGrayLowHighTimer.interval = 500
                    changeGrayLowHighTimer.start()
                }
                onReleased: {
                    changeGrayLowHighTimer.stop()
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
                font.family: "Digital Dismay"
                font.pixelSize: 29
                horizontalAlignment: TextInput.AlignRight
                selectByMouse: true
                selectionColor: "white"
                selectedTextColor: "black"
                wrapMode: TextEdit.NoWrap
                validator: IntValidator {
                    bottom: 0
                    top: 65535
                }
                onFocusChanged: {
                    if (focus) {
                        buttonHighSub.opacity = 0.9
                        buttonHighAdd.opacity = 0.9
                    } else {
                        buttonHighSub.opacity = 0
                        buttonHighAdd.opacity = 0
                    }
                }
                onTextEdited: {
                    var i = parseInt(text)
                    if (i > 65535)
                        text = 65535
                }
                onEditingFinished: {
                    var newGray = parseInt(text)
                    if (newGray <= grayRange.first.value) {
                        if (grayRange.first.value === 65535)
                            newGray = 65535
                        else
                            newGray = Math.round(grayRange.first.value) + 1
                    }
                    grayRange.second.value = newGray
                    if (isOnlyShowWhiteImage()) {
                        imageService.lowMarker = Math.round(grayRange.first.value)
                        imageService.highMarker = Math.round(grayRange.second.value)
                        imageService.updateLowHighMarker(imageService.lowMarker, imageService.highMarker)
                        PageImageController.saveMarkerLowHigh()
                    } else {
                        imageService.low = Math.round(grayRange.first.value)
                        imageService.high = Math.round(grayRange.second.value)
                        imageService.updateLowHigh(imageService.low, imageService.high)
                        PageImageController.saveLowHigh()
                    }
                    imageService.updateView()
                    buttonHighSub.opacity = 0
                    buttonHighAdd.opacity = 0
                }
                MouseArea {
                    anchors.fill: parent
                    propagateComposedEvents: true
                    cursorShape: Qt.IBeamCursor
                    onClicked: mouse.accepted = false
                    onPressed: {
                        buttonHighSub.opacity = 0.9
                        buttonHighAdd.opacity = 0.9
                        mouse.accepted = false
                    }
                    onReleased: mouse.accepted = false
                    onDoubleClicked: mouse.accepted = false
                    onPositionChanged: mouse.accepted = false
                    onPressAndHold: mouse.accepted = false
                }
            }

            WzButton {
                id: buttonHighSub
                width: 18
                height: 18
                radius: 18
                text: "-"
                anchors.right: buttonHighAdd.left
                anchors.bottom: textInputGrayHigh.top
                visible: opacity > 0
                opacity: 0
                Behavior on opacity {NumberAnimation {duration: 500}}
                onClicked: {
                    var newHigh = grayRange.second.value - 1
                    if (newHigh > grayRange.first.value) {
                        grayRange.second.value = newHigh
                        grayLowHighChanged()
                    }
                }
                onPressed: {
                    changeGrayLowHighTimer.isChangeLow = false
                    changeGrayLowHighTimer.isSub = true
                    changeGrayLowHighTimer.interval = 500
                    changeGrayLowHighTimer.start()
                }
                onReleased: {
                    changeGrayLowHighTimer.stop()
                }
            }
            WzButton {
                id: buttonHighAdd
                width: 18
                height: 18
                radius: 18
                text: "+"
                anchors.right: textInputGrayHigh.right
                anchors.bottom: textInputGrayHigh.top
                visible: opacity > 0
                opacity: 0
                Behavior on opacity {NumberAnimation {duration: 500}}
                onClicked: {
                    var newHigh = grayRange.second.value + 1
                    if (newHigh < 65536) {
                        grayRange.second.value = newHigh
                        grayLowHighChanged()
                    }
                }
                onPressed: {
                    changeGrayLowHighTimer.isChangeLow = false
                    changeGrayLowHighTimer.isSub = false
                    changeGrayLowHighTimer.interval = 500
                    changeGrayLowHighTimer.start()
                }
                onReleased: {
                    changeGrayLowHighTimer.stop()
                }
            }

            Timer {
                id: changeGrayLowHighTimer
                property bool isChangeLow: true
                property bool isSub: true
                property int changeStep: 1
                property int tick: 0
                repeat: true
                onTriggered: {
                    if (interval != 100) {
                        interval = 100
                        tick = 1000
                    }
                    tick += interval
                    if (tick > 1000)
                        changeStep = (tick / 500) * (isSub ? -1 : 1)
                    if (isChangeLow) {
                        var newLow = grayRange.first.value + changeStep
                        if (newLow < 1) {
                            newLow = 0
                        } else if (newLow >= grayRange.second.value) {
                            newLow = grayRange.second.value - 1
                        }
                        grayRange.first.value = Math.round(newLow)
                        grayLowHighChanged()
                    } else {
                        var newHigh = grayRange.second.value + changeStep
                        if (newHigh > 65535) {
                            newHigh = 65535
                        } else if (newHigh <= grayRange.first.value) {
                            newHigh = grayRange.first.value + 1
                        }
                        grayRange.second.value = Math.round(newHigh)
                        grayLowHighChanged()
                    }
                }
            }

            WzRangeSlider {
                id: grayRange
                property bool disableValueChanged: false
                width: 460
                stepSize: 1
                first.value: 500
                second.value: 20000
                from: 0
                to: 65535                
                anchors.left: parent.left
                anchors.bottom: checkboxAutoLowHigh.top
                anchors.bottomMargin: 5
                first.onValueChanged: {
                    textInputGrayLow.text = Math.round(first.value)
                    if (disableValueChanged)
                        return
                    if (first.pressed || updateLowHighWhenValueChanged)
                        grayLowHighChanged()
                }
                first.onPressedChanged: {
                    if (!first.pressed) {
                        grayLowHighChanged()
                    }
                }
                second.onValueChanged: {
                    textInputGrayHigh.text = Math.round(second.value)
                    if (disableValueChanged)
                        return
                    if (second.pressed || updateLowHighWhenValueChanged)
                        grayLowHighChanged()
                }
                second.onPressedChanged: {
                    if (!second.pressed) {                        
                        grayLowHighChanged()
                    }
                }
            }

            WzCheckBox {
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
                imageVisible: true
                imageSourceNormal: "qrc:/images/flash_b4b4b4.svg"
                image.sourceSize.width:  16
                image.sourceSize.height: 16
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
                    pseudoColorInImage.rgbList = arr
                    imageView.colorTable = arr
                    imageView.colorTableInvert = isColorTableInvert()
                    imageView.update()
                    imageService.updateView()
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

    Platform.FileDialog {
        id: openImageDialog
        property bool isOpenMarker: false
        fileMode: Platform.FileDialog.OpenFile
        title: qsTr("打开图片文件")
        nameFilters: ["TIFF(*.tif *.tiff)"]//, "JPEG(*.jpg *.jpeg)", "PNG(*.png)", "Bitmap(*.bmp)"]
        onAccepted: {            
            // 异步函数, 回调函数是 imageOpened, 定义在 imageService 声明的地方
            if (isOpenMarker) {
                isOpenMarker = false
                var imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)
                imageInfo.imageWhiteFile = WzUtils.toLocalFile(openImageDialog.currentFile)
                root.imageWhiteFile = imageInfo.imageWhiteFile
                thumbnail.setImageInfo(thumbnail.activeIndex, imageInfo)
                var imageInfoNew = {
                    imageFile: imageInfo.imageFile,
                    imageWhiteFile: imageInfo.imageWhiteFile
                }
                dbService.saveImage(imageInfoNew)
                imageService.markerImageName = root.imageWhiteFile
                if (!switchWhiteImage.checked)
                    switchWhiteImage.checked = true
            } else
                imageService.openImage(openImageDialog.currentFile)
        }
        onRejected: {

        }
    }

    Platform.FileDialog {
        id: saveImageDialog
        title: qsTr("另存图片")
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: [qsTr("16位TIFF(*.tif)"), qsTr("8位 TIFF(*.tif)"), qsTr("24位 TIFF(*.tif)") , "JPEG(*.jpg)", "PNG(*.png)"]
        onAccepted: {
            if (selectedNameFilter.index === 1 && paletteName !== "Gray") {
                msgBox.show(qsTr("使用了伪彩时无法保存为8位的图片"), qsTr("确定"))
                return
            }

            console.log(saveImageDialog.currentFile)
            dbService.saveStrOption("save_image_path", saveImageDialog.folder)
            var formats = ["tiff16", "tiff8", "tiff24", "jpeg", "png"]
            var selectedFormat = formats[selectedNameFilter.index]//所选格式
            var imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)//在下方滑轨栏选择图片
            var saveAsParams =  {
                format: selectedFormat,           //所选格式
                isSaveMarker: isOverlapShowMode(),//是否保存marker图
                fileName: WzUtils.toLocalFile(saveImageDialog.currentFile),
                isMedFilTiff8: WzUtils.isMedFilTiff8bit() && pageOption.isMedFilTiff8,
                medFilSize: pageOption.medFilSize
            }
            for (var prop in imageInfo)
                saveAsParams[prop] = imageInfo[prop]
            var ret = imageService.saveAsImage(saveAsParams)
            if (ret) {
                if (switchChemiImage.checked && switchWhiteImage.checked) {//判断2个按键是否触发
                    var autoSaveChemiMarker = dbService.readIntOption("autoSaveChemiMarker", 0)
                    if (autoSaveChemiMarker) {
                        imageService.saveChemiMarker(saveImageDialog.currentFile, selectedFormat)//保存化学发光marker图
                    }
                }
                msgBox.buttonClicked.connect(imageSaveSuccess)//判读摁下的是哪个按钮
                msgBox.show(qsTr("保存成功"), qsTr("确定"), qsTr("打开文件夹"))
            } else {
                msgBox.show(qsTr("保存失败"), qsTr("确定"))
            }
            dbService.saveIntOption("save_image_filter", selectedNameFilter.index)
        }
    }

    Dialogs.FileDialog {
        id: saveImagesDialog
        title: qsTr("批量导出图片")
        selectExisting: false
        selectFolder: true
        onAccepted: {
            console.log(saveImagesDialog.folder)
            exportImages(saveImagesDialog.folder, selectImageFormat.imageFormat)
            dbService.saveStrOption("save_image_path", saveImagesDialog.folder)
            msgBox.show(qsTr("保存成功"), qsTr("确定"))
        }
    }

    Rectangle {
        id: rectShade
        anchors.fill: parent
        color: "#000000"
        visible: opacity > 0
        opacity: 0
        z: 10
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            preventStealing: true
        }
        Behavior on opacity {NumberAnimation {duration: 300}}
    }

    SelectImageFormat {
        id: selectImageFormat
        anchors.centerIn: parent
        onConfirm: {
            rectShade.opacity = 0
            saveImagesDialog.open()
        }
        onHide: {
            rectShade.opacity = 0
        }
        z: 11
    }

    WzImageExporter {
        id: imageExporter
    }

    function exportImages(path, format) {
        var images = []
        for (var i = 0; i < thumbnail.listModelSelected.count; i++) {
            var src = thumbnail.listModelSelected.get(i)
            var imageInfo = {
                exportPath: path,
                exportFormat: format,
                imageFile: src.imageFile,
                imageWhiteFile: src.imageWhiteFile,
                grayLow: src.grayLow,
                grayHigh: src.grayHigh,
                sampleName: src.sampleName
            }
            images.push(imageInfo)
        }
        var exportParams = {
            exportPath: path,
            exportFormat: format,
            images: images
        }
        imageExporter.exportImages(exportParams)
    }

    function grayLowHighChanged() {
        console.info("grayLowHighChanged")
        if (imageService.isColorChannel && undefined !== imageColorChannel && imageColorChannel > 0) {
            imageService.setColorChannelLowHigh(imageColorChannel,
                                                grayRange.first.value,
                                                grayRange.second.value)
            imageService.low = Math.round(grayRange.first.value)
            imageService.high = Math.round(grayRange.second.value)
            imageService.updateLowHigh(imageService.low, imageService.high)
            PageImageController.saveLowHigh()
        } else if (isOnlyShowWhiteImage()) {
            console.info("isOnlyShowWhiteImage")
            imageService.lowMarker = Math.round(grayRange.first.value)
            imageService.highMarker = Math.round(grayRange.second.value)
            imageService.updateLowHighMarker(imageService.lowMarker, imageService.highMarker)
            PageImageController.saveMarkerLowHigh()
        } else {
            imageService.low = Math.round(grayRange.first.value)
            imageService.high = Math.round(grayRange.second.value)
            imageService.updateLowHigh(imageService.low, imageService.high)
            PageImageController.saveLowHigh()
        }
        imageService.updateView()
        updatePseudoColorBarInImage()
    }

    function updatePseudoColorBarInImage() {
        textPseudoColorGrayHigh.text = Math.round(grayRange.second.value)
        textPseudoColorGrayLow.text = Math.round(grayRange.first.value)
        textPseudoColorGrayMiddle.text = Math.round((grayRange.second.value - grayRange.first.value) / 2 + grayRange.first.value)
        imageView.grayHigh = Math.round(grayRange.second.value)
        imageView.grayLow = Math.round(grayRange.first.value)
        imageView.update()
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

    function imageSaveSuccess(buttonID) {
        msgBox.buttonClicked.disconnect(imageSaveSuccess)
        if (buttonID === 2) {
            WzUtils.openPathByFileName(saveImageDialog.currentFile)
        }
    }

    function selectWhiteImage(buttonID) {
        msgBox.buttonClicked.disconnect(selectWhiteImage)
        if (buttonID === 2) {
            openImageDialog.isOpenMarker = true
            openImageDialog.open()
        }
    }
}


/*##^##
Designer {
    D{i:0;autoSize:true;height:800;width:1280}D{i:26;anchors_y:99}D{i:27;anchors_width:95;anchors_x:2;anchors_y:99}
D{i:28;anchors_y:99}
}
##^##*/
