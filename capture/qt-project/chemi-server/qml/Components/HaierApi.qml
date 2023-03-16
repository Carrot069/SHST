/************************************************************
  运行顺序：
  1 获取Token
  1.1 获取成功-跳转到 <2><4><5>
  1.2 获取失败-开启定时器继续尝试获取 <暂未实现>
  2 向平台添加设备 - saveDevice, saveDeviceHandler
  2.1 添加成功
  2.2 添加失败-跳转到 <3>
  3 向平台更新设备 - updateDevice
  3.1 更新成功
  3.2 更新失败
  4. 启动发送心跳的定时器
  4.1 当前没有正在发送的请求, 发送心跳信息
  4.2 当前有正在发送的请求, 暂缓发送, 等待下一次定时器事件触发再发送
  5. 启动周期性数据发送定时器
  5.1 发送成功 - 不输出到日志
  5.2 发送失败 - 输出到日志
  6 有实时状态需要发送到平台
************************************************************/

import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Window 2.12
import QtQuick.Layouts 1.12
import QtQuick.Controls.Material 2.12

import WzCapture 1.0

Item {
    id: root

    // TODO 1、需要增加连接超时的处理能力


    property bool isConnected: false
    property bool isConnecting: false

    WzHaierConf {
        id: haierConf
    }

    // step 1
    function connect() {
        if (haierConf.isExists()) {
            if (haierConf.readConfig()) {
                privateObject.clientId = haierConf.clientId
                privateObject.clientSecret = haierConf.clientSecret
                privateObject.username = haierConf.username
                privateObject.password = haierConf.password
                privateObject.devCode = haierConf.devCode
                privateObject.devName = haierConf.devName
                privateObject.devModel = haierConf.devModel
                privateObject.office = haierConf.office
                privateObject.serverUrl = haierConf.serverUrl
                privateObject.tokenUrl = haierConf.tokenUrl
                privateObject.heartUrl = haierConf.heartUrl
                privateObject.saveDeviceUrl = haierConf.saveDeviceUrl
                privateObject.updateDeviceUrl = haierConf.updateDeviceUrl
                privateObject.saveDeviceDataUrl = haierConf.saveDeviceDataUrl
                privateObject.getToken()
            }
        }
    }

    QtObject {
        id: privateObject
        property var tokenObject
        property int doorOpened: 0
        property string clientId
        property string clientSecret
        property string username
        property string password
        property string devCode
        property string devName
        property string devModel
        property string office
        property string serverUrl
        property string tokenUrl
        property string heartUrl
        property string saveDeviceUrl
        property string updateDeviceUrl
        property string saveDeviceDataUrl
        property string currentRequest
        property bool isLogInfoEnabled: true
        property bool isLogDebugEnabled: false

        property var previousDeviceStatus

        function getToken() {
            isConnected = false
            isConnecting = true
            var headers = {
                "client-id": clientId,
                "client-secret": clientSecret
            }
            var params = {
                username: username,
                password: password
            }
            currentRequest = "getToken"
            httpPost(privateObject.tokenUrl, headers, params)
        }
        function tokenHandler(jsonObject) {
            if (!checkObjectNull(jsonObject, "jsonObject")) return
            if (!checkObjectNull(jsonObject.access_token, "jsonObject.access_token")) return
            if (!checkObjectNull(jsonObject.token_type, "jsonObject.token_type")) return

            tokenObject = jsonObject
            isConnected = true
            isConnecting = false

            logInfo("获取授权成功")

            saveDevice() // step 2
            startHeart() // step 4
            startSendingDeviceStatus() // step 5
        }

        // step 2
        // 添加设备
        function saveDevice() {
            var headers = {
                Authorization: tokenObject.token_type + " " + tokenObject.access_token
            }
            var params = {
                devCode: devCode,
                devName: devName,
                devModel: devModel,
                office: office
            }
            logInfo("saveDevice")
            currentRequest = "saveDevice"
            httpPost(saveDeviceUrl, headers, params)
        }
        function saveDeviceHandler(jsonObject) {
            if (!checkObjectNull(jsonObject, "jsonObject")) return
            if (!checkObjectNull(jsonObject.status, "jsonObject.status")) return
            // 保存失败, 可能是已经添加过, 所以调用更新设备的api
            if (jsonObject.status === 1) {
                logInfo("添加设备失败:" + jsonObject.message)
                updateDevice()
            } else if (jsonObject.status === 0) {
                logInfo("添加设备成功")
            } else {
                logInfo("添加设备时返回了未知状态:" +
                        jsonObject.status + ", " +
                        jsonObject.message)
            }
        }

        // 更新设备
        function updateDevice() {
            logInfo("updateDevice")
            var headers = {
                Authorization: tokenObject.token_type + " " + tokenObject.access_token
            }
            var params = {
                devCode: devCode,
                devName: devName,
                devModel: devModel,
                office: office
            }
            currentRequest = "updateDevice"
            httpPost(updateDeviceUrl, headers, params)
        }
        function updateDeviceHandler(jsonObject) {
            if (!checkObjectNull(jsonObject, "jsonObject")) return
            if (!checkObjectNull(jsonObject.status, "jsonObject.status")) return
            if (jsonObject.status === 0)
                logInfo("更新设备成功")
            else
                logInfo("更新设备失败:" + jsonObject.message)
        }

        // step 4
        // 开始定时发送心跳
        function startHeart() {
            heartTimer.running = true
        }

        // step 4.1
        // 发送心跳
        function sendHeart() {
            if (currentRequest !== "")
                return
            var headers = {
                Authorization: tokenObject.token_type + " " + tokenObject.access_token
            }
            var params = {
                devCode: devCode
            }
            currentRequest = "heart"
            logInfo("发送心跳信息")
            httpPost(heartUrl, headers, params)
        }
        function heartHandler(jsonObject) {
            if (!checkObjectNull(jsonObject, "jsonObject")) return
            if (!checkObjectNull(jsonObject.status, "jsonObject.status")) return
            if (jsonObject.status === 0)
                logInfo("发送心跳成功")
            else
                logInfo("发送心跳失败:" + jsonObject.message)
        }

        // step 5
        function startSendingDeviceStatus() {
            sendDeviceStatusTimer.running = true
        }
        // step 5.1
        function sendDeviceStatus() {
            var headers = {
                Authorization: tokenObject.token_type + " " + tokenObject.access_token
            }
            var newDeviceStatus = getDeviceStatus()
            var isHasNewStatus = false
            if (previousDeviceStatus !== undefined) {
                for(var key in newDeviceStatus) {
                    if (newDeviceStatus[key] !== previousDeviceStatus[key]) {
                        isHasNewStatus = true
                    }
                }
            } else {
                isHasNewStatus = true
            }
            if (isHasNewStatus)
                previousDeviceStatus = newDeviceStatus
            else
                return
            currentRequest = "saveDeviceData"
            logInfo("发送周期性设备状态")
            logDebug(JSON.stringify(newDeviceStatus))
            httpPost(saveDeviceDataUrl, headers, {},
                     "data=" + encodeURIComponent(JSON.stringify(newDeviceStatus)))
        }
        function sendDeviceStatusHandler(jsonObject) {
            if (!checkObjectNull(jsonObject, "jsonObject")) return
            if (!checkObjectNull(jsonObject.status, "jsonObject.status")) return
            if (jsonObject.status !== 0)
                logInfo("发送周期性设备状态失败:" + jsonObject.message)
            else
                logInfo("发送周期性设备状态成功")
        }

        function httpPost(url, headers, params, bodyData) {
            var xmlHttpRequest = new XMLHttpRequest();

            xmlHttpRequest.onreadystatechange = function() {
                var stateText = ["UNSENT", "OPENED", "HEADERS_RECEIVED", "LOADING", "DONE"]
                logInfo("===readyState===\n" + stateText[xmlHttpRequest.readyState])

                if (xmlHttpRequest.readyState === XMLHttpRequest.HEADERS_RECEIVED) {

                    logInfo("===HEADERS_RECEIVED\n");
                    logDebug(xmlHttpRequest.getAllResponseHeaders());

                } else if (xmlHttpRequest.readyState === XMLHttpRequest.DONE) {

                    logDebug(xmlHttpRequest.responseText)

                    var responseJsonObject = JSON.parse(xmlHttpRequest.responseText)
                    if (responseJsonObject.resp_code === 400) {
                        logInfo("HTTP 400")
                        logDebug(xmlHttpRequest.responseText)
                        return                        
                    }

                    var currentReq = currentRequest
                    currentRequest = ""

                    if (currentReq === "getToken")
                        tokenHandler(responseJsonObject)
                    else if (currentReq === "saveDevice")
                        saveDeviceHandler(responseJsonObject)
                    else if (currentReq === "updateDevice")
                        updateDeviceHandler(responseJsonObject)
                    else if (currentReq === "heart")
                        heartHandler(responseJsonObject)
                    else if (currentReq === "saveDeviceData")
                        sendDeviceStatusHandler(responseJsonObject)
                }
            }

            var urlEncodedData = ""
            var urlEncodedDataPairs = []

            // 将数据对象转换为URL编码的键/值对数组。
            for(var name in params) {
                urlEncodedDataPairs.push(encodeURIComponent(name) + '=' + encodeURIComponent(params[name]))
            }
            urlEncodedData = urlEncodedDataPairs.join('&').replace(/%20/g, '+')
            if (urlEncodedData !== "")
                urlEncodedData = "?" + urlEncodedData

            var fullUrl = privateObject.serverUrl + url + urlEncodedData
            logDebug("===Send Request===\n " + fullUrl + "\n " + urlEncodedData)
            xmlHttpRequest.open("POST", fullUrl)
            for(var headerName in headers)
                xmlHttpRequest.setRequestHeader(headerName, headers[headerName])
            if (bodyData === undefined)
                xmlHttpRequest.send()
            else {
                xmlHttpRequest.setRequestHeader("Content-Type", "application/x-www-form-urlencoded")
                xmlHttpRequest.send(bodyData)
            }
        }

        function logDebug(log) {
            if (!isLogDebugEnabled)
                return
            console.debug("[Haier]", log)
        }
        function logInfo(log) {
            if (!isLogInfoEnabled)
                return
            console.info("[Haier]", log)
        }

        function checkObjectNull(obj, objName) {
            if (obj === undefined) {
                logInfo(objName + " is undefined")
                return false
            }
            if (obj === "") {
                logInfo(objName + " is empty")
                return false
            }
            return true
        }

        function getDeviceStatus() {
            var deviceStatus = {
                devCode: devCode,
                door: doorOpened,
                temp: pageCapture.cameraTemperature,
                uvTrans: "uv_penetrate" === pageCapture.mcu.latestLightType && pageCapture.mcu.latestLightOpened ? 1 : 0,
                uvlight_state: 0,
                whiteDown: "white_down" === pageCapture.mcu.latestLightType && pageCapture.mcu.latestLightOpened ? 1 : 0,
                whiteUp: "white_up" === pageCapture.mcu.latestLightType && pageCapture.mcu.latestLightOpened ? 1 :0 ,
                fluorescentRed: "red" === pageCapture.mcu.latestLightType && pageCapture.mcu.latestLightOpened ? 1 : 0,
                fluorescentGreen: "green" === pageCapture.mcu.latestLightType && pageCapture.mcu.latestLightOpened ? 1 : 0,
                fluorescentBlue: "blue" === pageCapture.mcu.latestLightType && pageCapture.mcu.latestLightOpened ? 1 : 0
            }

            logDebug("pageCapture.captureService.cameraState:" + pageCapture.captureService.cameraState)
            switch (pageCapture.captureService.cameraState) {
            case WzCameraState.Connecting:
                deviceStatus.workingState = "init"
                deviceStatus.workingStateText = qsTr("正在初始化")
                break
            case WzCameraState.Connected:
            case WzCameraState.CaptureFinished:
            case WzCameraState.PreviewStopped:
            case WzCameraState.CaptureAborted:
                deviceStatus.workingState = "idle"
                deviceStatus.workingStateText = qsTr("空闲")
                break
            case WzCameraState.Disconnected:
            case WzCameraState.CameraNotFound:
            case WzCameraState.Error:
                deviceStatus.workingState = "error"
                deviceStatus.workingStateText = qsTr("异常")
                break
            case WzCameraState.PreviewStarted:
                deviceStatus.workingState = "preview"
                deviceStatus.workingStateText = qsTr("正在预览")
                break
            case WzCameraState.AutoExposure:
            case WzCameraState.CaptureInit:
            case WzCameraState.Exposure:
            case WzCameraState.Image:
                deviceStatus.workingState = "capture"
                deviceStatus.workingStateText = qsTr("正在拍摄")
                break
            default:
                break
            }

            if (deviceStatus.workingState === "capture") {
                deviceStatus.exposureMSec = pageCapture.captureService.exposureMs
                deviceStatus.exposurePercent = pageCapture.captureService.exposurePercent
                deviceStatus.captureCount = pageCapture.captureService.captureCount
                deviceStatus.captureIndex = pageCapture.captureService.captureIndex
                deviceStatus.capturedCount = pageCapture.captureService.capturedCount
                deviceStatus.leftExposureMSec = pageCapture.captureService.leftExposureMs
                deviceStatus.elapsedExposureMSec = pageCapture.captureService.elapsedExposureMs
            }

            return deviceStatus
        }
    }

    Timer {
        id: heartTimer
        running: false
        interval: 1000 * 60
        repeat: true
        onTriggered: {
            privateObject.sendHeart()
        }
    }

    // 定时发送仪器的某些状态, 如相机温度
    Timer {
        id: sendDeviceStatusTimer
        running: false
        interval: 1000
        repeat: true
        onTriggered: {
            privateObject.sendDeviceStatus()
        }
    }

    Connections {
        target: pageCapture.mcu
        function onLightSwitched(lightType, isOpened) {
            privateObject.sendDeviceStatus()
        }
        function onDoorOpened() {
            privateObject.doorOpened = 1
            privateObject.sendDeviceStatus()
        }
        function onDoorClosed() {
            privateObject.doorOpened = 0
            privateObject.sendDeviceStatus()
        }
    }
}
