import QtQuick 2.12
import QtQuick.Controls 2.12

import WzUtils 1.0

Item {
    id: root

    signal ready()
    signal responseTimeout(string timeoutCmd)

    property string mainFirmwareVersion: ""
    property date mainFirmwareDate
    property string lensFirmwareVersion: ""
    property date lensFirmwareDate

    Connections {
        target: mcu

        onOpened: {
            func.queryMcuInfo()
        }

        onResponse: {
            // main firmware version
            if (response.startsWith("data_received,5a,a5,0,5,a,e3")) {
                func.unsetResponseTimeout()
                var stringList = response.split(",")
                if (stringList.length === 12) {
                    var versionNumbers = [0, 0, 0]
                    versionNumbers[0] = parseInt(stringList[7], 16)
                    versionNumbers[1] = parseInt(stringList[8], 16)
                    versionNumbers[2] = parseInt(stringList[9], 16)
                    root.mainFirmwareVersion =
                            versionNumbers[0].toString() + "." +
                            versionNumbers[1].toString() + "." +
                            versionNumbers[2].toString()
                    root.mainFirmwareDate = new Date(versionNumbers[0] + 2000,
                                                     versionNumbers[1] - 1,
                                                     versionNumbers[2])
                    console.info("[mcu]mainFirmwareVersion:", root.mainFirmwareVersion)
                    console.info("[mcu]mainFirmwareDate:", root.mainFirmwareDate)
                }
                if (func.isQueryLens()) {
                    prop.retryCount = 3
                    func.queryLensInfo()
                }
                else
                    root.ready()
            }
            // lens firmware version
            else if (response.startsWith("data_received,5a,a5,0,5,1,e3")) {
                func.unsetResponseTimeout()
                var lensResponseStringList = response.split(",")
                if (lensResponseStringList.length === 12) {
                    var lensFirmwareVersionNumbers = [0, 0, 0]
                    lensFirmwareVersionNumbers[0] = parseInt(lensResponseStringList[7], 16)
                    lensFirmwareVersionNumbers[1] = parseInt(lensResponseStringList[8], 16)
                    lensFirmwareVersionNumbers[2] = parseInt(lensResponseStringList[9], 16)
                    root.lensFirmwareVersion =
                            lensFirmwareVersionNumbers[0].toString() + "." +
                            lensFirmwareVersionNumbers[1].toString() + "." +
                            lensFirmwareVersionNumbers[2].toString()
                    root.lensFirmwareDate = new Date(lensFirmwareVersionNumbers[0] + 2000,
                                                     lensFirmwareVersionNumbers[1] - 1,
                                                     lensFirmwareVersionNumbers[2])
                    console.info("[mcu]lensFirmwareVersion:", root.lensFirmwareVersion)
                    console.info("[mcu]lensFirmwareDate:", root.lensFirmwareDate)
                }
                root.ready()
                execCommand("lens,info")
            }
        }
    }

    Component.onCompleted: {
        if (WzUtils.isDemo()) {
            root.ready()
        }
    }

    Connections {
        target: window
        onDebugCommand: {
            if (cmd === "lens,info")
                debugCommandResponse("lens,info," + root.lensFirmwareVersion)
            else if (cmd === "main,info")
                debugCommandResponse("main,info," + root.mainFirmwareVersion)
        }
    }

    Timer {
        id: timerResponseTimeout
        repeat: false
        running: false
        onTriggered: {
            if (prop.activeCmd === "query_main_info")
                if (prop.connectMainCount > 0) {
                    prop.connectMainCount = prop.connectMainCount - 1
                    // Windows 待机后被唤醒, 主电路板只能收到消息但无法向电脑发送消息
                    // 此时需要发送一个指令让主板断开usb连接重连, 然后软件再重新打开连接
                    func.reconnectMcu()
                } else {
                    root.responseTimeout(prop.activeCmd)
                }
            else if (prop.activeCmd === "query_lens_info") {
                if (prop.retryCount > 0) {
                    prop.retryCount = prop.retryCount - 1
                    console.info("[mcu]queryLensInfo timeout")
                    func.queryLensInfo()
                } else {
                    // TODO 需要在更上层的业务逻辑中显示出这个错误
                    root.responseTimeout(prop.activeCmd)
                    // 进行到这一步主板已就绪, 所以通知上层业务逻辑可进行一部分电路板操作, 比如开关白光反射
                    root.ready()
                }
            }
        }
    }

    Timer {
        id: timerReconnectMcu
        repeat: true
        interval: 500
        property string step: ""
        onTriggered: {
            console.info("[mcu]timerReconnectMcu, step:", step)
            if (step == "reconnect_usb") {
                step = "mcu_uninit"
                execCommand("mcu,send_data," + mcu.getOpenId() + ",5a,a5,0,2,a,a1,aa,bb")
            } else if (step === "mcu_uninit") {
                step = "mcu_init"
                mcu.uninit()
            } else if (step === "mcu_init") {
                mcu.init()
                step = ""
                timerReconnectMcu.stop()
            } else {
                timerReconnectMcu.stop()
            }
        }
    }

    QtObject {
        id: prop
        property string activeCmd: ""
        property int connectMainCount: 3
        property int retryCount: 0
    }

    QtObject {
        id: func

        function queryMcuInfo() {
            console.info("[mcu]queryMcuInfo")
            queryMainInfo()
        }

        function isQueryLens() {
            return !WzUtils.isMini()
        }

        function queryLensInfo() {
            prop.activeCmd = "query_lens_info"
            setResponseTimeout(300)
            execCommand("mcu,send_data," + mcu.getOpenId() + ",5a,a5,0,2,1,e3,aa,bb")
        }

        function queryMainInfo() {
            prop.activeCmd = "query_main_info"
            execCommand("mcu,send_data," + mcu.getOpenId() + ",5a,a5,0,2,a,e3,aa,bb")
            setResponseTimeout(1000)
        }

        function setResponseTimeout(ms) {
            timerResponseTimeout.stop()
            timerResponseTimeout.interval = ms
            timerResponseTimeout.start()
        }

        function unsetResponseTimeout() {
            timerResponseTimeout.stop()
        }

        function reconnectMcu() {
            console.info("[mcu]reconnectMcu")
            timerReconnectMcu.step = "reconnect_usb"
            timerReconnectMcu.start()
        }
    }
}
