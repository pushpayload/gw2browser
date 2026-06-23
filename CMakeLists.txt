cmake_minimum_required(VERSION 3.10)
project(gw2browser)
set(NAME gw2browser)

include(GNUInstallDirs)

# Set a default build type if none was specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message("Setting build type to 'RelWithDebInfo' as none was specified.")
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()

set(GW2BROWSER_SOURCE_DIR ${PROJECT_SOURCE_DIR}/src)
set(GW2BROWSER_INCLUDE_DIRECTORY ${PROJECT_SOURCE_DIR}/src)
set(GW2BROWSER_DATA_DIR ${PROJECT_SOURCE_DIR}/data)

set(GW2BROWSER_SOURCE_FILES
    ${GW2BROWSER_SOURCE_DIR}/AsyncFileLoader.cpp
    ${GW2BROWSER_SOURCE_DIR}/BrowserWindow.cpp
    ${GW2BROWSER_SOURCE_DIR}/CategoryTree.cpp
    ${GW2BROWSER_SOURCE_DIR}/Data.cpp
    ${GW2BROWSER_SOURCE_DIR}/DatFile.cpp
    ${GW2BROWSER_SOURCE_DIR}/DatIndex.cpp
    ${GW2BROWSER_SOURCE_DIR}/DatIndexIO.cpp
    ${GW2BROWSER_SOURCE_DIR}/EventId.h
    ${GW2BROWSER_SOURCE_DIR}/Exception.cpp
    ${GW2BROWSER_SOURCE_DIR}/Exporter.cpp
    ${GW2BROWSER_SOURCE_DIR}/FileReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Gw2Browser.cpp
    ${GW2BROWSER_SOURCE_DIR}/IndexCompareDialog.cpp
    ${GW2BROWSER_SOURCE_DIR}/PackFile.cpp
    ${GW2BROWSER_SOURCE_DIR}/PreviewGLCanvas.cpp
    ${GW2BROWSER_SOURCE_DIR}/PreviewPanel.cpp
    ${GW2BROWSER_SOURCE_DIR}/ProgressStatusBar.cpp
    ${GW2BROWSER_SOURCE_DIR}/stdafx.cpp
    ${GW2BROWSER_SOURCE_DIR}/Task.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewer.cpp
    ${GW2BROWSER_SOURCE_DIR}/Imported/crc.cpp
    ${GW2BROWSER_SOURCE_DIR}/Imported/half.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/AFNTReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/asndMP3Reader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/ContentReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/EulaReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/ImageReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/MapReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/ModelReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/PackedSoundReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/SoundBankReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/StringReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Readers/TextReader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Tasks/ReadIndexTask.cpp
    ${GW2BROWSER_SOURCE_DIR}/Tasks/ScanDatTask.cpp
    ${GW2BROWSER_SOURCE_DIR}/Tasks/WriteIndexTask.cpp
    ${GW2BROWSER_SOURCE_DIR}/Util/Misc.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/BinaryViewer/BinaryViewer.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/BinaryViewer/HexControl.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ImageViewer/ImageControl.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ImageViewer/ImageViewer.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Camera.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/FrameBuffer.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/IndexBuffer.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Light.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/LightBox.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Model.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Renderer.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Shader.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/ShaderManager.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Text2D.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Texture2D.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/TextureManager.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/VertexBuffer.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/SoundPlayer/OggCallback.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/SoundPlayer/SoundDecoder.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/SoundPlayer/SoundPlayer.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/StringViewer/StringViewer.cpp
    ${GW2BROWSER_SOURCE_DIR}/Viewers/TextViewer/TextViewer.cpp
)

