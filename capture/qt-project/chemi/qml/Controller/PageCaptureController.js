function captureWhiteImageConfirm(step) {
    if (step === 0 && !isPreviewing) {
        switchPreview.checked = true
        previewStartingRect.opacity = 1
        tmrCaptureWhiteImage.repeat = false
        tmrCaptureWhiteImage.running = false
        tmrCaptureWhiteImage.interval = 2000
        tmrCaptureWhiteImage.running = true
        return
    }
    var hadMarkerImage = false
    var isCheckMarkerDark = "1" === dbService.readStrOption("isCheckMarkerDark", "0")
    var checkMarkerDarkThreshold = parseInt(dbService.readStrOption("checkMarkerDarkThreshold", "100"))
    for (var i = 0; i < 3; i++) {
        captureService.getMarkerImage()
        if (isCheckMarkerDark && captureService.hasMarkerImage(checkMarkerDarkThreshold)) {
            hadMarkerImage = true
            break
        }
    }
    if (!hadMarkerImage) {
        msgBox.buttonCount = 1
        msgBox.show(qsTr("获取Marker图时出现错误, 请重试"), "确定")
        return
    }

    var fileName = captureService.saveMarkerImage(new Date())
    if ("" === fileName) {
        msgBox.buttonCount = 1
        msgBox.show(qsTr("保存Marker图时出现错误, 请重试"), "确定")
        return
    }

    markerImageSaved(fileName)

    captureWhiteImageRect.opacity = 0
    switchPreview.checked = false
}

function previewExposureIsVisible() {
    return switchPreview.checked && capturePlanIsChemiFluor() && !isMini
}

function capturePlanIsChemiFluor() {
    return capturePlan.planName === "chemi" || capturePlan.planName === "fluor"
}

function capturePlanIsChemi() {
    return capturePlan.planName === "chemi" || isMini
}

function capturePlanIsFluor() {
    return capturePlan.planName === "fluor"
}

function enablePreview(isEnabled) {
    console.info("CaptureController.enablePreview")

    if (isEnabled) {
        if (isMini) {
            // 在Mini模块中有相关代码处理这个部分
        } else if (capturePlanIsChemiFluor()) {
            previewExposure.exposureTime.forceActiveFocus()
            captureService.previewExposureMs = previewExposure.exposureTime.value           
        } else {
            captureService.previewExposureMs = exposureTime.exposureMs
        }

        console.info("enablePreview, open light")
        //if (!mcu.latestLightOpened) {
            var lightType
            if (isMini) {
                lightType = "white_up"
            } else {
                lightType = capturePlan.getLightNameFromIndex(capturePlan.params.light)
            }
            mcu.switchLight(lightType, true)
        //}

        captureService.enableCameraPreview(true, buttonBinning.binning, adminParams.pvcamSlowPreview)
    } else {
        if (mcu.latestLightOpened && mcu.latestLightType === "white_up") {
            mcu.closeAllLight()
        }
        captureService.enableCameraPreview(false, buttonBinning.binning)
    }
}

function miniAutoStartPreview() {
    console.info("miniAutoStartPreview, step1")
    if (getTemperatureTimer.currentTemp > -20)
        return
    var cs = captureService.cameraState
    console.info("\t", cs, captureService.captureCount, captureService.capturedCount)
    if (cs === WzCameraState.CaptureFinished) {
        if (captureService.captureCount > 0 &&
                captureService.capturedCount < captureService.captureCount)
            return
    }
    console.info("miniAutoStartPreview, 2")
    if (cs === WzCameraState.Connected ||
            cs === WzCameraState.CaptureFinished ||
            cs === WzCameraState.PreviewStopped ||
            cs === WzCameraState.CaptureAborted) {
        liveImageView.visible = true
        getTemperatureTimer.targetTemp = -30
        getTemperatureTimer.isVirtual = true
        enablePreview(true)
        miniPreviewRectangle.visible = true
        rectangleLeft.visible = true
    }
}

function atikCameraTemperature() {
    var cannotGetTemperature = captureService.getCameraParam("AtikTemperatureSensorZero")
    if (cannotGetTemperature)
        getTemperatureTimer.isVirtual2 = true
    if (getTemperatureTimer.isVirtual2) {
        // 240 seconds = 4 minutes
        if (getPowerOnTickTimer.tick >= 240)
            cameraTemperature = -30
        else
            cameraTemperature = 10 - (getPowerOnTickTimer.tick / 6)
        getTemperatureTimer.currentTemp = cameraTemperature
        return true
    } else {
        return false
    }
}
