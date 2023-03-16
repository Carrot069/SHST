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
    property alias filter6: filter6
    property alias filter7: filter7
    property alias filter8: filter8
    property var filters: [filter1, filter2, filter3, filter4, filter5, filter6, filter7, filter8]

    signal switchFilter(int filterIndex)

    function setActiveFilter(index) {
        filters[index].active = true
        for (var i = 0; i < filters.length; i++)
            if (i !== index) {
                filters[i].active = false
            }
    }

    function setOptions(options) {
        console.info("FilterWheel8.setOptions")
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
        border.color: rectFilterWheel.border.color
        font.pixelSize: 10
        width: 30
        height: width
        onClicked: {
            if (!active) {
                setActiveFilter(0)
            }
        }
    }

    Filter {
        id: filter2
        isNull: false
        active: false
        anchors.left: filter1.right
        anchors.rightMargin: 3
        anchors.bottom: filter3.top
        border.color: "red"
        text: "690"
        font: filter1.font
        width: filter1.width
        height: filter1.height
        onClicked: {
            if (!active) {
                setActiveFilter(1)
            }
        }
    }


    Filter {
        id: filter3
        text: "530"
        active: false
        isNull: false
        border.color: "green"
        anchors.verticalCenter: rectFilterWheel.verticalCenter
        anchors.right: rectFilterWheel.right
        anchors.rightMargin: 3
        font: filter1.font
        width: filter1.width
        height: filter1.height
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
        anchors.top: filter3.bottom
        anchors.left: filter2.left
        font: filter1.font
        width: filter1.width
        height: filter1.height
        onClicked: {
            if (!active) {
                setActiveFilter(3)
            }
        }
    }


    Filter {
        id: filter5
        text: "470"
        isNull: false
        active: false
        anchors.horizontalCenter: rectFilterWheel.horizontalCenter
        anchors.bottom: rectFilterWheel.bottom
        anchors.bottomMargin: 3
        border.color: "#11b9ff"
        font: filter1.font
        width: filter1.width
        height: filter1.height
        onClicked: {
            if (!active) {
                setActiveFilter(4)
            }
        }
    }

    Filter {
        id: filter6
        isNull: true
        active: false
        anchors.right: filter5.left
        anchors.bottom: filter4.bottom
        border.color: rectFilterWheel.border.color
        font: filter1.font
        width: filter1.width
        height: filter1.height
        onClicked: {
            if (!active) {
                setActiveFilter(5)
            }
        }
    }

    Filter {
        id: filter7
        isNull: true
        active: false
        anchors.left: rectFilterWheel.left
        anchors.leftMargin: 3
        anchors.bottom: filter3.bottom
        border.color: rectFilterWheel.border.color
        font: filter1.font
        width: filter1.width
        height: filter1.height
        onClicked: {
            if (!active) {
                setActiveFilter(6)
            }
        }
    }

    Filter {
        id: filter8
        isNull: true
        active: false
        anchors.right: filter1.left
        anchors.rightMargin: 0
        anchors.bottom: filter2.bottom
        border.color: rectFilterWheel.border.color
        font: filter1.font
        width: filter1.width
        height: filter1.height
        onClicked: {
            if (!active) {
                setActiveFilter(7)
            }
        }
    }
}

/*##^## Designer {
    D{i:0;autoSize:true;height:480;width:640}
}
 ##^##*/
