#include "ElevationStore.h"
#include "DtmReader.h"

#include "SettingsHandler.h"

#include <iostream>

#include <QRegularExpression>

// Initialize static members
ElevationStore* ElevationStore::instancePtr = nullptr;
std::mutex ElevationStore::mtx;

ElevationStore::ElevationStore() : QObject()
{
    loadSettings();
}

ElevationStore::~ElevationStore()
{
}

void ElevationStore::loadSettings()
{
    SettingsHandler* settingsHandler = SettingsHandler::instance();

    if(!settingsHandler->settingsLoaded())
    {
        return;
    }

    const QByteArray envSrcPath = qgetenv("SRC_PATH");
    const QString relativePath = settingsHandler->dtmTilePath();
    if(!envSrcPath.isEmpty() && !relativePath.isEmpty())
    {
        QString dtmResourcesPath = QString::fromUtf8(envSrcPath) + "/"+ relativePath;
        m_dtmResourcePath = dtmResourcesPath.toStdString();
    }

    m_pointsPerTile = settingsHandler->dtmTileResolution();

    const QString topLeftTile = settingsHandler->dtmTopLeftTile();
    const QString bottomRightTile = settingsHandler->dtmBottomRightTile();

    if(topLeftTile.front() != 'N' || bottomRightTile.front() != 'N')
    {
        qWarning() << __FUNCTION__ << " invalid tile";
        return;
    }

    const QRegularExpression re("[EW]");

    auto getLatitude = [&](QString str)
    {
        const int8_t prefixIndex = str.indexOf(re);
        if(prefixIndex == -1)
        {
            qWarning() << __FUNCTION__ << " invalid tile latitude";
            return 0;
        }
        const QString prefix = str.mid(1,prefixIndex-1);
        return prefix.toInt();
    };

    auto getLongitude = [&](QString str)
    {
        const int8_t prefixIndex = str.indexOf(re);
        if(prefixIndex == -1)
        {
            qWarning() << __FUNCTION__ << " invalid tile longitude";
            return 0;
        }

        const int8_t dotIndex = str.indexOf(".");
        QString suffix;
        if(dotIndex == -1)
        {
            suffix = str.mid(prefixIndex + 1);
        }
        else
        {
            suffix = str.mid(prefixIndex + 1, str.size() - (dotIndex+1));
        }
        return suffix.toInt();
    };

    uint8_t topLeftLatitude = getLatitude(topLeftTile);
    uint8_t topLeftLongitude = getLongitude(topLeftTile);
    uint8_t bottomRightLatitude = getLatitude(bottomRightTile);
    uint8_t bottomRightLongitude = getLongitude(bottomRightTile);

    for(uint8_t lat = topLeftLatitude; lat <= bottomRightLatitude; ++lat)
    {
        m_tilesLatitude.push_back(lat);
    }

    for(uint8_t lon = topLeftLongitude; lon <= bottomRightLongitude; ++lon)
    {
        m_tilesLongitude.push_back(lon);
        //todo: add E or W
        m_tilesLonHemisphere.push_back('E');
    }

    m_longitudeStart = m_tilesLongitude.front();
    m_latitudeStart = m_tilesLatitude.front();
    m_longitudeEnd = m_tilesLongitude.back() + 1;
    m_latitudeEnd = m_tilesLatitude.back() + 1;

    //todo: add leading zero yes or no
    m_leadingZero = true;

    m_initialized = true;
}

