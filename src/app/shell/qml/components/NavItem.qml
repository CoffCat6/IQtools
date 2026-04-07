import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Button {
    id: root

    property string label: ""
    property string route: ""
    readonly property bool active: navigationController.currentRoute === route

    text: label
    flat: true
    hoverEnabled: true

    onClicked: navigationController.navigateTo(route)

    contentItem: Text {
        text: root.text
        color: root.active ? themeController.palette.textPrimary : themeController.palette.textSecondary
        font.pixelSize: 15
        font.bold: root.active
        verticalAlignment: Text.AlignVCenter
    }

    background: Rectangle {
        radius: 18
        color: root.active
               ? themeController.palette.bgCardHighlight
               : (root.hovered ? themeController.palette.bgCard : "transparent")
        border.width: root.active ? 1 : 0
        border.color: themeController.palette.borderDefault
    }

    Layout.preferredHeight: 44
}
