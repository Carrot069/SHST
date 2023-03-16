function updateMarkerImage2(markerImageFile) {
    var imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)
    if (undefined !== imageInfo.imageWhiteFile &&
            "" !== imageInfo.imageWhiteFile) {
        WzUtils.deleteFile(imageInfo.imageWhiteFile)
    }

    imageInfo.imageWhiteFile = markerImageFile
    thumbnail.setImageInfo(thumbnail.activeIndex, imageInfo)

    var newImageInfo = {
        imageFile: imageInfo.imageFile,
        imageWhiteFile: markerImageFile
    }
    dbService.saveImage(newImageInfo)

    imageService.markerImageName = markerImageFile

    updateView()
}

function isOnlyShowWhiteImage() {
    if (fluorRGBVisible) {
        return switchWhiteImage.checked && !switchFluorRed.checked && !switchFluorGreen.checked && !switchFluorBlue.checked
    } else {
        return switchWhiteImage.visible && switchWhiteImage.checked && !switchChemiImage.checked
    }
}

// 启用了"记忆调整过的灰阶值"后将保存在数据库中的灰阶值恢复到界面中
function restoreLowHigh(imageInfo) {
    console.info("restoreLowHigh")
    if (!rememberLowHigh) {
        console.info("rememberLowHigh === false, exit")
        return
    }
    // 没有显示叠加选项, 这种情况是紫外透射或者蓝白光透射拍的, 只有一张图片所以直接恢复
    if (!chemiWhiteVisible) {
        console.info("chemiWhiteVisible === false, can restore")
    } else {
        // 只显示了白光, 不需要恢复
        if (isOnlyShowWhiteImage()) {
            console.info("isOnlyShowWhiteImage, not restore")
            return
        }
    }
    console.info("grayLow = ", imageInfo.grayLow, ", grayHigh = ", imageInfo.grayHigh)
    if (imageInfo.grayLow === 0 && imageInfo.grayHigh === 0) {
        return
    }
    grayRange.disableValueChanged = true
    grayRange.first.value = imageInfo.grayLow
    grayRange.second.value = imageInfo.grayHigh
    grayRange.disableValueChanged = false
}

function saveLowHigh() {
    var imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)

    var imageInfoNew = {
        imageFile: imageInfo.imageFile,
        grayLow: imageService.low,
        grayHigh: imageService.high
    }
    dbService.saveImage(imageInfoNew)

    imageInfo.grayLow = imageInfoNew.grayLow
    imageInfo.grayHigh = imageInfoNew.grayHigh
    thumbnail.setImageInfo(thumbnail.activeIndex, imageInfo)
}

function saveMarkerLowHigh() {
    var imageInfo = thumbnail.getImageInfo(thumbnail.activeIndex)

    var imageInfoNew = {
        imageFile: imageInfo.imageFile,
        grayLowMarker: imageService.lowMarker,
        grayHighMarker: imageService.highMarker
    }
    dbService.saveImage(imageInfoNew)

    imageInfo.grayLowMarker = imageInfoNew.grayLowMarker
    imageInfo.grayHighMarker = imageInfoNew.grayHighMarker
    thumbnail.setImageInfo(thumbnail.activeIndex, imageInfo)
}
