import QtQuick 2.12
import QtQuick.Controls 2.12

import WzI18N 1.0
import WzUtils 1.0
import WzEnum 1.0

import "../WzControls"

Item {
    id: root
    property var params
    property alias comboBoxCapturePlan: comboBoxCapturePlan
    property alias firstSelectPlanTimer: firstSelectPlanTimer
    property string planName: "chemi"
    // 权宜之计, 正常情况应该从高级后台读取
    property var uvReflex1Params
    property var uvReflex2Params
    function init() {func.init()}
    function isPlan(planIndex) {
        return planName === planNames[planIndex % 4]
    }

    // private
    property int adjustHardwareStep
    property var planNames: ["chemi", "rna", "protein", "fluor"]
    property bool noSaveIndex: true
    property bool noPreview: true // 软件启动后切换到特定方案之后也不自动预览, 为了让相机降温之后获取温度, 一旦开始预览就无法获取温度
    property string latestLightTypeForFluor: "red"
    property bool noSwitchLight: false
    property bool noClearMarker: false

    Timer {
        id: firstSelectPlanTimer
        running: false
        repeat: false
        interval: 1500
        onTriggered: {
            console.debug("firstSelectPlanTimer.onTriggered")
            noSaveIndex = false
            comboBoxCapturePlan.onCurrentIndexChanged()
            noPreview = false
        }
    }

    WzComboBox {
        id: comboBoxCapturePlan
        anchors.fill: parent
        popup.y: root.height
        font.family: WzI18N.font.family

        model: [qsTr("化学发光"), qsTr("核酸胶"), qsTr("蛋白胶"), qsTr("荧光")]

        background: Rectangle {
            implicitWidth: 240
            implicitHeight: 40
            border.color: "#222222"
            border.width: 1
            color: comboBoxCapturePlan.hovered ? "#202020" : "#090909"
            radius: 2
        }

        onCurrentIndexChanged: {
            if (noSaveIndex)
                return
            root.planName = planNames[currentIndex]
            console.debug("onCurrentIndexChanged, planName:", root.planName)
            enabled = false
            var currentParams = getCurrentParams()
            console.info("currentParams:", JSON.stringify(currentParams))
            changeCapturePlan(currentParams)
            dbService.saveIntOption("capture_plan", currentIndex)
            if (noClearMarker) {
            } else {
                captureService.clearMarkerImage()
            }
        }
    }

    function getLightNameFromIndex(index) {
        // "无", "白光反射", "紫外反射", "蓝白光透射", "紫外透射", "红色荧光", "绿色荧光", "蓝色荧光"
        switch(index) {
        case 0:
            return ""
        case 1:
            return "white_up"
        case 2:
            return ""
        case 3:
            return "white_down"
        case 4:
            return "uv_penetrate"
        case 5:
            return "red"
        case 6:
            return "green"
        case 7:
            return "blue"
        default:
            return WzUtils.lightTypeStr(index)
        }
    }

    function getCurrentParams() {
        switch(comboBoxCapturePlan.currentIndex) {
            case 0: return adminParams.chemi
            case 1: return adminParams.rna
            case 2: return adminParams.protein
            case 3:
                if (latestLightTypeForFluor === "red")
                    return adminParams.red
                else if (latestLightTypeForFluor === "green")
                    return adminParams.green
                else if (latestLightTypeForFluor === "blue")
                    return adminParams.blue
                else if (latestLightTypeForFluor === "uv_reflex1")
                    return uvReflex1Params
                else if (latestLightTypeForFluor === "uv_reflex2")
                    return uvReflex2Params
                else
                    return adminParams.red
        }
    }

    Timer {
        id: adjustHardwareTimer
        running: false
        repeat: false
        onTriggered: {
            switch(adjustHardwareStep) {
            case 0: // 调整完光源
                adjustHardwareStep = 1
                // 2020-09-15 注释了这行代码, 原因是部分仪器会出现滤镜轮乱转的问题
                // 注释此行代码后切换模式之后会多一次调整滤镜轮的机会, 可能会纠正在错误
                // 位置上的滤镜轮
                //if (params.filter !== mcu.filter) {
                if (true) {
                    mcu.switchFilterWheel(params.filter)
                    fluorescence.setActiveFilter(params.filter)
                    adjustHardwareTimer.interval = 4000
                    adjustHardwareTimer.running = true
                } else {
                    adjustHardwareTimer.interval = 1
                    adjustHardwareTimer.running = true
                }
                break
            case 1: // 调整完滤镜轮
                adjustHardwareStep = 2
                if (params.aperture !== mcu.aperture) {
                    mcu.switchAperture(params.aperture)
                    apertureSlider.setApertureIndex(params.aperture)
                }
                adjustHardwareTimer.interval = 1000
                adjustHardwareTimer.running = true
                break
            case 2:
                comboBoxCapturePlan.enabled = true
            }
        }
    }

    function changeCapturePlan(params) {
        console.debug("changeCapturePlan, params: ", JSON.stringify(params))
        root.params = params
        adjustHardwareStep = 0
        if (planName === "fluor") {
            if (!noSwitchLight) {
                mcu.switchLight(getLightNameFromIndex(params.light), true)
            }
        } else {
            if (params.light === 0)
                mcu.closeAllLight()
            else
                mcu.switchLight(getLightNameFromIndex(params.light), true)
        }
        buttonBinning.binning = params.binning + 1
        exposureTime.noSaveExposureMs = true
        if (planName === "chemi") {
            previewExposure.exposureTime.value = params.previewExposureMs
            exposureTime.exposureMs = params.exposureMs
            if (!switchPreview.checked && !noPreview)
                switchPreview.checked = true
        } else if (planName === "fluor") {
            previewExposure.exposureTime.value = adminParams.fluorPreviewExposureMs
            exposureTime.exposureMs = params.exposureMs
            if (!switchPreview.checked && !noPreview)
                switchPreview.checked = true
        } else {
            exposureTime.exposureMs = params.exposureMs
            if (!noPreview)
                switchPreview.checked = true
        }
        exposureTime.noSaveExposureMs = false
        captureService.setCameraParam("Binning", buttonBinning.binning)
        if (planName === "chemi" || planName === "fluor")
            captureService.previewExposureMs = previewExposure.exposureTime.value
        else
            captureService.previewExposureMs = exposureTime.exposureMs
        captureService.resetPreview()

        adjustHardwareTimer.interval = 500
        adjustHardwareTimer.running = true
    }

    function switchFluorescence(lightType) {
        noSwitchLight = true
        latestLightTypeForFluor = lightType
        if (planName === "fluor")
            comboBoxCapturePlan.onCurrentIndexChanged()
        else
            comboBoxCapturePlan.currentIndex = 3
        noSwitchLight = false
    }

    function switchLight(lightType) {
        noSwitchLight = true        
        if (lightType === "white_down")
            comboBoxCapturePlan.currentIndex = 2
        else if (lightType === "uv_penetrate" || lightType === "uv_penetrate_force" ||
                 lightType === "blue_penetrate")
            comboBoxCapturePlan.currentIndex = 1
        if (comboBoxCapturePlan.currentIndex === WzEnum.CaptureMode_Fluor)
            latestLightTypeForFluor = lightType
        noSwitchLight = false
    }

    function setBinning(binning) {
        var binningIndex = binning - 1
        switch(comboBoxCapturePlan.currentIndex) {
            case 0:
                adminParams.chemi.binning = binningIndex
                dbService.saveValueToDisk("chemi/binning", adminParams.chemi.binning)
                break
            case 1:
                adminParams.rna.binning = binningIndex
                dbService.saveValueToDisk("rna/binning", adminParams.rna.binning)
                break
            case 2:
                adminParams.protein.binning = binningIndex
                dbService.saveValueToDisk("protein/binning", adminParams.protein.binning)
                break
            case 3:
                if (latestLightTypeForFluor === "red")
                {
                    adminParams.red.binning = binningIndex
                    dbService.saveValueToDisk("red/binning", adminParams.red.binning)
                }
                else if (latestLightTypeForFluor === "green")
                {
                    adminParams.green.binning = binningIndex
                    dbService.saveValueToDisk("green/binning", adminParams.green.binning)
                }
                else if (latestLightTypeForFluor === "blue")
                {
                    adminParams.blue.binning = binningIndex
                    dbService.saveValueToDisk("blue/binning", adminParams.blue.binning)
                }
                else if (latestLightTypeForFluor === "uv_reflex1")
                    uvReflex1Params.binning = binningIndex
                else if (latestLightTypeForFluor === "uv_reflex2")
                    uvReflex2Params.binning = binningIndex
                else
                {
                    adminParams.red.binning = binningIndex
                    dbService.saveValueToDisk("red/binning", adminParams.red.binning)
                }
                break
        }
    }

    Component.onCompleted: {
        uvReflex1Params = {
            light: WzEnum.Light_UvReflex1,
            filter: 1,
            aperture: 6,
            binning: 3,
            exposureMs: 10
        }
        uvReflex2Params = uvReflex1Params
        uvReflex2Params.light = WzEnum.Light_UvReflex2
    }

    QtObject {
        id: func
        function init() {
            firstSelectPlanTimer.start()
        }
    }
}