set(GW2BROWSER_HEADER_FILES
    ${GW2BROWSER_SOURCE_DIR}/ANetStructs.h
    ${GW2BROWSER_SOURCE_DIR}/AsyncFileLoader.h
    ${GW2BROWSER_SOURCE_DIR}/BrowserWindow.h
    ${GW2BROWSER_SOURCE_DIR}/CategoryTree.h
    ${GW2BROWSER_SOURCE_DIR}/Data.h
    ${GW2BROWSER_SOURCE_DIR}/DatFile.h
    ${GW2BROWSER_SOURCE_DIR}/DatIndex.h
    ${GW2BROWSER_SOURCE_DIR}/DatIndexIO.h
    ${GW2BROWSER_SOURCE_DIR}/Exception.h
    ${GW2BROWSER_SOURCE_DIR}/Exporter.h
    ${GW2BROWSER_SOURCE_DIR}/FileReader.h
    ${GW2BROWSER_SOURCE_DIR}/Gw2Browser.h
    ${GW2BROWSER_SOURCE_DIR}/IndexCompareDialog.h
    ${GW2BROWSER_SOURCE_DIR}/PackFile.h
    ${GW2BROWSER_SOURCE_DIR}/PreviewGLCanvas.h
    ${GW2BROWSER_SOURCE_DIR}/PreviewPanel.h
    ${GW2BROWSER_SOURCE_DIR}/ProgressStatusBar.h
    ${GW2BROWSER_SOURCE_DIR}/stdafx.h
    ${GW2BROWSER_SOURCE_DIR}/Task.h
    ${GW2BROWSER_SOURCE_DIR}/version.h
    ${GW2BROWSER_SOURCE_DIR}/Viewer.h
    ${GW2BROWSER_SOURCE_DIR}/wx_pch.h
    ${GW2BROWSER_SOURCE_DIR}/Imported/crc.h
    ${GW2BROWSER_SOURCE_DIR}/Imported/half.h
    ${GW2BROWSER_SOURCE_DIR}/Imported/half.inl
    ${GW2BROWSER_SOURCE_DIR}/Readers/AFNTReader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/asndMP3Reader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/ContentReader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/EulaReader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/ImageReader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/MapReader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/ModelReader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/PackedSoundReader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/SoundBankReader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/StringReader.h
    ${GW2BROWSER_SOURCE_DIR}/Readers/TextReader.h
    ${GW2BROWSER_SOURCE_DIR}/Tasks/ReadIndexTask.h
    ${GW2BROWSER_SOURCE_DIR}/Tasks/ScanDatTask.h
    ${GW2BROWSER_SOURCE_DIR}/Tasks/WriteIndexTask.h
    ${GW2BROWSER_SOURCE_DIR}/Util/Array.h
    ${GW2BROWSER_SOURCE_DIR}/Util/Ensure.h
    ${GW2BROWSER_SOURCE_DIR}/Util/Misc.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/BinaryViewer/BinaryViewer.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/BinaryViewer/HexControl.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ImageViewer/ImageControl.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ImageViewer/ImageViewer.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Camera.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/FrameBuffer.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/IndexBuffer.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Light.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/LightBox.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Model.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Renderer.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Shader.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/ShaderManager.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Text2D.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/Texture2D.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/TextureManager.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/ModelViewer/VertexBuffer.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/SoundPlayer/OggCallback.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/SoundPlayer/SoundDecoder.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/SoundPlayer/SoundPlayer.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/StringViewer/StringViewer.h
    ${GW2BROWSER_SOURCE_DIR}/Viewers/TextViewer/TextViewer.h
)

# Create the executable
add_executable(${NAME} ${GW2BROWSER_SOURCE_FILES} ${GW2BROWSER_HEADER_FILES})

target_include_directories(${NAME} PRIVATE ${GW2BROWSER_INCLUDE_DIRECTORY})

set_property(TARGET ${NAME} PROPERTY DEBUG_POSTFIX d)

find_package(PkgConfig REQUIRED)

pkg_check_modules(PC_GLM glm)
find_path(GLM_INCLUDE_DIRS NAMES glm/glm.hpp HINTS ${PC_GLM_INCLUDE_DIRS} PATH_SUFFIXES glm)

pkg_check_modules(PC_GLEW glew)
find_path(GLEW_INCLUDE_DIRS NAMES GL/glew.h HINTS ${PC_GLEW_INCLUDE_DIRS} PATH_SUFFIXES GLEW)
find_library(GLEW_LIBRARIES NAMES GLEW HINTS ${PC_GLEW_LIBRARY_DIRS})

pkg_check_modules(PC_OPENAL openal)
find_path(OPENAL_INCLUDE_DIRS NAMES AL/al.h HINTS ${PC_OPENAL_INCLUDE_DIRS} PATH_SUFFIXES openal)
find_library(OPENAL_LIBRARIES NAMES openal HINTS ${PC_OPENAL_LIBRARY_DIRS})

pkg_check_modules(PC_OGG ogg)
find_path(OGG_INCLUDE_DIRS NAMES ogg/ogg.h HINTS ${PC_OGG_INCLUDE_DIRS} PATH_SUFFIXES ogg)
find_library(OGG_LIBRARIES NAMES ogg HINTS ${PC_OGG_LIBRARY_DIRS})

pkg_check_modules(PC_VORBIS vorbis)
find_library(VORBIS_LIBRARIES NAMES vorbis HINTS ${PC_VORBIS_LIBRARY_DIRS})

