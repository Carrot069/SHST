import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
    id: root
    property var params
    property alias comboBoxCapturePlan: comboBoxCapturePlan
    property string planName: "chemi"

    // private
    property int adjustHardwareStep
    property var planNames: ["chemi", "rna", "protein", "fluor"]
    property bool noSaveIndex: true
    property bool noPreview: true // 软件启动后切换到特定方案之后也不自动预览, 为了让相机降温之后获取温度, 一旦开始预览就无法获取温度
    property string latestLightType: "red"
    property bool noSwitchLight: false

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

    ComboBox {
        id: comboBoxCapturePlan
        anchors.fill: parent
        popup.y: root.height

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
            if (adminParams === undefined)
                adminParams = dbService.readAdminSetting()
            changeCapturePlan(getCurrentParams())
            dbService.saveIntOption("capture_plan", currentIndex)
            chemiHasWhiteImage = false
        }

        Component.onCompleted: {
            var i = dbService.readIntOption("capture_plan", 0)
            console.debug("capture_plan, index: ", i)
            currentIndex = i
            enabled = false
            firstSelectPlanTimer.running = true
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
        }
    }

    function getCurrentParams() {
        switch(comboBoxCapturePlan.currentIndex) {
            case 0: return adminParams.chemi
            case 1: return adminParams.rna
            case 2: return adminParams.protein
            case 3:
                if (latestLightType === "red")
                    return adminParams.red
                else if (latestLightType === "green")
                    return adminParams.green
                else if (latestLightType === "blue")
                    return adminParams.blue
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
                if (params.filter !== mcu.filter) {
                    mcu.switchFilterWheel(params.filter)
                    fluorescence.setActiveFilter(params.filter)
                    adjustHardwareTimer.interval = 2000
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
        } else {
            exposureTime.exposureMs = params.exposureMs
            if (!noPreview)
                switchPreview.checked = true
        }
        exposureTime.noSaveExposureMs = false
        captureService.setCameraParam("Binning", buttonBinning.binning)
        if (planName === "chemi")
            captureService.previewExposureMs = previewExposure.exposureTime.value
        else
            captureService.previewExposureMs = exposureTime.exposureMs
        captureService.resetPreview()

        adjustHardwareTimer.interval = 500
        adjustHardwareTimer.running = true
    }

    function switchFluorescence(lightType) {
        noSwitchLight = true
        latestLightType = lightType
        if (planName === "fluor")
            comboBoxCapturePlan.onCurrentIndexChanged()
        else
            comboBoxCapturePlan.currentIndex = 3
        noSwitchLight = false
    }

    function switchLight(lightType) {
        noSwitchLight = true
        latestLightType = lightType
        if (lightType === "white_down")
            comboBoxCapturePlan.currentIndex = 2
        else if (lightType === "uv_penetrate" || lightType === "uv_penetrate_force")
            comboBoxCapturePlan.currentIndex = 1
        noSwitchLight = false
    }

    function setBinning(binning) {
        var binningIndex = binning - 1
        switch(comboBoxCapturePlan.currentIndex) {
            case 0:
                adminParams.chemi.binning = binningIndex
                break
            case 1:
                adminParams.rna.binning = binningIndex
                break
            case 2:
                adminParams.protein.binning = binningIndex
                break
            case 3:
                if (latestLightType === "red")
                    adminParams.red.binning = binningIndex
                else if (latestLightType === "green")
                    adminParams.green.binning = binningIndex
                else if (latestLightType === "blue")
                    adminParams.blue.binning = binningIndex
                else
                    adminParams.red.binning = binningIndex
                break
        }
    }
}
