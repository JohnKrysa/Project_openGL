# Grid Dodge — Build Instructions

## Závislosti

| Knihovna | Proč |
|----------|------|
| GLFW 3   | okno + vstup |
| GLEW     | OpenGL extension loader |
| OpenGL   | rendering |
| miniaudio | audio (header-only, jako git submodule) |

---

## 1. Inicializace miniaudio submodulu

Po prvním klonování repozitáře (nebo pokud ještě nemáš `deps/miniaudio`):

```bash
git submodule add https://github.com/mackron/miniaudio.git deps/miniaudio
git submodule update --init
```

Při dalším klonování repozitáře jiný spolupracovník spustí jen:
```bash
git clone --recurse-submodules <tvůj-repo-url>
# nebo po klasickém clone:
git submodule update --init
```

### Update miniaudio na novou verzi
```bash
git submodule update --remote deps/miniaudio
git add deps/miniaudio
git commit -m "chore: update miniaudio to latest"
```

Pro konkrétní tag:
```bash
cd deps/miniaudio
git checkout v0.11.21
cd ..
git add deps/miniaudio
git commit -m "chore: pin miniaudio to v0.11.21"
```

---

## 2a. Build přes CMake (doporučeno)

```bash
# Nainstaluj závislosti (MSYS2/pacman příklad):
pacman -S mingw-w64-x86_64-glew mingw-w64-x86_64-glfw mingw-w64-x86_64-ninja

# Debug build
cmake --preset debug
cmake --build --preset debug

# Release build (staticky linkovaný exe, bez externích DLL)
cmake --preset release
cmake --build --preset release
```

Výsledný `GridDodge.exe` se objeví přímo v kořeni projektu vedle složek `audio/` a `levels/`.

### Visual Studio (MSVC)
```bash
cmake --preset msvc-release
# Otevři build/msvc-release/GridDodge.sln ve Visual Studiu
# nebo rovnou sestav:
cmake --build --preset msvc-release
```

Statický CRT (`/MT`) je nastavený automaticky — výsledný exe nepotřebuje MSVC redistributable.

---

## 2b. Build přes Makefile (MinGW)

Uprav cesty `GLEW_DIR` a `GLFW_DIR` v `Makefile` (nebo předej na příkazové řádce):

```bash
# Release
make GLEW_DIR=C:/msys64/mingw64 GLFW_DIR=C:/msys64/mingw64

# Debug
make debug

# Vyčistit
make clean
```

---

## 3. Statické linkování — co to znamená

Při `-static -static-libgcc -static-libstdc++` (MinGW) nebo `/MT` (MSVC) se do exe vloží:
- C runtime (msvcrt / ucrt)
- C++ runtime (libstdc++ / msvcp)
- libgcc helpers

**Výhody:** exe funguje na čistém Windows bez instalace redistributablů.  
**Nevýhody:** exe je větší (cca +1–3 MB); bezpečnostní patche runtime je třeba řešit novým buildem.

---

## 4. Změna include cesty pro miniaudio

Soubor `audio.cpp` v současnosti includuje `miniaudio.h` relativně:
```cpp
#include "miniaudio.h"
```

Po přidání submodulu změň na:
```cpp
#include "miniaudio/miniaudio.h"
```

(nebo nech jak je — CMake/Makefile přidá `deps/miniaudio` do include path, takže obojí funguje)