void ElevationStore::loadElevations()
{
    if(m_dtmResourcePath.empty())
    {
        return;
    }

    std::cout << __FUNCTION__ << " start" << std::endl;

    const uint8_t tileCountLat = m_tilesLatitude.size();
    const uint8_t tileCountLon = m_tilesLongitude.size();

    const uint32_t totalPointsLat = tileCountLat * m_pointsPerTile;
    const uint32_t totalPointsLon = tileCountLon * m_pointsPerTile;

    m_elevationData.resize(totalPointsLat * totalPointsLon);

    uint32_t offsetX = 0; // srtmSize;
    uint32_t offsetY = 0; // srtmSize;

    for (uint8_t y = 0; y < tileCountLat; ++y)
    {
        for (uint8_t x = 0; x < tileCountLon; ++x)
        {
            // const std::string strLon = std::to_string(m_tilesLongitude[x]);
            std::string leadingZero = "";
            if(m_leadingZero)
            {
                leadingZero = "0";
                if(m_tilesLongitude[x] < 10)
                {
                    leadingZero += "0";
                }
            }

            const std::string file = "N" + std::to_string(m_tilesLatitude[y]) + m_tilesLonHemisphere[x] + leadingZero + std::to_string(m_tilesLongitude[x]);
            readHgtFile(file, offsetX, offsetY, totalPointsLon);
            offsetX += m_pointsPerTile;
        }
        offsetX = 0;
        offsetY += m_pointsPerTile;
    }

    std::cout << __FUNCTION__ << " end" << std::endl;
}

void ElevationStore::readHgtFile(const std::string strFile, const uint32_t offsetX, const uint32_t offsetY, const uint32_t sizeX)
{

    if(m_dtmResourcePath.empty())
    {
        return;
    }
    std::string hgtPath = m_dtmResourcePath;
    // if(m_pointsPerTile == globals::TilePixels1ArcSecond)
    // {
    //     hgtPath += globals::DtmFolder_1ArcSecond + "/";
    // }
    // else if(m_pointsPerTile == globals::TilePixels3ArcSecond)
    // {
    //     hgtPath += globals::DtmFolder_3ArcSecond + "/";
    // }

    const uint32_t elevationPoints = m_pointsPerTile * m_pointsPerTile;
    std::vector<int16_t> tempElevationData;
    tempElevationData.resize(elevationPoints);

    const bool result = DtmReader::readElevationData(hgtPath+strFile+".hgt", m_pointsPerTile, tempElevationData);

    uint32_t tempY = 0;
    for (uint32_t y = offsetY; y < offsetY + m_pointsPerTile; ++y)
    {
        uint32_t tempX = 0;
        for (uint32_t x = offsetX; x < offsetX + m_pointsPerTile; ++x)
        {
            const uint32_t index = x + y * sizeX;
            int16_t height = 0;
            if(result)
            {
                const uint32_t tempIndex = tempX + tempY * m_pointsPerTile;
                height = tempElevationData[tempIndex];
            }
            m_elevationData[index] = height;

            if(height > 10000)
            {
                qDebug();
            }

            ++tempX;
        }
        ++tempY;
    }
}

QGeoCoordinate ElevationStore::requestHighestPoint(const QGeoCoordinate coordinate, const float radius)
{
    if(!coordsInBounds(coordinate))
    {
        qWarning() << __FUNCTION__ << " coord " << coordinate << " is out of bounds!";
        return QGeoCoordinate(0,0);
    }

    const QPointF center = convertLatLongToPos(coordinate);
    const uint32_t center_x = center.x();
    const uint32_t center_y = center.y();
    const uint32_t x_min = center_x - radius;
    const uint32_t y_min = center_y - radius;
    const uint32_t x_max = center_x + radius;
    const uint32_t y_max = center_y + radius;
    const uint32_t totalPointsLon = m_tilesLongitude.size() * m_pointsPerTile;

    int16_t max_height = -10000;
    QPoint point{0,0};

    for (uint32_t y = y_min; y <= y_max; ++y)
    {
        for (uint32_t x = x_min; x <= x_max; ++x)
        {
            const uint32_t index = x + y * totalPointsLon;
            const int16_t height = m_elevationData[index];

            if (max_height < height)
            {
                max_height = height;
                point = QPoint(x,y);
            }
        }
    }

    const QGeoCoordinate coord = convertPosToLatLong(point);

    //const float radiusInMeter = radius * m_arcSeconds * globals::ArcSecondToMeters;
    //qDebug() << "highest height in " << radiusInMeter << "m radius around " << coordinate << " : " << max_height << "m at " << coord;

    return coord;
}

