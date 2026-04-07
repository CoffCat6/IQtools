import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: themeController.palette.spacingMd

        Label {
            text: "翻译工具"
            color: themeController.palette.textPrimary
            font.pixelSize: 30
            font.bold: true
        }

        Label {
            text: "统一翻译流程 UI 已预留，后续可接入多个 provider。"
            color: themeController.palette.textSecondary
            wrapMode: Text.Wrap
        }
    }
}
