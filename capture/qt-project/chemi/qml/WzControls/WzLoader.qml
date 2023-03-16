import QtQuick 2.12
import QtQuick.Controls 2.12

Loader {
    onStatusChanged: {
        if (status === Loader.Ready || status === Loader.Error) {
            loading.running = false
            loading.visible = false
        } else if (status === Loader.Loading) {
            loading.running = true
            loading.visible = true
        }
    }

    BusyIndicator {
        id: loading
        anchors.centerIn: parent
        width: 50
        height: 50
        visible: false
    }
}