QGeoCoordinate ElevationStore::requestLowestPoint(const QGeoCoordinate coordinate, const float radius)
{
    if(!coordsInBounds(coordinate))
    {
        qWarning() << __FUNCTION__ << " coord " << coordinate << " is out of bounds!";
        return QGeoCoordinate(0,0);
    }

    const QPointF center = convertLatLongToPos(coordinate);
    const uint32_t center_x = std::round(center.x());
    const uint32_t center_y = std::round(center.y());
    const uint32_t x_min = center_x - radius;
    const uint32_t y_min = center_y - radius;
    const uint32_t x_max = center_x + radius;
    const uint32_t y_max = center_y + radius;
    const uint32_t totalPointsLon = m_tilesLongitude.size() * m_pointsPerTile;

    int16_t min_height = 10000;
    QPoint point{0,0};

    for (uint32_t y = y_min; y <= y_max; ++y)
    {
        for (uint32_t x = x_min; x <= x_max; ++x)
        {
            const uint32_t index = x + y * totalPointsLon;
            const int16_t height = m_elevationData[index];

            if (min_height > height)
            {
                min_height = height;
                point = QPoint(x,y);
            }
        }
    }

    const QGeoCoordinate coord = convertPosToLatLong(point);

    //const float radiusInMeter = radius * m_arcSeconds * globals::ArcSecondToMeters;
    //qDebug() << "lowest height in " << radiusInMeter << "m radius around " << coordinate << " : " << min_height << "m at " << coord;

    return coord;
}

QPointF ElevationStore::convertLatLongToPos(const QGeoCoordinate coordinate)
{
    const uint8_t longitudeStart = m_tilesLongitude.front();
    const uint8_t latitudeStart = m_tilesLatitude.front();

    QPointF scale{1.0,1.0};
    QPointF point;
    const double x = (coordinate.longitude() - longitudeStart) * m_pointsPerTile;
    const double y = (coordinate.latitude() - latitudeStart)  * m_pointsPerTile;
    point.setX(scale.x() * x);
    point.setY(scale.y() * y);

    return point;
}

QGeoCoordinate ElevationStore::convertPosToLatLong(const QPointF point)
{
    const uint8_t longitudeStart = m_tilesLongitude.front();
    const uint8_t latitudeStart = m_tilesLatitude.front();
    const uint8_t longitudeEnd = m_tilesLongitude.back() + 1;
    const uint8_t latitudeEnd = m_tilesLatitude.back() + 1;

    QGeoCoordinate coordinate;
    const double xRelativeWorld = point.x() / (m_pointsPerTile * m_tilesLongitude.size());
    const double yRelativeWorld = point.y() / (m_pointsPerTile * m_tilesLatitude.size());
    const double xRelativeMap = xRelativeWorld * (longitudeEnd - longitudeStart);
    const double yRelativeMap = yRelativeWorld * (latitudeEnd - latitudeStart);
    const double longitude = xRelativeMap + longitudeStart;
    const double latitude = yRelativeMap + latitudeStart;
    coordinate = QGeoCoordinate(latitude, longitude);

    return coordinate;
}

bool ElevationStore::coordsInBounds(const QGeoCoordinate coordinate)
{
    return (coordinate.latitude() >= m_latitudeStart && coordinate.latitude() <= m_latitudeEnd && coordinate.longitude() >= m_longitudeStart && coordinate.longitude() <= m_longitudeEnd);
}

bool ElevationStore::coordsInBounds(const double lat, const double lon)
{
    return (lat >= m_latitudeStart && lat <= m_latitudeEnd && lon >= m_longitudeStart && lon <= m_longitudeEnd);
}

