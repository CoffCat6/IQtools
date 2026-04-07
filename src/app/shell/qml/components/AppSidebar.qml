import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Rectangle {
    radius: themeController.palette.radiusPanel
    color: themeController.palette.bgPanel
    border.width: 1
    border.color: themeController.palette.borderDefault

    ColumnLayout {
        anchors.fill: parent
        anchors.margins: themeController.palette.spacingLg
        spacing: themeController.palette.spacingSm

        Label {
            text: "IQtools"
            color: themeController.palette.textPrimary
            font.pixelSize: 28
            font.bold: true
        }

        Label {
            text: themeController.dark ? "QML Dark" : "QML Light"
            color: themeController.palette.textMuted
            font.pixelSize: 12
        }

        Item { Layout.preferredHeight: themeController.palette.spacingMd }

        Repeater {
            model: navigationController.navigationItems

            delegate: NavItem {
                Layout.fillWidth: true
                label: modelData.label
                route: modelData.route
            }
        }

        Item { Layout.fillHeight: true }

        ThemedButton {
            Layout.fillWidth: true
            text: themeController.dark ? "切换到浅色" : "切换到深色"
            onClicked: themeController.toggleTheme()
        }
    }
}
