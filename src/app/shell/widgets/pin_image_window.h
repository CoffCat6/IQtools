#pragma once

#include <QtCore/QPoint>
#include <QtGui/QImage>
#include <QtWidgets/QWidget>

class QLabel;
class QMouseEvent;
class QResizeEvent;

namespace iqtools::app::widgets {

class PinImageWindow : public QWidget {
public:
    explicit PinImageWindow(QWidget* parent = nullptr);

    void setImage(const QImage& image);

protected:
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;

private:
    void updatePreview();

private:
    QImage m_image;
    QLabel* m_previewLabel {nullptr};
    QPoint m_dragStartGlobal;
    QPoint m_dragStartWindowTopLeft;
};

}  // namespace iqtools::app::widgets
