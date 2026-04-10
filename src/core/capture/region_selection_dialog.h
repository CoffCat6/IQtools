#pragma once

#include <QtCore/QPoint>
#include <QtCore/QRect>
#include <QtGui/QPixmap>
#include <QtWidgets/QDialog>

namespace iqtools::core {

class RegionSelectionDialog final : public QDialog {
public:
    RegionSelectionDialog(const QPixmap& screenshot,
                          const QRect& virtualGeometry,
                          QWidget* parent = nullptr);

    QRect selectedRegion() const;
    bool hasSelection() const;

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void keyPressEvent(QKeyEvent* event) override;

private:
    enum class DragMode {
        None,
        Creating,
        Moving,
        ResizeTopLeft,
        ResizeTop,
        ResizeTopRight,
        ResizeRight,
        ResizeBottomRight,
        ResizeBottom,
        ResizeBottomLeft,
        ResizeLeft,
    };

    DragMode hitTest(const QPoint& pos) const;
    void updateSelection(const QPoint& pos);
    void updateCursor(const QPoint& pos);
    void normalizeAndClampSelection();

    static QRect anchorRect(const QPoint& center, int radius);
    static QRect boundedRect(const QRect& source, const QRect& bounds);

private:
    QPixmap m_screenshot;
    QRect m_selection;
    QRect m_selectionOnPress;
    QPoint m_pressPos;
    DragMode m_dragMode {DragMode::None};
    bool m_hasSelection {false};
};

}  // namespace iqtools::core
