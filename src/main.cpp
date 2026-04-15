#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QDir>
#include <QLoggingCategory>

#include "SettingsHandler.h"
#include "ElevationStore.h"

using namespace Qt::Literals::StringLiterals;

int main(int argc, char* argv[])
{
    QGuiApplication app(argc, argv);

    QQmlApplicationEngine engine;

    QCoreApplication::setApplicationName("OsmTerrainTool");
    QCoreApplication::setApplicationVersion("1.0.0");

    QLoggingCategory::setFilterRules("qt.network.http2=false");

    const QByteArray envSrcPath = qgetenv("SRC_PATH");
    QString srcPath;
    if(!envSrcPath.isEmpty())
    {
        srcPath = QString::fromUtf8(envSrcPath);
        QUrl urlGeoJson = QUrl(QStringLiteral("file://")
                        + srcPath
                        + QStringLiteral("/resources/maps"));
        qDebug() << " url: " << urlGeoJson;
        engine.rootContext()->setContextProperty("GeoJsonMapsPath", urlGeoJson);

        QUrl urlProjects = QUrl(QStringLiteral("file://")
                           + srcPath
                           + QStringLiteral("/resources/projects"));
        qDebug() << " url: " << urlProjects;
        engine.rootContext()->setContextProperty("ProjectsPath", urlProjects);
    }


    SettingsHandler* settingsHandler = SettingsHandler::instance();
    settingsHandler->loadSettings();

    QUrl urlDtmTilePath = QUrl(QStringLiteral("file://") + settingsHandler->dtmTilePath());
    engine.rootContext()->setContextProperty("DtmMapsPath", urlDtmTilePath);

    ElevationStore* elevationStore = ElevationStore::instance();
    elevationStore->loadElevations();

    engine.rootContext()->setContextProperty("SettingsHandler", settingsHandler);
    engine.rootContext()->setContextProperty("ElevationStore", elevationStore);

    engine.load(QUrl(QStringLiteral("qrc:/Main.qml")));

    QObject::connect(
        &engine, &QQmlApplicationEngine::objectCreationFailed, &app, []() { QCoreApplication::exit(-1); },
        Qt::QueuedConnection);

    if (engine.rootObjects().isEmpty())
        return -1;

    return app.exec();
}