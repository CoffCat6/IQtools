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

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: themeController.palette.spacingLg

            // ─── Page Title ───
            Label {
                text: qsTr("设置")
                color: themeController.palette.textPrimary
                font.pixelSize: 30
                font.bold: true
                font.family: "Segoe UI Variable Display"
            }

            Label {
                text: qsTr("配置文件: ") + settingsCtrl.settingsFilePath
                color: themeController.palette.textMuted
                font.pixelSize: 12
                font.family: "Segoe UI"
                Layout.bottomMargin: 4

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: settingsCtrl.openSettingsFile()
                }
            }

            // ═══════════════════════════════════════════
            // 1. 外观设置 (Appearance)
            // ═══════════════════════════════════════════
            BentoCard {
                Layout.fillWidth: true
                title: qsTr("外观设置")
                description: qsTr("选择默认主题和界面语言。")
                meta: "Appearance"
                variant: "highlight"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: themeController.palette.spacingMd

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        Label {
                            text: qsTr("默认主题")
                            color: themeController.palette.textSecondary
                            font.pixelSize: 14
                            Layout.preferredWidth: 80
                        }

                        ThemedButton {
                            text: qsTr("浅色")
                            opacity: settingsCtrl.defaultTheme === "light" ? 1.0 : 0.5
                            onClicked: settingsCtrl.defaultTheme = "light"
                        }

                        ThemedButton {
                            text: qsTr("深色")
                            opacity: settingsCtrl.defaultTheme === "dark" ? 1.0 : 0.5
                            onClicked: settingsCtrl.defaultTheme = "dark"
                        }

                        Item { Layout.fillWidth: true }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        Label {
                            text: qsTr("界面语言")
                            color: themeController.palette.textSecondary
                            font.pixelSize: 14
                            Layout.preferredWidth: 80
                        }

                        ComboBox {
                            id: langCombo
                            Layout.preferredWidth: 180
                            model: settingsCtrl.languageOptions
                            currentIndex: Math.max(0, model.indexOf(settingsCtrl.language))
                            onActivated: settingsCtrl.language = currentText
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            // ═══════════════════════════════════════════
            // 2. 通用设置 (General)
            // ═══════════════════════════════════════════
            BentoCard {
                Layout.fillWidth: true
                title: qsTr("通用设置")
                description: qsTr("管理启动行为和系统集成选项。")
                meta: "General"
                variant: "quiet"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 12

                    SettingSwitch {
                        label: qsTr("开机自启动")
                        subtitle: qsTr("系统启动时自动运行 IQtools")
                        checked: settingsCtrl.autoStart
                        onToggled: settingsCtrl.autoStart = checked
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: themeController.palette.borderDefault
                        opacity: 0.5
                    }

                    SettingSwitch {
                        label: qsTr("自动检查更新")
                        subtitle: qsTr("启动时检查是否有新版本可用")
                        checked: settingsCtrl.checkUpdate
                        onToggled: settingsCtrl.checkUpdate = checked
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: themeController.palette.borderDefault
                        opacity: 0.5
                    }

                    SettingSwitch {
                        label: qsTr("最小化到托盘")
                        subtitle: qsTr("关闭窗口时最小化到系统托盘而非退出")
                        checked: settingsCtrl.minimizeToTray
                        onToggled: settingsCtrl.minimizeToTray = checked
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: themeController.palette.borderDefault
                        opacity: 0.5
                    }

                    SettingSwitch {
                        label: qsTr("退出确认")
                        subtitle: qsTr("退出程序前弹出确认对话框")
                        checked: settingsCtrl.confirmOnExit
                        onToggled: settingsCtrl.confirmOnExit = checked
                    }
                }
            }

            // ═══════════════════════════════════════════
            // 3. 日志设置 (Logging)
            // ═══════════════════════════════════════════
            BentoCard {
                Layout.fillWidth: true
                title: qsTr("日志设置")
                description: qsTr("配置控制台输出、文件输出、最低日志级别与日志目录。")
                meta: "Logging"
                variant: "quiet"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: themeController.palette.spacingMd

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        CheckBox {
                            id: consoleCheck
                            text: qsTr("控制台输出")
                            checked: loggingSettings.consoleEnabled
                            onToggled: loggingSettings.consoleEnabled = checked
                        }

                        CheckBox {
                            id: fileCheck
                            text: qsTr("文件输出")
                            checked: loggingSettings.fileEnabled
                            onToggled: loggingSettings.fileEnabled = checked
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        Label {
                            text: qsTr("最低级别")
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
                            text: qsTr("日志目录")
                            color: themeController.palette.textSecondary
                        }

                        TextField {
                            Layout.fillWidth: true
                            text: loggingSettings.logDirectory
                            placeholderText: qsTr("例如 C:/Users/xxx/AppData/Roaming/IQtools/logs")
                            onTextEdited: loggingSettings.logDirectory = text
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        Button {
                            text: qsTr("默认目录")
                            onClicked: loggingSettings.useDefaultDirectory()
                        }

                        Button {
                            text: qsTr("重置")
                            enabled: loggingSettings.dirty
                            onClicked: loggingSettings.resetPending()
                        }

                        Item { Layout.fillWidth: true }

                        ThemedButton {
                            text: qsTr("应用日志设置")
                            enabled: loggingSettings.dirty
                            onClicked: root.applyLoggingSettings()
                        }
                    }

                    Label {
                        visible: loggingSettings.statusMessage.length > 0
                        text: loggingSettings.statusMessage
                        color: loggingSettings.statusMessage.indexOf(qsTr("失败")) >= 0
                               || loggingSettings.statusMessage.indexOf(qsTr("不可写")) >= 0
                               ? "#ff8a8a"
                               : themeController.palette.accentSecondary
                        wrapMode: Text.Wrap
                    }
                }
            }

            // ═══════════════════════════════════════════
            // 4. 关于与更新 (About & Updates)
            // ═══════════════════════════════════════════
            BentoCard {
                Layout.fillWidth: true
                title: qsTr("关于 IQtools")
                description: qsTr("版本信息与更新检查。")
                meta: "About"
                variant: "highlight"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: themeController.palette.spacingMd

                    // Version info row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingLg

                        ColumnLayout {
                            spacing: 4

                            Label {
                                text: qsTr("版本")
                                color: themeController.palette.textMuted
                                font.pixelSize: 12
                            }

                            Label {
                                text: updateController.currentVersion
                                color: themeController.palette.textPrimary
                                font.pixelSize: 16
                                font.bold: true
                            }
                        }

                        ColumnLayout {
                            spacing: 4

                            Label {
                                text: qsTr("构建日期")
                                color: themeController.palette.textMuted
                                font.pixelSize: 12
                            }

                            Label {
                                text: updateController.buildDate
                                color: themeController.palette.textPrimary
                                font.pixelSize: 16
                            }
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: themeController.palette.borderDefault
                        opacity: 0.5
                    }

                    // Update check row
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: updateController.hasUpdate
                                      ? qsTr("发现新版本: ") + updateController.latestVersion
                                      : qsTr("检查更新")
                                color: updateController.hasUpdate
                                       ? themeController.palette.accentPrimary
                                       : themeController.palette.textPrimary
                                font.pixelSize: 14
                                font.bold: updateController.hasUpdate
                            }

                            Label {
                                visible: updateController.statusMessage.length > 0
                                text: updateController.statusMessage
                                color: themeController.palette.textMuted
                                font.pixelSize: 12
                            }
                        }

                        ThemedButton {
                            text: updateController.checking ? qsTr("检查中...") : qsTr("检查更新")
                            enabled: !updateController.checking
                            onClicked: updateController.checkNow()
                        }

                        ThemedButton {
                            visible: updateController.hasUpdate
                            text: qsTr("查看发布")
                            onClicked: updateController.openReleasePage()
                        }
                    }

                    // Release notes (if update available)
                    ColumnLayout {
                        visible: updateController.hasUpdate && updateController.releaseNotes.length > 0
                        Layout.fillWidth: true
                        spacing: 4

                        Label {
                            text: qsTr("更新说明")
                            color: themeController.palette.textMuted
                            font.pixelSize: 12
                        }

                        ScrollView {
                            Layout.fillWidth: true
                            Layout.preferredHeight: Math.min(contentHeight, 120)
                            clip: true

                            Label {
                                width: parent.width
                                text: updateController.releaseNotes
                                color: themeController.palette.textSecondary
                                font.pixelSize: 13
                                wrapMode: Text.Wrap
                            }
                        }
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        Layout.preferredHeight: 1
                        color: themeController.palette.borderDefault
                        opacity: 0.5
                    }

                    // Credits
                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        Label {
                            text: "© 2024-2026 IQtools"
                            color: themeController.palette.textMuted
                            font.pixelSize: 12
                        }

                        Label {
                            text: "•"
                            color: themeController.palette.textMuted
                            font.pixelSize: 12
                        }

                        Label {
                            text: "MIT License"
                            color: themeController.palette.textMuted
                            font.pixelSize: 12
                        }

                        Label {
                            text: "•"
                            color: themeController.palette.textMuted
                            font.pixelSize: 12
                        }

                        Label {
                            text: "Qt 6"
                            color: themeController.palette.textMuted
                            font.pixelSize: 12
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            // ═══════════════════════════════════════════
            // 5. 操作栏 (Actions)
            // ═══════════════════════════════════════════
            RowLayout {
                Layout.fillWidth: true
                spacing: themeController.palette.spacingMd

                ThemedButton {
                    text: qsTr("打开配置文件")
                    onClicked: settingsCtrl.openSettingsFile()
                }

                ThemedButton {
                    text: qsTr("恢复所有默认设置")
                    onClicked: settingsCtrl.resetToDefaults()
                }

                Item { Layout.fillWidth: true }
            }

            // ─── Status message ───
            Label {
                visible: settingsCtrl.statusMessage.length > 0
                text: settingsCtrl.statusMessage
                color: themeController.palette.accentSecondary
                font.pixelSize: 13
                font.family: "Segoe UI"

                Behavior on opacity {
                    NumberAnimation { duration: 300 }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }

    // ─── Inline SettingSwitch component ───
    component SettingSwitch: RowLayout {
        id: settingSwitchRoot
        property string label: ""
        property string subtitle: ""
        property alias checked: toggle.checked

        signal toggled()

        Layout.fillWidth: true
        spacing: themeController.palette.spacingMd

        ColumnLayout {
            Layout.fillWidth: true
            spacing: 2

            Label {
                text: settingSwitchRoot.label
                color: themeController.palette.textPrimary
                font.pixelSize: 14
                font.bold: true
                font.family: "Segoe UI"
            }

            Label {
                text: settingSwitchRoot.subtitle
                color: themeController.palette.textMuted
                font.pixelSize: 12
                font.family: "Segoe UI"
                visible: settingSwitchRoot.subtitle.length > 0
            }
        }

        Switch {
            id: toggle

            onToggled: settingSwitchRoot.toggled()
        }
    }
}
