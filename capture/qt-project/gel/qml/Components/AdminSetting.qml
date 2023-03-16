import QtQuick 2.12
import QtQuick.Controls 2.12

import WzUtils 1.0

import "../WzControls"
//import "AdminSetting"

Rectangle {
    id: root
    color: "black"
    focus: true    

    signal confirm(var params)
    signal cancel()

    Item {
        id: itemPassword
        anchors.fill: parent
        visible: true

        TextField {
            id: textFieldPassword
            width: 500
            anchors.verticalCenter: parent.verticalCenter
            anchors.horizontalCenter: parent.horizontalCenter
            passwordCharacter: "*"
            echoMode: TextInput.Password
            onEditingFinished: {
                var now = new Date()
                var year = now.getYear() + 1900
                var month = now.getMonth() + 1
                var day = now.getDate()
                var hour = now.getHours()
                var minute = now.getMinutes()
                var week = now.getDay()
                if (week === 0) week = 7

                //var password = (year + 1234) + "" + (month + 5) + "" + (day + 6) + "" + (hour + 7) + "" + (minute + 8)
                var password = "numberone" + (hour * 2 + year)
                if (password === textFieldPassword.text) {
                    itemPassword.visible = false
                    content.visible = true
                    setParams(dbService.readAdminSetting())
                }
            }
        }
    }

    Rectangle {
        id: content
        visible: false
        width: 650
        height: 500
        color: "transparent"
        border.color: "gray"
        border.width: 3
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.verticalCenter: parent.verticalCenter

        TabBar {
            id: tabBar
            clip: true
            anchors.left: parent.left
            anchors.leftMargin: 3
            anchors.right: parent.right
            anchors.rightMargin: 3
            anchors.top: parent.top
            anchors.topMargin: 3
            contentHeight: 50

            background: Rectangle {
                color: "#151515"
            }

            /*
            WzTabButton {
                text: qsTr("化学发光")
                onClicked: {
                    contentView.currentIndex = tabBar.currentIndex
                }
            }
            WzTabButton {
                text: qsTr("核酸胶")
                onClicked: {
                    contentView.currentIndex = tabBar.currentIndex
                }
            }
            WzTabButton {
                text: qsTr("蛋白胶")
                onClicked: {
                    contentView.currentIndex = tabBar.currentIndex
                }
            }
            WzTabButton {
                text: qsTr("红色荧光")
                onClicked: {
                    contentView.currentIndex = tabBar.currentIndex
                }
            }
            WzTabButton {
                text: qsTr("绿色荧光")
                onClicked: {
                    contentView.currentIndex = tabBar.currentIndex
                }
            }
            WzTabButton {
                text: qsTr("蓝色荧光")
                onClicked: {
                    contentView.currentIndex = tabBar.currentIndex
                }
            }
            */
            WzTabButton {
                text: qsTr("其它选项")
                onClicked: {
                    contentView.currentIndex = tabBar.currentIndex
                }
            }
        }

        SwipeView {
            id: contentView
            clip: true
            interactive: false
            currentIndex: 0
            anchors.top: tabBar.bottom
            anchors.bottom: itemBottom.top
            anchors.left: parent.left
            anchors.right: parent.right

            /*
            CaptureParams {
                id: captureParamsChemi
                height: 120

                Text {
                    text: qsTr("预览时间")
                    anchors.verticalCenter: exposureTimePreview.verticalCenter
                    x: 20
                    color: "#bbbbbb"
                }

                WzExposureTime {
                    id: exposureTimePreview
                    x: 100
                    y: 250
                }
            }
            CaptureParams {
                id: captureParamsRNA
            }
            CaptureParams {
                id: captureParamsProtein
            }
            CaptureParams {
                id: captureParamsRed
            }
            CaptureParams {
                id: captureParamsGreen
            }
            CaptureParams {
                id: captureParamsBlue
            }
            */

            Item {
                id: itemOption1
                /*
                CheckBox {
                    id: checkBoxBinningVisible
                    text: qsTr("显示像素合并选项")
                    x: 10
                    y: 10
                    height: 30
                }

                CheckBox {
                    id: checkBoxGrayAccumulateAddExposure
                    text: qsTr("灰度累积模式拍摄后的图片曝光时间也累加")
                    x: 10
                    height: 30
                    anchors.top: checkBoxBinningVisible.bottom
                }

                CheckBox {
                    id: checkBoxRepeatSetFilter
                    text: qsTr("定时设置滤镜轮")
                    x: 10
                    height: 30
                    anchors.top: checkBoxGrayAccumulateAddExposure.bottom
                }

                WzButton {
                    id: buttonOpenDataPath
                    text: qsTr("打开数据目录")
                    x: 30
                    width: 100
                    height: 30
                    anchors.top: checkBoxRepeatSetFilter.bottom
                    anchors.topMargin: 30
                    onClicked: {
                        WzUtils.openDataPath()
                    }
                }

                WzButton {
                    id: buttonOpenDbgView
                    text: qsTr("打开日志工具")
                    x: 30
                    width: 100
                    height: 30
                    anchors.top: buttonOpenDataPath.top
                    anchors.left: buttonOpenDataPath.right
                    anchors.leftMargin: 5
                    onClicked: {
                        WzUtils.openDbgView()
                    }
                }

                CheckBox {
                    id: checkBoxPvcamSlowPreview
                    text: qsTr("Q 相机慢速预览")
                    x: 10
                    height: 30
                    anchors.top: buttonOpenDbgView.bottom
                    anchors.topMargin: 20
                }
                */

                CheckBox {
                    id: checkBoxAutoExposureVisible
                    text: qsTr("显示自动曝光选项")
                    x: 10
                    y: 10
                    height: 30
                }
            }
        }

        Item {
            id: itemBottom
            width: parent.width
            height: 50
            anchors.bottom: parent.bottom

            WzButton {
                id: buttonOK
                width: 100
                height: 40
                radius: 5
                label.font.pixelSize: 20
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.horizontalCenterOffset: -(55)
                text: qsTr("确定")
                onClicked: {
                    var params = getParams()
                    dbService.saveAdminSetting(params)
                    confirm(params)
                    root.destroy()
                }
            }

            WzButton {
                id: buttonCancel
                width: 100
                height: 40
                radius: 5
                label.font.pixelSize: 20
                anchors.left: buttonOK.right
                anchors.leftMargin: 10
                text: qsTr("取消")
                onClicked: {
                    var params = dbService.readAdminSetting()
                    setParams(params)
                    cancel()
                    root.destroy()
                }
            }
        }
    }

    Component.onCompleted: {
        textFieldPassword.forceActiveFocus()
        var params = dbService.readAdminSetting()
        setParams(params)
    }

    function getParams() {
        var params = {
            /*
            chemi: captureParamsChemi.getParams(),
            rna: captureParamsRNA.getParams(),
            protein: captureParamsProtein.getParams(),
            red: captureParamsRed.getParams(),
            green: captureParamsGreen.getParams(),
            blue: captureParamsBlue.getParams()
            */
        }
        /*
        params.chemi.previewExposureMs = exposureTimePreview.exposureMs
        params.binningVisible = checkBoxBinningVisible.checked
        params.grayAccumulateAddExposure = checkBoxGrayAccumulateAddExposure.checked
        params.repeatSetFilter = checkBoxRepeatSetFilter.checked
        params.pvcamSlowPreview = checkBoxPvcamSlowPreview.checked
        */
        params.autoExposure = checkBoxAutoExposureVisible.checked
        return params
    }

    function setParams(params) {
        /*
        captureParamsChemi.setParams(params.chemi)
        exposureTimePreview.exposureMs = params.chemi.previewExposureMs
        captureParamsRNA.setParams(params.rna)
        captureParamsProtein.setParams(params.protein)
        captureParamsRed.setParams(params.red)
        captureParamsGreen.setParams(params.green)
        captureParamsBlue.setParams(params.blue)
        checkBoxBinningVisible.checked = params.binningVisible
        checkBoxGrayAccumulateAddExposure.checked = params.grayAccumulateAddExposure
        checkBoxRepeatSetFilter.checked = params.repeatSetFilter
        checkBoxPvcamSlowPreview.checked = params.pvcamSlowPreview
        */
        checkBoxAutoExposureVisible.checked = params.autoExposure
    }
}

/*##^##
Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
##^##*/
