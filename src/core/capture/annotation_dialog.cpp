#include "annotation_dialog.h"

#include <cmath>

#include <QtCore/QPointF>
#include <QtCore/QtMath>
#include <QtGui/QKeySequence>
#include <QtGui/QKeyEvent>
#include <QtGui/QMouseEvent>
#include <QtGui/QPainter>
#include <QtGui/QPainterPath>
#include <QtWidgets/QInputDialog>
#include <QtWidgets/QLineEdit>

namespace iqtools::core {

namespace {

constexpr int kMargin = 16;
constexpr int kTopBarHeight = 92;
constexpr int kToolbarHeight = 28;
constexpr int kToolbarSpacing = 8;
constexpr int kToolbarLeft = 12;
constexpr int kToolbarFirstRowTop = 8;
constexpr int kToolbarSecondRowTop = 44;

QKeySequence eventKeySequence(const QKeyEvent* event)
{
    if (event == nullptr) {
        return {};
    }

    const int key = event->key();
    if (key == Qt::Key_unknown) {
        return {};
    }

    const Qt::KeyboardModifiers modifiers =
        event->modifiers() &
        (Qt::ShiftModifier | Qt::ControlModifier | Qt::AltModifier | Qt::MetaModifier);
    return QKeySequence(modifiers | key);
}

bool matchesShortcut(const QKeyEvent* event, const QKeySequence& shortcut)
{
    if (shortcut.isEmpty()) {
        return false;
    }

    return shortcut.matches(eventKeySequence(event)) == QKeySequence::ExactMatch;
}

QKeySequence parseShortcut(const QString& text, const QKeySequence& fallback)
{
    const QString trimmed = text.trimmed();
    if (trimmed.isEmpty()) {
        return fallback;
    }

    QKeySequence sequence = QKeySequence::fromString(trimmed, QKeySequence::PortableText);
    if (!sequence.isEmpty()) {
        return sequence;
    }

    sequence = QKeySequence::fromString(trimmed, QKeySequence::NativeText);
    if (!sequence.isEmpty()) {
        return sequence;
    }

    return fallback;
}

const QVector<QColor>& annotationColorPalette()
{
    static const QVector<QColor> kColors {
        QColor(255, 64, 64),
        QColor(255, 170, 0),
        QColor(74, 174, 255),
        QColor(74, 212, 149),
        QColor(255, 255, 255),
    };
    return kColors;
}

QColor averageColor(const QImage& image, const QRect& rect)
{
    if (!rect.isValid() || rect.isEmpty()) {
        return Qt::black;
    }

    qint64 r = 0;
    qint64 g = 0;
    qint64 b = 0;
    qint64 a = 0;
    qint64 count = 0;

    for (int y = rect.top(); y <= rect.bottom(); ++y) {
        const QRgb* line = reinterpret_cast<const QRgb*>(image.scanLine(y));
        for (int x = rect.left(); x <= rect.right(); ++x) {
            const QColor color = QColor::fromRgba(line[x]);
            r += color.red();
            g += color.green();
            b += color.blue();
            a += color.alpha();
            ++count;
        }
    }

    if (count <= 0) {
        return Qt::black;
    }

    return QColor(static_cast<int>(r / count),
                  static_cast<int>(g / count),
                  static_cast<int>(b / count),
                  static_cast<int>(a / count));
}

}  // namespace

AnnotationDialog::AnnotationDialog(const QImage& source, QWidget* parent)
    : AnnotationDialog(source, Options {}, parent)
{
}

AnnotationDialog::AnnotationDialog(const QImage& source,
                                   const Options& options,
                                   QWidget* parent)
    : QDialog(parent)
    , m_workingImage(source)
{
    applyOptions(options);

    setWindowFlags(windowFlags() | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint);
    setWindowTitle(QStringLiteral("截图标注"));
    setModal(true);
    setMouseTracking(true);
    resize(1200, 760);
}

QImage AnnotationDialog::annotatedImage() const
{
    return m_workingImage;
}

AnnotationDialog::Options AnnotationDialog::options() const
{
    Options value;
    value.lineWidth = m_lineWidth;
    value.textPixelSize = m_textPixelSize;
    value.mosaicBlockSize = m_mosaicBlockSize;
    value.colorIndex = m_colorIndex;
    value.shortcutRectangle = m_shortcutToolRectangle.toString(QKeySequence::PortableText);
    value.shortcutArrow = m_shortcutToolArrow.toString(QKeySequence::PortableText);
    value.shortcutMosaic = m_shortcutToolMosaic.toString(QKeySequence::PortableText);
    value.shortcutText = m_shortcutToolText.toString(QKeySequence::PortableText);
    value.shortcutUndo = m_shortcutUndo.toString(QKeySequence::PortableText);
    value.shortcutRedo = m_shortcutRedo.toString(QKeySequence::PortableText);
    value.shortcutColorCycle = m_shortcutColorCycle.toString(QKeySequence::PortableText);
    return value;
}

void AnnotationDialog::paintEvent(QPaintEvent* event)
{
    Q_UNUSED(event);

    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.fillRect(rect(), QColor(18, 24, 32));

    const QRect drawRect = imageRect();
    if (drawRect.isValid() && !m_workingImage.isNull()) {
        painter.drawImage(drawRect, m_workingImage);
    }

    if (m_dragging) {
        QPen previewPen(currentColor(), m_lineWidth, Qt::DashLine);
        previewPen.setCapStyle(Qt::RoundCap);
        painter.setPen(previewPen);
        const QRect preview = QRect(clampToImageRect(m_startPoint),
                                    clampToImageRect(m_currentPoint)).normalized();
        if (m_tool == Tool::Rectangle || m_tool == Tool::Mosaic) {
            painter.drawRect(preview);
        } else if (m_tool == Tool::Arrow) {
            drawArrow(painter,
                      clampToImageRect(m_startPoint),
                      clampToImageRect(m_currentPoint),
                      QPen(currentColor(), m_lineWidth));
        }
    }

    painter.setPen(Qt::NoPen);
    painter.setBrush(QColor(0, 0, 0, 170));
    painter.drawRect(0, 0, width(), kTopBarHeight);

    drawToolbar(painter);

    painter.setPen(QColor(230, 235, 243));
    const QString info = QStringLiteral(
                             "当前: %1 | 线宽 %2 | 字号 %3 | 马赛克 %4 | 工具键 %5/%6/%7/%8 | 撤销 %9 重做 %10 换色 %11 | Enter 保存")
                             .arg(toolLabel())
                             .arg(m_lineWidth)
                             .arg(m_textPixelSize)
                             .arg(m_mosaicBlockSize)
                             .arg(shortcutLabel(m_shortcutToolRectangle))
                             .arg(shortcutLabel(m_shortcutToolArrow))
                             .arg(shortcutLabel(m_shortcutToolMosaic))
                             .arg(shortcutLabel(m_shortcutToolText))
                             .arg(shortcutLabel(m_shortcutUndo))
                             .arg(shortcutLabel(m_shortcutRedo))
                             .arg(shortcutLabel(m_shortcutColorCycle));
    painter.drawText(QRect(12, kTopBarHeight - 24, width() - 24, 20),
                     Qt::AlignVCenter | Qt::AlignLeft,
                     info);
}

void AnnotationDialog::mousePressEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QDialog::mousePressEvent(event);
        return;
    }

    if (event->pos().y() <= kTopBarHeight) {
        handleToolbarClick(event->pos());
        return;
    }

    if (!imageRect().contains(event->pos())) {
        return;
    }

    if (m_tool == Tool::Text) {
        insertTextAt(event->pos());
        return;
    }

    m_dragging = true;
    m_startPoint = clampToImageRect(event->pos());
    m_currentPoint = m_startPoint;
    update();
}

