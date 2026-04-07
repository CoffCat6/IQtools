import QtQuick
import QtQuick.Controls
import IQtools

Button {
    id: root

    flat: true
    hoverEnabled: true

    contentItem: Text {
        text: root.text
        color: "white"
        horizontalAlignment: Text.AlignHCenter
        verticalAlignment: Text.AlignVCenter
        font.pixelSize: 14
        font.bold: true
    }

    background: Rectangle {
        radius: 18
        color: root.down ? Qt.darker(themeController.palette.accentPrimary, 1.15)
                         : (root.hovered ? Qt.lighter(themeController.palette.accentPrimary, 1.08)
                                         : themeController.palette.accentPrimary)
    }
}
