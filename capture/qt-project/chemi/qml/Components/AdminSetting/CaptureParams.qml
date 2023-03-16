import QtQuick 2.12
import QtQuick.Controls 2.1
import "../../WzControls"

Item {
    property alias light: comboBoxLight.currentIndex
    property alias filter: comboBoxFilter.currentIndex
    property alias aperture: comboBoxAperture.currentIndex
    property alias binning: comboBoxBinning.currentIndex
    property alias exposureMs: exposureTime.exposureMs

    function getParams() {
        var params = {
            light: comboBoxLight.currentIndex,
            filter: comboBoxFilter.currentIndex,
            aperture: comboBoxAperture.currentIndex,
            binning: comboBoxBinning.currentIndex,
            exposureMs: exposureTime.exposureMs
        }
        return params
    }

    function setParams(params) {
        comboBoxLight.currentIndex = params.light
        comboBoxFilter.currentIndex = params.filter
        comboBoxAperture.currentIndex = params.aperture
        comboBoxBinning.currentIndex = params.binning
        exposureTime.exposureMs = params.exposureMs
    }

    Text {
        text: qsTr("光源")
        x: 20
        anchors.verticalCenter: comboBoxLight.verticalCenter
        color: "#bbbbbb"
    }

    ComboBox {
        id: comboBoxLight
        x: 100
        y: 15
        width: 200
        model: [qsTr("无光源"), qsTr("白光反射"), qsTr("紫外反射"), qsTr("蓝白光透射"), qsTr("紫外透射"), qsTr("红色荧光"), qsTr("绿色荧光"), qsTr("蓝色荧光"), qsTr("蓝光透射")]
    }

    Text {
        text: qsTr("滤镜")
        x: 20
        anchors.verticalCenter: comboBoxFilter.verticalCenter
        color: "#bbbbbb"
    }

    ComboBox {
        id: comboBoxFilter
        x: 100
        anchors.top: comboBoxLight.bottom
        anchors.topMargin: 0
        width: 200
        model: ["1", "2", "3", "4", "5"]
    }

    Text {
        text: qsTr("光圈")
        anchors.verticalCenter: comboBoxAperture.verticalCenter
        x: 20
        color: "#bbbbbb"
    }

    ComboBox {
        id: comboBoxAperture
        x: 100
        anchors.top: comboBoxFilter.bottom
        width: 200
        model: ["1", "2", "3", "4", "5", "6", "7"]
    }

    Text {
        text: qsTr("像素合并")
        anchors.verticalCenter: comboBoxBinning.verticalCenter
        x: 20
        color: "#bbbbbb"
    }

    ComboBox {
        id: comboBoxBinning
        x: 100
        anchors.top: comboBoxAperture.bottom
        width: 200
        model: ["1", "2", "3", "4"]
    }

    Text {
        text: qsTr("曝光时间")
        anchors.verticalCenter: exposureTime.verticalCenter
        x: 20
        color: "#bbbbbb"
    }

    WzExposureTime {
        id: exposureTime
        x: 100
        anchors.top: comboBoxBinning.bottom
    }
}
