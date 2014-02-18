cmake_minimum_required(VERSION 2.8)

# This is NOT intended as a stand-alone build system! Instead, you should include this from the main CMakeLists of your project.
# Make sure to link against Ogre, boost::filesystem and boost::wave.

find_package(Boost REQUIRED QUIET COMPONENTS system filesystem wave)

option(SHINY_BUILD_OGRE_PLATFORM "build the Ogre platform" ON)

set(SHINY_LIBRARY "shiny")
set(SHINY_OGREPLATFORM_LIBRARY "shiny.OgrePlatform")

# Sources of shiny
set(SOURCE_FILES
    Main/Factory.cpp
    Main/MaterialInstance.cpp
    Main/MaterialInstancePass.cpp
    Main/MaterialInstanceTextureUnit.cpp
    Main/Platform.cpp
    Main/Preprocessor.cpp
    Main/PropertyBase.cpp
    Main/ScriptLoader.cpp
    Main/ShaderInstance.cpp
    Main/ShaderSet.cpp
)

include_directories(${Boost_INCLUDE_DIRS})
add_definitions(-DBOOST_ALL_NO_LIB)
add_library(${SHINY_LIBRARY} STATIC ${SOURCE_FILES})

set(SHINY_LIBRARIES ${SHINY_LIBRARY})

if (SHINY_BUILD_OGRE_PLATFORM)
    if(NOT DEFINED OGRE_INCLUDE_DIRS)
        # Set path so CMake can find FindOGRE.cmake
        if(WIN32)
            set(CMAKE_MODULE_PATH "$ENV{OGRE_HOME}/CMake/;${CMAKE_MODULE_PATH}")
        endif(WIN32)
        if(UNIX)
            if(EXISTS "/usr/local/lib/OGRE/cmake")
                set(CMAKE_MODULE_PATH "/usr/local/lib/OGRE/cmake/;${CMAKE_MODULE_PATH}")
            elseif(EXISTS "/usr/lib/OGRE/cmake")
                set(CMAKE_MODULE_PATH "/usr/lib/OGRE/cmake/;${CMAKE_MODULE_PATH}")
            else()
                message(SEND_ERROR "Failed to find module path for OGRE.")
            endif(EXISTS "/usr/local/lib/OGRE")
        endif(UNIX)
        # NOTE: I have not found other way to disable status messages for OGRE Components
        # Also I have not found way how to disable "Could NOT find PkgConfig (missing:  PKG_CONFIG_EXECUTABLE)" message
        set(OGRE_Paging_FIND_QUIETLY TRUE)
        set(OGRE_Terrain_FIND_QUIETLY TRUE)
        set(OGRE_Property_FIND_QUIETLY TRUE)
        set(OGRE_RTShaderSystem_FIND_QUIETLY TRUE)
        set(OGRE_Volume_FIND_QUIETLY TRUE)
        set(OGRE_Overlay_FIND_QUIETLY TRUE)
        find_package(OGRE REQUIRED QUIET)
    endif()
    include_directories(${Boost_INCLUDE_DIRS} ${OGRE_INCLUDE_DIRS})

    # Sources of shiny.OgrePlatform
    set(OGRE_PLATFORM_SOURCE_FILES
        Platforms/Ogre/OgreGpuProgram.cpp
        Platforms/Ogre/OgreMaterial.cpp
        Platforms/Ogre/OgreMaterialSerializer.cpp
        Platforms/Ogre/OgrePass.cpp
        Platforms/Ogre/OgrePlatform.cpp
        Platforms/Ogre/OgreTextureUnitState.cpp
    )
    file(GLOB OGRE_PLATFORM_SOURCE_FILES Platforms/Ogre/*.cpp)

    add_definitions(-DBOOST_ALL_NO_LIB)
    add_library(${SHINY_OGREPLATFORM_LIBRARY} STATIC ${OGRE_PLATFORM_SOURCE_FILES})
    add_dependencies(${SHINY_OGREPLATFORM_LIBRARY} ${SHINY_LIBRARY})
    set(SHINY_LIBRARIES ${SHINY_LIBRARIES} ${SHINY_OGREPLATFORM_LIBRARY})
endif()

set(SHINY_LIBRARY ${SHINY_LIBRARY} PARENT_SCOPE)

if (DEFINED SHINY_BUILD_MATERIAL_EDITOR)
    add_subdirectory(Editor)

    set(SHINY_BUILD_EDITOR_FLAG ${SHINY_BUILD_EDITOR_FLAG} PARENT_SCOPE)
endif()

link_directories(${CMAKE_CURRENT_BINARY_DIR})
set(SHINY_LIBRARIES ${SHINY_LIBRARIES} PARENT_SCOPE)
