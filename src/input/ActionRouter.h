#pragma once

#include <QHash>
#include <QObject>
#include <QTimer>
#include <QVariantList>

class GamepadInput;

// Turns raw button presses into named actions for the current context
// ("launcher", "browse", "player"), driven by config/bindings.json.
//
// Bindings are strings of the form "name" or "name:arg", e.g. "seek:+10".
// A button can have both a tap and a "hold:<button>" binding; when it does,
// the tap fires on release so the two never both trigger.
class ActionRouter : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString context READ context WRITE setContext NOTIFY contextChanged)
    Q_PROPERTY(QVariantList hints READ hints NOTIFY contextChanged)
    // True when the current context binds right-stick rotation, in which
    // case the stick jogs instead of moving the pointer.
    Q_PROPERTY(bool rightStickJog READ rightStickJog NOTIFY contextChanged)

public:
    explicit ActionRouter(GamepadInput *input, QObject *parent = nullptr);

    // Later files override earlier ones per context/button.
    void loadBindings(const QStringList &paths);

    QString context() const { return m_context; }
    void setContext(const QString &context);

    // [{button, label, hold}] for the on-screen button bar.
    QVariantList hints() const;

    bool rightStickJog() const;

    // Lets QML feed keyboard input through the same path (desktop testing).
    Q_INVOKABLE void inject(const QString &button, bool pressed);

signals:
    void action(const QString &name, const QString &arg);
    void contextChanged();

private:
    struct Binding {
        QString action;
        QString label;
    };
    using ContextMap = QHash<QString, Binding>;

    void onPressed(const QString &button);
    void onReleased(const QString &button);
    const Binding *lookup(const QString &key) const;
    void fire(const Binding &binding);

    GamepadInput *m_input;
    QHash<QString, ContextMap> m_contexts;
    QString m_context = QStringLiteral("launcher");

    QStringList m_repeatButtons;
    int m_repeatDelayMs = 380;
    int m_repeatIntervalMs = 110;
    int m_holdMs = 650;

    QString m_activeButton; // the button currently driving repeat/hold
    bool m_holdFired = false;
    QTimer m_repeatTimer;
    QTimer m_holdTimer;
};
