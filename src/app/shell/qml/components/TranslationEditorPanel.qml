import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

FocusScope {
    id: root

    property string title: ""
    property string subtitle: ""
    property string placeholderText: ""
    property string helperText: ""
    property string badgeText: ""
    property string textValue: ""
    property bool readOnly: false
    property bool panelEnabled: true
    property int minimumEditorHeight: 220
    property bool syncInProgress: false

    signal textEdited(string text)

    implicitWidth: Math.max(420, contentColumn.implicitWidth + 28)
    implicitHeight: Math.max(minimumEditorHeight + 104, contentColumn.implicitHeight + 28)

    onTextValueChanged: {
        if (syncInProgress || editor.text === textValue) {
            return
        }
        syncInProgress = true
        editor.text = textValue
        syncInProgress = false
    }

    Rectangle {
        id: panel

        anchors.fill: parent
        radius: themeController.palette.radiusCard
        color: themeController.palette.bgCardHighlight
        border.width: 1
        border.color: editor.activeFocus
                      ? themeController.palette.accentPrimary
                      : themeController.palette.borderDefault

        Behavior on border.color {
            ColorAnimation {
                duration: 160
                easing.type: Easing.OutQuad
            }
        }

        ColumnLayout {
            id: contentColumn

            anchors.fill: parent
            anchors.margins: 14
            spacing: 10

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                ColumnLayout {
                    Layout.fillWidth: true
                    spacing: 2

                    Label {
                        text: root.title
                        color: themeController.palette.textPrimary
                        font.pixelSize: 16
                        font.bold: true
                        font.family: "Segoe UI Variable Display"
                    }

                    Label {
                        visible: root.subtitle.length > 0
                        text: root.subtitle
                        color: themeController.palette.textMuted
                        font.pixelSize: 12
                        font.family: "Segoe UI"
                        wrapMode: Text.Wrap
                    }
                }

                Rectangle {
                    visible: root.badgeText.length > 0
                    radius: 12
                    color: Qt.rgba(0, 0, 0, themeController.dark ? 0.16 : 0.05)
                    border.width: 1
                    border.color: themeController.palette.borderDefault
                    implicitHeight: 24
                    implicitWidth: badgeLabel.implicitWidth + 18

                    Label {
                        id: badgeLabel

                        anchors.centerIn: parent
                        text: root.badgeText
                        color: themeController.palette.accentPrimary
                        font.pixelSize: 11
                        font.bold: true
                        font.family: "Segoe UI"
                    }
                }
            }

            Rectangle {
                Layout.fillWidth: true
                Layout.fillHeight: true
                Layout.minimumHeight: root.minimumEditorHeight
                Layout.preferredHeight: root.minimumEditorHeight
                implicitHeight: root.minimumEditorHeight
                radius: themeController.palette.radiusCard - 4
                color: themeController.palette.bgPanel
                border.width: 1
                border.color: themeController.palette.borderDefault
                clip: true

                ScrollView {
                    id: editorScrollView

                    anchors.fill: parent
                    anchors.margins: 12
                    clip: true
                    ScrollBar.horizontal.policy: ScrollBar.AsNeeded
                    ScrollBar.vertical.policy: ScrollBar.AsNeeded

                    background: null

                    TextArea {
                        id: editor

                        width: editorScrollView.availableWidth
                        readOnly: root.readOnly
                        enabled: root.panelEnabled
                        color: root.panelEnabled
                               ? themeController.palette.textPrimary
                               : themeController.palette.textMuted
                        font.pixelSize: 15
                        font.family: "Segoe UI"
                        wrapMode: TextArea.Wrap
                        placeholderText: root.placeholderText
                        placeholderTextColor: themeController.palette.textMuted
                        selectByMouse: true
                        activeFocusOnPress: true
                        persistentSelection: true
                        padding: 0
                        background: null

                        Component.onCompleted: {
                            syncInProgress = true
                            text = root.textValue
                            syncInProgress = false
                        }

                        onTextChanged: {
                            if (!root.syncInProgress) {
                                root.textEdited(text)
                            }
                        }
                    }
                }
            }

            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    Layout.fillWidth: true
                    visible: root.helperText.length > 0
                    text: root.helperText
                    color: themeController.palette.textMuted
                    font.pixelSize: 11
                    font.family: "Segoe UI"
                    wrapMode: Text.Wrap
                }

                Rectangle {
                    radius: 12
                    color: Qt.rgba(0, 0, 0, themeController.dark ? 0.16 : 0.05)
                    border.width: 1
                    border.color: themeController.palette.borderDefault
                    implicitHeight: 24
                    implicitWidth: countLabel.implicitWidth + 18

                    Label {
                        id: countLabel

                        anchors.centerIn: parent
                        text: editor.length + " 字"
                        color: themeController.palette.textMuted
                        font.pixelSize: 11
                        font.bold: true
                        font.family: "Segoe UI"
                    }
                }
            }
        }

        TapHandler {
            acceptedButtons: Qt.LeftButton
            onTapped: {
                if (!root.readOnly && root.panelEnabled) {
                    editor.forceActiveFocus()
                }
            }
        }
    }
}
