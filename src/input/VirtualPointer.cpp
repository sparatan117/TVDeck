#include "VirtualPointer.h"
#include "GamepadInput.h"

#include <QCoreApplication>
#include <QMouseEvent>
#include <QQuickWindow>
#include <QtMath>

namespace {
constexpr qreal kMaxSpeed = 1600.0; // px/s at full deflection, before DPR
constexpr int kHideAfterMs = 2500;
} // namespace

VirtualPointer::VirtualPointer(GamepadInput *input, QObject *parent)
    : QObject(parent)
{
    connect(input, &GamepadInput::rightStickActive, this, &VirtualPointer::onStick);
    m_hideTimer.setSingleShot(true);
    m_hideTimer.setInterval(kHideAfterMs);
    connect(&m_hideTimer, &QTimer::timeout, this, &VirtualPointer::hide);
}

void VirtualPointer::setWindow(QQuickWindow *window)
{
    m_window = window;
    if (window)
        m_pos = QPointF(window->width() / 2.0, window->height() / 2.0);
}

void VirtualPointer::setEnabled(bool enabled)
{
    if (enabled == m_enabled)
        return;
    m_enabled = enabled;
    if (!enabled)
        hide();
    emit enabledChanged();
}

void VirtualPointer::setVisible(bool visible)
{
    if (visible == m_visible)
        return;
    m_visible = visible;
    emit visibleChanged();
}

void VirtualPointer::hide()
{
    m_hideTimer.stop();
    setVisible(false);
}

void VirtualPointer::onStick(const QPointF &v, qreal dt)
{
    if (!m_enabled || !m_window)
        return;
    // Quadratic response: fine control near the center, fast at the edge.
    const qreal mag = qSqrt(v.x() * v.x() + v.y() * v.y());
    const QPointF delta = v * (mag * kMaxSpeed * qMin(dt, 0.05));
    m_pos.setX(qBound(0.0, m_pos.x() + delta.x(), qreal(m_window->width() - 1)));
    m_pos.setY(qBound(0.0, m_pos.y() + delta.y(), qreal(m_window->height() - 1)));

    setVisible(true);
    m_hideTimer.start();
    emit moved();
    sendMouse(QEvent::MouseMove, Qt::NoButton, Qt::NoButton);
}

void VirtualPointer::click()
{
    if (!m_visible)
        return;
    sendMouse(QEvent::MouseButtonPress, Qt::LeftButton, Qt::LeftButton);
    sendMouse(QEvent::MouseButtonRelease, Qt::LeftButton, Qt::NoButton);
    m_hideTimer.start();
}

void VirtualPointer::sendMouse(int type, Qt::MouseButton button, Qt::MouseButtons buttons)
{
    if (!m_window)
        return;
    const QPointF global = m_window->mapToGlobal(m_pos);
    QMouseEvent ev(static_cast<QEvent::Type>(type), m_pos, m_pos, global,
                   button, buttons, Qt::NoModifier);
    QCoreApplication::sendEvent(m_window, &ev);
}
