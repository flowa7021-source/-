cmake_minimum_required(VERSION 3.20)
project(DocVision VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# Windows-specific settings
if(WIN32)
    add_definitions(-DUNICODE -D_UNICODE -DNOMINMAX -DWIN32_LEAN_AND_MEAN)
    set(CMAKE_WIN32_EXECUTABLE ON)
endif()

# Options
option(DOCVISION_ENABLE_PDF "Enable PDF support (MuPDF + QPDF)" ON)
option(DOCVISION_ENABLE_DJVU "Enable DjVu support (DjVuLibre)" ON)
option(DOCVISION_ENABLE_CBZ "Enable CBZ support" ON)
option(DOCVISION_ENABLE_EPUB "Enable EPUB support (WebView2)" ON)
option(DOCVISION_ENABLE_OCR "Enable OCR support (Tesseract)" ON)

# Find dependencies via vcpkg or system paths
if(DOCVISION_ENABLE_PDF)
    find_package(mupdf CONFIG QUIET)
    find_package(qpdf CONFIG QUIET)
endif()

if(DOCVISION_ENABLE_DJVU)
    find_package(DjVuLibre CONFIG QUIET)
endif()

if(DOCVISION_ENABLE_OCR)
    find_package(Tesseract CONFIG QUIET)
    find_package(Leptonica CONFIG QUIET)
endif()

find_package(nlohmann_json CONFIG QUIET)
find_package(SQLite3 QUIET)

# Source files - Core
set(CORE_SOURCES
    src/core/document.cpp
    src/core/render_pipeline.cpp
    src/core/page_cache.cpp
    src/core/selection_manager.cpp
    src/core/viewport_manager.cpp
    src/core/navigation_history.cpp
    src/core/config_manager.cpp
)

# Source files - UI
set(UI_SOURCES
    src/ui/main_window.cpp
    src/ui/ribbon_controller.cpp
    src/ui/tab_manager.cpp
    src/ui/side_panel.cpp
    src/ui/status_bar.cpp
    src/ui/home_screen.cpp
    src/ui/command_manager.cpp
    src/ui/hotkey_manager.cpp
    src/ui/theme_manager.cpp
    src/ui/command_palette.cpp
    src/ui/pages_overview.cpp
)

# Source files - Format plugins
set(FORMAT_SOURCES
    src/formats/format_registry.cpp
    src/formats/pdf_plugin.cpp
    src/formats/djvu_plugin.cpp
    src/formats/cbz_plugin.cpp
    src/formats/epub_plugin.cpp
)

# Source files - Subsystems
set(SUBSYSTEM_SOURCES
    src/annotations/annotation_manager.cpp
    src/annotations/annotation_types.cpp
    src/annotations/annotation_renderer.cpp
    src/ocr/ocr_engine.cpp
    src/ocr/ocr_pipeline.cpp
    src/ocr/ocr_index.cpp
    src/search/search_engine.cpp
    src/search/text_search.cpp
    src/search/ocr_search.cpp
    src/print/print_manager.cpp
    src/library/library_manager.cpp
    src/library/session_manager.cpp
    src/diagnostics/logger.cpp
    src/diagnostics/crash_handler.cpp
    src/utils/string_utils.cpp
    src/utils/file_utils.cpp
    src/utils/image_utils.cpp
)

# Main executable
add_executable(DocVision WIN32
    src/main.cpp
    ${CORE_SOURCES}
    ${UI_SOURCES}
    ${FORMAT_SOURCES}
    ${SUBSYSTEM_SOURCES}
    resources/DocVision.rc
)

target_include_directories(DocVision PRIVATE
    ${CMAKE_SOURCE_DIR}/include
    ${CMAKE_SOURCE_DIR}/third_party
)

# Link Windows libraries
if(WIN32)
    target_link_libraries(DocVision PRIVATE
        user32 gdi32 shell32 ole32 oleaut32
        comctl32 comdlg32 advapi32 shlwapi
        d2d1 dwrite windowscodecs
        dwmapi uxtheme
        dbghelp  # for minidump
        winspool # for printing
    )
endif()

# Link optional dependencies
if(DOCVISION_ENABLE_PDF AND mupdf_FOUND)
    target_link_libraries(DocVision PRIVATE mupdf::mupdf)
    target_compile_definitions(DocVision PRIVATE DOCVISION_HAS_MUPDF)
endif()

if(DOCVISION_ENABLE_PDF AND qpdf_FOUND)
    target_link_libraries(DocVision PRIVATE qpdf::qpdf)
    target_compile_definitions(DocVision PRIVATE DOCVISION_HAS_QPDF)
endif()

if(DOCVISION_ENABLE_DJVU AND DjVuLibre_FOUND)
    target_link_libraries(DocVision PRIVATE DjVuLibre::DjVuLibre)
    target_compile_definitions(DocVision PRIVATE DOCVISION_HAS_DJVULIBRE)
endif()

if(DOCVISION_ENABLE_OCR AND Tesseract_FOUND)
    target_link_libraries(DocVision PRIVATE Tesseract::libtesseract)
    target_compile_definitions(DocVision PRIVATE DOCVISION_HAS_TESSERACT)
endif()

if(nlohmann_json_FOUND)
    target_link_libraries(DocVision PRIVATE nlohmann_json::nlohmann_json)
endif()

if(SQLite3_FOUND)
    target_link_libraries(DocVision PRIVATE SQLite::SQLite3)
endif()

# Application manifest for DPI awareness
if(WIN32)
    target_sources(DocVision PRIVATE resources/DocVision.manifest)
    set_target_properties(DocVision PROPERTIES
        LINK_FLAGS "/MANIFEST:EMBED /MANIFESTINPUT:${CMAKE_SOURCE_DIR}/resources/DocVision.manifest"
    )
endif()

# Install rules
install(TARGETS DocVision RUNTIME DESTINATION .)
install(FILES
    ${CMAKE_SOURCE_DIR}/resources/default_settings.json
    ${CMAKE_SOURCE_DIR}/resources/default_hotkeys.json
    DESTINATION config
)
install(FILES ${CMAKE_SOURCE_DIR}/LICENSE DESTINATION .)

# Create runtime directories via install
install(DIRECTORY DESTINATION logs)
install(DIRECTORY DESTINATION cache)
install(DIRECTORY DESTINATION cache/ocr)
install(DIRECTORY DESTINATION tessdata)

# Portable marker file
install(CODE "file(WRITE \"\${CMAKE_INSTALL_PREFIX}/portable.txt\" \"Portable installation of DocVision.\\nAll data is stored in this directory.\\n\")")

# Copy config templates to build directory for development
file(COPY ${CMAKE_SOURCE_DIR}/resources/default_settings.json DESTINATION ${CMAKE_BINARY_DIR}/config/)
file(COPY ${CMAKE_SOURCE_DIR}/resources/default_hotkeys.json DESTINATION ${CMAKE_BINARY_DIR}/config/)

# ---- CPack Installer Configuration ----
set(CPACK_PACKAGE_NAME "DocVision")
set(CPACK_PACKAGE_VENDOR "DocVision Project")
set(CPACK_PACKAGE_DESCRIPTION_SUMMARY "Offline multi-format document viewer and annotator")
set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
set(CPACK_PACKAGE_VERSION_MAJOR "${PROJECT_VERSION_MAJOR}")
set(CPACK_PACKAGE_VERSION_MINOR "${PROJECT_VERSION_MINOR}")
set(CPACK_PACKAGE_VERSION_PATCH "${PROJECT_VERSION_PATCH}")
set(CPACK_PACKAGE_INSTALL_DIRECTORY "DocVision")
set(CPACK_RESOURCE_FILE_LICENSE "${CMAKE_SOURCE_DIR}/LICENSE")

# Use Inno Setup (produces standard Windows .exe installer) + ZIP for portable
set(CPACK_GENERATOR "INNOSETUP;ZIP")

# Inno Setup settings
set(CPACK_INNOSETUP_INSTALL_ROOT "{autopf}")
set(CPACK_PACKAGE_EXECUTABLES "DocVision" "DocVision")
set(CPACK_CREATE_DESKTOP_LINKS "DocVision")

include(CPack)
