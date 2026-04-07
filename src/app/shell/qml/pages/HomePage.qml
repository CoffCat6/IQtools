import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

ScrollView {
    id: root

    clip: true

    readonly property int pagePadding: width >= 1440 ? 30 : (width >= 980 ? 24 : 16)
    readonly property int denseSpacing: width >= 1440 ? 18 : 14
    readonly property int columns: width >= 1540 ? 4 : (width >= 1120 ? 3 : (width >= 760 ? 2 : 1))

    ColumnLayout {
        width: root.availableWidth
        spacing: root.denseSpacing

        Rectangle {
            Layout.fillWidth: true
            Layout.preferredHeight: root.width >= 1120 ? 232 : 260
            radius: themeController.palette.radiusPanel
            color: themeController.palette.bgCard
            border.width: 1
            border.color: themeController.palette.borderDefault

            Rectangle {
                anchors.fill: parent
                radius: parent.radius
                color: "transparent"

                gradient: Gradient {
                    orientation: Gradient.Horizontal
                    GradientStop {
                        position: 0.0
                        color: Qt.rgba(124 / 255, 156 / 255, 1.0, themeController.dark ? 0.24 : 0.14)
                    }
                    GradientStop {
                        position: 0.56
                        color: Qt.rgba(28 / 255, 198 / 255, 183 / 255, themeController.dark ? 0.16 : 0.10)
                    }
                    GradientStop {
                        position: 1.0
                        color: Qt.rgba(255 / 255, 255 / 255, 255 / 255, 0)
                    }
                }
            }

            RowLayout {
                anchors.fill: parent
                anchors.margins: root.pagePadding
                spacing: root.denseSpacing

                ColumnLayout {
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    spacing: 8

                    Label {
                        text: "IQtools · HOME"
                        color: themeController.palette.textMuted
                        font.pixelSize: 12
                        font.bold: true
                        font.family: "Segoe UI"
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "现代化工具工作台"
                        color: themeController.palette.textPrimary
                        font.pixelSize: root.width >= 980 ? 44 : 36
                        font.bold: true
                        font.family: "Segoe UI Variable Display"
                        wrapMode: Text.Wrap
                    }

                    Label {
                        Layout.fillWidth: true
                        text: "一屏聚合截图、翻译与插件能力，使用更清晰的信息分层与节奏组织高频操作。"
                        color: themeController.palette.textSecondary
                        font.pixelSize: 15
                        wrapMode: Text.Wrap
                        font.family: "Segoe UI"
                    }

                    Item { Layout.fillHeight: true }

                    RowLayout {
                        spacing: 10

                        ThemedButton {
                            text: "进入截图"
                            onClicked: navigationController.navigateTo("capture")
                        }

                        Button {
                            id: themeSwitchButton

                            text: "切换主题"
                            flat: true
                            hoverEnabled: true
                            onClicked: themeController.toggleTheme()

                            contentItem: Text {
                                text: themeSwitchButton.text
                                color: themeController.palette.textPrimary
                                horizontalAlignment: Text.AlignHCenter
                                verticalAlignment: Text.AlignVCenter
                                font.pixelSize: 13
                                font.bold: true
                            }

                            background: Rectangle {
                                radius: 18
                                color: themeSwitchButton.down
                                       ? Qt.darker(themeController.palette.bgCardHighlight, 1.1)
                                       : (themeSwitchButton.hovered
                                          ? Qt.lighter(themeController.palette.bgCardHighlight, themeController.dark ? 1.08 : 1.03)
                                          : themeController.palette.bgCardHighlight)
                                border.width: 1
                                border.color: themeController.palette.borderDefault
                            }
                        }
                    }
                }

                Rectangle {
                    Layout.preferredWidth: root.width >= 1120 ? 280 : 0
                    Layout.fillHeight: true
                    visible: root.width >= 1120
                    radius: themeController.palette.radiusCard - 4
                    color: Qt.rgba(0, 0, 0, themeController.dark ? 0.18 : 0.03)
                    border.width: 1
                    border.color: themeController.palette.borderDefault

                    ColumnLayout {
                        anchors.fill: parent
                        anchors.margins: 16
                        spacing: 8

                        Label {
                            text: "状态速览"
                            color: themeController.palette.textPrimary
                            font.pixelSize: 16
                            font.bold: true
                            font.family: "Segoe UI Variable Display"
                        }

                        Label {
                            text: themeController.dark ? "Dark Theme Active" : "Light Theme Active"
                            color: themeController.palette.accentPrimary
                            font.pixelSize: 12
                            font.bold: true
                            font.family: "Segoe UI"
                        }

                        Rectangle {
                            Layout.fillWidth: true
                            Layout.preferredHeight: 1
                            color: themeController.palette.borderDefault
                        }

                        Label {
                            Layout.fillWidth: true
                            text: "• Bento 结构已接管首页\n• 保持 bridge 接口不变\n• 支持深浅主题视觉一致"
                            color: themeController.palette.textSecondary
                            wrapMode: Text.Wrap
                            lineHeight: 1.25
                            font.pixelSize: 13
                            font.family: "Segoe UI"
                        }

                        Item { Layout.fillHeight: true }

                        Label {
                            text: "Ready"
                            color: themeController.palette.textMuted
                            font.pixelSize: 12
                            font.bold: true
                            font.family: "Segoe UI"
                        }
                    }
                }
            }
        }

        GridLayout {
            Layout.fillWidth: true
            columns: root.columns
            rowSpacing: root.denseSpacing
            columnSpacing: root.denseSpacing

            BentoCard {
                Layout.fillWidth: true
                Layout.preferredHeight: root.columns >= 3 ? 238 : 220
                Layout.columnSpan: root.columns >= 3 ? 2 : 1
                title: "工作流总览"
                description: "围绕高频任务做卡片分区：先给出最优入口，再承接细分工具，降低切页与搜索成本。"
                meta: "Bento Overview"
                variant: "highlight"
                revealDelay: 40
                clickable: true
                onClicked: navigationController.navigateTo("plugins")
            }

            BentoCard {
                Layout.fillWidth: true
                Layout.preferredHeight: root.columns >= 3 ? 238 : 180
                title: "一键截图"
                description: "直接进入截图页，后续可扩展区域截取与标注链路。"
                meta: "Capture"
                variant: "accent"
                clickable: true
                revealDelay: 80
                onClicked: navigationController.navigateTo("capture")
            }

            Repeater {
                model: toolListModel

                delegate: BentoCard {
                    required property int index
                    required property string name
                    required property string description
                    required property string route
                    required property bool highlighted

                    Layout.fillWidth: true
                    Layout.preferredHeight: 176
                    title: name
                    meta: route
                    variant: highlighted ? "accent" : "default"
                    description: description
                    clickable: true
                    revealDelay: 120 + index * 70
                    onClicked: navigationController.navigateTo(route)
                }
            }

            BentoCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 176
                title: "主题系统"
                description: "基于 palette token 进行深浅主题映射，视觉语义保持一致且可即时切换。"
                meta: "Theme"
                variant: "quiet"
                revealDelay: 300
            }

            BentoCard {
                Layout.fillWidth: true
                Layout.preferredHeight: 176
                title: "日志系统"
                description: "延续既有日志基础设施，QML 导航行为可继续复用同一追踪入口。"
                meta: "Logging"
                variant: "quiet"
                revealDelay: 360
            }
        }

        Item { Layout.preferredHeight: 6 }
    }
}
