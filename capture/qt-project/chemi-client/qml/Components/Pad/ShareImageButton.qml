import QtQuick 2.12
import WzI18N 1.0
import "../../WzControls"

WzButton {
    width: 100
    height: 44
    radius: 4
    text: qsTr("分享")
    normalColor: "transparent"
    label.anchors.horizontalCenterOffset: 12
    label.anchors.verticalCenterOffset: 1
    label.font.pixelSize: {
        switch(WzI18N.language) {
        case "zh": return 18
        case "en": return 15
        }
    }
    imageVisible: true
    imageSourceNormal: "qrc:/images/button_share.svg"
    image.sourceSize.width: 32
    image.sourceSize.height: 32
    image.anchors.horizontalCenterOffset: -26
    onClicked: {
        if (!imageService.canSaveAsImage())
            return

        var images = [thumbnail.listModel.get(thumbnail.activeIndex)]
        imageFormatSelectII.images = images
        imageFormatSelectII.show()
        rectangleShade.opacity = 0.7
    }
}
