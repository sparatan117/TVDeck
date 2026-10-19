#include <QCommandLineParser>
#include <QDebug>
#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQuickWindow>
#include <QStandardPaths>
#include <QtWebEngineQuick/qtwebenginequickglobal.h>

#include "input/ActionRouter.h"
#include "input/GamepadInput.h"
#include "input/VirtualPointer.h"
#include "platform/Platform.h"
#include "services/ServiceRegistry.h"

namespace {
// Chromium switches, tuned for battery life on the Deck's AMD APU.
// Everything here should be backed by a measurement (see docs/POWER.md).
QByteArray chromiumFlags(const QString &widevinePath)
{
    QStringList flags = {
        // Hardware video decode through VA-API (Mesa radeonsi on SteamOS).
        // Decryption stays in the Widevine CDM; decode can still hit the GPU.
        QStringLiteral("--enable-features=VaapiVideoDecoder,VaapiVideoDecodeLinuxGL"),
        QStringLiteral("--enable-gpu-rasterization"),
        QStringLiteral("--enable-zero-copy"),
        // Only one site is ever open; don't spin up spare renderers.
        QStringLiteral("--renderer-process-limit=1"),
        QStringLiteral("--disable-background-networking"),
        QStringLiteral("--disable-component-update"),
        QStringLiteral("--disable-breakpad"),
    };
    if (!widevinePath.isEmpty())
        flags << QStringLiteral("--widevine-path=") + widevinePath;

    // Respect anything the user exported explicitly; theirs come last and win.
    QByteArray result = flags.join(QLatin1Char(' ')).toUtf8();
    const QByteArray user = qgetenv("QTWEBENGINE_CHROMIUM_FLAGS");
    if (!user.isEmpty())
        result += ' ' + user;
    return result;
}
} // namespace

int main(int argc, char *argv[])
{
    QCoreApplication::setOrganizationName(QStringLiteral("streamdeck"));
    QCoreApplication::setApplicationName(QStringLiteral("streamdeck"));
    QCoreApplication::setApplicationVersion(QStringLiteral(PROJECT_VERSION_STRING));

    // Flags must be in the environment before Qt WebEngine starts Chromium.
    const QString widevine = Platform::findWidevine();
    qputenv("QTWEBENGINE_CHROMIUM_FLAGS", chromiumFlags(widevine));
    QtWebEngineQuick::initialize();

    QGuiApplication app(argc, argv);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Controller-first launcher for streaming services"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption windowed(QStringLiteral("windowed"), QStringLiteral("Run in a window instead of fullscreen."));
    QCommandLineOption service(QStringLiteral("service"),
                               QStringLiteral("Skip the launcher and open a service directly (e.g. netflix)."),
                               QStringLiteral("id"));
    parser.addOptions({windowed, service});
    parser.process(app);

    Platform::Options options;
    options.widevinePath = widevine;
    options.windowed = parser.isSet(windowed);
    options.startupService = parser.value(service);
    if (widevine.isEmpty())
        qWarning() << "Widevine CDM not found; DRM video will not play. Run streamdeck-install-widevine.";
    else
        qInfo() << "Using Widevine CDM at" << widevine;

    GamepadInput gamepad;
    ActionRouter router(&gamepad);
    router.loadBindings({
        QStringLiteral(":/config/bindings.json"),
        QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation) + QStringLiteral("/bindings.json"),
    });
    VirtualPointer pointer(&gamepad);
    ServiceRegistry services;
    Platform platform(options);

    // Poll the controller less often when Steam's overlay has focus.
    QObject::connect(&app, &QGuiApplication::applicationStateChanged, &gamepad,
                     [&gamepad](Qt::ApplicationState state) {
                         gamepad.setThrottled(state != Qt::ApplicationActive);
                     });

    qmlRegisterSingletonInstance("StreamDeck.Native", 1, 0, "Gamepad", &gamepad);
    qmlRegisterSingletonInstance("StreamDeck.Native", 1, 0, "Router", &router);
    qmlRegisterSingletonInstance("StreamDeck.Native", 1, 0, "Pointer", &pointer);
    qmlRegisterSingletonInstance("StreamDeck.Native", 1, 0, "Services", &services);
    qmlRegisterSingletonInstance("StreamDeck.Native", 1, 0, "Platform", &platform);

    QQmlApplicationEngine engine;
    QObject::connect(&engine, &QQmlApplicationEngine::objectCreationFailed, &app,
                     [] { QCoreApplication::exit(1); }, Qt::QueuedConnection);
    engine.load(QUrl(QStringLiteral("qrc:/StreamDeck/qml/Main.qml")));
    if (engine.rootObjects().isEmpty())
        return 1;

    pointer.setWindow(qobject_cast<QQuickWindow *>(engine.rootObjects().constFirst()));
    return app.exec();
}
