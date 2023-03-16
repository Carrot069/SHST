import QtQuick 2.12
import QtQuick.Controls 2.12
import QtQuick.Controls.Material 2.12

import WzI18N 1.0

ComboBox {
    id: control
    font.family: WzI18N.font.family
    delegate: ItemDelegate {
        width: control.width
        contentItem: Text {
            text: modelData
            color: control.currentIndex === model.index ? Material.accent : Material.foreground
            font: control.font
            elide: Text.ElideRight
            verticalAlignment: Text.AlignVCenter
        }
        highlighted: control.highlightedIndex === model.index
    }

    popup.y: control.height
}
