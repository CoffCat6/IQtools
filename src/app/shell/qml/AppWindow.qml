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

    // ─── Attach WindowController to this window for close-event interception ───
    Component.onCompleted: {
        windowController.attachToWindow(window)
    }

    // ─── Connect WindowController signals ───
    Connections {
        target: windowController

        function onExitConfirmationNeeded() {
            exitDialog.open()
        }

        function onQuitRequested() {
            Qt.quit()
        }
    }

    // ─── Connect SettingsController signals for tray visibility ───
    Connections {
        target: settingsCtrl

        function onMinimizeToTrayChanged(enabled) {
            windowController.setTrayVisible(enabled)
        }
    }

    // ─── Exit Confirmation Dialog ───
    Dialog {
        id: exitDialog

        anchors.centerIn: parent
        width: 400
        modal: true
        title: "退出确认"

        background: Rectangle {
            radius: themeController.palette.radiusCard
            color: themeController.palette.bgCard
            border.width: 1
            border.color: themeController.palette.borderDefault

            // Subtle gradient overlay
            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"
                opacity: 0.4
                gradient: Gradient {
                    orientation: Gradient.Vertical
                    GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, themeController.dark ? 0.08 : 0.4) }
                    GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, themeController.dark ? 0.08 : 0.02) }
                }
            }
        }

        header: Item {
            implicitHeight: 56

            Label {
                anchors.left: parent.left
                anchors.verticalCenter: parent.verticalCenter
                anchors.leftMargin: themeController.palette.spacingLg
                text: "退出确认"
                color: themeController.palette.textPrimary
                font.pixelSize: 18
                font.bold: true
                font.family: "Segoe UI Variable Display"
            }
        }

        contentItem: ColumnLayout {
            spacing: themeController.palette.spacingMd

            Label {
                Layout.fillWidth: true
                Layout.leftMargin: themeController.palette.spacingLg
                Layout.rightMargin: themeController.palette.spacingLg
                text: "确定要退出 IQtools 吗？"
                color: themeController.palette.textSecondary
                font.pixelSize: 14
                font.family: "Segoe UI"
                wrapMode: Text.Wrap
            }
        }

        footer: Item {
            implicitHeight: 56

            RowLayout {
                anchors.fill: parent
                anchors.margins: themeController.palette.spacingMd
                spacing: themeController.palette.spacingMd

                Item { Layout.fillWidth: true }

                Button {
                    text: "取消"
                    flat: true
                    onClicked: exitDialog.close()

                    contentItem: Text {
                        text: parent.text
                        color: themeController.palette.textSecondary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font.pixelSize: 14
                        font.bold: true
                    }

                    background: Rectangle {
                        radius: 18
                        color: parent.down ? Qt.darker(themeController.palette.bgCardHighlight, 1.1)
                                           : (parent.hovered ? Qt.lighter(themeController.palette.bgCardHighlight, 1.05)
                                                             : themeController.palette.bgCardHighlight)
                        border.width: 1
                        border.color: themeController.palette.borderDefault
                    }
                }

                ThemedButton {
                    text: "退出"
                    onClicked: {
                        exitDialog.close()
                        Qt.quit()
                    }
                }
            }
        }
    }

    // ─── Main Layout ───
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
