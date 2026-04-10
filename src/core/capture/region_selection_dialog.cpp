#include "region_selection_dialog.h"

#include <QtCore/QRectF>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtWidgets/QStyleOption>

namespace iqtools::core {

namespace {

constexpr int kAnchorRadius = 4;
constexpr int kSelectionMinSize = 2;

QRect normalizedRect(const QPoint& start, const QPoint& end)
{
    return QRect(start, end).normalized();
}

}  // namespace

RegionSelectionDialog::RegionSelectionDialog(const QPixmap& screenshot,
                                             const QRect& virtualGeometry,
                                             QWidget* parent)
    : QDialog(parent)
    , m_screenshot(screenshot)
{
    setWindowFlags(Qt::FramelessWindowHint | Qt::Dialog |
                   Qt::WindowStaysOnTopHint | Qt::Tool);
    setWindowModality(Qt::ApplicationModal);
    setAttribute(Qt::WA_TranslucentBackground, false);
    setAttribute(Qt::WA_DeleteOnClose, false);
    setMouseTracking(true);
    setCursor(Qt::CrossCursor);

    setGeometry(virtualGeometry);
    setFixedSize(virtualGeometry.size());
}

QRect RegionSelectionDialog::selectedRegion() const
{
    return m_selection;
}

bool RegionSelectionDialog::hasSelection() const
{
    return m_hasSelection && m_selection.isValid() && !m_selection.isEmpty();
}

void RegionSelectionDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);

    painter.drawPixmap(rect(), m_screenshot, m_screenshot.rect());
    painter.fillRect(rect(), QColor(0, 0, 0, 120));

    if (!hasSelection()) {
        return;
    }

    painter.drawPixmap(m_selection, m_screenshot, m_selection);
    painter.setPen(QPen(QColor(0, 174, 255), 1.5));
    painter.setBrush(Qt::NoBrush);
    painter.drawRect(m_selection.adjusted(0, 0, -1, -1));

    const QPoint topLeft = m_selection.topLeft();
    const QPoint topRight = m_selection.topRight();
    const QPoint bottomLeft = m_selection.bottomLeft();
    const QPoint bottomRight = m_selection.bottomRight();
    const QPoint topCenter((topLeft.x() + topRight.x()) / 2, topLeft.y());
    const QPoint rightCenter(topRight.x(), (topRight.y() + bottomRight.y()) / 2);
    const QPoint bottomCenter((bottomLeft.x() + bottomRight.x()) / 2, bottomRight.y());
    const QPoint leftCenter(topLeft.x(), (topLeft.y() + bottomLeft.y()) / 2);

    const QList<QPoint> anchors = {
        topLeft,
        topCenter,
        topRight,
        rightCenter,
        bottomRight,
        bottomCenter,
        bottomLeft,
        leftCenter,
    };

    painter.setPen(QPen(QColor(0, 120, 200), 1));
    painter.setBrush(QColor(255, 255, 255));
    for (const QPoint& anchor : anchors) {
        painter.drawRect(anchorRect(anchor, kAnchorRadius));
    }

    const QString sizeText = QStringLiteral("%1 x %2")
                                 .arg(m_selection.width())
                                 .arg(m_selection.height());
    const QFontMetrics metrics(font());
    const QSize textSize = metrics.size(Qt::TextSingleLine, sizeText);
    QRect textRect(m_selection.left(),
                   m_selection.top() - textSize.height() - 10,
                   textSize.width() + 14,
                   textSize.height() + 6);
    if (textRect.top() < 0) {
        textRect.moveTop(m_selection.top() + 6);
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(16, 24, 40, 210));
    painter.drawRoundedRect(textRect, 6, 6);
    painter.setPen(Qt::white);
    painter.drawText(textRect, Qt::AlignCenter, sizeText);
}

void RegionSelectionDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QDialog::mousePressEvent(event);
        return;
    }

    m_pressPos = event->pos();
    m_selectionOnPress = m_selection;

    if (hasSelection()) {
        const DragMode hit = hitTest(event->pos());
        if (hit == DragMode::Moving ||
            hit == DragMode::ResizeTopLeft ||
            hit == DragMode::ResizeTop ||
            hit == DragMode::ResizeTopRight ||
            hit == DragMode::ResizeRight ||
            hit == DragMode::ResizeBottomRight ||
            hit == DragMode::ResizeBottom ||
            hit == DragMode::ResizeBottomLeft ||
            hit == DragMode::ResizeLeft) {
            m_dragMode = hit;
            return;
        }
    }

    m_dragMode = DragMode::Creating;
    m_selection = QRect(m_pressPos, QSize(1, 1));
    m_hasSelection = true;
    update();
}

void RegionSelectionDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (!(event->buttons() & Qt::LeftButton) || m_dragMode == DragMode::None) {
        updateCursor(event->pos());
        return;
    }

    updateSelection(event->pos());
    normalizeAndClampSelection();
    update();
}

void RegionSelectionDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QDialog::mouseReleaseEvent(event);
        return;
    }

    normalizeAndClampSelection();
    if (m_selection.width() < kSelectionMinSize ||
        m_selection.height() < kSelectionMinSize) {
        m_selection = QRect();
        m_hasSelection = false;
    }

    m_dragMode = DragMode::None;
    updateCursor(event->pos());
    update();
}

void RegionSelectionDialog::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && hasSelection() &&
        m_selection.contains(event->pos())) {
        accept();
        return;
    }
    QDialog::mouseDoubleClickEvent(event);
}

void RegionSelectionDialog::keyPressEvent(QKeyEvent* event)
{
    if (event->key() == Qt::Key_Escape) {
        reject();
        return;
    }

    if (event->key() == Qt::Key_Return || event->key() == Qt::Key_Enter) {
        if (hasSelection()) {
            accept();
        } else {
            reject();
        }
        return;
    }

    QDialog::keyPressEvent(event);
}

RegionSelectionDialog::DragMode RegionSelectionDialog::hitTest(const QPoint& pos) const
{
    if (!hasSelection()) {
        return DragMode::None;
    }

    const QPoint topLeft = m_selection.topLeft();
    const QPoint topRight = m_selection.topRight();
    const QPoint bottomLeft = m_selection.bottomLeft();
    const QPoint bottomRight = m_selection.bottomRight();
    const QPoint topCenter((topLeft.x() + topRight.x()) / 2, topLeft.y());
    const QPoint rightCenter(topRight.x(), (topRight.y() + bottomRight.y()) / 2);
    const QPoint bottomCenter((bottomLeft.x() + bottomRight.x()) / 2, bottomRight.y());
    const QPoint leftCenter(topLeft.x(), (topLeft.y() + bottomLeft.y()) / 2);

    if (anchorRect(topLeft, kAnchorRadius).contains(pos)) {
        return DragMode::ResizeTopLeft;
    }
    if (anchorRect(topCenter, kAnchorRadius).contains(pos)) {
        return DragMode::ResizeTop;
    }
    if (anchorRect(topRight, kAnchorRadius).contains(pos)) {
        return DragMode::ResizeTopRight;
    }
    if (anchorRect(rightCenter, kAnchorRadius).contains(pos)) {
        return DragMode::ResizeRight;
    }
    if (anchorRect(bottomRight, kAnchorRadius).contains(pos)) {
        return DragMode::ResizeBottomRight;
    }
    if (anchorRect(bottomCenter, kAnchorRadius).contains(pos)) {
        return DragMode::ResizeBottom;
    }
    if (anchorRect(bottomLeft, kAnchorRadius).contains(pos)) {
        return DragMode::ResizeBottomLeft;
    }
    if (anchorRect(leftCenter, kAnchorRadius).contains(pos)) {
        return DragMode::ResizeLeft;
    }

    if (m_selection.contains(pos)) {
        return DragMode::Moving;
    }

    return DragMode::None;
}

