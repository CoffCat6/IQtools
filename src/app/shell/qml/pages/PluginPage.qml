import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: themeController.palette.spacingMd

        Label {
            text: "插件管理"
            color: themeController.palette.textPrimary
            font.pixelSize: 30
            font.bold: true
        }

        Label {
            text: "插件系统后续将接入列表、启停状态与插件能力详情。"
            color: themeController.palette.textSecondary
            wrapMode: Text.Wrap
        }
    }
}
