#pragma once

#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtCore/QString>
#include <QtCore/QVector>
#include <QtGui/QColor>
#include <QtGui/QImage>
#include <QtGui/QKeySequence>
#include <QtWidgets/QDialog>

namespace iqtools::core {

class AnnotationDialog final : public QDialog {
public:
    struct Options {
        int lineWidth {3};
        int textPixelSize {28};
        int mosaicBlockSize {10};
        int colorIndex {0};

        QString shortcutRectangle;
        QString shortcutArrow;
        QString shortcutMosaic;
        QString shortcutText;
        QString shortcutUndo;
        QString shortcutRedo;
        QString shortcutColorCycle;
    };

    enum class Tool {
        Rectangle,
        Arrow,
        Mosaic,
        Text,
    };

    explicit AnnotationDialog(const QImage& source,
                              QWidget* parent = nullptr);
    AnnotationDialog(const QImage& source,
                     const Options& options,
                     QWidget* parent = nullptr);

    QImage annotatedImage() const;
    Options options() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    enum class ToolbarAction {
        ToolRectangle,
        ToolArrow,
        ToolMosaic,
        ToolText,
        Undo,
        Redo,
        LineWidthDown,
        LineWidthUp,
        TextSizeDown,
        TextSizeUp,
        MosaicDown,
        MosaicUp,
        CycleColor,
    };

    struct ToolbarButton {
        QRect rect;
        QString label;
        ToolbarAction action;
        bool enabled {true};
        bool active {false};
    };

    QRect imageRect() const;
    QPoint widgetToImage(const QPoint& point) const;
    QPoint clampToImageRect(const QPoint& point) const;
    QColor currentColor() const;
    QString shortcutLabel(const QKeySequence& shortcut) const;
    void applyOptions(const Options& options);

    void drawArrow(QPainter& painter,
                   const QPoint& start,
                   const QPoint& end,
                   const QPen& pen) const;
    bool drawToolbarGlyph(QPainter& painter,
                          const ToolbarButton& button,
                          const QColor& glyphColor) const;
    void drawUndoRedoGlyph(QPainter& painter,
                           const QRectF& area,
                           bool redo,
                           const QColor& glyphColor) const;
    void drawToolbar(QPainter& painter);
    QVector<ToolbarButton> buildToolbarButtons() const;
    void handleToolbarClick(const QPoint& point);
    void applyToolbarAction(ToolbarAction action);
    void applyMosaic(const QRect& rect);
    void commitShape();
    void insertTextAt(const QPoint& widgetPoint);
    void pushUndoSnapshot();
    void undo();
    void redo();
    bool canUndo() const;
    bool canRedo() const;
    void adjustLineWidth(int delta);
    void adjustTextSize(int delta);
    void adjustMosaicBlockSize(int delta);
    void cycleColor();
    QString toolLabel() const;

private:
    QImage m_workingImage;

    Tool m_tool {Tool::Rectangle};
    bool m_dragging {false};
    QPoint m_startPoint;
    QPoint m_currentPoint;

    QVector<ToolbarButton> m_toolbarButtons;
    QVector<QImage> m_undoStack;
    QVector<QImage> m_redoStack;

    int m_historyLimit {30};
    int m_lineWidth {3};
    int m_textPixelSize {28};
    int m_mosaicBlockSize {10};
    int m_colorIndex {0};

    QKeySequence m_shortcutToolRectangle {QKeySequence(Qt::Key_1)};
    QKeySequence m_shortcutToolArrow {QKeySequence(Qt::Key_2)};
    QKeySequence m_shortcutToolMosaic {QKeySequence(Qt::Key_3)};
    QKeySequence m_shortcutToolText {QKeySequence(Qt::Key_4)};
    QKeySequence m_shortcutUndo {QKeySequence::Undo};
    QKeySequence m_shortcutRedo {QKeySequence::Redo};
    QKeySequence m_shortcutColorCycle {QKeySequence(Qt::Key_C)};
};

}  // namespace iqtools::core
