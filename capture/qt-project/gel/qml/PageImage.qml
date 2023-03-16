import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Dialogs 1.2 as Dialogs
import QtQuick.Window 2.12
import Qt.labs.platform 1.1 as Platform

import WzImage 1.0
import WzUtils 1.0
import WzEnum 1.0
import WzI18N 1.0

import "WzControls"
import "WzControls.2"
import "Components"

Item {
    id: root
    signal pageChange(int pageIndex)

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
    property bool isRGBImage: false

    property alias captureProgress: captureProgressMini

    function closeActiveImage() {
        root.imageFile = ""
        imageService.closeActiveImage()
    }

    function openImage(imageFile, showOptions) {
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

        textInputSampleName.text = root.imageFile === "" ? "" : (root.sampleName === "" ? qsTr("<点击输入>") : root.sampleName)

        imageService.markerImageName = root.imageWhiteFile

        if (imageInfo.imageInvert === undefined)
            imageInvertFlag = 0
        else
            imageInvertFlag = imageInfo.imageInvert

        var imageInvert = true
        if (imageInfo.imageInvert === undefined) {
            if (WzUtils.isGelCapture())
                imageInvert = false
            else
                imageInvert = true
        } else if (imageInfo.imageInvert === 1) {
            if (WzUtils.isGelCapture())
                imageInvert = false
            else
                imageInvert = true
        } else if (imageInfo.imageInvert === 2 || imageInfo.imageInvert === 4)
            imageInvert = false
        else {
            if (WzUtils.isGelCapture())
                imageInvert = false
            else
                imageInvert = true
        }

        var colorTableInvert = isColorTableInvert2()

        if (WzUtils.isGelCapture()) {
            imageService.invert = imageInvert
            imageService.showMarker = false
            imageService.showChemi = true
            imageView.colorTableInvert = colorTableInvert
        } else if (!switchChemiImage.checked && !switchWhiteImage.checked) {
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
        } else if (!switchChemiImage.checked && switchWhiteImage.checked) {
            imageService.invert = false
            imageService.showMarker = true
            imageService.showChemi = false
            imageView.colorTableInvert = true
        }

        if (!checkboxAutoLowHigh.checked) {
            if (imageShowOptions) {
                if (imageShowOptions.fromPageCapture) {
                    var isCaptureSamePreview = dbService.readIntOption("isCaptureSamePreview", 1)
                    if (isCaptureSamePreview) {
                        grayRange.first.value = 0
                        grayRange.second.value = 65535
                    }
                }
            }
        }

        imageService.updateLowHigh(grayRange.first.value, grayRange.second.value)
        imageService.updateView()
    }

    function isColorTableInvert2() {
        var b = isColorTableInvert()
        if (buttonInvert.checked)
            b = !b
        return b
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
        checkBoxColorBar.checked = dbService.readIntOption("color_bar_in_image", 0) === 1
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
                msgBox.show(qsTr("文件打开失败, 错误代码:") + errorCode, qsTr("确定"))
                return
            }            
            if (checkboxAutoLowHigh.checked) {
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
            root.isRGBImage = false
            var imageInfo = imageService.getActiveImageInfo()            
            if (imageInfo !== undefined) {
                if (imageInfo.bitDepth === 8 && imageInfo.samplesPerPixel === 3) {
                    root.isRGBImage = true
                    textImageFileNameTitleAnimation.easing.type = Easing.InQuint
                    textColorBar.opacity = 0
                    textGrayHighLow.opacity = 0
                    rectangleLine1.visible = false
                    textImageFileNameTitle.anchors.bottomMargin = 10
                } else {
                    textImageFileNameTitleAnimation.easing.type = Easing.OutQuint
                    textColorBar.opacity = 1
                    textGrayHighLow.opacity = 1
                    rectangleLine1.visible = true
                    textImageFileNameTitle.anchors.bottomMargin = 20 + (textImageFileNameTitle.parent.height - rectangleLine1.y)
                }
            }
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
            contentWidth: imageView.showWidth < width ? width : imageView.showWidth
            contentHeight: imageView.showHeight < height ? height : imageView.showHeight

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
                imageView.x = contentX
                imageView.update()
            }
            visibleArea.onYPositionChanged: {
                if (isIgnorePosChanged) {
                    return
                }
                previousYPos = visibleArea.yPosition
                imageView.y = contentY
                imageView.update()
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
                    color: "#606060"
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
                    color: "#606060"
                    radius: 15
                }
            }

            Rectangle {
                color: "#7f7f7f"
                anchors.fill: parent
            }

            WzImageView {
                id: imageView
                width: imageViewWrapper.width
                height: imageViewWrapper.height
                showColorBar: false//checkBoxColorBar.checked &&
                              //width < imageViewWrapper.width && height < imageViewWrapper.height

                onImageRotate: imageService.imageRotate(angle, true)

                onZoomChanging: imageViewWrapper.isIgnorePosChanged = true
                onZoomChanged: {
                    imageViewWrapper.isIgnorePosChanged = false
                    x = imageViewWrapper.contentX
                    y = imageViewWrapper.contentY
                }
                onCropImageSelected: {
                    buttonCropImageApply.x = rect.x + rect.width - buttonCropImageApply.width + 3
                    buttonCropImageApply.y = rect.y + rect.height + 3
                    buttonCropImageApply.opacity = 1
                    buttonCropImageReselect.opacity = 1
                    buttonCropImageCancel.opacity = 1
                }
                onCropImageUnselect: {
                    buttonCropImageApply.opacity = 0
                    buttonCropImageReselect.opacity = 0
                    buttonCropImageCancel.opacity = 0
                }
                onCropImageRectMoving: {
                    buttonCropImageApply.x = rect.x + rect.width - buttonCropImageApply.width + 3
                    buttonCropImageApply.y = rect.y + rect.height + 3
                }

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

                onCurrentGrayChanged: {
                    //textCurrentGray.text = gray + "(X:" + x + ", Y:" + y + ")"
                }

                onPopupMenu: {
                    imagePopupMenu.customizeItems()
                    imagePopupMenu.popup(x, y)
                }

                ImagePopupMenu {
                    id: imagePopupMenu
                }                

                WzButton {
                    id: buttonCropImageApply
                    text: qsTr("确定")
                    height: 25
                    width: WzI18N.isZh ? 40 : 60
                    visible: opacity > 0
                    opacity: 0
                    onClicked: {
                        buttonCropImageApply.opacity = 0
                        buttonCropImageReselect.opacity = 0
                        buttonCropImageCancel.opacity = 0
                        imageService.cropImage(imageView.selectedRectNoZoom)
                        imageView.cropImageApply()
                    }
                    Behavior on opacity { NumberAnimation { duration: 200 }}
                }
                WzButton {
                    id: buttonCropImageReselect
                    text: qsTr("重选")
                    height: buttonCropImageApply.height
                    width: buttonCropImageApply.width
                    anchors.bottom: buttonCropImageApply.bottom
                    anchors.right: buttonCropImageApply.left
                    anchors.rightMargin: 3
                    visible: opacity > 0
                    opacity: 0
                    onClicked: imageView.cropImageReselect()
                    Behavior on opacity { NumberAnimation { duration: 200 }}
                }
                WzButton {
                    id: buttonCropImageCancel
                    text: qsTr("取消")
                    height: buttonCropImageApply.height
                    width: buttonCropImageApply.width
                    anchors.bottom: buttonCropImageApply.bottom
                    anchors.right: buttonCropImageReselect.left
                    anchors.rightMargin: 3
                    visible: opacity > 0
                    opacity: 0
                    onClicked: imageView.cropImageCancel()
                    Behavior on opacity { NumberAnimation { duration: 200 }}
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
            width: 180
            anchors.right: imageViewWrapper.right
            anchors.rightMargin: imageViewWrapper.height > imageView.height ? 10 : 25
            anchors.bottom: imageViewWrapper.bottom
            anchors.bottomMargin: imageViewWrapper.width > imageView.width ? 10 : 25

            Behavior on anchors.rightMargin { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
            Behavior on anchors.bottomMargin { NumberAnimation { duration: 500; easing.type: Easing.InOutQuad } }
        }
        WzButton {
            id: buttonInvert
            checkable: true
            visible: rectZoom.visible
            anchors.right: buttonZoomOriginal.left
            anchors.rightMargin: 5
            anchors.verticalCenter: textInputZoom.verticalCenter
            width: 20
            height: width
            radius: width
            border.color: "#a5a5a5"
            normalColor: "#dddddd"
            hotColor: "#ffffff"
            downColor: "#cccccc"
            normalFontColor: "#333333"
            imageVisible: true
            image.source: "qrc:/images/invert.svg"
            image.sourceSize.width: 18
            image.sourceSize.height: 18
            imageSourceChecked: "qrc:/images/invert2.svg"
            imageChecked.sourceSize.width: 18
            imageChecked.sourceSize.height: 18
            onClicked: {
                forceActiveFocus()
                updateView()
            }
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
            visible: !isRGBImage && checkBoxColorBar.checked && imageFile !== ""/* && !imageView.showColorBar*/
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
                imageService.openImage(imageInfo.imageFile)
            }
            onDeleteImage: {
                if (isDeleteFile)
                    imageService.deleteImage(imageInfo.imageFile)
                dbService.deleteImageByFileName(imageInfo.imageFile)
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
            TextInput {
                id: textInputSampleName
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
                selectByMouse: true
                text: root.imageFile === "" ? "" : (root.sampleName === "" ? qsTr("<点击输入>") : root.sampleName)
                Keys.onReturnPressed: {
                    if (text === qsTr("<点击输入>"))
                        root.sampleName = ""
                    else
                        root.sampleName = text
                    thumbnail.activeSampleName = root.sampleName
                    var imageInfo = {
                        imageFile: root.imageFile,
                        sampleName: root.sampleName
                    }
                    dbService.saveImage(imageInfo)
                    focus = false
                }
                onFocusChanged: {
                    if (focus && root.sampleName === "")
                        text = ""
                    else if (!focus && text === "")
                        text = qsTr("<点击输入>")
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
                anchors.left: textInputSampleName.left
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
                anchors.left: textInputSampleName.left
                anchors.verticalCenter: textImageCaptureTimeTitle.verticalCenter
                elide: Text.ElideRight
                font.pixelSize: 18
            }

            Text {
                id: textImageFileNameTitle
                color: "#838383"
                text: qsTr("存储位置:")
                elide: Text.ElideRight
                anchors.bottomMargin: 20 + (rectangleLine1.parent.height - rectangleLine1.y)
                anchors.bottom: parent.bottom
                font.pixelSize: 18
                horizontalAlignment: textSampleNameTitle.horizontalAlignment
                width: textSampleNameTitle.width
                Behavior on anchors.bottomMargin {
                    NumberAnimation {
                        id: textImageFileNameTitleAnimation
                        duration: 1000
                        easing.type: Easing.OutQuint
                    }
                }
            }
            Text {
                id: textImageFileName
                color: "#838383"
                text: root.imageFile
                elide: Text.ElideLeft
                anchors.left: textInputSampleName.left
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
                anchors.bottomMargin: 21
                anchors.bottom: textImageMerge.visible ? textImageMerge.top : textColorBar.top
            }

            /********************* 图片叠加 *********************/
            Text {
                id: textImageMerge
                visible: !WzUtils.isGelCapture()
                x: -1
                y: 0
                color: "#323232"
                text: qsTr("图片叠加")
                anchors.bottomMargin: 15
                anchors.bottom: textWhiteImage.top
                font.pixelSize: 18
            }

            Text {
                id: textChemiImage
                visible: textImageMerge.visible
                x: 0
                y: 7
                color: "#838383"
                text: qsTr("化学发光")
                anchors.horizontalCenter: switchChemiImage.horizontalCenter
                anchors.bottom: textWhiteImage.bottom
                font.pixelSize: 18
            }

            WzSwitch {
                id: switchChemiImage
                visible: textImageMerge.visible
                width: 50
                anchors.left: textWhiteImage.right
                anchors.leftMargin: 40
                anchors.bottom: switchWhiteImage.bottom
                indicator.height: 22
                textControl.font.pixelSize: 10
                textControl.anchors.verticalCenterOffset: 1
                textControl.x: checked ? 7 : 23
                slideBar.height: 16
                slideBar.width: 16
                onCheckedChanged: {
                    updateView()
                }
            }

            Text {
                id: textWhiteImage
                visible: textImageMerge.visible
                x: 0
                y: 7
                color: "#838383"
                text: qsTr("白光图")
                anchors.horizontalCenter: switchWhiteImage.horizontalCenter
                anchors.bottom: switchWhiteImage.top
                anchors.bottomMargin: 3
                font.pixelSize: 18
            }

            WzSwitch {
                id: switchWhiteImage
                visible: textImageMerge.visible
                width: 50
                anchors.bottomMargin: 11
                anchors.bottom: rectangleLine2.top
                indicator.height: 22
                textControl.font.pixelSize: 10
                textControl.anchors.verticalCenterOffset: 1
                textControl.x: checked ? 7 : 23
                slideBar.height: 16
                slideBar.width: 16
                onCheckedChanged: {
                    if (checked && imageWhiteFile === "") {
                        msgBox.show("没有指定白光图片, 所以无法显示", "确定")
                        checked = false
                    } else {
                        updateView()
                    }
                }
            }

            Rectangle {
                id: rectangleLine2
                visible: textImageMerge.visible
                x: 1
                y: 0
                width: parent.width
                height: 1
                color: "#161616"
                anchors.bottom: textColorBar.top
                anchors.bottomMargin: 15
            }

            /******************************** 颜色调整 ********************************/
            Text {
                id: textColorBar
                visible: opacity > 0
                color: "#323232"
                text: qsTr("伪彩")
                anchors.bottom: rectPseudoBar.top
                anchors.bottomMargin: 15
                font.pixelSize: 18
                Behavior on opacity {
                    NumberAnimation {
                        duration: 1000
                    }
                }
            }

            Rectangle {
                id: rectPseudoBar
                opacity: textColorBar.opacity
                visible: opacity > 0
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
                opacity: textColorBar.opacity
                visible: opacity > 0
                text: qsTr("在图片中显示 ColorBar")
                anchors.bottomMargin: 10
                anchors.bottom: rectangleLine3.top
                anchors.leftMargin: -8
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
                opacity: textColorBar.opacity
                visible: opacity > 0
                width: parent.width
                height: 1
                color: "#161616"
                anchors.bottom: textGrayHighLow.top
                anchors.bottomMargin: 15
            }

            /********************* 显示灰阶 *********************/
            Text {
                id: textGrayHighLow
                visible: opacity > 0
                color: "#323232"
                text: qsTr("显示灰阶")
                anchors.bottom: textInputGrayLow.top
                anchors.bottomMargin: 15
                font.pixelSize: 18
                Behavior on opacity {
                    NumberAnimation {
                        duration: 1000
                    }
                }
            }

            TextInput {
                id: textInputGrayLow
                opacity: textGrayHighLow.opacity
                visible: opacity > 0
                text: Math.round(grayRange.first.value)
                width: 120
                anchors.bottom: grayRange.top
                anchors.bottomMargin: 3
                font.pixelSize: 30
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
                    imageService.updateLowHigh(Math.round(grayRange.first.value),
                                               Math.round(grayRange.second.value))
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
                opacity: textGrayHighLow.opacity
                visible: opacity > 0
                color: "#b4b4b4"
                width: 120
                text: Math.round(grayRange.second.value)
                anchors.right: parent.right
                anchors.bottom: textInputGrayLow.bottom
                font.family: "Digital Dismay"
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
                    imageService.updateLowHigh(Math.round(grayRange.first.value),
                                               Math.round(grayRange.second.value))
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
                opacity: textGrayHighLow.opacity
                visible: opacity > 0
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

            WzCheckBox {
                id: checkboxAutoLowHigh
                opacity: textGrayHighLow.opacity
                visible: opacity > 0
                text: qsTr("自动调整")
                anchors.leftMargin: -10
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                onCheckedChanged: {
                    if (checked) {
                        imageService.calculateLowHigh()
                    }
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
                    imageView.colorTableInvert = isColorTableInvert2()
                    imageView.update()
                    pseudoBar.rgbList = arr
                    pseudoBar.update()
                }
                onClose: {
                    if (!isOk)
                        pseudoColor.restoreConfig()
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
        fileMode: Platform.FileDialog.OpenFile
        title: qsTr("打开图片文件")
        nameFilters: ["TIFF(*.tif *.tiff)"]//, "JPEG(*.jpg *.jpeg)", "PNG(*.png)", "Bitmap(*.bmp)"]
        onAccepted: {
            // 异步函数, 回调函数是 imageOpened, 定义在 imageService 声明的地方
            imageService.openImage(openImageDialog.currentFile)
        }
        onRejected: {

        }
    }

    Platform.FileDialog {
        id: saveImageDialog
        title: qsTr("另存图片")
        fileMode: Platform.FileDialog.SaveFile
        nameFilters: {
            if (root.isRGBImage)
                return [qsTr("24位 TIFF(*.tif)") , "JPEG(*.jpg)", "PNG(*.png)"]
            else
                return [qsTr("16位 TIFF(*.tif)"), qsTr("8位 TIFF(*.tif)"), qsTr("24位 TIFF(*.tif)") , "JPEG(*.jpg)", "PNG(*.png)"]
        }
        onAccepted: {
            if (selectedNameFilter.index === 1 && paletteName !== "Gray") {
                msgBox.show(qsTr("使用了伪彩时无法保存为8位的图片"), qsTr("确定"))
                return
            }
            console.log(saveImageDialog.currentFile)
            dbService.saveStrOption("save_image_path", saveImageDialog.folder)
            var formats = ["tiff16", "tiff8", "tiff24", "jpeg", "png"]
            if (root.isRGBImage)
                formats = ["tiff24", "jpeg", "png"]
            var selectedFormat = formats[selectedNameFilter.index]
            var imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)
            var saveAsParams =  {
                format: selectedFormat,
                isSaveMarker: false,
                fileName: WzUtils.toLocalFile(saveImageDialog.currentFile)
            }
            for (var prop in imageInfo)
                saveAsParams[prop] = imageInfo[prop]
            var ret = imageService.saveAsImage(saveAsParams)
            if (ret) {
                msgBox.show(qsTr("保存成功"), qsTr("确定"))
            } else {
                msgBox.show(qsTr("保存失败"), qsTr("确定"))
            }
            dbService.saveIntOption("save_image_filter", selectedNameFilter.index)
        }
    }

    function grayLowHighChanged() {
        imageService.updateLowHigh(Math.round(grayRange.first.value),
                                   Math.round(grayRange.second.value))
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
}


/*##^##
Designer {
    D{i:0;autoSize:true;height:800;width:1280}D{i:26;anchors_y:99}D{i:27;anchors_width:95;anchors_x:2;anchors_y:99}
D{i:28;anchors_y:99}
}
##^##*/