void RegionSelectionDialog::updateSelection(const QPoint& pos)
{
    const QRect bounds = rect();
    const QPoint boundedPos = QPoint(
        qBound(bounds.left(), pos.x(), bounds.right()),
        qBound(bounds.top(), pos.y(), bounds.bottom()));

    switch (m_dragMode) {
    case DragMode::Creating:
        m_selection = normalizedRect(m_pressPos, boundedPos);
        break;
    case DragMode::Moving: {
        const QPoint delta = boundedPos - m_pressPos;
        m_selection = boundedRect(m_selectionOnPress.translated(delta), bounds);
        break;
    }
    case DragMode::ResizeTopLeft:
        m_selection = normalizedRect(boundedPos, m_selectionOnPress.bottomRight());
        break;
    case DragMode::ResizeTop:
        m_selection = normalizedRect(QPoint(m_selectionOnPress.left(), boundedPos.y()),
                                     m_selectionOnPress.bottomRight());
        break;
    case DragMode::ResizeTopRight:
        m_selection = normalizedRect(QPoint(m_selectionOnPress.left(), boundedPos.y()),
                                     QPoint(boundedPos.x(), m_selectionOnPress.bottom()));
        break;
    case DragMode::ResizeRight:
        m_selection = normalizedRect(m_selectionOnPress.topLeft(),
                                     QPoint(boundedPos.x(), m_selectionOnPress.bottom()));
        break;
    case DragMode::ResizeBottomRight:
        m_selection = normalizedRect(m_selectionOnPress.topLeft(), boundedPos);
        break;
    case DragMode::ResizeBottom:
        m_selection = normalizedRect(m_selectionOnPress.topLeft(),
                                     QPoint(m_selectionOnPress.right(), boundedPos.y()));
        break;
    case DragMode::ResizeBottomLeft:
        m_selection = normalizedRect(QPoint(boundedPos.x(), m_selectionOnPress.top()),
                                     QPoint(m_selectionOnPress.right(), boundedPos.y()));
        break;
    case DragMode::ResizeLeft:
        m_selection = normalizedRect(QPoint(boundedPos.x(), m_selectionOnPress.top()),
                                     m_selectionOnPress.bottomRight());
        break;
    case DragMode::None:
        break;
    }
}

void RegionSelectionDialog::updateCursor(const QPoint& pos)
{
    switch (hitTest(pos)) {
    case DragMode::ResizeTopLeft:
    case DragMode::ResizeBottomRight:
        setCursor(Qt::SizeFDiagCursor);
        break;
    case DragMode::ResizeTopRight:
    case DragMode::ResizeBottomLeft:
        setCursor(Qt::SizeBDiagCursor);
        break;
    case DragMode::ResizeTop:
    case DragMode::ResizeBottom:
        setCursor(Qt::SizeVerCursor);
        break;
    case DragMode::ResizeLeft:
    case DragMode::ResizeRight:
        setCursor(Qt::SizeHorCursor);
        break;
    case DragMode::Moving:
        setCursor(Qt::SizeAllCursor);
        break;
    case DragMode::Creating:
    case DragMode::None:
    default:
        setCursor(Qt::CrossCursor);
        break;
    }
}

void RegionSelectionDialog::normalizeAndClampSelection()
{
    if (!m_selection.isValid()) {
        return;
    }
    m_selection = m_selection.normalized();
    m_selection = m_selection.intersected(rect());
}

QRect RegionSelectionDialog::anchorRect(const QPoint& center, int radius)
{
    return QRect(center.x() - radius,
                 center.y() - radius,
                 radius * 2 + 1,
                 radius * 2 + 1);
}

QRect RegionSelectionDialog::boundedRect(const QRect& source, const QRect& bounds)
{
    QRect result = source;
    if (result.left() < bounds.left()) {
        result.translate(bounds.left() - result.left(), 0);
    }
    if (result.top() < bounds.top()) {
        result.translate(0, bounds.top() - result.top());
    }
    if (result.right() > bounds.right()) {
        result.translate(bounds.right() - result.right(), 0);
    }
    if (result.bottom() > bounds.bottom()) {
        result.translate(0, bounds.bottom() - result.bottom());
    }
    return result;
}

}  // namespace iqtools::core
