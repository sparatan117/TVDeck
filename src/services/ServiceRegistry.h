#pragma once

#include <QObject>
#include <QVariantList>

// Loads service definitions (services/*.json) from the built-in resources and
// from $XDG_DATA_HOME/sofa/services, so users can add or patch services
// without rebuilding. A user file with the same "id" replaces the built-in.
class ServiceRegistry : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QVariantList services READ services CONSTANT)

public:
    explicit ServiceRegistry(QObject *parent = nullptr);

    QVariantList services() const { return m_services; }
    Q_INVOKABLE QVariantMap service(const QString &id) const;

    // All of a service's page scripts concatenated, ready for injection.
    Q_INVOKABLE QString scriptBundle(const QString &id) const;

private:
    void loadDir(const QString &dir, const QString &scriptRoot);

    QVariantList m_services;
};
