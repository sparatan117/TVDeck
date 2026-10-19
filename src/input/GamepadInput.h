#pragma once

#include <QObject>
#include <QPointF>
#include <QElapsedTimer>
#include <QSet>
#include <QTimer>

struct _SDL_GameController;

// Reads controllers through SDL2's GameController API.
//
// On SteamOS, Steam Input sits in front of the physical device and exposes a
// virtual gamepad to non-Steam shortcuts, so this sees whatever layout the
// user configured in Steam (Deck controls, DualSense, Xbox pads, ...).
//
// Battery: there is no SDL video subsystem, so SDL_WaitEvent would busy-poll
// at ~1 kHz. Instead we poll on a QTimer whose rate adapts: fast while a
// button is held or a stick is deflected, slow when idle.
class GamepadInput : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool connected READ connected NOTIFY connectedChanged)
    Q_PROPERTY(QString controllerName READ controllerName NOTIFY connectedChanged)

public:
    explicit GamepadInput(QObject *parent = nullptr);
    ~GamepadInput() override;

    bool connected() const { return !m_controllers.isEmpty(); }
    QString controllerName() const;

    // Deadzone-corrected, -1..1 on each axis.
    QPointF leftStick() const { return m_left; }
    QPointF rightStick() const { return m_right; }

    // Lower the idle rate further, e.g. while the app is in the background.
    void setThrottled(bool throttled);

signals:
    // Button names follow SDL's mapping strings: "a", "b", "dpleft",
    // "leftshoulder", ... plus "lefttrigger"/"righttrigger".
    void buttonPressed(const QString &button);
    void buttonReleased(const QString &button);
    // Emitted every poll while the right stick is deflected.
    void rightStickActive(const QPointF &value, qreal dtSeconds);
    void connectedChanged();

private:
    void poll();
    void openController(int deviceIndex);
    void closeController(int instanceId);
    void setButton(const QString &button, bool down);
    void updateTrigger(const QString &name, qint16 raw);
    void updateStickDpad();
    void scheduleNextPoll();

    QTimer m_timer;
    QElapsedTimer m_clock;
    QList<_SDL_GameController *> m_controllers;
    QSet<QString> m_held;
    QPointF m_left;
    QPointF m_right;
    bool m_throttled = false;
    bool m_ok = false;
};
