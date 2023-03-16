import QtQuick 2.12
import QtQuick.Controls 2.12

import WzI18N 1.0

import "../WzControls"

WzMenu {
    id: menu
    width: 150
    indicatorWidth: 8
    font.pixelSize: 18
    font.family: WzI18N.font.family

    Action {
        text: qsTr("定位到第1层")
        onTriggered: func.focusToLayer(1)
    }
    WzMenuSeparator {width: parent.width; color: "#525252"}
    Action {
        text: qsTr("定位到第2层")
        onTriggered: func.focusToLayer(2)
    }
    WzMenuSeparator {width: parent.width; color: "#525252"}
    Action {
        text: qsTr("定位到第3层")
        onTriggered: func.focusToLayer(3)
    }
    WzMenuSeparator {width: parent.width; color: "#525252"}
    Action {
        text: qsTr("定位到第4层")
        onTriggered: func.focusToLayer(4)
    }

    QtObject {
        id: func

        function focusToLayer(layerNumber) {
            execCommand("focus,tolayer," + layerNumber.toString())
        }
    }
}
