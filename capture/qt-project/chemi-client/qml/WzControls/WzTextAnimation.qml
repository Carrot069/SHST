import QtQuick 2.12

Item {
    id: root

    property Text target
    property string text

    function start(text) {
        root.text = text
        n1.start()
    }

    NumberAnimation {
        id: n1
        target: root.target
        properties: "opacity"
        duration: 400
        from: 1
        to: 0
        easing.type: Easing.OutQuad
        onFinished: {
            target.text = root.text
            n2.start()
        }
    }

    NumberAnimation {
        id: n2
        target: root.target
        properties: "opacity"
        duration: 500
        from: 0
        to: 1
        easing.type: Easing.InQuad
    }
}
