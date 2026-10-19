#pragma once

#include <QObject>
#include <QPointF>
#include <QPointer>
#include <QTimer>

class GamepadInput;
class QQuickWindow;

// Right-stick mouse for the parts of a site that spatial navigation can't
// reach. Events are synthesized as real Qt mouse events on the window, so
// the page sees trusted pointer input (hover menus, scrubbers, ...).
class VirtualPointer : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool enabled READ enabled WRITE setEnabled NOTIFY enabledChanged)
    Q_PROPERTY(bool visible READ visible NOTIFY visibleChanged)
    Q_PROPERTY(qreal x READ x NOTIFY moved)
    Q_PROPERTY(qreal y READ y NOTIFY moved)

public:
    VirtualPointer(GamepadInput *input, QObject *parent = nullptr);

    void setWindow(QQuickWindow *window);

    bool enabled() const { return m_enabled; }
    void setEnabled(bool enabled);
    bool visible() const { return m_visible; }
    qreal x() const { return m_pos.x(); }
    qreal y() const { return m_pos.y(); }

    Q_INVOKABLE void click();
    Q_INVOKABLE void hide();

signals:
    void enabledChanged();
    void visibleChanged();
    void moved();

private:
    void onStick(const QPointF &v, qreal dt);
    void setVisible(bool visible);
    void sendMouse(int type, Qt::MouseButton button, Qt::MouseButtons buttons);

    QPointer<QQuickWindow> m_window;
    QPointF m_pos;
    QTimer m_hideTimer;
    bool m_enabled = false;
    bool m_visible = false;
};
