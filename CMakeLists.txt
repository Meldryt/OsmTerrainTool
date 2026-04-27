cmake_minimum_required(VERSION 3.22)

set(CMAKE_CXX_STANDARD 20)

set(TARGET_NAME "OsmTerrainTool")

project(
  ${TARGET_NAME}
  VERSION 1.0
  LANGUAGES CXX
  DESCRIPTION "Elevation Checker")

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

find_package(QT NAMES Qt6 REQUIRED COMPONENTS Core)
find_package(
  Qt${QT_VERSION_MAJOR} REQUIRED
  COMPONENTS Core
             Gui
             Quick
             Qml
             Network
             Positioning
             Location
             ShaderTools)

MESSAGE(STATUS "Found Qt " ${Qt${QT_VERSION_MAJOR}_VERSION})

qt_standard_project_setup()

# Explicity set QT Policy 0001 to new variant, to avoid warnings.
qt_policy(SET QTP0001 NEW)

file(GLOB_RECURSE SRC CONFIGURE_DEPENDS
    "src/*.cpp"
)
file(GLOB_RECURSE HEADERS CONFIGURE_DEPENDS
    "include/*.h")

qt_add_executable(
  ${TARGET_NAME}
  ${HEADERS}
  ${SRC}
)

target_link_libraries(
  ${TARGET_NAME}
  PRIVATE
         Qt${QT_VERSION_MAJOR}::Core
         Qt${QT_VERSION_MAJOR}::Gui
         Qt${QT_VERSION_MAJOR}::Quick
         Qt${QT_VERSION_MAJOR}::Qml
         Qt${QT_VERSION_MAJOR}::Network
         Qt${QT_VERSION_MAJOR}::Positioning
         Qt${QT_VERSION_MAJOR}::Location
         Qt${QT_VERSION_MAJOR}::ShaderTools
)

set_target_properties(${TARGET_NAME} PROPERTIES POSITION_INDEPENDENT_CODE ON)

target_include_directories(${TARGET_NAME}
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  #PRIVATE
    #$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/xyz>
)

target_sources(${TARGET_NAME}
    PRIVATE ${SRC} ${HEADERS}
)

# Resources:
set(qml_resource_files
    "GeoJsonDelegate.qml"
    "Main.qml"
)

qt6_add_resources(${TARGET_NAME} "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)

qt_add_qml_module(${TARGET_NAME}
    URI
        osmterraintool
    QML_FILES
        Main.qml
        GeoJsonDelegate.qml
    RESOURCE_PREFIX
        "/"
    RESOURCES

    NO_RESOURCE_TARGET_PATH
)

qt_add_shaders(${TARGET_NAME} "${TARGET_NAME}_shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        osmterraintool
    FILES
        resources/shaders/color.vert
        resources/shaders/color.frag
)

qt_add_shaders(${TARGET_NAME} "${TARGET_NAME}_effect_shaders"
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        osmterraintool
    FILES
        resources/shaders/checker.frag
)

install(TARGETS ${TARGET_NAME}
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)

#qt_add_qml_module(${TARGET_NAME}
#    URI osmterraintool
#    VERSION 1.0
#    QML_FILES 
#        Main.qml
#)
