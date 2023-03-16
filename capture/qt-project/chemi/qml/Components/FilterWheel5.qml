import QtQuick 2.12
import "../WzControls"
import "."

Item {
    id: root
    width: 124
    height: width
    opacity: enabled ? 1 : 0.4
    property alias filter1: filter1
    property alias filter2: filter2
    property alias filter3: filter3
    property alias filter4: filter4
    property alias filter5: filter5
    property var filters: [filter1, filter2, filter3, filter4, filter5]
    signal switchFilter(int filterIndex)

    function setActiveFilter(index) {
        filters[index].active = true
        switchFilter(index)
        for (var i = 0; i < filters.length; i++)
            if (i !== index)
                filters[i].active = false
    }

    function setOptions(options) {
        console.info("FilterWheel5.setOptions")
        if (options) {
            var minLength = Math.min(filters.length, options.length)
            for (var i = 0; i < minLength; i++) {
                var opt = options[i]
                if (undefined !== opt.color)
                    filters[i].border.color = opt.color
                if (undefined !== opt.wavelength)
                    filters[i].text = opt.wavelength
            }
        }
    }

    Rectangle {
        id: rectFilterWheel
        antialiasing: true
        width: 124
        height: 124
        anchors.right: parent.right
        radius: 62
        border.width: 1
        border.color: "#666666"
        opacity: 1
        color: "#111111"
    }

    Filter {
        id: filter1
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 3
        isNull: true
        active: true
        border.color: "#eeeeee"
        onClicked: {
            if (!active) {
                setActiveFilter(0)
            }
        }
    }

    Filter {
        id: filter5
        text: "470"
        isNull: false
        active: false
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.horizontalCenterOffset: -35
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 30
        border.color: "#11b9ff"
        onClicked: {
            if (!active) {
                setActiveFilter(4)
            }
        }
    }

    Filter {
        id: filter3
        text: "530"
        active: false
        isNull: false
        border.color: "green"
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.horizontalCenterOffset: 22
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 72
        onClicked: {
            if (!active) {
                setActiveFilter(2)
            }
        }
    }

    Filter {
        id: filter4
        text: "590"
        active: false
        isNull: false
        border.color: "#F4CCB0"
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.horizontalCenterOffset: -22
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 72
        onClicked: {
            if (!active) {
                setActiveFilter(3)
            }
        }
    }

    Filter {
        id: filter2
        isNull: false
        active: false
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.horizontalCenterOffset: 35
        anchors.top: rectFilterWheel.top
        anchors.topMargin: 30
        border.color: "red"
        text: "690"
        onClicked: {
            if (!active) {
                setActiveFilter(1)
            }
        }
    }
}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
