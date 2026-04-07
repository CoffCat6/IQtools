import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

ApplicationWindow {
    id: window

    width: 1280
    height: 820
    visible: true
    title: "IQtools"
    color: themeController.palette.bgApp

    RowLayout {
        anchors.fill: parent
        anchors.margins: themeController.palette.spacingMd
        spacing: themeController.palette.spacingMd

        AppSidebar {
            Layout.fillHeight: true
            Layout.preferredWidth: 240
        }

        Rectangle {
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: themeController.palette.radiusPanel
            color: themeController.palette.bgPanel
            border.width: 1
            border.color: themeController.palette.borderDefault

            Loader {
                id: pageLoader
                anchors.fill: parent
                anchors.margins: themeController.palette.spacingLg

                sourceComponent: {
                    switch (navigationController.currentRoute) {
                    case "capture": return capturePage
                    case "translate": return translatePage
                    case "plugins": return pluginPage
                    case "logs": return logPage
                    case "settings": return settingsPage
                    default: return homePage
                    }
                }
            }
        }
    }

    Component { id: homePage; HomePage {} }
    Component { id: capturePage; CapturePage {} }
    Component { id: translatePage; TranslatePage {} }
    Component { id: pluginPage; PluginPage {} }
    Component { id: logPage; LogPage {} }
    Component { id: settingsPage; SettingsPage {} }
}
