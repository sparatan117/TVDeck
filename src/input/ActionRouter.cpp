#include "ActionRouter.h"
#include "GamepadInput.h"

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QDebug>

namespace {
const QString kHoldPrefix = QStringLiteral("hold:");

// Order used for the on-screen hints bar.
const QStringList kHintOrder = {
    QStringLiteral("a"), QStringLiteral("b"), QStringLiteral("x"), QStringLiteral("y"),
    QStringLiteral("dpleft"), QStringLiteral("dpright"), QStringLiteral("dpup"), QStringLiteral("dpdown"),
    QStringLiteral("leftshoulder"), QStringLiteral("rightshoulder"),
    QStringLiteral("lefttrigger"), QStringLiteral("righttrigger"),
    QStringLiteral("start"), QStringLiteral("back"),
};

// The left stick emits "ls*" so it can be bound separately, but by default
// it behaves exactly like the d-pad.
QString stickFallback(const QString &button)
{
    if (button.startsWith(QLatin1String("ls")))
        return QStringLiteral("dp") + button.mid(2);
    return {};
}
} // namespace

ActionRouter::ActionRouter(GamepadInput *input, QObject *parent)
    : QObject(parent)
{
    connect(input, &GamepadInput::buttonPressed, this, &ActionRouter::onPressed);
    connect(input, &GamepadInput::buttonReleased, this, &ActionRouter::onReleased);

    m_holdTimer.setSingleShot(true);
    connect(&m_holdTimer, &QTimer::timeout, this, [this] {
        if (const Binding *b = lookup(kHoldPrefix + m_activeButton)) {
            m_holdFired = true;
            fire(*b);
        }
    });

    connect(&m_repeatTimer, &QTimer::timeout, this, [this] {
        m_repeatTimer.setInterval(m_repeatIntervalMs);
        if (const Binding *b = lookup(m_activeButton))
            fire(*b);
    });
}

void ActionRouter::loadBindings(const QStringList &paths)
{
    for (const QString &path : paths) {
        QFile f(path);
        if (!f.open(QIODevice::ReadOnly))
            continue;
        QJsonParseError err;
        const QJsonObject root = QJsonDocument::fromJson(f.readAll(), &err).object();
        if (err.error != QJsonParseError::NoError) {
            qWarning() << "Ignoring bindings file" << path << ":" << err.errorString();
            continue;
        }

        const QJsonObject repeat = root.value(QLatin1String("repeat")).toObject();
        if (!repeat.isEmpty()) {
            m_repeatDelayMs = repeat.value(QLatin1String("delayMs")).toInt(m_repeatDelayMs);
            m_repeatIntervalMs = repeat.value(QLatin1String("intervalMs")).toInt(m_repeatIntervalMs);
            if (repeat.contains(QLatin1String("buttons"))) {
                m_repeatButtons.clear();
                for (const auto &v : repeat.value(QLatin1String("buttons")).toArray())
                    m_repeatButtons << v.toString();
            }
        }
        m_holdMs = root.value(QLatin1String("holdMs")).toInt(m_holdMs);

        const QJsonObject contexts = root.value(QLatin1String("contexts")).toObject();
        for (auto ctx = contexts.begin(); ctx != contexts.end(); ++ctx) {
            ContextMap &map = m_contexts[ctx.key()];
            const QJsonObject buttons = ctx.value().toObject();
            for (auto it = buttons.begin(); it != buttons.end(); ++it) {
                if (it.value().isNull()) { // allows a user file to unbind
                    map.remove(it.key());
                } else if (it.value().isString()) {
                    map.insert(it.key(), {it.value().toString(), {}});
                } else {
                    const QJsonObject o = it.value().toObject();
                    map.insert(it.key(), {o.value(QLatin1String("action")).toString(),
                                          o.value(QLatin1String("label")).toString()});
                }
            }
        }
        qInfo() << "Loaded bindings from" << path;
    }
    emit contextChanged();
}

void ActionRouter::setContext(const QString &context)
{
    if (context == m_context)
        return;
    m_context = context;
    // Don't let a held button carry its repeat into a different context.
    m_repeatTimer.stop();
    m_holdTimer.stop();
    m_activeButton.clear();
    emit contextChanged();
}

QVariantList ActionRouter::hints() const
{
    QVariantList out;
    const ContextMap map = m_contexts.value(m_context);
    for (const QString &button : kHintOrder) {
        for (const QString &key : {button, kHoldPrefix + button}) {
            auto it = map.constFind(key);
            if (it != map.constEnd() && !it->label.isEmpty())
                out.append(QVariantMap{{QStringLiteral("button"), button},
                                       {QStringLiteral("label"), it->label},
                                       {QStringLiteral("hold"), key != button}});
        }
    }
    return out;
}

const ActionRouter::Binding *ActionRouter::lookup(const QString &key) const
{
    auto ctx = m_contexts.constFind(m_context);
    if (ctx == m_contexts.constEnd())
        return nullptr;
    auto it = ctx->constFind(key);
    if (it != ctx->constEnd())
        return &*it;

    const bool hold = key.startsWith(kHoldPrefix);
    const QString button = hold ? key.mid(kHoldPrefix.size()) : key;
    const QString alias = stickFallback(button);
    if (alias.isEmpty())
        return nullptr;
    it = ctx->constFind(hold ? kHoldPrefix + alias : alias);
    return it != ctx->constEnd() ? &*it : nullptr;
}

void ActionRouter::fire(const Binding &binding)
{
    const int colon = binding.action.indexOf(QLatin1Char(':'));
    if (colon < 0)
        emit action(binding.action, QString());
    else
        emit action(binding.action.left(colon), binding.action.mid(colon + 1));
}

void ActionRouter::onPressed(const QString &button)
{
    m_repeatTimer.stop();
    m_holdTimer.stop();
    m_activeButton = button;
    m_holdFired = false;

    if (lookup(kHoldPrefix + button)) {
        m_holdTimer.start(m_holdMs); // tap is decided on release
        return;
    }

    const Binding *b = lookup(button);
    if (!b)
        return;
    fire(*b);

    const QString base = stickFallback(button);
    if (m_repeatButtons.contains(button) || (!base.isEmpty() && m_repeatButtons.contains(base)))
        m_repeatTimer.start(m_repeatDelayMs);
}

void ActionRouter::onReleased(const QString &button)
{
    if (button != m_activeButton)
        return;
    m_repeatTimer.stop();

    if (m_holdTimer.isActive()) {
        m_holdTimer.stop();
        if (const Binding *b = lookup(button))
            fire(*b);
    }
    m_activeButton.clear();
}

void ActionRouter::inject(const QString &button, bool pressed)
{
    if (pressed)
        onPressed(button);
    else
        onReleased(button);
}
