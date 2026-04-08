import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Item {
    id: root

    default property alias cardContent: contentContainer.data

    property string title: ""
    property string description: ""
    property string meta: ""
    property string variant: "default"
    property bool clickable: false
    property int revealDelay: 0
    property bool revealOnLoad: true
    property int radius: themeController.palette.radiusCard

    readonly property bool hovered: clickable && hitArea.containsMouse
    readonly property bool pressed: clickable && hitArea.pressed
    readonly property bool hasCustomContent: contentContainer.children.length > 0
    readonly property color baseColor: {
        if (variant === "highlight") {
            return themeController.palette.bgCardHighlight
        }
        if (variant === "accent") {
            return Qt.tint(themeController.palette.bgCardHighlight,
                           Qt.rgba(124 / 255, 156 / 255, 1.0, themeController.dark ? 0.22 : 0.14))
        }
        if (variant === "quiet") {
            return Qt.tint(themeController.palette.bgCard,
                           Qt.rgba(1, 1, 1, themeController.dark ? 0.03 : 0.36))
        }
        return themeController.palette.bgCard
    }

    implicitWidth: 320
    implicitHeight: Math.max(186, contentColumn.implicitHeight + themeController.palette.spacingLg * 2)
    opacity: revealOnLoad ? 0 : 1

    signal clicked()

    Rectangle {
        id: shadowLayer

        anchors {
            fill: surface
            topMargin: root.pressed ? 8 : (root.hovered ? 4 : 6)
            leftMargin: root.hovered ? 1 : 0
            rightMargin: root.hovered ? -1 : 0
        }

        radius: surface.radius + 2
        color: themeController.palette.shadowColor
        opacity: root.hovered ? (themeController.dark ? 0.72 : 0.25) : (themeController.dark ? 0.52 : 0.18)
        z: -1

        Behavior on opacity {
            NumberAnimation {
                duration: 180
                easing.type: Easing.OutQuad
            }
        }
    }

    Rectangle {
        id: surface

        anchors.fill: parent
        radius: root.radius
        antialiasing: true
        clip: true
        color: root.pressed
               ? Qt.darker(root.baseColor, themeController.dark ? 1.16 : 1.05)
               : (root.hovered ? Qt.lighter(root.baseColor, themeController.dark ? 1.08 : 1.025) : root.baseColor)
        border.width: root.variant === "accent" ? 1.3 : 1
        border.color: root.hovered
                      ? Qt.lighter(themeController.palette.borderDefault, themeController.dark ? 1.24 : 1.06)
                      : themeController.palette.borderDefault

        Behavior on color {
            ColorAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }

        Behavior on border.color {
            ColorAnimation {
                duration: 180
                easing.type: Easing.OutCubic
            }
        }

        transform: Scale {
            origin.x: surface.width / 2
            origin.y: surface.height / 2
            xScale: root.pressed ? 0.986 : (root.hovered ? 1.012 : 1.0)
            yScale: root.pressed ? 0.986 : (root.hovered ? 1.012 : 1.0)

            Behavior on xScale {
                NumberAnimation {
                    duration: 160
                    easing.type: Easing.OutQuad
                }
            }

            Behavior on yScale {
                NumberAnimation {
                    duration: 160
                    easing.type: Easing.OutQuad
                }
            }
        }

        Rectangle {
            anchors.fill: parent
            radius: parent.radius
            color: "transparent"
            opacity: root.variant === "quiet" ? 0.2 : 0.42

            gradient: Gradient {
                orientation: Gradient.Vertical
                GradientStop { position: 0.0; color: Qt.rgba(1, 1, 1, themeController.dark ? 0.11 : 0.56) }
                GradientStop { position: 0.55; color: Qt.rgba(1, 1, 1, 0.0) }
                GradientStop { position: 1.0; color: Qt.rgba(0, 0, 0, themeController.dark ? 0.14 : 0.03) }
            }
        }

        Rectangle {
            width: Math.max(120, parent.width * 0.42)
            height: width
            radius: width / 2
            anchors {
                right: parent.right
                top: parent.top
                rightMargin: -width * 0.35
                topMargin: -width * 0.36
            }
            color: variant === "accent"
                   ? Qt.rgba(110 / 255, 231 / 255, 216 / 255, themeController.dark ? 0.20 : 0.16)
                   : Qt.rgba(124 / 255, 156 / 255, 1.0, themeController.dark ? 0.16 : 0.11)
            opacity: root.variant === "default" ? 0.75 : 1.0
        }

        ColumnLayout {
            id: contentColumn

            anchors.fill: parent
            anchors.margins: themeController.palette.spacingLg
            spacing: 10

            Rectangle {
                Layout.alignment: Qt.AlignLeft
                Layout.preferredHeight: 24
                radius: 12
                color: Qt.rgba(0, 0, 0, themeController.dark ? 0.16 : 0.04)
                border.width: 1
                border.color: Qt.rgba(1, 1, 1, themeController.dark ? 0.18 : 0.35)
                visible: root.meta.length > 0

                implicitWidth: metaLabel.implicitWidth + 18

                Label {
                    id: metaLabel

                    anchors.centerIn: parent
                    text: root.meta
                    color: root.variant === "accent" ? themeController.palette.accentSecondary : themeController.palette.accentPrimary
                    font.pixelSize: 11
                    font.bold: true
                    font.family: "Segoe UI"
                }
            }

            Label {
                text: root.title
                color: themeController.palette.textPrimary
                font.pixelSize: 23
                font.bold: true
                font.family: "Segoe UI Variable Display"
                wrapMode: Text.Wrap
                Layout.fillWidth: true
            }

            Label {
                id: descriptionLabel

                Layout.fillWidth: true
                Layout.fillHeight: visible && !root.hasCustomContent
                text: root.description
                color: themeController.palette.textSecondary
                wrapMode: Text.Wrap
                lineHeight: 1.18
                font.pixelSize: 14
                font.family: "Segoe UI"
                visible: root.description.length > 0
            }

            Item {
                id: contentContainer

                Layout.fillWidth: true
                Layout.fillHeight: root.hasCustomContent
                implicitHeight: root.hasCustomContent ? childrenRect.height : 0
                visible: root.hasCustomContent
            }

            RowLayout {
                Layout.fillWidth: true
                visible: root.clickable

                Item { Layout.fillWidth: true }

                Label {
                    text: root.pressed ? "正在打开" : "点击进入"
                    color: themeController.palette.textMuted
                    font.pixelSize: 12
                    font.bold: true
                    font.family: "Segoe UI"
                }
            }
        }

        MouseArea {
            id: hitArea

            anchors.fill: parent
            enabled: root.clickable
            hoverEnabled: root.clickable
            cursorShape: root.clickable ? Qt.PointingHandCursor : Qt.ArrowCursor
            onClicked: root.clicked()
        }
    }

    SequentialAnimation {
        id: revealAnimation

        running: root.revealOnLoad
        PauseAnimation { duration: root.revealDelay }

        ParallelAnimation {
            NumberAnimation {
                target: root
                property: "opacity"
                from: 0
                to: 1
                duration: 420
                easing.type: Easing.OutCubic
            }

            NumberAnimation {
                target: surface
                property: "y"
                from: 12
                to: 0
                duration: 500
                easing.type: Easing.OutCubic
            }
        }
    }
}
