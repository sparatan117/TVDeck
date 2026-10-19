#pragma once

#include <QObject>

// SteamOS / desktop integration that doesn't belong to input or services.
class Platform : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool drmAvailable READ drmAvailable CONSTANT)
    Q_PROPERTY(QString widevinePath READ widevinePath CONSTANT)
    Q_PROPERTY(bool gamingMode READ gamingMode CONSTANT)
    Q_PROPERTY(bool windowed READ windowed CONSTANT)
    Q_PROPERTY(QString startupService READ startupService CONSTANT)
    Q_PROPERTY(int keepWarmMinutes READ keepWarmMinutes CONSTANT)

public:
    struct Options {
        QString widevinePath;
        QString startupService;
        bool windowed = false;
    };

    explicit Platform(const Options &options, QObject *parent = nullptr);
    ~Platform() override;

    // Locates libwidevinecdm.so. Must run before Qt WebEngine is initialized.
    static QString findWidevine();

    bool drmAvailable() const { return !m_options.widevinePath.isEmpty(); }
    QString widevinePath() const { return m_options.widevinePath; }
    bool gamingMode() const;
    bool windowed() const { return m_options.windowed; }
    QString startupService() const { return m_options.startupService; }
    int keepWarmMinutes() const;

    // Keeps the screen on during playback (org.freedesktop.ScreenSaver).
    Q_INVOKABLE void setInhibitSleep(bool inhibit);

    // Opens Steam's on-screen keyboard when running under Steam.
    Q_INVOKABLE void showKeyboard();

    // Sends a trusted key press/release to the focused window. Accepts
    // QKeySequence names: "Space", "Left", "Escape", "S", ...
    Q_INVOKABLE void sendKey(const QString &key);

private:
    Options m_options;
    uint m_inhibitCookie = 0;
};
