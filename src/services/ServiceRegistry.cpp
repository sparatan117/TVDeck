#include "ServiceRegistry.h"

#include <QDir>
#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QStandardPaths>
#include <QDebug>

namespace {
QString readFile(const QString &path)
{
    QFile f(path);
    if (!f.open(QIODevice::ReadOnly)) {
        qWarning() << "Missing service script" << path;
        return {};
    }
    return QString::fromUtf8(f.readAll());
}
} // namespace

ServiceRegistry::ServiceRegistry(QObject *parent)
    : QObject(parent)
{
    loadDir(QStringLiteral(":/services"), QStringLiteral(":/scripts"));

    const QString user = QStandardPaths::writableLocation(QStandardPaths::GenericDataLocation)
                         + QStringLiteral("/sofa");
    loadDir(user + QStringLiteral("/services"), user + QStringLiteral("/scripts"));
}

void ServiceRegistry::loadDir(const QString &dir, const QString &scriptRoot)
{
    const QDir d(dir);
    for (const QString &name : d.entryList({QStringLiteral("*.json")}, QDir::Files, QDir::Name)) {
        QFile f(d.filePath(name));
        if (!f.open(QIODevice::ReadOnly))
            continue;
        QJsonParseError err;
        QVariantMap svc = QJsonDocument::fromJson(f.readAll(), &err).object().toVariantMap();
        if (err.error != QJsonParseError::NoError || svc.value(QStringLiteral("id")).toString().isEmpty()) {
            qWarning() << "Ignoring invalid service file" << f.fileName() << err.errorString();
            continue;
        }
        svc.insert(QStringLiteral("scriptRoot"), scriptRoot);

        const QString id = svc.value(QStringLiteral("id")).toString();
        bool replaced = false;
        for (auto &existing : m_services) {
            if (existing.toMap().value(QStringLiteral("id")).toString() == id) {
                existing = svc;
                replaced = true;
            }
        }
        if (!replaced)
            m_services.append(svc);
    }
}

QVariantMap ServiceRegistry::service(const QString &id) const
{
    for (const auto &v : m_services) {
        const QVariantMap m = v.toMap();
        if (m.value(QStringLiteral("id")).toString() == id)
            return m;
    }
    return {};
}

QString ServiceRegistry::scriptBundle(const QString &id) const
{
    const QVariantMap svc = service(id);
    const QString root = svc.value(QStringLiteral("scriptRoot")).toString();
    QString bundle;
    for (const auto &s : svc.value(QStringLiteral("scripts")).toList()) {
        const QString rel = s.toString();
        // User-supplied scripts win, built-ins fill the gaps (shared helpers).
        QString path = root + QLatin1Char('/') + rel;
        if (!QFile::exists(path))
            path = QStringLiteral(":/scripts/") + rel;
        bundle += QStringLiteral("\n;// ---- %1\n").arg(rel) + readFile(path);
    }
    return bundle;
}
