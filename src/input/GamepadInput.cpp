#include "GamepadInput.h"

#include <QDebug>
#include <QtMath>

#include <SDL.h>

namespace {
constexpr int kActiveIntervalMs = 8;     // ~120 Hz: smooth pointer, tight repeat
constexpr int kIdleIntervalMs = 33;      // ~30 Hz: imperceptible for menu presses
constexpr int kThrottledIntervalMs = 100;
constexpr qreal kStickDeadzone = 0.22;
constexpr qreal kStickDpadPress = 0.65;  // left stick doubles as a d-pad
constexpr qreal kStickDpadRelease = 0.40;
constexpr qint16 kTriggerPress = 16000;
constexpr qint16 kTriggerRelease = 8000;

QPointF applyDeadzone(qint16 rx, qint16 ry)
{
    QPointF v(rx / 32767.0, ry / 32767.0);
    const qreal mag = qSqrt(v.x() * v.x() + v.y() * v.y());
    if (mag < kStickDeadzone)
        return {};
    const qreal scaled = qMin(1.0, (mag - kStickDeadzone) / (1.0 - kStickDeadzone));
    return v * (scaled / mag);
}
} // namespace

GamepadInput::GamepadInput(QObject *parent)
    : QObject(parent)
{
    // We never create an SDL window, so SDL must not wait for window focus.
    SDL_SetHint(SDL_HINT_JOYSTICK_ALLOW_BACKGROUND_EVENTS, "1");
    if (SDL_Init(SDL_INIT_GAMECONTROLLER) != 0) {
        qWarning() << "SDL_Init failed, gamepad input disabled:" << SDL_GetError();
        return;
    }
    m_ok = true;

    m_timer.setSingleShot(true);
    m_timer.setTimerType(Qt::CoarseTimer); // lets the kernel coalesce wakeups
    connect(&m_timer, &QTimer::timeout, this, &GamepadInput::poll);
    m_clock.start();
    m_timer.start(0);
}

GamepadInput::~GamepadInput()
{
    for (auto *c : std::as_const(m_controllers))
        SDL_GameControllerClose(c);
    if (m_ok)
        SDL_Quit();
}

QString GamepadInput::controllerName() const
{
    if (m_controllers.isEmpty())
        return {};
    return QString::fromUtf8(SDL_GameControllerName(m_controllers.constFirst()));
}

void GamepadInput::setThrottled(bool throttled)
{
    m_throttled = throttled;
}

void GamepadInput::openController(int deviceIndex)
{
    if (!SDL_IsGameController(deviceIndex))
        return;
    if (auto *c = SDL_GameControllerOpen(deviceIndex)) {
        m_controllers.append(c);
        qInfo() << "Controller connected:" << SDL_GameControllerName(c);
        emit connectedChanged();
    }
}

void GamepadInput::closeController(int instanceId)
{
    for (int i = 0; i < m_controllers.size(); ++i) {
        auto *c = m_controllers.at(i);
        if (SDL_JoystickInstanceID(SDL_GameControllerGetJoystick(c)) == instanceId) {
            SDL_GameControllerClose(c);
            m_controllers.removeAt(i);
            emit connectedChanged();
            return;
        }
    }
}

void GamepadInput::setButton(const QString &button, bool down)
{
    if (down == m_held.contains(button))
        return;
    if (down) {
        m_held.insert(button);
        emit buttonPressed(button);
    } else {
        m_held.remove(button);
        emit buttonReleased(button);
    }
}

void GamepadInput::updateTrigger(const QString &name, qint16 raw)
{
    if (raw > kTriggerPress)
        setButton(name, true);
    else if (raw < kTriggerRelease)
        setButton(name, false);
}

void GamepadInput::updateStickDpad()
{
    // Separate names so a stick release never cancels a held physical d-pad.
    auto axis = [this](const QString &neg, const QString &pos, qreal v) {
        const bool negDown = m_held.contains(neg);
        const bool posDown = m_held.contains(pos);
        setButton(neg, negDown ? v < -kStickDpadRelease : v < -kStickDpadPress);
        setButton(pos, posDown ? v > kStickDpadRelease : v > kStickDpadPress);
    };
    axis(QStringLiteral("lsleft"), QStringLiteral("lsright"), m_left.x());
    axis(QStringLiteral("lsup"), QStringLiteral("lsdown"), m_left.y());
}

void GamepadInput::updateJog()
{
    const int steps = m_jog.update(m_right);
    const QString button = steps > 0 ? QStringLiteral("rscw") : QStringLiteral("rsccw");
    for (int i = 0; i < qAbs(steps); ++i) {
        emit buttonPressed(button);
        emit buttonReleased(button);
    }
}

void GamepadInput::poll()
{
    const qreal dt = m_clock.restart() / 1000.0;

    SDL_Event e;
    while (SDL_PollEvent(&e)) {
        switch (e.type) {
        case SDL_CONTROLLERDEVICEADDED:
            openController(e.cdevice.which);
            break;
        case SDL_CONTROLLERDEVICEREMOVED:
            closeController(e.cdevice.which);
            break;
        case SDL_CONTROLLERBUTTONDOWN:
        case SDL_CONTROLLERBUTTONUP: {
            const char *name = SDL_GameControllerGetStringForButton(
                static_cast<SDL_GameControllerButton>(e.cbutton.button));
            if (name)
                setButton(QString::fromLatin1(name), e.type == SDL_CONTROLLERBUTTONDOWN);
            break;
        }
        default:
            break;
        }
    }

    // Read axes as state rather than events; with several pads attached
    // (Deck + external controller) the most deflected one wins.
    QPointF left, right;
    qint16 lt = 0, rt = 0;
    for (auto *c : std::as_const(m_controllers)) {
        const QPointF l = applyDeadzone(SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX),
                                        SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY));
        const QPointF r = applyDeadzone(SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTX),
                                        SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTY));
        if (l.manhattanLength() > left.manhattanLength())
            left = l;
        if (r.manhattanLength() > right.manhattanLength())
            right = r;
        lt = qMax(lt, SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT));
        rt = qMax(rt, SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT));
    }
    m_left = left;
    m_right = right;
    updateTrigger(QStringLiteral("lefttrigger"), lt);
    updateTrigger(QStringLiteral("righttrigger"), rt);
    updateStickDpad();
    updateJog();

    if (!m_right.isNull())
        emit rightStickActive(m_right, dt);

    scheduleNextPoll();
}

void GamepadInput::scheduleNextPoll()
{
    const bool active = !m_held.isEmpty() || !m_right.isNull() || !m_left.isNull();
    int interval = active ? kActiveIntervalMs : kIdleIntervalMs;
    if (m_throttled && !active)
        interval = kThrottledIntervalMs;
    // Precise timing only matters while the user is steering something.
    m_timer.setTimerType(active ? Qt::PreciseTimer : Qt::CoarseTimer);
    m_timer.start(interval);
}
