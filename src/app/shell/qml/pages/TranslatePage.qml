import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Item {
    id: root

    readonly property bool wideLayout: width >= 1120
    readonly property string effectiveSourceLanguage: translationController.resultSourceLanguage.length > 0
                                                    ? translationController.resultSourceLanguage
                                                    : translationController.sourceLanguage
    readonly property color statusBackground: {
        switch (translationController.statusLevel) {
        case "success":
            return Qt.rgba(110 / 255, 231 / 255, 216 / 255, themeController.dark ? 0.16 : 0.12)
        case "warning":
            return Qt.rgba(247 / 255, 178 / 255, 103 / 255, themeController.dark ? 0.18 : 0.14)
        case "error":
            return Qt.rgba(255 / 255, 122 / 255, 122 / 255, themeController.dark ? 0.20 : 0.14)
        default:
            return Qt.rgba(124 / 255, 156 / 255, 1.0, themeController.dark ? 0.16 : 0.10)
        }
    }
    readonly property color statusTextColor: {
        switch (translationController.statusLevel) {
        case "success":
            return themeController.dark ? "#7ef0e1" : "#0f8f7b"
        case "warning":
            return themeController.dark ? "#f7c67e" : "#a56400"
        case "error":
            return themeController.dark ? "#ff9e9e" : "#c0392b"
        default:
            return themeController.dark ? "#9dc0ff" : "#2d5fb8"
        }
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: themeController.palette.spacingLg

            Label {
                text: qsTr("翻译工具")
                color: themeController.palette.textPrimary
                font.pixelSize: 30
                font.bold: true
                font.family: "Segoe UI Variable Display"
            }

            Label {
                text: qsTr("面向短文本和段落翻译，优化了语言切换、状态反馈与剪贴板交互。")
                color: themeController.palette.textSecondary
                wrapMode: Text.Wrap
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: themeController.palette.spacingMd

                Rectangle {
                    radius: 14
                    color: Qt.rgba(0, 0, 0, themeController.dark ? 0.16 : 0.04)
                    border.width: 1
                    border.color: themeController.palette.borderDefault
                    implicitHeight: 28
                    implicitWidth: providerLabel.implicitWidth + 20

                    Label {
                        id: providerLabel

                        anchors.centerIn: parent
                        text: "MyMemory API"
                        color: themeController.palette.accentPrimary
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                Rectangle {
                    radius: 14
                    color: Qt.rgba(0, 0, 0, themeController.dark ? 0.16 : 0.04)
                    border.width: 1
                    border.color: themeController.palette.borderDefault
                    implicitHeight: 28
                    implicitWidth: routeLabel.implicitWidth + 20

                    Label {
                        id: routeLabel

                        anchors.centerIn: parent
                        text: translationController.languageDisplayName(root.effectiveSourceLanguage)
                              + " → "
                              + translationController.languageDisplayName(translationController.targetLanguage)
                        color: themeController.palette.textSecondary
                        font.pixelSize: 12
                        font.bold: true
                    }
                }

                Item { Layout.fillWidth: true }

                Label {
                    text: translationController.isTranslating
                          ? qsTr("翻译进行中")
                          : qsTr("准备就绪")
                    color: themeController.palette.textMuted
                    font.pixelSize: 12
                    font.bold: true
                }
            }

            BentoCard {
                Layout.fillWidth: true
                title: qsTr("翻译工作台")
                description: qsTr("左侧编辑原文，右侧查看译文。自动检测模式下完成一次翻译后可交换语言。")
                meta: "Translate Workspace"
                variant: "highlight"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: themeController.palette.spacingMd

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: qsTr("源语言")
                                color: themeController.palette.textMuted
                                font.pixelSize: 12
                            }

                            ComboBox {
                                Layout.fillWidth: true
                                model: translationController.sourceLanguages
                                textRole: "nativeName"
                                currentIndex: translationController.sourceLanguageIndex
                                onActivated: translationController.setSourceLanguageByIndex(currentIndex)
                            }
                        }

                        Button {
                            id: swapButton
                            Layout.preferredWidth: 44
                            Layout.preferredHeight: 44
                            enabled: translationController.canSwapLanguages()
                            text: "⇄"
                            font.pixelSize: 18
                            font.bold: true
                            onClicked: translationController.swapLanguages()

                            background: Rectangle {
                                radius: 22
                                color: swapButton.down
                                       ? Qt.darker(themeController.palette.bgCardHighlight, 1.08)
                                       : (swapButton.hovered
                                          ? Qt.lighter(themeController.palette.bgCardHighlight, 1.05)
                                          : themeController.palette.bgCardHighlight)
                                border.width: 1
                                border.color: translationController.canSwapLanguages()
                                              ? themeController.palette.borderDefault
                                              : themeController.palette.textMuted
                                opacity: translationController.canSwapLanguages() ? 1.0 : 0.55
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            spacing: 4

                            Label {
                                text: qsTr("目标语言")
                                color: themeController.palette.textMuted
                                font.pixelSize: 12
                            }

                            ComboBox {
                                Layout.fillWidth: true
                                model: translationController.targetLanguages
                                textRole: "nativeName"
                                currentIndex: translationController.targetLanguageIndex
                                onActivated: translationController.setTargetLanguageByIndex(currentIndex)
                            }
                        }
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: translationController.sourceLanguage === "auto"
                                 && !translationController.canSwapLanguages()
                        text: qsTr("提示：自动检测模式下，请先完成一次翻译后再交换语言。")
                        color: themeController.palette.textMuted
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }

                    Rectangle {
                        Layout.fillWidth: true
                        radius: themeController.palette.radiusCard
                        color: root.statusBackground
                        border.width: 1
                        border.color: Qt.rgba(1, 1, 1, themeController.dark ? 0.18 : 0.28)

                        RowLayout {
                            anchors.fill: parent
                            anchors.margins: 12
                            spacing: 10

                            Label {
                                text: translationController.statusLevel === "success" ? "完成"
                                      : translationController.statusLevel === "warning" ? "提醒"
                                      : translationController.statusLevel === "error" ? "错误"
                                      : "状态"
                                color: root.statusTextColor
                                font.pixelSize: 12
                                font.bold: true
                            }

                            Rectangle {
                                Layout.preferredWidth: 1
                                Layout.fillHeight: true
                                color: Qt.rgba(1, 1, 1, themeController.dark ? 0.15 : 0.35)
                            }

                            Label {
                                Layout.fillWidth: true
                                text: translationController.statusMessage
                                color: themeController.palette.textPrimary
                                font.pixelSize: 13
                                wrapMode: Text.Wrap
                            }

                            Label {
                                visible: translationController.resultSourceLanguage.length > 0
                                text: qsTr("检测到：%1")
                                      .arg(translationController.languageDisplayName(translationController.resultSourceLanguage))
                                color: themeController.palette.textMuted
                                font.pixelSize: 12
                            }
                        }
                    }

                    GridLayout {
                        Layout.fillWidth: true
                        Layout.fillHeight: true
                        columns: root.wideLayout ? 2 : 1
                        rowSpacing: themeController.palette.spacingMd
                        columnSpacing: themeController.palette.spacingMd

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: themeController.palette.spacingMd

                            TranslationEditorPanel {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: root.wideLayout ? 320 : 280
                                Layout.preferredHeight: root.wideLayout ? 360 : 320
                                title: qsTr("原文")
                                subtitle: translationController.languageDisplayName(translationController.sourceLanguage)
                                badgeText: translationController.sourceText.trim().length > 0 ? qsTr("可翻译") : qsTr("待输入")
                                textValue: translationController.sourceText
                                placeholderText: qsTr("在此输入或粘贴要翻译的文本…")
                                helperText: translationController.resultStale
                                            ? qsTr("原文已修改，建议重新翻译以刷新结果。")
                                            : qsTr("支持多行文本，适合短文本或段落翻译。")
                                panelEnabled: !translationController.isTranslating
                                onTextEdited: translationController.sourceText = text
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: themeController.palette.spacingMd

                                ThemedButton {
                                    text: translationController.isTranslating ? qsTr("取消") : qsTr("开始翻译")
                                    onClicked: {
                                        if (translationController.isTranslating) {
                                            translationController.cancelTranslation()
                                        } else {
                                            translationController.translate()
                                        }
                                    }
                                }

                                Button {
                                    text: qsTr("粘贴")
                                    onClicked: translationController.pasteSource()
                                }

                                Button {
                                    text: qsTr("复制原文")
                                    enabled: translationController.sourceText.length > 0
                                    onClicked: translationController.copySource()
                                }

                                Button {
                                    text: qsTr("清空")
                                    enabled: translationController.sourceText.length > 0
                                    onClicked: translationController.clear()
                                }

                                Item { Layout.fillWidth: true }
                            }
                        }

                        ColumnLayout {
                            Layout.fillWidth: true
                            Layout.fillHeight: true
                            spacing: themeController.palette.spacingMd

                            TranslationEditorPanel {
                                Layout.fillWidth: true
                                Layout.fillHeight: true
                                Layout.minimumHeight: root.wideLayout ? 320 : 280
                                Layout.preferredHeight: root.wideLayout ? 360 : 320
                                title: qsTr("译文")
                                subtitle: translationController.languageDisplayName(translationController.targetLanguage)
                                badgeText: translationController.translatedText.length > 0 ? qsTr("已生成") : qsTr("等待结果")
                                textValue: translationController.translatedText
                                placeholderText: qsTr("翻译结果会显示在这里")
                                helperText: translationController.resultStale
                                            ? qsTr("当前译文可能已过期，请重新翻译以同步最新原文。")
                                            : qsTr("结果区域只读，方便复制和对照。")
                                readOnly: true
                                panelEnabled: true
                            }

                            RowLayout {
                                Layout.fillWidth: true
                                spacing: themeController.palette.spacingMd

                                ThemedButton {
                                    text: qsTr("复制译文")
                                    enabled: translationController.translatedText.length > 0
                                    onClicked: translationController.copyResult()
                                }

                                Label {
                                    visible: translationController.resultStale
                                    text: qsTr("译文待刷新")
                                    color: "#ff8a8a"
                                    font.pixelSize: 12
                                    font.bold: true
                                }

                                Item { Layout.fillWidth: true }
                            }
                        }
                    }
                }
            }

            BentoCard {
                Layout.fillWidth: true
                title: qsTr("快捷目标语言")
                description: qsTr("点击常用语言可快速切换目标语言。")
                meta: "Quick Targets"
                variant: "quiet"

                Flow {
                    Layout.fillWidth: true
                    spacing: 8

                    Repeater {
                        model: translationController.targetLanguages

                        Rectangle {
                            required property int index
                            required property var modelData

                            radius: 16
                            color: translationController.targetLanguage === modelData.code
                                   ? Qt.rgba(124 / 255, 156 / 255, 1.0, themeController.dark ? 0.22 : 0.14)
                                   : themeController.palette.bgCardHighlight
                            border.width: 1
                            border.color: translationController.targetLanguage === modelData.code
                                          ? themeController.palette.accentPrimary
                                          : themeController.palette.borderDefault
                            implicitWidth: chipLabel.implicitWidth + 24
                            implicitHeight: 32

                            Label {
                                id: chipLabel

                                anchors.centerIn: parent
                                text: modelData.nativeName
                                color: translationController.targetLanguage === modelData.code
                                       ? themeController.palette.accentPrimary
                                       : themeController.palette.textSecondary
                                font.pixelSize: 12
                                font.family: "Segoe UI"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: translationController.setTargetLanguageByIndex(index)
                            }
                        }
                    }
                }
            }

            BentoCard {
                Layout.fillWidth: true
                title: qsTr("体验建议")
                meta: "Tips"
                variant: "quiet"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 8

                    Label {
                        text: "• " + qsTr("自动检测适合快速上手；固定源语言可减少误判并提高交换语言的可预期性。")
                        color: themeController.palette.textSecondary
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        wrapMode: Text.Wrap
                    }

                    Label {
                        text: "• " + qsTr("输入区支持直接粘贴，结果区保持只读，适合边看边复制。")
                        color: themeController.palette.textSecondary
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        wrapMode: Text.Wrap
                    }

                    Label {
                        text: "• " + qsTr("出现“译文待刷新”提示时，说明原文或语言已经变化，需要重新翻译。")
                        color: themeController.palette.textSecondary
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        wrapMode: Text.Wrap
                    }

                    Label {
                        text: "• " + qsTr("MyMemory 更适合通用文本和轻量使用；高频或专业术语场景建议后续接入可配置 Provider。")
                        color: themeController.palette.textSecondary
                        font.pixelSize: 13
                        font.family: "Segoe UI"
                        wrapMode: Text.Wrap
                    }
                }
            }

            Item {
                Layout.fillHeight: true
            }
        }
    }
}
