cmake_minimum_required(VERSION 3.14)
project(Spy_Tasker)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Find Qt6 modules
find_package(Qt6 REQUIRED COMPONENTS Core Widgets Gui Network)

# Grab all .cpp and .h files including smtp/
file(GLOB_RECURSE SOURCES "src/*.cpp" "src/*.h")

# Add executable with WIN32 to suppress console window on Windows
add_executable(${PROJECT_NAME}
    WIN32
    ${SOURCES}
    resources/resources.qrc
)

# Include directories (especially for smtp/)
target_include_directories(${PROJECT_NAME}
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/src/smtp
)

# Link Qt libraries and required Windows system libraries
target_link_libraries(${PROJECT_NAME}
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Network
    wevtapi
)


# Windows-specific settings
if(WIN32)


    set(MANIFEST_FILE "${CMAKE_CURRENT_SOURCE_DIR}/app.manifest")
    set_source_files_properties(${MANIFEST_FILE} PROPERTIES HEADER_FILE_ONLY TRUE)
    target_sources(${PROJECT_NAME} PRIVATE ${MANIFEST_FILE})

    set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/resources.rc")
    target_sources(${PROJECT_NAME} PRIVATE ${APP_ICON_RESOURCE_WINDOWS})

endif()
