#pragma once

#include <mutex>

#include <QObject>
#include <QString>
#include <QSettings>

class SettingsHandler : public QObject
{
    Q_OBJECT

public:
    explicit SettingsHandler();
    ~SettingsHandler() override;

    Q_PROPERTY(bool settingsLoaded READ settingsLoaded WRITE setSettingsLoaded NOTIFY settingsLoadedChanged)

    Q_PROPERTY(uint16_t mapResolution READ mapResolution WRITE setMapResolution NOTIFY mapResolutionChanged)
    Q_PROPERTY(bool showLegend READ showLegend WRITE setShowLegend NOTIFY showLegendChanged)
    Q_PROPERTY(QString tileServerUrl READ tileServerUrl WRITE setTileServerUrl NOTIFY tileServerUrlChanged)

    Q_PROPERTY(QString dtmTilePath READ dtmTilePath WRITE setDtmTilePath NOTIFY dtmTilePathChanged)
    Q_PROPERTY(QString dtmTileFormat READ dtmTileFormat WRITE setDtmTileFormat NOTIFY dtmTileFormatChanged)
    Q_PROPERTY(uint16_t dtmTileResolution READ dtmTileResolution WRITE setDtmTileResolution NOTIFY dtmTileResolutionChanged)
    Q_PROPERTY(QString dtmTopLeftTile READ dtmTopLeftTile WRITE setDtmTopLeftTile NOTIFY dtmTopLeftTileChanged)
    Q_PROPERTY(QString dtmBottomRightTile READ dtmBottomRightTile WRITE setDtmBottomRightTile NOTIFY dtmBottomRightTileChanged)


    // Static method to get the Singleton instance
    static SettingsHandler* instance() {
        if (instancePtr == nullptr) {
            std::lock_guard<std::mutex> lock(mtx);
            if (instancePtr == nullptr) {
                instancePtr = new SettingsHandler();
            }
        }
        return instancePtr;
    }

    void loadSettings();
    void saveSettings();

    bool settingsLoaded() const { return m_settingsLoaded; }
    void setSettingsLoaded(bool value);

    uint16_t mapResolution() const { return m_mapResolution; }
    void setMapResolution(uint16_t value);
    bool showLegend() const { return m_showLegend; }
    void setShowLegend(bool value);
    QString tileServerUrl() const { return m_tileServerUrl; }
    void setTileServerUrl(QString value);

    QString dtmTilePath() const { return m_dtmTilePath; }
    void setDtmTilePath(QString value);
    QString dtmTileFormat() const { return m_dtmTileFormat; }
    void setDtmTileFormat(QString value);
    uint16_t dtmTileResolution() const { return m_dtmTileResolution; }
    void setDtmTileResolution(uint16_t value);
    QString dtmTopLeftTile() const { return m_dtmTopLeftTile; }
    void setDtmTopLeftTile(QString value);
    QString dtmBottomRightTile() const { return m_dtmBottomRightTile; }
    void setDtmBottomRightTile(QString value);

signals:
    void settingsLoadedChanged();

    void mapResolutionChanged();
    void showLegendChanged();
    void tileServerUrlChanged();

    void dtmTilePathChanged();
    void dtmTileFormatChanged();
    void dtmTileResolutionChanged();
    void dtmTopLeftTileChanged();
    void dtmBottomRightTileChanged();

private:
    // Mutex to ensure thread safety
    static std::mutex mtx;

    // Static pointer to the Singleton instance
    static SettingsHandler* instancePtr;

    QScopedPointer<QSettings> m_settings;
    QString m_filePath;
    QString m_fileName{"settings.ini"};

    bool m_settingsLoaded{false};

    uint16_t m_mapResolution;
    bool m_showLegend;
    QString m_tileServerUrl;

    QString m_dtmTilePath;
    QString m_dtmTileFormat;
    uint16_t m_dtmTileResolution;
    QString m_dtmTopLeftTile;
    QString m_dtmBottomRightTile;
};