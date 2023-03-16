import QtQuick 2.12
import QtQuick.Controls 2.12

Item {
    id: root
    width: 200
    height: 50

    signal mcuCmd(string cmd)
    signal nearest()
    signal farthest()
    signal nearDone()
    signal farDone()
    signal afDone()
    signal stop()
    signal timeout(string msg)
    signal requestImageDiff()

    property alias waitDiffMs: timerRequestImageDiff.interval

    function autoFocus() {execCmd("focus,auto")}
    function stopFocus() {execCmd("focus,stop")}
    function ensureStopFocus() {func.ensureStopFocus()}
    function execCmd(cmd) {func.execCmd(cmd)}
    function setImageDiff(diff) {func.setImageDiff(diff)}
    function setLayerPWMCount(layerPWMCount) {func.setLayerPWMCount(layerPWMCount)}

    Rectangle {
        color: "white"
        opacity: 0.8
        anchors.fill: parent
        radius: 5
    }

    Item {
        anchors {
            fill: parent
            margins: 10
        }

        Text {
            id: textImageDiff
            font.pointSize: 15
        }
    }

    Timer {
        id: timerRefresh
        interval: 500
        repeat: true
        onTriggered: {
            textImageDiff.text = captureService.getPreviewImageDiff()
        }
    }

    Timer {
        id: timerTimeout
        property string timeoutMsg
        repeat: false
        running: false
        onTriggered: {
            console.info("[focus]timeout,", interval, timeoutMsg);
            func.setNotBusy()
            func.setAfState("idle")
            root.timeout(timeoutMsg)
        }
    }

    Timer {
        id: timerRequestImageDiff
        repeat: false
        running: false
        interval: 1000
        onTriggered: {
            root.requestImageDiff()
        }
    }

    Timer {
        id: timerDelayCmd
        repeat: false
        running: false
        interval: 300
        property string cmd
        onTriggered: {
            root.execCmd(cmd)
        }
    }

    Timer {
        id: timerTestAf
        repeat: true
        running: false
        interval: 15000
        onTriggered: {
            root.execCmd("focus,auto")
        }
    }

    // 发送3次后自动停止, 或者收到单片机返回的已收到的状态指令后停止
    Timer {
        id: timerFocusStop
        property int countdown: 0
        interval: 100
        repeat: true
        running: false
        onTriggered: {
            execCommand("focus,stop")
            if (timerFocusStop.countdown <= 0)
                timerFocusStop.stop()
            else
                timerFocusStop.countdown = timerFocusStop.countdown - 1
        }
    }

    Timer {
        id: timerFocusRepeat
        repeat: true
        running: false
        onTriggered: {
            func.focus()
        }
    }

    QtObject {
        id: prop
        property bool isBusy: false
        property bool isFromNearest: false // 先转到最近, 然后再开始往远处转
        property bool isFromFarthest: false // 先转到最远, 然后再开始往近处转
        property bool isRepeatFocus: false // 是否间隔和重复发送聚焦指令
        property int focusDirection
        property int focusPWMCount: 0
        property int focusTargetSpeed: 59
        property int focusAccelerationCount: 2
        property int focusStartSpeed: 199
        property bool focusIsNotReply: false

        property int focusNearCmd: 0x63
        property int focusFarCmd: 0x64
        property int focusStopCmd: 0x67

        property string afState: "idle"
        property var layerDiff: [0, 0, 0, 0]
        property var layerAbsPWMCountFromNear: [3300, 9500, 17100, -1]
        property var layerAbsPWMCountFromFar: [20200, 14600, 7300, -1]
    }

    QtObject {
        id: func
        function setBusy() {prop.isBusy = true}
        function setNotBusy() {prop.isBusy = false}
        function setTimeout(interval, msg) {
            timerTimeout.stop()
            timerTimeout.interval = interval + root.waitDiffMs
            timerTimeout.timeoutMsg = msg
            timerTimeout.start()
        }
        function unsetTimeout() {timerTimeout.stop()}
        function setAfState(state) {prop.afState = state}
        function getAfState(){return prop.afState}
        function isAfState(state) {return getAfState() === state}
        function saveDiff(layer, diff) {prop.layerDiff[layer-1] = diff}
        function getDiff(layer) {return prop.layerDiff[layer-1]}
        function getMaxDiffOfLayer() {
            var maxDiff = 0
            var layer = -1
            console.info("[focus]diffs:", prop.layerDiff)
            for (var i = 1; i < 5; i++)
                if (maxDiff < parseInt(func.getDiff(i))) {
                    layer = i
                    maxDiff = parseInt(func.getDiff(i))
                }
            console.info("[focus]layer,", layer)
            return layer
        }

        // 延迟几秒再获取图片信息, 因为聚焦完成后画面有一定延迟
        function requestImageDiff() {
            timerRequestImageDiff.stop()
            timerRequestImageDiff.start()
        }

        function setImageDiff(diff) {
            console.info("[focus]setImageDiff,", diff)
            if (func.isAfState("waitingL1Diff")) {
                func.saveDiff(1, diff)
                func.setAfState("hadL1Diff")
                func.af()
            }
            else if (func.isAfState("waitingL2Diff")) {
                func.saveDiff(2, diff)
                func.setAfState("hadL2Diff")
                func.af()
            }
            else if (func.isAfState("waitingL3Diff")) {
                func.saveDiff(3, diff)
                func.setAfState("hadL3Diff")
                func.af()
            }
            else if (func.isAfState("waitingL4Diff")) {
                func.saveDiff(4, diff)
                func.setAfState("hadL4Diff")
                func.af()
            }
            else {
                console.info("[focus]Unknown afState")
            }
        }

        function setLayerPWMCount(layerPWMCount) {
            if (layerPWMCount && layerPWMCount.length) {
                for (var i = 0; i < 4; i++) {
                    if (i < layerPWMCount.length) {
                        prop.layerAbsPWMCountFromNear[i] = layerPWMCount[i]
                    }
                }
                for (var j = 0; j < 4; j++) {
                    var k = j + 4
                    if (k < layerPWMCount.length) {
                        prop.layerAbsPWMCountFromFar[j] = layerPWMCount[k]
                    }
                }
            }
        }

        function execCmd(cmd) {
            console.info("[focus]execCmd,", cmd)
            if (cmd.startsWith("show af")) {
                root.visible = !root.visible
                timerRefresh.running = root.visible

            } else if (cmd.startsWith("focus,")) {
                func.parseFocusParams(cmd)
            }
        }
        function execCmdDelay(cmd, delay) {
            timerDelayCmd.interval = delay
            timerDelayCmd.cmd = cmd
            timerDelayCmd.start()
        }

        function isNearDone(response) {
            return response === "data_received,5a,a5,0,3,1,63,1,aa,bb"
        }
        function isFarDone(response) {
            return response === "data_received,5a,a5,0,3,1,64,1,aa,bb"
        }
        function isNearestDone(response) {
            return response === "data_received,5a,a5,0,2,1,63,aa,bb"
        }
        function isFarthestDone(response) {
            return response === "data_received,5a,a5,0,2,1,64,aa,bb"
        }
        function isFocusStop(response) {
            return response === "data_received,5a,a5,0,2,1,67,aa,bb"
        }

        function isFromNearest(param) {
            return param === "nearest"
        }
        function isFromFarthest(param) {
            return param === "farthest"
        }
        function isToNear(param) {
            return param === "near" || isFromNearest(param)
        }
        function isToFar(param) {
            return param === "far" || isFromFarthest(param)
        }

        function invertFocusDirection() {
            prop.focusDirection = prop.focusDirection === prop.focusNearCmd ? prop.focusFarCmd : prop.focusNearCmd
        }

        // focus,auto|stop|far|farthest|near|nearest[,转动步数][,目标速度][,加速步数][,启动速度][,是否返回指令]
        function parseFocusParams(str) {
            var focusParams = str.split(",")
            if (focusParams.length < 2)
                return false

            if (focusParams[1] === "setlayer") {
                var layerPWMCount = []
                var layerPWMCountStr = ""
                for (var i = 2; i < focusParams.length; i++)
                {
                    layerPWMCount.push(parseInt(focusParams[i]));
                    if (layerPWMCountStr === "")
                        layerPWMCountStr = focusParams[i]
                    else
                        layerPWMCountStr = layerPWMCountStr + "," + focusParams[i]
                }
                adminParams.layerPWMCount = layerPWMCount
                dbService.saveValueToDisk("common/layerPWMCount", layerPWMCountStr)
                root.setLayerPWMCount(layerPWMCount)
                execCommand("adminparams,save")
                return false
            }

            if (focusParams[1] === "tolayer") {
                var layerNumber = parseInt(focusParams[2])
                execCommand("focus,nearest," + prop.layerAbsPWMCountFromNear[layerNumber-1])
                return
            }

            if (focusParams[1] === "auto") {
                if (focusParams.length === 2) {
                    func.af()
                } else {
                    if (focusParams[2] === "test") {
                        timerTestAf.running = !timerTestAf.running
                    }
                }
                return true
            }
            else if (focusParams[1] === "stop") {
                prop.focusDirection = prop.focusStopCmd
                func.focus()
                return true
            }
            else if (focusParams[1] === "far_repeat" ||
                     focusParams[1] === "near_repeat") {
                if (focusParams.length < 4)
                    return false
                if (focusParams[1] === "far_repeat")
                    focusParams[1] = "far"
                else
                    focusParams[1] = "near"
                timerFocusRepeat.interval = parseInt(focusParams[2])
                prop.isRepeatFocus = true
                for (var i = 3; i < focusParams.length; i++)
                    focusParams[i-1] = focusParams[i]
                timerFocusRepeat.start()
            }

            // 判断是否先转到最近或最远后再反向转动
            var focusDirectionStr = focusParams[1]
            prop.isFromNearest = func.isFromNearest(focusDirectionStr)
            prop.isFromFarthest = func.isFromFarthest(focusDirectionStr)

            // 解析转动方向,转动步数,目标速度,启动速度,加速步数
            prop.focusDirection = func.isToNear(focusDirectionStr) ? prop.focusNearCmd : prop.focusFarCmd
            prop.focusPWMCount = 0
            prop.focusTargetSpeed = 59
            prop.focusAccelerationCount = 2
            prop.focusStartSpeed = 199

            if (focusParams.length > 2)
                prop.focusPWMCount = parseInt(focusParams[2])

            if (focusParams.length > 3)
                prop.focusTargetSpeed = parseInt(focusParams[3])

            if (focusParams.length > 4)
                prop.focusAccelerationCount = parseInt(focusParams[4])

            if (focusParams.length > 5)
                prop.focusStartSpeed = parseInt(focusParams[5])

            if (focusParams.length > 6)
                prop.focusIsNotReply = parseInt(focusParams[6]) === 1
            else
                prop.focusIsNotReply = false

            func.focus()
            return true
        }

        function waitMcuResponse(){
            // go to parseMcuResponse
        }
        function parseMcuResponse(response) {
            // 到达最近
            if (func.isNearestDone(response)) {
                root.nearest()
                if (prop.isFromNearest) {
                    prop.isFromNearest = false
                    func.invertFocusDirection()
                    func.focus()
                } else {
                    func.setNotBusy()
                }
            }
            // 到达最远
            else if (func.isFarthestDone(response)) {
                console.info("[focus]farthestDone")
                root.farthest()
                if (prop.isFromFarthest) {
                    prop.isFromFarthest = false
                    func.invertFocusDirection()
                    func.focus()
                }
                else if (func.isAfState("waitingFocusToL4")) {
                    console.info("[focus]focusToL4Done")
                    func.setAfState("focusToL4Done")
                    func.af()
                }
                else {
                    func.setNotBusy()
                }
            }
            // 转动特定步数完成
            else if (func.isNearDone(response)) {
                console.info("[focus]nearDone")
                root.nearDone()
                func.setNotBusy()
                if (func.isAfState("finishing")) {
                    func.unsetTimeout()
                    func.setAfState("done")
                    root.afDone()
                }
            }
            else if (func.isFarDone(response)) {
                console.info("[focus]farDone")
                root.farDone()
                if (func.isAfState("waitingFocusToL1")) {
                    console.info("[focus]focusToL1Done")
                    func.setAfState("focusToL1Done")
                    func.af()
                }
                else if (func.isAfState("waitingFocusToL2")) {
                    console.info("[focus]focusToL2Done")
                    func.setAfState("focusToL2Done")
                    func.af()
                }
                else if (func.isAfState("waitingFocusToL3")) {
                    console.info("[focus]focusToL3Done")
                    func.setAfState("focusToL3Done")
                    func.af()
                }
                else {
                    func.setNotBusy()
                }
            }
            else if (func.isFocusStop(response)) {
                if (timerFocusStop.running)
                    timerFocusStop.stop()
                func.setNotBusy()
                root.stop()
            }
        }

        function focus() {
            if (prop.focusDirection == prop.focusStopCmd) {
                func.setAfState("idle")
                timerTimeout.stop()
                if (timerFocusRepeat.running)
                    timerFocusRepeat.stop()
                mcuCmd('5a,a5,00,02,01,67,aa,bb')
                func.setNotBusy()
                root.stop()
                return
            }

            var focusCmd = '5a,a5,00,a,01,' + prop.focusDirection.toString(16)

            // 如果需要先转到最近处或者最远处, 则第一次转动不需要指定步数
            if (prop.isFromNearest || prop.isFromFarthest)
                focusCmd += ",0,0,0"
            else
                focusCmd += "," + ((prop.focusPWMCount & 0xFF0000) >> 16).toString(16) +
                            "," + ((prop.focusPWMCount & 0x00FF00) >> 8).toString(16) +
                            "," + ((prop.focusPWMCount & 0x0000FF) >> 0).toString(16)

            focusCmd += "," + prop.focusTargetSpeed.toString(16)
            focusCmd += "," + prop.focusAccelerationCount.toString(16)
            focusCmd += "," + (((prop.focusStartSpeed & 0xFF00) >> 8).toString(16))
            focusCmd += "," + (((prop.focusStartSpeed & 0x00FF) >> 0).toString(16))
            focusCmd += "," + (prop.focusIsNotReply ? 1 : 0)

            focusCmd += ",aa,bb"
            setBusy()
            mcuCmd(focusCmd)
        }

        function af() {
            console.info("[focus]afState:", func.getAfState())

            if (func.isAfState("done"))
                func.setAfState("idle")

            if (func.isAfState("idle")) {
                func.setAfState("waitingFocusToL1")
                root.execCmd("focus,nearest," + prop.layerAbsPWMCountFromNear[0])
                func.setTimeout(4000, qsTr("调整聚焦超时(EAF001)")) // Error auto focus 001
                func.waitMcuResponse()
            }
            else if (func.isAfState("focusToL1Done")) {
                func.unsetTimeout()
                func.setAfState("waitingL1Diff")
                func.requestImageDiff()
                func.waitMcuResponse()
            }
            else if (func.isAfState("hadL1Diff")) {                
                func.setAfState("waitingFocusToL2")
                root.execCmd("focus,far," + (prop.layerAbsPWMCountFromNear[1] - prop.layerAbsPWMCountFromNear[0]))
                func.setTimeout(2000, qsTr("调整聚焦超时(EAF002)"))
                func.waitMcuResponse()
            }
            else if (func.isAfState("focusToL2Done")) {
                func.unsetTimeout()
                func.setAfState("waitingL2Diff")
                func.requestImageDiff()
            }
            else if (func.isAfState("hadL2Diff")) {
                if (1 === 2 && func.getDiff(1) > func.getDiff(2)) {
                    func.setAfState("finishing")
                    root.execCmd("focus,near,6000")
                    func.setTimeout(2000, qsTr("调整聚焦超时(EAF003)"))
                    func.waitMcuResponse()
                }
                else {
                    func.setAfState("waitingFocusToL3")
                    root.execCmd("focus,far," + (prop.layerAbsPWMCountFromNear[2] - prop.layerAbsPWMCountFromNear[1]))
                    func.setTimeout(3000, qsTr("调整聚焦超时(EAF004)"))
                    func.waitMcuResponse()
                }
            }
            else if (func.isAfState("focusToL3Done")) {
                func.unsetTimeout()
                func.setAfState("waitingL3Diff")
                func.requestImageDiff()
            }
            else if (func.isAfState("hadL3Diff")) {
                if (1 === 2 && func.getDiff(2) > func.getDiff(3)) {
                    func.setAfState("finishing")
                    root.execCmd("focus,near,8000")
                    func.setTimeout(3000, qsTr("调整聚焦超时(EAF005)"))
                    func.waitMcuResponse()
                }
                else {
                    func.setAfState("waitingFocusToL4")
                    root.execCmd("focus,far")
                    func.setTimeout(3000, qsTr("调整聚焦超时(EAF006)"))
                    func.waitMcuResponse()
                }
            }
            else if (func.isAfState("focusToL4Done")) {
                func.unsetTimeout()
                func.setAfState("waitingL4Diff")
                func.requestImageDiff()
            }
            else if (func.isAfState("hadL4Diff")) {
                var layer = func.getMaxDiffOfLayer()
                if (layer < 5) {
                    func.setAfState("finishing")
                    var absPWMCount = prop.layerAbsPWMCountFromFar[layer-1]
                    root.execCmd("focus,near," + absPWMCount)
                    func.setTimeout(5000, qsTr("调整聚焦超时(EAF007)"))
                    func.waitMcuResponse()
                }
                else {
                    func.unsetTimeout()
                    func.setAfState("done")
                    root.afDone()
                }
            }

            console.info("[focus]afState(New):", getAfState())
        }

        function ensureStopFocus() {
            root.stopFocus()
            timerFocusStop.countdown = 5
            timerFocusStop.start()
        }
    }

    Connections {
        target: window
        onDebugCommand: {
            execCmd(cmd)
        }
    }

    Connections {
        target: mcu
        onResponse: {
            console.info("[focus]mcu.onResponse:", response)
            func.parseMcuResponse(response)
        }
    }
}
