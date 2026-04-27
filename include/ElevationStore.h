#pragma once

#include <cstdint>
#include <string>
#include <vector>
#include <mutex>

#include <QObject>
#include <QPointF>
#include <QGeoCoordinate>
#include <QGeoRectangle>

class DtmReader;

class ElevationStore : public QObject
{
    Q_OBJECT

public:
    explicit ElevationStore();
    ~ElevationStore() override;

    Q_PROPERTY(uint32_t heightCount READ heightCount WRITE setHeightCount NOTIFY heightCountChanged)
    Q_PROPERTY(int16_t minHeight READ minHeight WRITE setMinHeight NOTIFY minHeightChanged)
    Q_PROPERTY(int16_t maxHeight READ maxHeight WRITE setMaxHeight NOTIFY maxHeightChanged)

    static ElevationStore* instance() {
        if (instancePtr == nullptr) {
            std::lock_guard<std::mutex> lock(mtx);
            if (instancePtr == nullptr) {
                instancePtr = new ElevationStore();
            }
        }
        return instancePtr;
    }

    void loadElevations();

    Q_INVOKABLE QGeoCoordinate requestHighestPoint(const QGeoCoordinate coordinate, const float radius);
    Q_INVOKABLE QGeoCoordinate requestLowestPoint(const QGeoCoordinate coordinate, const float radius);
    Q_INVOKABLE QPoint convertLatLongToPos(const QGeoCoordinate coordinate);
    Q_INVOKABLE QGeoCoordinate convertPosToLatLong(const QPointF point);
    Q_INVOKABLE float distanceLatLongToMeters(const QGeoCoordinate coord1, const QGeoCoordinate coord2);
    Q_INVOKABLE void requestHeights(const QGeoCoordinate mapCenter, const float zoomLevel, const QGeoRectangle mapBounds);
    Q_INVOKABLE QGeoCoordinate getNewCoordinate(const QGeoCoordinate oldCoord, const float dxMeters, const float dyMeters);

    std::vector<int16_t>& requestedHeights();

    int16_t minHeight() const { return m_minHeight; }
    void setMinHeight(int16_t value);

    int16_t maxHeight() const { return m_maxHeight; }
    void setMaxHeight(int16_t value);

    uint32_t heightCount() const { return m_heightCount; }
    void setHeightCount(uint32_t count);

signals:
    void minHeightChanged();
    void maxHeightChanged();
    void heightCountChanged();

private:
    void loadSettings();

    bool m_initialized{false};

    // Mutex to ensure thread safety
    static std::mutex mtx;

    // Static pointer to the Singleton instance
    static ElevationStore* instancePtr;

    void readHgtFile(const std::string strFile, const uint32_t offsetX, const uint32_t offsetY, const uint32_t sizeX);
    bool coordsInBounds(const QGeoCoordinate coordinate);
    bool coordsInBounds(const double lat, const double lon);

    const uint32_t m_StartHeightCount{512};

    std::vector<int16_t> m_elevationData;
    std::string m_dtmResourcePath;
    //uint32_t m_arcSeconds{3};
    uint32_t m_pointsPerTile{0};

    std::vector<int16_t> m_requestedHeights;

    int16_t m_minHeight;
    int16_t m_maxHeight;
    uint32_t m_heightCount{m_StartHeightCount};

    std::vector<uint8_t> m_tilesLatitude;//{43,44,45,46,47,48,49,50};
    std::vector<uint8_t> m_tilesLongitude;//{5,6,7,8,9,10,11,12,13,14,15,16};
    std::vector<char> m_tilesLonHemisphere;
    bool m_leadingZero{false};

    uint8_t m_longitudeStart;
    uint8_t m_latitudeStart;
    uint8_t m_longitudeEnd;
    uint8_t m_latitudeEnd;
};
