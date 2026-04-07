import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Item {
    id: root

    ColumnLayout {
        anchors.fill: parent
        spacing: themeController.palette.spacingMd

        // ── Header ──
        RowLayout {
            Layout.fillWidth: true
            spacing: themeController.palette.spacingMd

            Label {
                text: "实时日志"
                color: themeController.palette.textPrimary
                font.pixelSize: 30
                font.bold: true
                font.family: "Segoe UI Variable Display"
            }

            Item { Layout.fillWidth: true }

            // Level filter
            Rectangle {
                Layout.preferredHeight: 32
                Layout.preferredWidth: levelRow.implicitWidth + 20
                radius: 16
                color: Qt.rgba(0, 0, 0, themeController.dark ? 0.22 : 0.05)
                border.width: 1
                border.color: themeController.palette.borderDefault

                RowLayout {
                    id: levelRow
                    anchors.centerIn: parent
                    spacing: 4

                    Repeater {
                        model: [
                            { label: "ALL", level: 0 },
                            { label: "INFO", level: 1 },
                            { label: "WARN", level: 2 },
                            { label: "ERR", level: 3 }
                        ]

                        delegate: Rectangle {
                            required property var modelData
                            width: filterLabel.implicitWidth + 14
                            height: 24
                            radius: 12
                            color: logConsole.filterLevel === modelData.level
                                   ? themeController.palette.accentPrimary
                                   : "transparent"

                            Label {
                                id: filterLabel
                                anchors.centerIn: parent
                                text: modelData.label
                                color: logConsole.filterLevel === modelData.level
                                       ? "#ffffff"
                                       : themeController.palette.textMuted
                                font.pixelSize: 11
                                font.bold: true
                                font.family: "Segoe UI"
                            }

                            MouseArea {
                                anchors.fill: parent
                                cursorShape: Qt.PointingHandCursor
                                onClicked: logConsole.filterLevel = modelData.level
                            }
                        }
                    }
                }
            }

            // Pause / Resume
            Rectangle {
                Layout.preferredHeight: 32
                Layout.preferredWidth: pauseLabel.implicitWidth + 28
                radius: 16
                color: logConsole.paused
                       ? Qt.rgba(255/255, 138/255, 138/255, themeController.dark ? 0.18 : 0.12)
                       : Qt.rgba(0, 0, 0, themeController.dark ? 0.22 : 0.05)
                border.width: 1
                border.color: logConsole.paused
                              ? Qt.rgba(255/255, 138/255, 138/255, 0.4)
                              : themeController.palette.borderDefault

                Behavior on color { ColorAnimation { duration: 200 } }

                Label {
                    id: pauseLabel
                    anchors.centerIn: parent
                    text: logConsole.paused ? "▶ 继续" : "⏸ 暂停"
                    color: logConsole.paused
                           ? "#ff8a8a"
                           : themeController.palette.textSecondary
                    font.pixelSize: 12
                    font.bold: true
                    font.family: "Segoe UI"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: logConsole.paused = !logConsole.paused
                }
            }

            // Clear
            Rectangle {
                Layout.preferredHeight: 32
                Layout.preferredWidth: clearLabel.implicitWidth + 28
                radius: 16
                color: Qt.rgba(0, 0, 0, themeController.dark ? 0.22 : 0.05)
                border.width: 1
                border.color: themeController.palette.borderDefault

                Label {
                    id: clearLabel
                    anchors.centerIn: parent
                    text: "清空"
                    color: themeController.palette.textSecondary
                    font.pixelSize: 12
                    font.bold: true
                    font.family: "Segoe UI"
                }

                MouseArea {
                    anchors.fill: parent
                    cursorShape: Qt.PointingHandCursor
                    onClicked: logConsole.clear()
                }
            }

            // Entry count badge
            Rectangle {
                Layout.preferredHeight: 24
                Layout.preferredWidth: countLabel.implicitWidth + 16
                radius: 12
                color: Qt.rgba(0, 0, 0, themeController.dark ? 0.16 : 0.04)
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, themeController.dark ? 0.18 : 0.35)

                Label {
                    id: countLabel
                    anchors.centerIn: parent
                    text: logConsole.entries.length + " 条"
                    color: themeController.palette.textMuted
                    font.pixelSize: 11
                    font.bold: true
                    font.family: "Segoe UI"
                }
            }
        }

        // ── Console area ──
        Rectangle {
            id: consoleArea

            // Theme-aware console palette
            readonly property color consoleBg: themeController.dark ? "#0a0f1a" : "#f7f8fa"
            readonly property color consoleBorder: themeController.dark ? "#1e2a42" : "#d9e1ef"
            readonly property color consoleTextDefault: themeController.dark ? "#c8d0e0" : "#2a3548"
            readonly property color consoleTextMuted: themeController.dark ? Qt.rgba(1, 1, 1, 0.25) : Qt.rgba(0, 0, 0, 0.28)
            readonly property color consoleRowAlt: themeController.dark ? Qt.rgba(1, 1, 1, 0.015) : Qt.rgba(0, 0, 0, 0.018)
            readonly property color scrollThumb: themeController.dark ? Qt.rgba(1, 1, 1, 0.15) : Qt.rgba(0, 0, 0, 0.18)

            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: themeController.palette.radiusCard
            color: consoleBg
            border.width: 1
            border.color: consoleBorder
            clip: true

            Behavior on color { ColorAnimation { duration: 260; easing.type: Easing.OutCubic } }
            Behavior on border.color { ColorAnimation { duration: 260; easing.type: Easing.OutCubic } }

            // Inner glow top edge
            Rectangle {
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 1
                height: 40
                z: 2

                gradient: Gradient {
                    GradientStop { position: 0.0; color: Qt.rgba(consoleArea.consoleBg.r, consoleArea.consoleBg.g, consoleArea.consoleBg.b, 0.85) }
                    GradientStop { position: 1.0; color: "transparent" }
                }
            }

            ListView {
                id: logList
                anchors.fill: parent
                anchors.margins: 12
                model: logConsole.entries
                clip: true
                boundsBehavior: Flickable.StopAtBounds
                spacing: 1

                // Auto-scroll to bottom
                property bool autoScroll: true

                onContentHeightChanged: {
                    if (autoScroll && !logConsole.paused) {
                        positionViewAtEnd()
                    }
                }

                onMovingChanged: {
                    if (moving) {
                        autoScroll = false
                    }
                }

                // Detect if scrolled to bottom to re-enable auto-scroll
                onContentYChanged: {
                    if (!moving) return
                    const atBottom = (contentY + height + 30) >= contentHeight
                    if (atBottom) {
                        autoScroll = true
                    }
                }

                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                    contentItem: Rectangle {
                        implicitWidth: 6
                        radius: 3
                        color: consoleArea.scrollThumb
                    }
                }

                delegate: Rectangle {
                    required property int index
                    required property string modelData
                    width: logList.width
                    height: logText.implicitHeight + 6
                    radius: 4
                    color: {
                        if (modelData.indexOf("[WARNING]") >= 0) {
                            return Qt.rgba(255/255, 193/255, 7/255, themeController.dark ? 0.06 : 0.10)
                        }
                        if (modelData.indexOf("[CRITICAL]") >= 0 || modelData.indexOf("[FATAL]") >= 0) {
                            return Qt.rgba(244/255, 67/255, 54/255, themeController.dark ? 0.08 : 0.10)
                        }
                        return index % 2 === 0 ? "transparent" : consoleArea.consoleRowAlt
                    }

                    Text {
                        id: logText
                        anchors.left: parent.left
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        anchors.leftMargin: 8
                        anchors.rightMargin: 8
                        text: formatLogLine(modelData)
                        textFormat: Text.RichText
                        wrapMode: Text.WrapAnywhere
                        font.pixelSize: 12
                        font.family: "Cascadia Code, Consolas, monospace"
                        lineHeight: 1.4
                    }

                    // Appear animation
                    opacity: 0
                    Component.onCompleted: opacity = 1
                    Behavior on opacity {
                        NumberAnimation { duration: 150; easing.type: Easing.OutQuad }
                    }
                }

                // Empty state
                Label {
                    anchors.centerIn: parent
                    visible: logConsole.entries.length === 0
                    text: "暂无日志\n操作应用以产生日志输出"
                    color: consoleArea.consoleTextMuted
                    font.pixelSize: 16
                    font.family: "Segoe UI"
                    horizontalAlignment: Text.AlignHCenter
                    lineHeight: 1.6
                }
            }

            // Scroll-to-bottom FAB
            Rectangle {
                visible: !logList.autoScroll && logConsole.entries.length > 0
                anchors.right: parent.right
                anchors.bottom: parent.bottom
                anchors.margins: 20
                width: 42
                height: 42
                radius: 21
                color: themeController.palette.accentPrimary
                opacity: fabArea.containsMouse ? 1.0 : 0.8
                z: 10

                Behavior on opacity {
                    NumberAnimation { duration: 120 }
                }

                Label {
                    anchors.centerIn: parent
                    text: "↓"
                    color: "#ffffff"
                    font.pixelSize: 20
                    font.bold: true
                }

                MouseArea {
                    id: fabArea
                    anchors.fill: parent
                    hoverEnabled: true
                    cursorShape: Qt.PointingHandCursor
                    onClicked: {
                        logList.autoScroll = true
                        logList.positionViewAtEnd()
                    }
                }
            }

            // Paused overlay indicator
            Rectangle {
                visible: logConsole.paused
                anchors.top: parent.top
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.topMargin: 10
                width: pausedIndicatorLabel.implicitWidth + 24
                height: 28
                radius: 14
                color: Qt.rgba(255/255, 138/255, 138/255, 0.15)
                border.width: 1
                border.color: Qt.rgba(255/255, 138/255, 138/255, 0.3)
                z: 10

                Label {
                    id: pausedIndicatorLabel
                    anchors.centerIn: parent
                    text: "⏸ 日志已暂停"
                    color: "#ff8a8a"
                    font.pixelSize: 12
                    font.bold: true
                    font.family: "Segoe UI"
                }

                SequentialAnimation on opacity {
                    loops: Animation.Infinite
                    NumberAnimation { from: 1.0; to: 0.5; duration: 1200; easing.type: Easing.InOutSine }
                    NumberAnimation { from: 0.5; to: 1.0; duration: 1200; easing.type: Easing.InOutSine }
                }
            }
        }
    }

    // ── Theme-aware syntax coloring ──
    readonly property color tsColor: themeController.dark ? "#5a7aa8" : "#6a8ab8"
    readonly property color debugColor: themeController.dark ? "#8291ad" : "#7f90ac"
    readonly property color infoColor: themeController.dark ? "#6ee7d8" : "#0e967e"
    readonly property color warnColor: themeController.dark ? "#ffc107" : "#b58900"
    readonly property color critColor: themeController.dark ? "#f44336" : "#c62828"
    readonly property color fatalColor: themeController.dark ? "#ff1744" : "#b71c1c"
    readonly property color defaultTextColor: themeController.dark ? "#c8d0e0" : "#2a3548"

    function formatLogLine(line) {
        let result = escapeHtml(line)

        // Wrap the whole line in the default text color
        result = '<span style="color: ' + defaultTextColor + '">' + result + '</span>'

        // Colorize timestamp [2026-04-07 ...]
        result = result.replace(
            /\[(\d{4}-\d{2}-\d{2}\s+\d{2}:\d{2}:\d{2}\.\d+)\]/,
            '<span style="color: ' + tsColor + '">[$1]</span>'
        )

        // Colorize level
        result = result.replace(/\[DEBUG\]/, '<span style="color: ' + debugColor + '">[DEBUG]</span>')
        result = result.replace(/\[INFO\]/, '<span style="color: ' + infoColor + '">[INFO]</span>')
        result = result.replace(/\[WARNING\]/, '<span style="color: ' + warnColor + '">[WARNING]</span>')
        result = result.replace(/\[CRITICAL\]/, '<span style="color: ' + critColor + '; font-weight: bold">[CRITICAL]</span>')
        result = result.replace(/\[FATAL\]/, '<span style="color: ' + fatalColor + '; font-weight: bold">[FATAL]</span>')

        // Colorize category [xxx|yyy]
        result = result.replace(
            /\[([^\]]+\|[^\]]+)\]/,
            '<span style="color: ' + themeController.palette.accentPrimary + '">[$1]</span>'
        )

        return result
    }

    function escapeHtml(text) {
        return text.replace(/&/g, '&amp;').replace(/</g, '&lt;').replace(/>/g, '&gt;')
    }
}
