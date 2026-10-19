#include "Platform.h"

#include <QCoreApplication>
#include <QDBusConnection>
#include <QDBusMessage>
#include <QDBusReply>
#include <QDesktopServices>
#include <QDir>
#include <QFile>
#include <QGuiApplication>
#include <QKeyEvent>
#include <QKeySequence>
#include <QSettings>
#include <QStandardPaths>
#include <QUrl>
#include <QWindow>
#include <QDebug>

namespace {
const QString kSsService = QStringLiteral("org.freedesktop.ScreenSaver");
const QString kSsPath = QStringLiteral("/org/freedesktop/ScreenSaver");
} // namespace

Platform::Platform(const Options &options, QObject *parent)
    : QObject(parent)
    , m_options(options)
{
}

Platform::~Platform()
{
    setInhibitSleep(false);
}

QString Platform::findWidevine()
{
    const QString lib = QStringLiteral("libwidevinecdm.so");
    QStringList candidates;
    if (qEnvironmentVariableIsSet("STREAMDECK_WIDEVINE_PATH"))
        candidates << qEnvironmentVariable("STREAMDECK_WIDEVINE_PATH");
    // Where scripts/install-widevine.sh puts it (also works inside Flatpak).
    candidates << QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                      + QStringLiteral("/streamdeck/widevine/_platform_specific/linux_x64/") + lib;
    // System browsers that already ship the CDM.
    candidates << QStringLiteral("/opt/google/chrome/WidevineCdm/_platform_specific/linux_x64/") + lib
               << QStringLiteral("/usr/lib/chromium/WidevineCdm/_platform_specific/linux_x64/") + lib
               << QStringLiteral("/usr/lib64/chromium-browser/WidevineCdm/_platform_specific/linux_x64/") + lib;

    for (const QString &path : std::as_const(candidates)) {
        if (QFile::exists(path))
            return path;
    }
    return {};
}

bool Platform::gamingMode() const
{
    // Steam sets these for apps launched from Game Mode (gamescope session).
    return qEnvironmentVariableIsSet("SteamGamepadUI") || qEnvironmentVariableIsSet("SteamDeck")
           || qEnvironmentVariableIsSet("GAMESCOPE_WAYLAND_DISPLAY");
}

int Platform::keepWarmMinutes() const
{
    // How long a backgrounded service stays frozen in memory before it is
    // discarded. 0 discards immediately (lowest memory, slowest resume).
    return QSettings().value(QStringLiteral("session/keepWarmMinutes"), 10).toInt();
}

void Platform::setInhibitSleep(bool inhibit)
{
    if (inhibit == (m_inhibitCookie != 0))
        return;
    auto bus = QDBusConnection::sessionBus();
    if (inhibit) {
        auto msg = QDBusMessage::createMethodCall(kSsService, kSsPath, kSsService, QStringLiteral("Inhibit"));
        msg << QCoreApplication::applicationName() << QStringLiteral("Playing video");
        QDBusReply<uint> reply = bus.call(msg, QDBus::Block, 500);
        if (reply.isValid())
            m_inhibitCookie = reply.value();
        else
            qInfo() << "Screen saver inhibit unavailable:" << reply.error().message();
    } else {
        auto msg = QDBusMessage::createMethodCall(kSsService, kSsPath, kSsService, QStringLiteral("UnInhibit"));
        msg << m_inhibitCookie;
        bus.call(msg, QDBus::NoBlock);
        m_inhibitCookie = 0;
    }
}

void Platform::showKeyboard()
{
    if (!gamingMode())
        return; // on a desktop there's a real keyboard
    QDesktopServices::openUrl(QUrl(QStringLiteral("steam://open/keyboard")));
}

void Platform::sendKey(const QString &key)
{
    const QKeySequence seq = QKeySequence::fromString(key);
    QWindow *window = QGuiApplication::focusWindow();
    if (seq.isEmpty() || !window)
        return;
    const QKeyCombination combo = seq[0];
    const QString text = key.size() == 1 ? key.toLower() : (key == QLatin1String("Space") ? QStringLiteral(" ") : QString());
    QKeyEvent press(QEvent::KeyPress, combo.key(), combo.keyboardModifiers(), text);
    QKeyEvent release(QEvent::KeyRelease, combo.key(), combo.keyboardModifiers(), text);
    QCoreApplication::sendEvent(window, &press);
    QCoreApplication::sendEvent(window, &release);
}
