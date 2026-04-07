import QtQuick
import QtQuick.Controls
import QtQuick.Layouts
import IQtools

Item {
    ColumnLayout {
        anchors.fill: parent
        spacing: themeController.palette.spacingMd

        Label {
            text: "截图工具"
            color: themeController.palette.textPrimary
            font.pixelSize: 30
            font.bold: true
        }

        Label {
            text: "区域截图 / 全屏截图 / 结果处理链将在后续阶段接入。"
            color: themeController.palette.textSecondary
            wrapMode: Text.Wrap
        }
    }
}
