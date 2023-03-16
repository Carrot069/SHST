import QtQuick 2.12
import QtQuick.Controls 2.12

import WzI18N 1.0
import WzEnum 1.0

import "../WzControls"

WzMenu {
    id: laneMenu
    width: WzI18N.isZh ? 180 : 280
    indicatorWidth: 20
    font: WzI18N.font
    autoExclusive: false

    property var actionNoneObject
    function customizeItems() {
        if (!actionNoneObject)
            actionNoneObject = actionNoneComponent.createObject(laneMenu)
        if (imageView.action === WzEnum.AnalysisActionNone) {
            if (laneMenu.actionAt(0) === actionNoneObject)
                laneMenu.removeAction(actionNoneObject)
        } else {
            if (laneMenu.actionAt(0) !== actionNoneObject)
                laneMenu.insertAction(0, actionNoneObject)
        }
    }

    Component {
        id: actionNoneComponent
        Action {
            id: actionNone
            text: qsTr("取消")
            onTriggered: {
                if (imageView.action === WzEnum.CropImage)
                    imageView.cropImageCancel()
                imageView.action = WzEnum.AnalysisActionNone
            }
        }
    }
    Action {
        id: actionImageReset
        text: qsTr("还原图片")
        onTriggered: {
            imageService.resetImage()
        }
    }
    Action {
        id: actionImageCrop
        text: qsTr("裁减图片")
        onTriggered: {
            imageView.action = WzEnum.CropImage
        }
    }
    Action {
        id: actionHorzLine
        text: qsTr("按水平线调整")
        onTriggered: {
            imageView.action = WzEnum.HorizontalRotate
        }
    }
}