pkg_check_modules(PC_VORBISFILE vorbisfile)
find_path(VORBISFILE_INCLUDE_DIRS NAMES vorbis/vorbisfile.h HINTS ${PC_VORBISFILE_INCLUDE_DIRS} PATH_SUFFIXES vorbisfile)
find_library(VORBISFILE_LIBRARIES NAMES vorbisfile HINTS ${PC_VORBISFILE_LIBRARY_DIRS})

pkg_check_modules(PC_MPG123 libmpg123)
find_path(MPG123_INCLUDE_DIRS NAMES mpg123.h HINTS ${PC_MPG123_INCLUDE_DIRS} PATH_SUFFIXES mpg123)
find_library(MPG123_LIBRARIES NAMES mpg123 HINTS ${PC_MPG123_LIBRARY_DIRS})

pkg_check_modules(PC_WEBP libwebp)
find_path(LIBWEBP_INCLUDE_DIRS NAMES webp/decode.h HINTS ${PC_LIBWEBP_INCLUDE_DIRS} PATH_SUFFIXES webp)
find_library(LIBWEBP_LIBRARIES NAMES webp HINTS ${PC_LIBWEBP_LIBRARY_DIRS})

include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(GLM DEFAULT_MSG GLM_INCLUDE_DIRS)
find_package_handle_standard_args(GLEW DEFAULT_MSG GLEW_INCLUDE_DIRS GLEW_LIBRARIES)
find_package_handle_standard_args(OPENAL DEFAULT_MSG OPENAL_INCLUDE_DIRS OPENAL_LIBRARIES)
find_package_handle_standard_args(OGG DEFAULT_MSG OGG_INCLUDE_DIRS OGG_LIBRARIES)
find_package_handle_standard_args(VORBIS DEFAULT_MSG VORBIS_LIBRARIES)
find_package_handle_standard_args(VORBISFILE DEFAULT_MSG VORBISFILE_INCLUDE_DIRS VORBISFILE_LIBRARIES)
find_package_handle_standard_args(MPG123 DEFAULT_MSG MPG123_INCLUDE_DIRS MPG123_LIBRARIES)
find_package_handle_standard_args(LIBWEBP DEFAULT_MSG LIBWEBP_INCLUDE_DIRS LIBWEBP_LIBRARIES)

find_library(TINYXML_LIBRARY NAMES tinyxml2)
if (TINYXML_FOUND)
    include_directories(${TINYXML_INCLUDE_DIRS})
    link_libraries(${TINYXML_LIBRARIES})
endif()

include(FindFreetype)
find_library(FREETYPE_LIBRARIES NAMES libfreetype)
if (FREETYPE_FOUND)
    include_directories(${FREETYPE_INCLUDE_DIRS})
    link_libraries(${FREETYPE_LIBRARIES})
endif()

find_package(wxWidgets COMPONENTS adv aui gl core base)
if (wxWidgets_FOUND)
    include(${wxWidgets_USE_FILE})
    target_link_libraries(${NAME} ${wxWidgets_LIBRARIES})
    target_compile_definitions(${NAME} PRIVATE WX_PRECOMP)
endif()

# Already installed with cmake
find_package(libgw2dattools CONFIG REQUIRED)
find_package(libgw2formats CONFIG REQUIRED)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(${NAME} PRIVATE -Wall PRIVATE -fopenmp -Wno-deprecated-declarations -Wno-switch)
    set(LIBS gomp)
    if(WIN32)
        set(LIBS opengl32 ${LIBS})
    elseif(UNIX AND NOT APPLE)
        set(LIBS GL ${LIBS})
    endif(WIN32)
elseif ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "MSVC")
    target_compile_options(${NAME} PRIVATE /W4)
    set(LIBS opengl32)
endif()

target_link_libraries(${NAME} ${LIBS}
    GLEW
    openal
    ogg
    vorbis
    vorbisfile
    mpg123
    webp
    tinyxml2
    freetype
    gw2dattools
    gw2formats
)

# Installation

# Disable RPATH stripping
set_target_properties(${NAME} PROPERTIES INSTALL_RPATH_USE_LINK_PATH TRUE)

install(TARGETS ${NAME}
    DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY
    ${GW2BROWSER_DATA_DIR}/fonts
    ${GW2BROWSER_DATA_DIR}/interface
    ${GW2BROWSER_DATA_DIR}/shaders
    DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/${NAME}
    FILES_MATCHING PATTERN "*.*"
)
