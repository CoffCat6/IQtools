import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Item {
    id: root

    function applyLoggingSettings() {
        const ok = loggingSettings.apply()
        if (ok) {
            appFacade.logInfo("settings.logging", "Logging settings applied from UI")
        }
    }

    ColumnLayout {
        anchors.fill: parent
        spacing: themeController.palette.spacingLg

        Label {
            text: "设置"
            color: themeController.palette.textPrimary
            font.pixelSize: 30
            font.bold: true
        }

        BentoCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 220
            title: "主题设置"
            description: themeController.dark
                         ? "当前为深色主题，可切换到浅色主题。"
                         : "当前为浅色主题，可切换到深色主题。"
            meta: "Theme Settings"
            variant: "highlight"
        }

        RowLayout {
            spacing: themeController.palette.spacingMd

            ThemedButton {
                text: "浅色主题"
                onClicked: themeController.setTheme("light")
            }

            ThemedButton {
                text: "深色主题"
                onClicked: themeController.setTheme("dark")
            }
        }

        BentoCard {
            Layout.fillWidth: true
            Layout.preferredHeight: 360
            title: "日志设置"
            description: "配置控制台输出、文件输出、最低日志级别与日志目录。"
            meta: "Logging Settings"
            variant: "quiet"

            ColumnLayout {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: themeController.palette.spacingLg
                spacing: themeController.palette.spacingMd

                RowLayout {
                    spacing: themeController.palette.spacingMd

                    CheckBox {
                        id: consoleCheck
                        text: "控制台输出"
                        checked: loggingSettings.consoleEnabled
                        onToggled: loggingSettings.consoleEnabled = checked
                    }

                    CheckBox {
                        id: fileCheck
                        text: "文件输出"
                        checked: loggingSettings.fileEnabled
                        onToggled: loggingSettings.fileEnabled = checked
                    }
                }

                RowLayout {
                    spacing: themeController.palette.spacingMd

                    Label {
                        text: "最低级别"
                        color: themeController.palette.textSecondary
                    }

                    ComboBox {
                        id: levelCombo
                        model: loggingSettings.levelOptions
                        currentIndex: Math.max(0, model.indexOf(loggingSettings.minimumLevel))
                        onActivated: loggingSettings.minimumLevel = currentText
                    }
                }

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 6

                    Label {
                        text: "日志目录"
                        color: themeController.palette.textSecondary
                    }

                    TextField {
                        Layout.fillWidth: true
                        text: loggingSettings.logDirectory
                        placeholderText: "例如 C:/Users/xxx/AppData/Roaming/IQtools/IQtools/logs"
                        onTextEdited: loggingSettings.logDirectory = text
                    }
                }

                RowLayout {
                    spacing: themeController.palette.spacingMd

                    Button {
                        text: "默认目录"
                        onClicked: loggingSettings.useDefaultDirectory()
                    }

                    Button {
                        text: "重置"
                        enabled: loggingSettings.dirty
                        onClicked: loggingSettings.resetPending()
                    }

                    Item { Layout.fillWidth: true }

                    ThemedButton {
                        text: "应用日志设置"
                        enabled: loggingSettings.dirty
                        onClicked: root.applyLoggingSettings()
                    }
                }

                Label {
                    visible: loggingSettings.statusMessage.length > 0
                    text: loggingSettings.statusMessage
                    color: loggingSettings.statusMessage.indexOf("失败") >= 0
                           || loggingSettings.statusMessage.indexOf("不可写") >= 0
                           ? "#ff8a8a"
                           : themeController.palette.accentSecondary
                    wrapMode: Text.Wrap
                }
            }
        }
    }
}
