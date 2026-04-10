import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Item {
    id: root
    property int shortcutRevision: 0
    property var outputFormatItems: [
        { label: "PNG", value: "png" },
        { label: "JPG", value: "jpg" }
    ]
    property var dpiItems: [
        { label: qsTr("不写入"), value: 0 },
        { label: "96", value: 96 },
        { label: "144", value: 144 },
        { label: "300", value: 300 }
    ]

    property var annotationShortcutItems: [
        { actionKey: "rectangle", title: qsTr("矩形"), preferredWidth: 132 },
        { actionKey: "arrow", title: qsTr("箭头"), preferredWidth: 132 },
        { actionKey: "mosaic", title: qsTr("马赛克"), preferredWidth: 132 },
        { actionKey: "text", title: qsTr("文字"), preferredWidth: 132 },
        { actionKey: "undo", title: qsTr("撤销"), preferredWidth: 132 },
        { actionKey: "redo", title: qsTr("重做"), preferredWidth: 132 },
        { actionKey: "color_cycle", title: qsTr("换色"), preferredWidth: 148 }
    ]

    function applyRecordedShortcut(actionKey, portableText) {
        captureController.setAnnotationShortcutByKey(actionKey, portableText)
    }

    function shortcutText(actionKey) {
        return captureController.annotationShortcutByKey(actionKey)
    }

    function outputFormatIndex() {
        for (let i = 0; i < outputFormatItems.length; ++i) {
            if (outputFormatItems[i].value === captureController.outputFormat) {
                return i
            }
        }
        return 0
    }

    function dpiIndex() {
        for (let i = 0; i < dpiItems.length; ++i) {
            if (dpiItems[i].value === captureController.dpiValue) {
                return i
            }
        }
        return 0
    }

    ScrollView {
        anchors.fill: parent
        contentWidth: availableWidth
        clip: true

        ColumnLayout {
            width: parent.width
            spacing: themeController.palette.spacingLg

            Label {
                text: qsTr("截图工具")
                color: themeController.palette.textPrimary
                font.pixelSize: 30
                font.bold: true
                font.family: "Segoe UI Variable Display"
            }

            Label {
                text: qsTr("当前支持全屏/区域截图：包含多屏拼接、延时执行、分辨率倍率输出与剪贴板复制。")
                color: themeController.palette.textSecondary
                wrapMode: Text.Wrap
            }

            Label {
                Layout.fillWidth: true
                visible: captureController.platformCaptureHint.length > 0
                text: captureController.platformCaptureHint
                color: "#f7b267"
                wrapMode: Text.Wrap
                font.pixelSize: 12
            }

            BentoCard {
                Layout.fillWidth: true
                title: qsTr("快速截图")
                description: qsTr("使用按钮或快捷键触发截图，结果将保存为 PNG。")
                meta: "Capture MVP"
                variant: "highlight"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: themeController.palette.spacingMd

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        ThemedButton {
                            text: qsTr("全屏截图")
                            onClicked: captureController.captureNow()
                        }

                        ThemedButton {
                            text: qsTr("区域截图")
                            onClicked: captureController.captureRegion()
                        }

                        ThemedButton {
                            text: captureController.countdownActive
                                  ? qsTr("取消延时")
                                  : qsTr("延时全屏")
                            onClicked: {
                                if (captureController.countdownActive) {
                                    captureController.cancelDelayedCapture()
                                } else {
                                    captureController.captureWithDelay(-1)
                                }
                            }
                        }

                        Button {
                            text: qsTr("延时区域")
                            enabled: !captureController.countdownActive
                            onClicked: captureController.captureRegionWithDelay(-1)
                        }

                        Button {
                            text: qsTr("打开输出目录")
                            onClicked: captureController.openOutputDirectory()
                        }

                        Button {
                            text: qsTr("贴图最近截图")
                            onClicked: captureController.pinLastCapture()
                        }

                        Item { Layout.fillWidth: true }
                    }

                    Label {
                        Layout.fillWidth: true
                        text: captureController.captureShortcut.length > 0
                              ? qsTr("快捷键：%1").arg(captureController.captureShortcut)
                              : qsTr("快捷键：未绑定（可在设置页快捷键中配置 tool.capture）")
                        color: themeController.palette.textMuted
                        font.pixelSize: 12
                    }

                    Label {
                        Layout.fillWidth: true
                        text: qsTr("区域截图操作：拖拽创建选区，拖动锚点微调，Enter 确认，Esc 取消。")
                        color: themeController.palette.textMuted
                        font.pixelSize: 12
                        wrapMode: Text.Wrap
                    }

                    Label {
                        visible: captureController.countdownActive
                        text: qsTr("倒计时 %1 秒").arg(captureController.countdownRemaining)
                        color: themeController.palette.accentPrimary
                        font.bold: true
                    }

                    Label {
                        Layout.fillWidth: true
                        visible: captureController.statusMessage.length > 0
                        text: captureController.statusMessage
                        color: captureController.statusMessage.indexOf(qsTr("失败")) >= 0
                               ? "#ff8a8a"
                               : themeController.palette.accentSecondary
                        wrapMode: Text.Wrap
                    }
                }
            }

            BentoCard {
                Layout.fillWidth: true
                title: qsTr("截图选项")
                description: qsTr("控制延时、导出格式、DPI 元数据与输出目录。")
                meta: "Options"
                variant: "quiet"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: themeController.palette.spacingMd

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        Label {
                            text: qsTr("延时秒数")
                            color: themeController.palette.textSecondary
                        }

                        SpinBox {
                            from: 0
                            to: 10
                            value: captureController.delaySeconds
                            editable: true
                            onValueModified: captureController.delaySeconds = value
                        }

                        Item { Layout.fillWidth: true }

                        CheckBox {
                            text: qsTr("截图后自动复制到剪贴板")
                            checked: captureController.autoCopyToClipboard
                            onToggled: captureController.autoCopyToClipboard = checked
                        }

                        CheckBox {
                            text: qsTr("截图后进入标注编辑")
                            checked: captureController.annotationEnabled
                            onToggled: captureController.annotationEnabled = checked
                        }

                        CheckBox {
                            text: qsTr("截图后自动贴图")
                            checked: captureController.pinAfterCapture
                            onToggled: captureController.pinAfterCapture = checked
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        Label {
                            text: qsTr("输出格式")
                            color: themeController.palette.textSecondary
                        }

                        ComboBox {
                            id: outputFormatBox
                            model: root.outputFormatItems
                            textRole: "label"
                            currentIndex: root.outputFormatIndex()
                            onActivated: captureController.outputFormat = root.outputFormatItems[currentIndex].value
                        }

                        Label {
                            text: qsTr("DPI")
                            color: themeController.palette.textSecondary
                        }

                        ComboBox {
                            id: dpiBox
                            model: root.dpiItems
                            textRole: "label"
                            currentIndex: root.dpiIndex()
                            onActivated: captureController.dpiValue = root.dpiItems[currentIndex].value
                        }

                        Label {
                            text: qsTr("DPI 只影响部分文档/打印软件中的显示尺寸，不提升像素细节。")
                            color: themeController.palette.textMuted
                            wrapMode: Text.Wrap
                            Layout.fillWidth: true
                        }
                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        visible: captureController.outputFormat === "jpg"

                        Label {
                            text: qsTr("JPG 质量")
                            color: themeController.palette.textSecondary
                        }

                        SpinBox {
                            from: 80
                            to: 100
                            value: captureController.jpegQuality
                            editable: true
                            onValueModified: captureController.jpegQuality = value
                        }

                        Label {
                            text: qsTr("%1").arg(captureController.jpegQuality)
                            color: themeController.palette.textMuted
                        }

                        Item { Layout.fillWidth: true }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: qsTr("高级导出")
                            color: themeController.palette.textSecondary
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: themeController.palette.spacingMd

                            Label {
                                text: qsTr("输出倍率")
                                color: themeController.palette.textSecondary
                            }

                            SpinBox {
                                from: 50
                                to: 200
                                value: captureController.scalePercent
                                stepSize: 10
                                editable: true
                                onValueModified: captureController.scalePercent = value
                            }

                            Label {
                                text: qsTr("%1%").arg(captureController.scalePercent)
                                color: themeController.palette.textMuted
                            }

                            Item { Layout.fillWidth: true }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("输出倍率会改变导出像素尺寸，不会提升截图清晰度。推荐保持 100%。")
                            color: themeController.palette.textMuted
                            wrapMode: Text.Wrap
                            font.pixelSize: 12
                        }

                        Item { Layout.fillWidth: true }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: qsTr("标注默认样式")
                            color: themeController.palette.textSecondary
                        }

                        RowLayout {
                            Layout.fillWidth: true
                            spacing: themeController.palette.spacingMd

                            Label {
                                text: qsTr("线宽")
                                color: themeController.palette.textMuted
                            }

                            SpinBox {
                                from: 1
                                to: 12
                                value: captureController.annotationLineWidth
                                editable: true
                                onValueModified: captureController.annotationLineWidth = value
                            }

                            Label {
                                text: qsTr("字号")
                                color: themeController.palette.textMuted
                            }

                            SpinBox {
                                from: 12
                                to: 96
                                value: captureController.annotationTextSize
                                editable: true
                                onValueModified: captureController.annotationTextSize = value
                            }

                            Label {
                                text: qsTr("马赛克")
                                color: themeController.palette.textMuted
                            }

                            SpinBox {
                                from: 4
                                to: 64
                                value: captureController.annotationMosaicBlockSize
                                editable: true
                                onValueModified: captureController.annotationMosaicBlockSize = value
                            }

                            Label {
                                text: qsTr("颜色")
                                color: themeController.palette.textMuted
                            }

                            ComboBox {
                                id: colorIndexBox
                                model: [qsTr("红"), qsTr("橙"), qsTr("蓝"), qsTr("绿"), qsTr("白")]
                                currentIndex: Math.max(0, Math.min(4, captureController.annotationColorIndex))
                                onActivated: captureController.annotationColorIndex = currentIndex
                            }

                            Item { Layout.fillWidth: true }
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: qsTr("标注快捷键（可自定义）")
                            color: themeController.palette.textSecondary
                        }

                        Flow {
                            Layout.fillWidth: true
                            spacing: themeController.palette.spacingMd

                            Repeater {
                                model: root.annotationShortcutItems

                                delegate: Column {
                                    id: shortcutItem
                                    width: modelData.preferredWidth
                                    spacing: 4
                                    property string conflictActionKey: ""

                                    Label {
                                        text: modelData.title
                                        color: themeController.palette.textMuted
                                        font.pixelSize: 12
                                    }

                                    TextField {
                                        id: shortcutField
                                        width: parent.width
                                        readOnly: true
                                        selectByMouse: false
                                        focusPolicy: Qt.StrongFocus
                                        placeholderText: qsTr("点击后按键录制")
                                        text: {
                                            root.shortcutRevision
                                            return root.shortcutText(modelData.actionKey)
                                        }

                                        onActiveFocusChanged: {
                                            if (!activeFocus) {
                                                shortcutItem.conflictActionKey = ""
                                            }
                                        }

                                        Keys.onPressed: function(event) {
                                            event.accepted = true

                                            if (event.key === Qt.Key_Escape) {
                                                shortcutItem.conflictActionKey = ""
                                                shortcutField.focus = false
                                                return
                                            }

                                            if (event.key === Qt.Key_Backspace || event.key === Qt.Key_Delete) {
                                                root.applyRecordedShortcut(modelData.actionKey, "")
                                                shortcutItem.conflictActionKey = ""
                                                shortcutField.focus = false
                                                return
                                            }

                                            if (captureController.isModifierOnlyKey(event.key)) {
                                                return
                                            }

                                            const portable = captureController.shortcutPortableFromKeyEvent(event.key, event.modifiers)
                                            if (portable.length === 0) {
                                                return
                                            }

                                            const conflict = captureController.annotationShortcutConflictAction(
                                                portable,
                                                modelData.actionKey)
                                            shortcutItem.conflictActionKey = conflict

                                            if (conflict.length === 0) {
                                                root.applyRecordedShortcut(modelData.actionKey, portable)
                                                shortcutField.focus = false
                                            }
                                        }

                                        background: Rectangle {
                                            radius: themeController.palette.radiusCard
                                            color: themeController.palette.bgPanel
                                            border.width: 1
                                            border.color: shortcutItem.conflictActionKey.length > 0
                                                  ? "#ff7a7a"
                                                  : (shortcutField.activeFocus
                                                     ? themeController.palette.accentPrimary
                                                     : themeController.palette.borderDefault)
                                        }
                                    }

                                    Label {
                                        visible: shortcutItem.conflictActionKey.length > 0
                                        width: parent.width
                                        text: qsTr("与 %1 冲突")
                                              .arg(captureController.annotationShortcutActionLabel(shortcutItem.conflictActionKey))
                                        color: "#ff8a8a"
                                        font.pixelSize: 11
                                        wrapMode: Text.Wrap
                                    }
                                }
                            }
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("录制方式：点击输入框后直接按键。Delete/Backspace 恢复默认，Esc 取消录制。")
                            color: themeController.palette.textMuted
                            wrapMode: Text.Wrap
                            font.pixelSize: 12
                        }
                    }

                    ColumnLayout {
                        Layout.fillWidth: true
                        spacing: 6

                        Label {
                            text: qsTr("保存目录")
                            color: themeController.palette.textSecondary
                        }

                        TextField {
                            id: outputDirField
                            Layout.fillWidth: true
                            text: captureController.outputDirectory
                            placeholderText: captureController.defaultOutputDirectory
                            onEditingFinished: captureController.outputDirectory = text
                        }

                        Label {
                            Layout.fillWidth: true
                            text: qsTr("留空时使用默认目录：%1").arg(captureController.defaultOutputDirectory)
                            color: themeController.palette.textMuted
                            wrapMode: Text.Wrap
                            font.pixelSize: 12
                        }

                    }

                    RowLayout {
                        Layout.fillWidth: true
                        spacing: themeController.palette.spacingMd

                        Button {
                            text: qsTr("恢复默认目录")
                            onClicked: captureController.useDefaultOutputDirectory()
                        }

                        Button {
                            text: qsTr("打开当前目录")
                            onClicked: captureController.openOutputDirectory()
                        }

                        Item { Layout.fillWidth: true }
                    }
                }
            }

            BentoCard {
                Layout.fillWidth: true
                title: qsTr("最近结果")
                description: qsTr("显示最近一次截图输出路径。")
                meta: "Result"
                variant: "quiet"

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 10

                    Label {
                        Layout.fillWidth: true
                        text: captureController.lastCapturePath.length > 0
                              ? captureController.lastCapturePath
                              : qsTr("暂无截图记录")
                        color: captureController.lastCapturePath.length > 0
                               ? themeController.palette.textPrimary
                               : themeController.palette.textMuted
                        wrapMode: Text.WrapAnywhere
                    }
                }
            }

            Connections {
                target: captureController

                function onOutputDirectoryChanged() {
                    outputDirField.text = captureController.outputDirectory
                }

                function onAnnotationOptionsChanged() {
                    root.shortcutRevision += 1
                }
            }
        }
    }
}
