cmake_minimum_required(VERSION 3.20)
project(GridDodge CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ---------------------------------------------------------------------------
# Statické linkování runtime knihoven
# ---------------------------------------------------------------------------
# Na Windows (MSVC / MinGW) chceme, aby výsledné .exe nepotřebovalo
# žádné extra DLL pro C++ runtime.
if(MSVC)
    # Přepínáme z /MD (dynamický CRT) na /MT (statický CRT)
    foreach(flag_var
        CMAKE_CXX_FLAGS CMAKE_CXX_FLAGS_DEBUG CMAKE_CXX_FLAGS_RELEASE
        CMAKE_CXX_FLAGS_MINSIZEREL CMAKE_CXX_FLAGS_RELWITHDEBINFO)
        string(REPLACE "/MD" "/MT" ${flag_var} "${${flag_var}}")
    endforeach()
elseif(MINGW)
    # MinGW: staticky přilinkujeme libgcc, libstdc++ a libwinpthread
    add_link_options(-static -static-libgcc -static-libstdc++)
endif()

# ---------------------------------------------------------------------------
# Zdrojové soubory projektu
# ---------------------------------------------------------------------------
set(SOURCES
    main.cpp
    game.cpp
    utils.cpp
    globals.cpp
    audio.cpp
)

add_executable(${PROJECT_NAME} ${SOURCES})

# ---------------------------------------------------------------------------
# miniaudio jako header-only knihovna (git submodule v deps/miniaudio)
# ---------------------------------------------------------------------------
# Pokud ještě nemáš submodule inicializovaný, spusť:
#   git submodule add https://github.com/mackron/miniaudio.git deps/miniaudio
#   git submodule update --init
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/deps/miniaudio
)

# ---------------------------------------------------------------------------
# Vyhledání závislostí: GLEW a GLFW
# ---------------------------------------------------------------------------
# Preferujeme statické varianty knihoven (GLEW_USE_STATIC_LIBS, ...)
set(GLEW_USE_STATIC_LIBS ON)

find_package(GLEW REQUIRED)
find_package(glfw3 REQUIRED)
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# Linkování
# ---------------------------------------------------------------------------
target_link_libraries(${PROJECT_NAME} PRIVATE
    GLEW::GLEW
    glfw
    OpenGL::GL
)

# Na Windows je třeba přilinkovat WinMM a další systémové knihovny pro miniaudio
if(WIN32)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        winmm       # timeGetTime, waveOut* — používá miniaudio
        ole32       # CoInitialize
        uuid        # IUnknown GUID
    )
endif()

# Na Linuxu miniaudio potřebuje pthread a dl
if(UNIX AND NOT APPLE)
    target_link_libraries(${PROJECT_NAME} PRIVATE
        pthread
        dl
        m
    )
endif()

# ---------------------------------------------------------------------------
# Výstupní adresář — .exe skončí vedle assets složek (audio/, levels/)
# ---------------------------------------------------------------------------
set_target_properties(${PROJECT_NAME} PROPERTIES
    RUNTIME_OUTPUT_DIRECTORY "${CMAKE_SOURCE_DIR}"
)

# ---------------------------------------------------------------------------
# Rychlé preset-konfigurace pro CMake Presets (nepovinné)
# Použití: cmake --preset release
# ---------------------------------------------------------------------------
# Viz soubor CMakePresets.json vedle tohoto souboru.
