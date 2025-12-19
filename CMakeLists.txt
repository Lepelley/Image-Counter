cmake_minimum_required(VERSION 3.16)
project(ImageCounter VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Configuration Windows
if(WIN32)
    set(CMAKE_WIN32_EXECUTABLE ON)
    add_definitions(-DUNICODE -D_UNICODE)
endif()

# Trouver OpenCV
find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs highgui)

# Sources
set(SOURCES
    src/main.cpp
    src/ImageDetector.cpp
    src/MainWindow.cpp
    src/SettingsDialog.cpp
    src/Localization.cpp
)

set(HEADERS
    src/ImageCounter.h
    src/SettingsDialog.h
)

# Ressources Windows (icône)
set(RESOURCES
    resources.rc
)

# Créer l'exécutable
add_executable(${PROJECT_NAME} WIN32 ${SOURCES} ${HEADERS} ${RESOURCES})

# Lier les bibliothèques
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${OpenCV_LIBS}
    dwmapi
    ole32
    comctl32
    comdlg32
    winmm
)

target_include_directories(${PROJECT_NAME} PRIVATE
    ${OpenCV_INCLUDE_DIRS}
    ${CMAKE_SOURCE_DIR}/src
)

# Configuration spécifique MSVC
if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE /W4 /utf-8)
    # Manifest pour les contrôles visuels modernes
    set_target_properties(${PROJECT_NAME} PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_SOURCE_DIR}/app.manifest"
    )
endif()

# Configuration MinGW
if(MINGW)
    target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)
    target_link_options(${PROJECT_NAME} PRIVATE -mwindows)
endif()

# Copier les DLLs OpenCV à côté de l'exécutable (Windows)
if(WIN32)
    add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        $<TARGET_FILE:opencv_core>
        $<TARGET_FILE:opencv_imgproc>
        $<TARGET_FILE:opencv_imgcodecs>
        $<TARGET_FILE_DIR:${PROJECT_NAME}>
    )
endif()