void AnnotationDialog::mouseMoveEvent(QMouseEvent* event)
{
    if (!m_dragging) {
        QDialog::mouseMoveEvent(event);
        return;
    }

    m_currentPoint = clampToImageRect(event->pos());
    update();
}

void AnnotationDialog::mouseReleaseEvent(QMouseEvent* event)
{
    if (event->button() != Qt::LeftButton) {
        QDialog::mouseReleaseEvent(event);
        return;
    }

    if (!m_dragging) {
        return;
    }

    m_currentPoint = clampToImageRect(event->pos());
    commitShape();
    m_dragging = false;
    update();
}

void AnnotationDialog::mouseDoubleClickEvent(QMouseEvent* event)
{
    if (event->button() == Qt::LeftButton && imageRect().contains(event->pos())) {
        accept();
        return;
    }
    QDialog::mouseDoubleClickEvent(event);
}

void AnnotationDialog::keyPressEvent(QKeyEvent* event)
{
    if (matchesShortcut(event, m_shortcutUndo)) {
        undo();
        update();
        return;
    }

    if (matchesShortcut(event, m_shortcutRedo)) {
        redo();
        update();
        return;
    }

    if (matchesShortcut(event, m_shortcutToolRectangle)) {
        m_tool = Tool::Rectangle;
        update();
        return;
    }

    if (matchesShortcut(event, m_shortcutToolArrow)) {
        m_tool = Tool::Arrow;
        update();
        return;
    }

    if (matchesShortcut(event, m_shortcutToolMosaic)) {
        m_tool = Tool::Mosaic;
        update();
        return;
    }

    if (matchesShortcut(event, m_shortcutToolText)) {
        m_tool = Tool::Text;
        update();
        return;
    }

    if (matchesShortcut(event, m_shortcutColorCycle)) {
        cycleColor();
        update();
        return;
    }

    switch (event->key()) {
    case Qt::Key_BracketLeft:
        adjustLineWidth(-1);
        update();
        return;
    case Qt::Key_BracketRight:
        adjustLineWidth(1);
        update();
        return;
    case Qt::Key_Minus:
        adjustTextSize(-2);
        update();
        return;
    case Qt::Key_Equal:
        adjustTextSize(2);
        update();
        return;
    case Qt::Key_Comma:
        adjustMosaicBlockSize(-2);
        update();
        return;
    case Qt::Key_Period:
        adjustMosaicBlockSize(2);
        update();
        return;
    case Qt::Key_Return:
    case Qt::Key_Enter:
        accept();
        return;
    case Qt::Key_Escape:
        reject();
        return;
    default:
        break;
    }

    QDialog::keyPressEvent(event);
}