float ElevationStore::distanceLatLongToMeters(const QGeoCoordinate coord1, const QGeoCoordinate coord2)
{
    const double rad2degree = M_PI / 180.0;
    const double radiusEarth = 6378.137; // Radius of earth in KM
    const double dLat = fabs(coord2.latitude() - coord1.latitude()) * rad2degree;
    const double dLon = fabs(coord2.longitude() - coord1.longitude()) * rad2degree;
    const double a = std::pow(std::sin(dLat / 2.0), 2.0) +
                     std::cos(coord1.latitude() * rad2degree) * std::cos(coord2.latitude() * rad2degree) *
                                                               std::pow(std::sin(dLon / 2.0), 2.0);
    const double c = 2.0 * std::atan2(std::sqrt(a), std::sqrt(1.0 - a));
    const double d = radiusEarth * c;
    const double distance = (d * 1000.0); // meters

    qDebug() << "distance between " << coord1 << " and " << coord2 << " is " << distance << " m";

    return distance;
}

void ElevationStore::requestHeights(const QGeoCoordinate mapCenter, const float zoomLevel, const QGeoRectangle mapBounds)
{
    const QGeoCoordinate rectBottomLeft = mapBounds.bottomLeft();

    const QGeoCoordinate rectTopRight = mapBounds.topRight();

    const double dLatitude = rectTopRight.latitude() - rectBottomLeft.latitude();
    const double latitudeStep = dLatitude / m_heightCount;

    const double dLongitude = rectTopRight.longitude() - rectBottomLeft.longitude();
    const double longitudeStep = dLongitude / m_heightCount;

    const uint32_t totalPointsLon = m_tilesLongitude.size() * m_pointsPerTile;

    if(m_requestedHeights.empty() || (m_requestedHeights.size() != m_heightCount * m_heightCount))
    {
        m_requestedHeights.clear();
        m_requestedHeights.resize(m_heightCount * m_heightCount);
    }

    const uint8_t longitudeStart = m_tilesLongitude.front();
    const uint8_t latitudeStart = m_tilesLatitude.front();

    int16_t minHeight{10000};
    int16_t maxHeight{-10000};

    bool allCornersInBounds = false;
    if(coordsInBounds(rectBottomLeft.latitude(), rectBottomLeft.longitude()) && coordsInBounds(rectTopRight.latitude(), rectTopRight.longitude()))
    {
        allCornersInBounds = true;
    }

    for (uint32_t y = 0; y < m_heightCount; ++y)
    {
        for (uint32_t x = 0; x < m_heightCount; ++x)
        {
            const double pointLat = rectBottomLeft.latitude() + y*latitudeStep;
            const double pointLon = rectBottomLeft.longitude() + x*longitudeStep;
            int16_t height = -9999;

            if(allCornersInBounds || coordsInBounds(pointLat, pointLon))
            {
                const uint32_t x = (uint32_t)((pointLon - longitudeStart) * m_pointsPerTile);
                const uint32_t y = (uint32_t)((pointLat - latitudeStart)  * m_pointsPerTile);
                const uint32_t elevationDataIndex = x + y * totalPointsLon;
                height = m_elevationData[elevationDataIndex];

                if(height < minHeight)
                {
                    minHeight = height;
                }
                else if(height > maxHeight)
                {
                    maxHeight = height;
                }
            }

            const uint32_t index = x + y * m_heightCount;
            m_requestedHeights[index] = height;
        }
    }

    setMinHeight(minHeight);
    setMaxHeight(maxHeight);
}

std::vector<int16_t>& ElevationStore::requestedHeights()
{
    return m_requestedHeights;
}

void ElevationStore::setMinHeight(int16_t value)
{
    if (m_minHeight == value)
        return;

    m_minHeight = value;
    emit minHeightChanged();
}

void ElevationStore::setMaxHeight(int16_t value)
{
    if (m_maxHeight == value)
        return;

    m_maxHeight = value;
    emit maxHeightChanged();
}


void ElevationStore::setHeightCount(uint32_t count)
{
   if (m_heightCount == count)
       return;

   m_heightCount = count;
   emit heightCountChanged();
}
