import QtQuick
import QtQuick.Dialogs
import QtQuick.Controls
import QtQuick.Layouts
import QtPositioning
import QtLocation
import QtCore
import QtQuick.Shapes

import qtosmterraintool

ApplicationWindow {
    id: win
    visible: true
    width: 512
    height: 512
    menuBar: mainMenu
    title: qsTr("OsmTerrainTool: ") + view.map.center +
           " zoom " + view.map.zoomLevel.toFixed(3)
           + " min " + view.map.minimumZoomLevel +
           " max " + view.map.maximumZoomLevel

    FileDialog {
        visible: false
        id: fileOpenDialog
        title: "Select elevation data files (*.hgt)"
        fileMode: FileDialog.OpenFiles
        currentFolder: DtmMapsPath
        nameFilters: ["(*.hgt)"]
        onAccepted: {
        }
    }

    MenuBar {
        id: mainMenu

        Menu {
            title: "&File"
            id : geoJsonMenu
            MenuItem {
                text: "&Open"
                onTriggered: {
                    fileOpenDialog.open()
                }
            }
        }
    }

    Shortcut {
        enabled: view.map.zoomLevel < view.map.maximumZoomLevel
        sequence: StandardKey.ZoomIn
        onActivated: view.map.zoomLevel = Math.round(view.map.zoomLevel + 1)
    }
    Shortcut {
        enabled: view.map.zoomLevel > view.map.minimumZoomLevel
        sequence: StandardKey.ZoomOut
        onActivated: view.map.zoomLevel = Math.round(view.map.zoomLevel - 1)
    }

    GeoJsonData {
        id: geoDatabase
        sourceUrl: GeoJsonMapsPath + "/de_at_ch_it.json"
    }

    function updateHeights()
    {
        ElevationStore.requestHeights(view.map.center, view.map.zoomLevel, view.map.visibleRegion.boundingGeoRectangle())
        rhiItem.updateBuffers()
    }

    MapView {
        id: view
        width: win.width
        height: win.height
        anchors.fill: parent
        map.plugin:

            Plugin {
                id: mapPlugin
                name: "osm"

                PluginParameter {
                    name: "osm.mapping.providersrepository.address";
                    value: SettingsHandler.tileServerUrl
                }
                PluginParameter { name: "osm.mapping.highdpi_tiles"; value: true }
            }
        map.zoomLevel: 5.5
        map.center: QtPositioning.coordinate(48.1375, 11.5755)

        property variant unfinishedItem: undefined
        property bool autoFadeIn: false
        property variant referenceSurface: QtLocation.ReferenceSurface.Map

        signal showMainMenu(variant coordinate)

        map.onCenterChanged: {
            updateHeights()
        }

        function addGeoItem(item)
        {
            var co = Qt.createComponent('mapitems/'+item+'.qml')
            if (co.status === Component.Ready) {
                unfinishedItem = co.createObject(map)
                unfinishedItem.setGeometry(tapHandler.lastCoordinate)
                unfinishedItem.addGeometry(hoverHandler.currentCoordinate, false)
                view.map.addMapItem(unfinishedItem)
            } else {
                console.log(item + " is not supported right now, please call us later.")
            }
        }

        function finishGeoItem()
        {
            unfinishedItem.finishAddGeometry()
            geoDatabase.addItem(unfinishedItem)
            map.removeMapItem(unfinishedItem)
            unfinishedItem = undefined
        }

        function clearAllItems()
        {
            geoDatabase.clear();
        }

        component MapMarkerItem : MapQuickItem {
            id: mapClickedItem
            anchorPoint: Qt.point(e1.width * 0.5, e1.height + slideIn)
            property real slideIn : 0
            property color markerColor
            sourceItem: Shape {
                id: e1
                vendorExtensionsEnabled: false
                width: 32
                height: 32
                visible: true

                transform: Scale {
                    origin.y: e1.height * 0.5
                    yScale: -1
                }

                ShapePath {
                    id: c_sp1
                    strokeWidth: -1
                    fillColor: markerColor

                    property real half: e1.width * 0.5
                    property real quarter: e1.width * 0.25
                    property point center: Qt.point(e1.x + e1.width * 0.5 , e1.y + e1.height * 0.5)


                    property point top: Qt.point(center.x, center.y - half )
                    property point bottomLeft: Qt.point(center.x - half, center.y + half )
                    property point bottomRight: Qt.point(center.x + half, center.y + half )

                    startX: center.x;
                    startY: center.y + half

                    PathLine { x: c_sp1.bottomLeft.x; y: c_sp1.bottomLeft.y }
                    PathLine { x: c_sp1.top.x; y: c_sp1.top.y }
                    PathLine { x: c_sp1.bottomRight.x; y: c_sp1.bottomRight.y }
                    PathLine { x: c_sp1.center.x; y: c_sp1.center.y + c_sp1.half }
                }
            }
        }

        MapItemView {
            id: miv
            parent: view.map

            model: geoDatabase.model

            delegate: GeoJsonDelegate {
                id: geoDelegate
                map: view.map
            }

            MapMarkerItem {
                id: clickedMapMarker
                coordinate: geoDelegate.clickedCoordinate
                markerColor: Qt.rgba(1,0,1,1.0)
            }

            MapMarkerItem {
                id: highestMapMarker
                coordinate: geoDelegate.highestCoordinate
                markerColor: Qt.rgba(0,1,0,1.0)
            }

            MapMarkerItem {
                id: lowestMapMarker
                coordinate: geoDelegate.lowestCoordinate
                markerColor: Qt.rgba(0,0,1,1.0)
            }

            RhiItem {
                id: rhiItem

                width: view.width
                height: view.height

                sampleCount: 4
                alphaBlending: true
                colorBufferFormat: RhiItem.TextureFormat.RGBA8
                backgroundAlpha: 0.0
                opacity: 0.5

                onWidthChanged: {
                    updateHeights()
                }

                onHeightChanged: {
                    updateHeights()
                }

                Component.onCompleted: {
                    updateHeights()
                }
            }
        }

        HoverHandler {
            id: hoverHandler
            property variant currentCoordinate
            grabPermissions: PointerHandler.CanTakeOverFromItems | PointerHandler.CanTakeOverFromHandlersOfDifferentType

            onPointChanged: {
                currentCoordinate = view.map.toCoordinate(hoverHandler.point.position)
                if (view.unfinishedItem !== undefined)
                    view.unfinishedItem.addGeometry(view.map.toCoordinate(hoverHandler.point.position), true)
            }
        }

        TapHandler {
            id: tapHandler
            property variant lastCoordinate
            acceptedButtons: Qt.LeftButton | Qt.RightButton

            onSingleTapped: (eventPoint, button) => {
                lastCoordinate = view.map.toCoordinate(tapHandler.point.position)
                if (button === Qt.RightButton) {
                    if (view.unfinishedItem !== undefined) {
                        view.finishGeoItem()
                    }
                } else if (button === Qt.LeftButton) {
                    if (view.unfinishedItem !== undefined) {
                        if (view.unfinishedItem.addGeometry(view.map.toCoordinate(tapHandler.point.position), false)) {
                            view.finishGeoItem()
                        }
                    }
                }
            }
        }
        TapHandler {
            acceptedButtons: Qt.LeftButton | Qt.RightButton
            onDoubleTapped: (eventPoint, button) => {
                var preZoomPoint = view.map.toCoordinate(eventPoint.position);
                if (button === Qt.LeftButton)
                    view.map.zoomLevel = Math.floor(view.map.zoomLevel + 1)
                else
                    view.map.zoomLevel = Math.floor(view.map.zoomLevel - 1)
                var postZoomPoint = view.map.toCoordinate(eventPoint.position);
                var dx = postZoomPoint.latitude - preZoomPoint.latitude;
                var dy = postZoomPoint.longitude - preZoomPoint.longitude;
                view.map.center = QtPositioning.coordinate(view.map.center.latitude - dx,
                                                           view.map.center.longitude - dy);
            }
        }
    }

    Rectangle {
        width: parent.width
        height: 60
        color: "#200000FF"

        anchors.bottom: parent.bottom

        RowLayout
        {
            anchors.fill: parent

            RowLayout {
                width: 100
                spacing: 10

                Label {
                    text: "Map Resolution: "
                }

                Slider
                {
                    id: slider
                    from: 128
                    value: 512
                    to: 1024
                    stepSize: 64

                    onValueChanged : {
                        ElevationStore.heightCount = value
                        updateHeights()
                    }
                }

                Label {
                    // Displays the current value formatted to 0 decimal places
                    text: slider.value.toFixed(0)
                    width: 30
                }
            }

            CheckBox
            {
                id: checkBoxShowLegend

                width: 100
                text: "show legend"
                checked: false
            }
        }
    }

    Rectangle {
        id: rectLegend
        visible: checkBoxShowLegend.checked
        width: 50
        height: parent.height
        color: "#200000FF"

        anchors.right: parent.right

        property int rowSize: 20
        property int elevationCount: rectLegend.height/rowSize
        property int elevationRange: ElevationStore.maxHeight - ElevationStore.minHeight
        property real elevationStep: (1.0*elevationRange)/elevationCount

        readonly property color color_clear: "#000000"
        readonly property color color_a: "#0066FF"
        readonly property color color_b: "#00FF00"
        readonly property color color_c: "#FFFF00"
        readonly property color color_d: "#FF0000"
        readonly property color color_e: "#FFFFFF"

        function interpolateColor(color1, color2, factor): color
        {
            var newColor = Qt.rgba(
            color1.r + factor * (color2.r - color1.r),
            color1.g + factor * (color2.g - color1.g),
            color1.b + factor * (color2.b - color1.b),
            /*color1.a + factor * (color2.a - color1.a)*/1);
            return newColor;
        }

        function getColor(elevation): color
        {
            var percentage = (elevation-ElevationStore.minHeight)/(1.0*elevationRange);
            var newColor
            if(percentage < 0.0)
            {
                newColor = color_clear;
            }
            else if(percentage < 0.25)
            {
                newColor = interpolateColor(color_a,color_b, percentage / 0.25);
            }
            else if(percentage < 0.5)
            {
                newColor = interpolateColor(color_b,color_c, (percentage-0.25) / 0.25);
            }
            else if(percentage < 0.75)
            {
                newColor = interpolateColor(color_c,color_d, (percentage-0.5) / 0.25);
            }
            else
            {
                newColor = interpolateColor(color_d,color_e, (percentage-0.75) / 0.25);
            }
            return newColor
        }

        function getElevation(index): int
        {
            var elevation
            if(index < rectLegend.elevationCount-1)
            {
                elevation = ElevationStore.minHeight + rectLegend.elevationStep * index;
            }
            else
            {
                elevation = ElevationStore.maxHeight
            }
            return elevation
        }

        Column {

            Repeater {
                id: repeater
                model: rectLegend.elevationCount

                Row{
                    id: row
                    spacing: 5

                    property int elevation: rectLegend.getElevation(index)

                    Rectangle {
                        width: 50; height: 20
                        border.width: 1
                        color: rectLegend.getColor(row.elevation)

                        Label {
                            anchors.fill: parent
                            text: row.elevation + "m"
                            font.pointSize: 10
                            color: "black"
                            horizontalAlignment: Text.AlignHCenter
                        }
                    }
                }
            }
        }
    }
}