QRect AnnotationDialog::imageRect() const
{
    if (m_workingImage.isNull()) {
        return {};
    }

    const QRect available = rect().adjusted(kMargin,
                                            kTopBarHeight + kMargin,
                                            -kMargin,
                                            -kMargin);
    if (!available.isValid() || available.isEmpty()) {
        return {};
    }

    const qreal scale = qMin(static_cast<qreal>(available.width()) / m_workingImage.width(),
                             static_cast<qreal>(available.height()) / m_workingImage.height());
    if (scale <= 0.0) {
        return {};
    }

    const int w = qMax(1, static_cast<int>(m_workingImage.width() * scale));
    const int h = qMax(1, static_cast<int>(m_workingImage.height() * scale));
    const int x = available.x() + (available.width() - w) / 2;
    const int y = available.y() + (available.height() - h) / 2;
    return QRect(x, y, w, h);
}

QPoint AnnotationDialog::widgetToImage(const QPoint& point) const
{
    const QRect drawRect = imageRect();
    if (!drawRect.isValid() || drawRect.isEmpty() || m_workingImage.isNull()) {
        return {};
    }

    const qreal xRatio =
        static_cast<qreal>(point.x() - drawRect.left()) / drawRect.width();
    const qreal yRatio =
        static_cast<qreal>(point.y() - drawRect.top()) / drawRect.height();

    const int x = qBound(0,
                         static_cast<int>(xRatio * (m_workingImage.width() - 1)),
                         m_workingImage.width() - 1);
    const int y = qBound(0,
                         static_cast<int>(yRatio * (m_workingImage.height() - 1)),
                         m_workingImage.height() - 1);
    return QPoint(x, y);
}

QPoint AnnotationDialog::clampToImageRect(const QPoint& point) const
{
    const QRect drawRect = imageRect();
    if (!drawRect.isValid()) {
        return point;
    }

    return QPoint(qBound(drawRect.left(), point.x(), drawRect.right()),
                  qBound(drawRect.top(), point.y(), drawRect.bottom()));
}

