# =============================================================================
#  Makefile — Grid Dodge  (MinGW-w64 / GCC na Windows)
#  Alternativa ke CMake; jednodušší pro rychlé sestavení z příkazové řádky.
#
#  Použití:
#    make           — sestaví release build (GridDodge.exe)
#    make debug     — sestaví debug build
#    make clean     — smaže objektové soubory a exe
#
#  Předpoklady:
#    - MinGW-w64 v PATH (g++, windres)
#    - GLEW a GLFW3 nainstalovány; uprав GLEW_DIR / GLFW_DIR níže
# =============================================================================

CXX      := g++
TARGET   := GridDodge.exe

# ---------------------------------------------------------------------------
# Cesty ke knihovnám — uprav podle své instalace
# ---------------------------------------------------------------------------
# Příklady:
#   GLEW_DIR  = C:/msys64/mingw64
#   GLFW_DIR  = C:/msys64/mingw64
# Pokud používáš MSYS2/pacman, výchozí hodnoty by měly fungovat přímo.
GLEW_DIR  ?= C:/msys64/mingw64
GLFW_DIR  ?= C:/msys64/mingw64

INCLUDES  := -I. \
             -Ideps/miniaudio \
             -I$(GLEW_DIR)/include \
             -I$(GLFW_DIR)/include

# ---------------------------------------------------------------------------
# Statické linkování: -static přinutí ld hledat .a místo .dll
# -static-libgcc a -static-libstdc++ vloží gcc/c++ runtime přímo do exe
# ---------------------------------------------------------------------------
LDFLAGS_STATIC := -static -static-libgcc -static-libstdc++

LIBS      := -L$(GLEW_DIR)/lib \
             -L$(GLFW_DIR)/lib \
             -lglew32 \
             -lglfw3 \
             -lopengl32 \
             -lwinmm \
             -lole32 \
             -luuid \
             -lgdi32 \
             -luser32 \
             -lkernel32

# ---------------------------------------------------------------------------
# Přepínače kompilátoru
# ---------------------------------------------------------------------------
CXXFLAGS_COMMON := -std=c++17 -Wall -Wextra $(INCLUDES)
CXXFLAGS_REL    := $(CXXFLAGS_COMMON) -O2 -DNDEBUG
CXXFLAGS_DBG    := $(CXXFLAGS_COMMON) -g -O0 -DDEBUG

SRCS := main.cpp game.cpp utils.cpp globals.cpp audio.cpp
OBJS := $(SRCS:.cpp=.o)

# ---------------------------------------------------------------------------
# Cíle
# ---------------------------------------------------------------------------
.PHONY: all release debug clean submodule

all: release

release: CXXFLAGS := $(CXXFLAGS_REL)
release: $(TARGET)

debug: CXXFLAGS := $(CXXFLAGS_DBG)
debug: TARGET   := GridDodge_debug.exe
debug: $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS_STATIC) $(LIBS) -o GridDodge_debug.exe
	@echo ">>> Debug build: GridDodge_debug.exe"

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(LDFLAGS_STATIC) $(LIBS) -o $(TARGET)
	@echo ">>> Release build: $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ---------------------------------------------------------------------------
# Inicializace git submodule (miniaudio)
# ---------------------------------------------------------------------------
submodule:
	git submodule add https://github.com/mackron/miniaudio.git deps/miniaudio || true
	git submodule update --init --recursive

clean:
	del /Q $(OBJS) $(TARGET) GridDodge_debug.exe 2>nul || \
	rm -f $(OBJS) $(TARGET) GridDodge_debug.exe
