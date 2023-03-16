import QtQuick 2.12
import QtQuick.Controls 2.12

import WzCapture 1.0

import "../../WzControls"
import "../../Components/Mini"

SwipeView {
    id: root
    width: 300
    height: 250
    clip: true
    interactive: false

    property var params: {
        return {
            manualExposureMs: exposureTime.getExposureMs(),
            multiCaptureParams: multiCapture.params()
        }
    }

    Item {
    }

    Item {
        opacity: root.currentIndex === 1 ? 1 :0
        Behavior on opacity {NumberAnimation {duration: 500}}

        WzExposureTime {
            id: exposureTime
            anchors.horizontalCenter: parent.horizontalCenter
            anchors.horizontalCenterOffset: 2
            anchors.bottom: parent.bottom
            anchors.bottomMargin: 0
            fontColor: "white"
            fontPixelSize: 48
            onExposureMsChanged: {
                dbService.saveIntOption("miniManualExposureMs", exposureTime.getExposureMs())
            }
        }        
    }

    Item {
        opacity: root.currentIndex === 2 ? 1 :0
        Behavior on opacity {NumberAnimation {duration: 500}}
        MultiCapture {
            id: multiCapture
            anchors.fill: parent
        }
    }

    Component.onCompleted: {
        exposureTime.setExposureMs(dbService.readIntOption("miniManualExposureMs", 10000))
        var multiCaptureParams = JSON.parse(dbService.readStrOption("miniMultiCapture", "{}"))
        if (multiCaptureParams)
            multiCapture.setParams(multiCaptureParams)
    }
    Component.onDestruction: {
        dbService.saveStrOption("miniMultiCapture", JSON.stringify(multiCapture.params()))
    }

    Connections {
        target: captureService
        onCaptureStateChanged: {
            switch(captureState) {
            case WzCaptureService.AutoExposureFinished:
                exposureTime.exposureMs = captureService.autoExposureMs
                break
            }
        }
    }
}