QColor AnnotationDialog::currentColor() const
{
    const QVector<QColor>& colors = annotationColorPalette();
    if (colors.isEmpty()) {
        return QColor(255, 64, 64);
    }

    const int index = qBound(0, m_colorIndex, colors.size() - 1);
    return colors.at(index);
}

QString AnnotationDialog::shortcutLabel(const QKeySequence& shortcut) const
{
    const QString text = shortcut.toString(QKeySequence::NativeText).trimmed();
    if (text.isEmpty()) {
        return QStringLiteral("-");
    }
    return text;
}

void AnnotationDialog::applyOptions(const Options& options)
{
    m_lineWidth = qBound(1, options.lineWidth, 12);
    m_textPixelSize = qBound(12, options.textPixelSize, 96);
    m_mosaicBlockSize = qBound(4, options.mosaicBlockSize, 64);
    m_colorIndex = qBound(0, options.colorIndex, 7);

    m_shortcutToolRectangle = parseShortcut(options.shortcutRectangle,
                                            QKeySequence(Qt::Key_1));
    m_shortcutToolArrow = parseShortcut(options.shortcutArrow,
                                        QKeySequence(Qt::Key_2));
    m_shortcutToolMosaic = parseShortcut(options.shortcutMosaic,
                                         QKeySequence(Qt::Key_3));
    m_shortcutToolText = parseShortcut(options.shortcutText,
                                       QKeySequence(Qt::Key_4));
    m_shortcutUndo = parseShortcut(options.shortcutUndo,
                                   QKeySequence::Undo);
    m_shortcutRedo = parseShortcut(options.shortcutRedo,
                                   QKeySequence::Redo);
    m_shortcutColorCycle = parseShortcut(options.shortcutColorCycle,
                                         QKeySequence(Qt::Key_C));
}

void AnnotationDialog::drawArrow(QPainter& painter,
                                 const QPoint& start,
                                 const QPoint& end,
                                 const QPen& pen) const
{
    painter.save();
    painter.setPen(pen);
    painter.drawLine(start, end);

    constexpr qreal kPi = 3.14159265358979323846;
    const qreal angle = std::atan2(start.y() - end.y(), start.x() - end.x());
    const qreal arrowLength = qMax(10.0, static_cast<qreal>(pen.width()) * 4.0);
    const QPointF arrowP1 = end + QPointF(std::cos(angle + kPi / 6) * arrowLength,
                                          std::sin(angle + kPi / 6) * arrowLength);
    const QPointF arrowP2 = end + QPointF(std::cos(angle - kPi / 6) * arrowLength,
                                          std::sin(angle - kPi / 6) * arrowLength);
    painter.setBrush(pen.color());
    painter.drawPolygon(QPolygonF() << end << arrowP1 << arrowP2);
    painter.restore();
}

