import QtQuick 2.12
import QtQuick.Controls 2.12
import "WzControls"
import "Components"
import "."

ApplicationWindow {
    width: 800
    height: 600
    color: "#000000"
    visible: true
    title: "test"

    WzRadioButton {
        text: "checked"
        checked: true
        onCheckedChanged: rb3.enabled = checked
    }

    WzRadioButton {
        y: 40
        text: "checked"
        checked: false
        onCheckedChanged: rb3.enabled = !checked
    }

    WzRadioButton {
        id: rb3
        y: 80
        text: "disabled"
        enabled: false
    }

    PseudoColor {
        x: 20
        y: 120

        onPreview: {
            for (var i = 0; i < rgbList.count; i++)
                console.log(rgbList.get(i))
        }
    }


}

