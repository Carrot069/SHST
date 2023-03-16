function cancelCapture() {
    if (isWaitingMarkerImage()) {
        abortWaitingMarkerImage()
        return
    }

    captureService.abortCapture()
    captureProgress.isProgress = false
    captureProgress.text = qsTr("正在取消")
    captureProgress.buttonCancel2.visible = false
}

function hideCaptureProgress() {
    captureProgress.visible = false
    rectShade.opacity = 0
}

function showCaptureProgress() {
    rectShade.color = "#262626"
    rectShade.opacity = 0.7
    captureProgress.visible = true
}

function showCaptureProgressTextWithoutPercent(text) {
    captureProgress.buttonCancel2.visible = true
    captureProgress.isProgress = false
    captureProgress.text = text
    showCaptureProgress()
}

function isWaitingMarkerImage() {
    return waitMarkerImageTimer.running
}

function waitMarkerImage() {
    showCaptureProgressTextWithoutPercent(qsTr("正在保存Marker图"))
    waitMarkerImageTimer.running = true
}

function waitMarkerImageTimeout() {
    hideCaptureProgress()
    msgBox.show(qsTr("保存Marker超时"), qsTr("确定"))
}

function abortWaitingMarkerImage() {
    waitMarkerImageTimer.running = false
    hideCaptureProgress()
}

function getMarkerImageSuccess() {
    waitMarkerImageTimer.running = false
}

function previewExposureIsVisible() {
    return switchPreview.checked && capturePlanIsChemiFluor()
}

function capturePlanIsChemiFluor() {
    return capturePlan.planName === "chemi" || capturePlan.planName === "fluor"
}

function capturePlanIsChemi() {
    return capturePlan.planName === "chemi"
}

function capturePlanIsFluor() {
    return capturePlan.planName === "fluor"
}