bool AnnotationDialog::drawToolbarGlyph(QPainter& painter,
                                        const ToolbarButton& button,
                                        const QColor& glyphColor) const
{
    const QRectF area = button.rect.adjusted(9, 6, -9, -6);
    if (!area.isValid()) {
        return false;
    }

    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(glyphColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
    painter.setBrush(Qt::NoBrush);

    switch (button.action) {
    case ToolbarAction::ToolRectangle:
        painter.drawRect(area);
        painter.restore();
        return true;
    case ToolbarAction::ToolArrow: {
        const QPointF start(area.left(), area.center().y());
        const QPointF end(area.right(), area.center().y());
        painter.drawLine(start, end);
        const qreal arrowSize = qMin(area.width(), area.height()) * 0.35;
        painter.drawLine(end, end + QPointF(-arrowSize, -arrowSize * 0.7));
        painter.drawLine(end, end + QPointF(-arrowSize, arrowSize * 0.7));
        painter.restore();
        return true;
    }
    case ToolbarAction::ToolMosaic: {
        painter.setPen(QPen(glyphColor, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        const qreal cellW = area.width() / 3.0;
        const qreal cellH = area.height() / 3.0;
        for (int row = 0; row < 3; ++row) {
            for (int col = 0; col < 3; ++col) {
                const QRectF cell(area.left() + col * cellW,
                                  area.top() + row * cellH,
                                  cellW,
                                  cellH);
                if (((row + col) % 2) == 0) {
                    painter.fillRect(cell.adjusted(1, 1, -1, -1), glyphColor);
                }
                painter.drawRect(cell);
            }
        }
        painter.restore();
        return true;
    }
    case ToolbarAction::ToolText: {
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(static_cast<int>(qMax(10.0, area.height() + 2.0)));
        painter.setFont(font);
        painter.drawText(area, Qt::AlignCenter, QStringLiteral("T"));
        painter.restore();
        return true;
    }
    case ToolbarAction::Undo:
        painter.restore();
        drawUndoRedoGlyph(painter, area, false, glyphColor);
        return true;
    case ToolbarAction::Redo:
        painter.restore();
        drawUndoRedoGlyph(painter, area, true, glyphColor);
        return true;
    case ToolbarAction::LineWidthDown:
    case ToolbarAction::LineWidthUp: {
        painter.setPen(QPen(glyphColor, 1, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(area.left(), area.top() + area.height() * 0.3),
                         QPointF(area.right(), area.top() + area.height() * 0.3));
        painter.setPen(QPen(glyphColor, 3, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawLine(QPointF(area.left(), area.top() + area.height() * 0.72),
                         QPointF(area.right() - 6, area.top() + area.height() * 0.72));
        painter.setPen(QPen(glyphColor, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF center(area.right() - 3, area.center().y());
        painter.drawLine(center + QPointF(-3, 0), center + QPointF(3, 0));
        if (button.action == ToolbarAction::LineWidthUp) {
            painter.drawLine(center + QPointF(0, -3), center + QPointF(0, 3));
        }
        painter.restore();
        return true;
    }
    case ToolbarAction::TextSizeDown:
    case ToolbarAction::TextSizeUp: {
        QFont font = painter.font();
        font.setBold(true);
        font.setPixelSize(static_cast<int>(qMax(10.0, area.height() - 1.0)));
        painter.setFont(font);
        painter.drawText(QRectF(area.left(), area.top(), area.width() * 0.6, area.height()),
                         Qt::AlignCenter,
                         QStringLiteral("T"));
        painter.setPen(QPen(glyphColor, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF center(area.right() - 5, area.center().y());
        painter.drawLine(center + QPointF(-3, 0), center + QPointF(3, 0));
        if (button.action == ToolbarAction::TextSizeUp) {
            painter.drawLine(center + QPointF(0, -3), center + QPointF(0, 3));
        }
        painter.restore();
        return true;
    }
    case ToolbarAction::MosaicDown:
    case ToolbarAction::MosaicUp: {
        painter.setPen(QPen(glyphColor, 1, Qt::SolidLine, Qt::SquareCap, Qt::MiterJoin));
        const qreal gridW = area.width() * 0.62;
        const qreal gridH = area.height();
        const QRectF grid(area.left(), area.top(), gridW, gridH);
        const qreal cw = grid.width() / 2.0;
        const qreal ch = grid.height() / 2.0;
        for (int row = 0; row < 2; ++row) {
            for (int col = 0; col < 2; ++col) {
                const QRectF cell(grid.left() + col * cw, grid.top() + row * ch, cw, ch);
                if (((row + col) % 2) == 0) {
                    painter.fillRect(cell.adjusted(1, 1, -1, -1), glyphColor);
                }
                painter.drawRect(cell);
            }
        }
        painter.setPen(QPen(glyphColor, 1.8, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        const QPointF center(area.right() - 5, area.center().y());
        painter.drawLine(center + QPointF(-3, 0), center + QPointF(3, 0));
        if (button.action == ToolbarAction::MosaicUp) {
            painter.drawLine(center + QPointF(0, -3), center + QPointF(0, 3));
        }
        painter.restore();
        return true;
    }
    case ToolbarAction::CycleColor: {
        painter.setPen(QPen(glyphColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));
        painter.drawEllipse(QPointF(area.center().x() - 3, area.center().y()),
                            area.height() * 0.28,
                            area.height() * 0.28);
        painter.drawEllipse(QPointF(area.center().x() + 4, area.center().y()),
                            area.height() * 0.28,
                            area.height() * 0.28);
        painter.restore();
        return true;
    }
    default:
        painter.restore();
        return false;
    }
}

void AnnotationDialog::drawUndoRedoGlyph(QPainter& painter,
                                         const QRectF& area,
                                         bool redo,
                                         const QColor& glyphColor) const
{
    painter.save();
    painter.setRenderHint(QPainter::Antialiasing, true);
    painter.setPen(QPen(glyphColor, 2, Qt::SolidLine, Qt::RoundCap, Qt::RoundJoin));

    const QPointF start = redo
                              ? QPointF(area.left() + 2, area.center().y() + 3)
                              : QPointF(area.right() - 2, area.center().y() + 3);
    const QPointF end = redo
                            ? QPointF(area.right() - 2, area.center().y() - 1)
                            : QPointF(area.left() + 2, area.center().y() - 1);
    const QPointF control = QPointF(area.center().x(), area.top() - 2);

    QPainterPath path(start);
    path.quadTo(control, end);
    painter.drawPath(path);

    const qreal headSize = 5;
    if (redo) {
        painter.drawLine(end, end + QPointF(-headSize, -headSize + 1));
        painter.drawLine(end, end + QPointF(-headSize, headSize - 1));
    } else {
        painter.drawLine(end, end + QPointF(headSize, -headSize + 1));
        painter.drawLine(end, end + QPointF(headSize, headSize - 1));
    }

    painter.restore();
}

void AnnotationDialog::drawToolbar(QPainter& painter)
{
    m_toolbarButtons = buildToolbarButtons();

    for (const ToolbarButton& button : m_toolbarButtons) {
        QColor fillColor(53, 61, 74);
        QColor borderColor(70, 79, 93);
        QColor textColor(235, 239, 245);

        if (!button.enabled) {
            fillColor = QColor(44, 49, 60);
            borderColor = QColor(58, 64, 75);
            textColor = QColor(130, 139, 154);
        } else if (button.active) {
            fillColor = QColor(64, 121, 215);
            borderColor = QColor(90, 146, 235);
            textColor = Qt::white;
        }

        if (button.action == ToolbarAction::CycleColor && button.enabled) {
            fillColor = currentColor();
            borderColor = currentColor().lighter(120);
            const int luminance = (fillColor.red() * 299 + fillColor.green() * 587 +
                                   fillColor.blue() * 114) /
                                  1000;
            textColor = luminance > 150 ? QColor(20, 20, 20) : QColor(255, 255, 255);
        }

        painter.setPen(QPen(borderColor, 1));
        painter.setBrush(fillColor);
        painter.drawRoundedRect(button.rect, 4, 4);

        if (!drawToolbarGlyph(painter, button, textColor)) {
            painter.setPen(textColor);
            painter.drawText(button.rect, Qt::AlignCenter, button.label);
        }
    }
}

QVector<AnnotationDialog::ToolbarButton> AnnotationDialog::buildToolbarButtons() const
{
    QVector<ToolbarButton> buttons;
    buttons.reserve(13);

    auto appendButton = [&buttons](int x,
                                   int y,
                                   int width,
                                   const QString& label,
                                   ToolbarAction action,
                                   bool enabled,
                                   bool active) {
        buttons.push_back({QRect(x, y, width, kToolbarHeight),
                           label,
                           action,
                           enabled,
                           active});
    };

    int x = kToolbarLeft;
    appendButton(x,
                 kToolbarFirstRowTop,
                 48,
                 QStringLiteral("[]"),
                 ToolbarAction::ToolRectangle,
                 true,
                 m_tool == Tool::Rectangle);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarFirstRowTop,
                 48,
                 QStringLiteral("->"),
                 ToolbarAction::ToolArrow,
                 true,
                 m_tool == Tool::Arrow);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarFirstRowTop,
                 48,
                 QStringLiteral("MX"),
                 ToolbarAction::ToolMosaic,
                 true,
                 m_tool == Tool::Mosaic);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarFirstRowTop,
                 48,
                 QStringLiteral("Tx"),
                 ToolbarAction::ToolText,
                 true,
                 m_tool == Tool::Text);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarFirstRowTop,
                 48,
                 QStringLiteral("U"),
                 ToolbarAction::Undo,
                 canUndo(),
                 false);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarFirstRowTop,
                 48,
                 QStringLiteral("R"),
                 ToolbarAction::Redo,
                 canRedo(),
                 false);

    x = kToolbarLeft;
    appendButton(x,
                 kToolbarSecondRowTop,
                 48,
                 QStringLiteral("W-"),
                 ToolbarAction::LineWidthDown,
                 m_lineWidth > 1,
                 false);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarSecondRowTop,
                 48,
                 QStringLiteral("W+"),
                 ToolbarAction::LineWidthUp,
                 m_lineWidth < 12,
                 false);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarSecondRowTop,
                 48,
                 QStringLiteral("T-"),
                 ToolbarAction::TextSizeDown,
                 m_textPixelSize > 12,
                 false);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarSecondRowTop,
                 48,
                 QStringLiteral("T+"),
                 ToolbarAction::TextSizeUp,
                 m_textPixelSize < 96,
                 false);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarSecondRowTop,
                 48,
                 QStringLiteral("M-"),
                 ToolbarAction::MosaicDown,
                 m_mosaicBlockSize > 4,
                 false);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarSecondRowTop,
                 48,
                 QStringLiteral("M+"),
                 ToolbarAction::MosaicUp,
                 m_mosaicBlockSize < 64,
                 false);
    x += 48 + kToolbarSpacing;
    appendButton(x,
                 kToolbarSecondRowTop,
                 48,
                 QStringLiteral("C"),
                 ToolbarAction::CycleColor,
                 true,
                 false);

    return buttons;
}

void AnnotationDialog::handleToolbarClick(const QPoint& point)
{
    m_toolbarButtons = buildToolbarButtons();
    for (const ToolbarButton& button : m_toolbarButtons) {
        if (!button.enabled || !button.rect.contains(point)) {
            continue;
        }
        applyToolbarAction(button.action);
        update();
        return;
    }
}

void AnnotationDialog::applyToolbarAction(ToolbarAction action)
{
    switch (action) {
    case ToolbarAction::ToolRectangle:
        m_tool = Tool::Rectangle;
        break;
    case ToolbarAction::ToolArrow:
        m_tool = Tool::Arrow;
        break;
    case ToolbarAction::ToolMosaic:
        m_tool = Tool::Mosaic;
        break;
    case ToolbarAction::ToolText:
        m_tool = Tool::Text;
        break;
    case ToolbarAction::Undo:
        undo();
        break;
    case ToolbarAction::Redo:
        redo();
        break;
    case ToolbarAction::LineWidthDown:
        adjustLineWidth(-1);
        break;
    case ToolbarAction::LineWidthUp:
        adjustLineWidth(1);
        break;
    case ToolbarAction::TextSizeDown:
        adjustTextSize(-2);
        break;
    case ToolbarAction::TextSizeUp:
        adjustTextSize(2);
        break;
    case ToolbarAction::MosaicDown:
        adjustMosaicBlockSize(-2);
        break;
    case ToolbarAction::MosaicUp:
        adjustMosaicBlockSize(2);
        break;
    case ToolbarAction::CycleColor:
        cycleColor();
        break;
    default:
        break;
    }
}

void AnnotationDialog::applyMosaic(const QRect& rect)
{
    QRect bounded = rect.normalized().intersected(m_workingImage.rect());
    if (!bounded.isValid() || bounded.isEmpty()) {
        return;
    }

    QPainter painter(&m_workingImage);
    painter.setPen(Qt::NoPen);
    const int blockSize = qMax(2, m_mosaicBlockSize);
    for (int y = bounded.top(); y <= bounded.bottom(); y += blockSize) {
        for (int x = bounded.left(); x <= bounded.right(); x += blockSize) {
            QRect block(x,
                        y,
                        qMin(blockSize, bounded.right() - x + 1),
                        qMin(blockSize, bounded.bottom() - y + 1));
            const QColor fillColor = averageColor(m_workingImage, block);
            painter.setBrush(fillColor);
            painter.drawRect(block);
        }
    }
}

void AnnotationDialog::commitShape()
{
    const QPoint imgStart = widgetToImage(m_startPoint);
    const QPoint imgEnd = widgetToImage(m_currentPoint);
    const QRect imgRect = QRect(imgStart, imgEnd).normalized();

    if (!imgRect.isValid() || imgRect.isEmpty()) {
        return;
    }

    pushUndoSnapshot();

    QPainter painter(&m_workingImage);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QPen pen(currentColor(), m_lineWidth);
    pen.setCapStyle(Qt::RoundCap);
    pen.setJoinStyle(Qt::RoundJoin);
    painter.setPen(pen);

    if (m_tool == Tool::Rectangle) {
        painter.drawRect(imgRect);
        return;
    }

    if (m_tool == Tool::Arrow) {
        drawArrow(painter, imgStart, imgEnd, pen);
        return;
    }

    if (m_tool == Tool::Mosaic) {
        painter.end();
        applyMosaic(imgRect);
    }
}

void AnnotationDialog::insertTextAt(const QPoint& widgetPoint)
{
    bool ok = false;
    const QString text = QInputDialog::getText(this,
                                               QStringLiteral("输入文字"),
                                               QStringLiteral("文本内容："),
                                               QLineEdit::Normal,
                                               QString(),
                                               &ok);
    if (!ok || text.trimmed().isEmpty()) {
        return;
    }

    pushUndoSnapshot();

    const QPoint imgPoint = widgetToImage(widgetPoint);
    QPainter painter(&m_workingImage);
    painter.setRenderHint(QPainter::Antialiasing, true);
    QFont font = painter.font();
    font.setPixelSize(m_textPixelSize);
    font.setBold(true);
    painter.setFont(font);
    painter.setPen(QPen(currentColor(), qMax(1, m_lineWidth / 2)));
    painter.drawText(imgPoint, text);
    update();
}

void AnnotationDialog::pushUndoSnapshot()
{
    if (m_workingImage.isNull()) {
        return;
    }

    m_undoStack.push_back(m_workingImage);
    if (m_undoStack.size() > m_historyLimit) {
        m_undoStack.remove(0);
    }
    m_redoStack.clear();
}

void AnnotationDialog::undo()
{
    if (!canUndo()) {
        return;
    }

    m_redoStack.push_back(m_workingImage);
    if (m_redoStack.size() > m_historyLimit) {
        m_redoStack.remove(0);
    }

    m_workingImage = m_undoStack.takeLast();
}

void AnnotationDialog::redo()
{
    if (!canRedo()) {
        return;
    }

    m_undoStack.push_back(m_workingImage);
    if (m_undoStack.size() > m_historyLimit) {
        m_undoStack.remove(0);
    }

    m_workingImage = m_redoStack.takeLast();
}

bool AnnotationDialog::canUndo() const
{
    return !m_undoStack.isEmpty();
}

bool AnnotationDialog::canRedo() const
{
    return !m_redoStack.isEmpty();
}

void AnnotationDialog::adjustLineWidth(int delta)
{
    m_lineWidth = qBound(1, m_lineWidth + delta, 12);
}

void AnnotationDialog::adjustTextSize(int delta)
{
    m_textPixelSize = qBound(12, m_textPixelSize + delta, 96);
}

void AnnotationDialog::adjustMosaicBlockSize(int delta)
{
    m_mosaicBlockSize = qBound(4, m_mosaicBlockSize + delta, 64);
}

void AnnotationDialog::cycleColor()
{
    const QVector<QColor>& colors = annotationColorPalette();
    if (colors.isEmpty()) {
        return;
    }

    m_colorIndex = (m_colorIndex + 1) % colors.size();
}

QString AnnotationDialog::toolLabel() const
{
    switch (m_tool) {
    case Tool::Rectangle:
        return QStringLiteral("矩形");
    case Tool::Arrow:
        return QStringLiteral("箭头");
    case Tool::Mosaic:
        return QStringLiteral("马赛克");
    case Tool::Text:
        return QStringLiteral("文字");
    default:
        return QStringLiteral("未知");
    }
}

}  // namespace iqtools::core
