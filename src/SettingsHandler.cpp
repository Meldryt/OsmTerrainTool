#include "SettingsHandler.h"

// Initialize static members
SettingsHandler* SettingsHandler::instancePtr = nullptr;
std::mutex SettingsHandler::mtx;

SettingsHandler::SettingsHandler() : QObject()
{
    const QByteArray envSrcPath = qgetenv("SRC_PATH");
    QString srcPath;
    if(!envSrcPath.isEmpty())
    {
        srcPath = QString::fromUtf8(envSrcPath);
        m_filePath = srcPath + "/" +  m_fileName;
    }

    m_settings.reset(new QSettings(m_filePath,
                                   QSettings::IniFormat));

}

SettingsHandler::~SettingsHandler()
{
}

void SettingsHandler::loadSettings()
{
    if(m_filePath.isEmpty())
    {
        return;
    }

    m_settings->beginGroup("map");
    setMapResolution(m_settings->value("map_resolution").toInt());
    setShowLegend(m_settings->value("show_legend").toBool());
    setTileServerUrl(m_settings->value("tile_server_url").toString());
    m_settings->endGroup();

    m_settings->beginGroup("elevation_data");
    setDtmTilePath(m_settings->value("dtm_tile_path").toString());
    setDtmTileFormat(m_settings->value("dtm_tile_format").toString());
    setDtmTileResolution(m_settings->value("dtm_tile_resolution").toUInt());
    setDtmTopLeftTile(m_settings->value("dtm_top_left_tile").toString());
    setDtmBottomRightTile(m_settings->value("dtm_bottom_right_tile").toString());
    m_settings->endGroup();

    setSettingsLoaded(true);

    qDebug() << __FUNCTION__ << "\n"
    << "mapResolution: " << m_mapResolution << "\n"
    << "showLegend: " << m_showLegend << "\n"
    << "tileServerUrl: " << m_tileServerUrl << "\n"
    << "dtmTilePath: " << m_dtmTilePath << "\n"
    << "dtmTileFormat: " << m_dtmTileFormat << "\n"
    << "dtmTileResolution: " << m_dtmTileResolution << "\n"
    << "dtmTopLeftTile: " << m_dtmTopLeftTile << "\n"
    << "dtmBottomRightTile: " << m_dtmBottomRightTile << "\n";
}

void SettingsHandler::saveSettings()
{
    if(m_filePath.isEmpty())
    {
        return;
    }

    m_settings->beginGroup("main");
    m_settings->setValue("map_resolution", m_mapResolution);
    m_settings->setValue("show_legend", m_showLegend);
    m_settings->setValue("tile_server_url", m_tileServerUrl);
    m_settings->endGroup();

    m_settings->beginGroup("elevation_data");
    m_settings->setValue("dtm_tile_path", m_dtmTilePath);
    m_settings->setValue("dtm_tile_format", m_dtmTileFormat);
    m_settings->setValue("dtm_tile_resolution", m_dtmTileResolution);
    m_settings->setValue("dtm_top_left_tile", m_dtmTopLeftTile);
    m_settings->setValue("dtm_bottom_right_tile", m_dtmBottomRightTile);
    m_settings->endGroup();
}

void SettingsHandler::setSettingsLoaded(bool value)
{
    if (m_settingsLoaded == value)
        return;

    m_settingsLoaded = value;
    emit settingsLoadedChanged();
}

void SettingsHandler::setMapResolution(uint16_t value)
{
    if (m_mapResolution == value)
        return;

    m_mapResolution = value;
    emit mapResolutionChanged();
}

void SettingsHandler::setShowLegend(bool value)
{
    if (m_showLegend == value)
        return;

    m_showLegend = value;
    emit showLegendChanged();
}

void SettingsHandler::setTileServerUrl(QString value)
{
    if (m_tileServerUrl == value)
        return;

    m_tileServerUrl = value;
    emit tileServerUrlChanged();
}

void SettingsHandler::setDtmTilePath(QString value)
{
    if (m_dtmTilePath == value)
        return;

    m_dtmTilePath = value;
    emit dtmTilePathChanged();
}

void SettingsHandler::setDtmTileFormat(QString value)
{
    if (m_dtmTileFormat == value)
        return;

    m_dtmTileFormat = value;
    emit dtmTileFormatChanged();
}

void SettingsHandler::setDtmTileResolution(uint16_t value)
{
    if (m_dtmTileResolution == value)
        return;

    m_dtmTileResolution = value;
    emit dtmTileResolutionChanged();
}

void SettingsHandler::setDtmTopLeftTile(QString value)
{
    if (m_dtmTopLeftTile == value)
        return;

    m_dtmTopLeftTile = value;
    emit dtmTopLeftTileChanged();
}

void SettingsHandler::setDtmBottomRightTile(QString value)
{
    if (m_dtmBottomRightTile == value)
        return;

    m_dtmBottomRightTile = value;
    emit dtmBottomRightTileChanged();
}