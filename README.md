# Grid Dodge

A grid-based dodge game written in C++ using OpenGL, GLFW, GLEW and miniaudio.  
School project.

---

## Requirements

- Windows 64-bit only (Mac/Linux not supported)
- [MSYS2](https://www.msys2.org/) with MinGW64

---

## Build

### 1. Install MSYS2 dependencies

Open **MSYS2 MinGW64** terminal and run:

```bash
pacman -S mingw-w64-x86_64-gcc mingw-w64-x86_64-cmake mingw-w64-x86_64-ninja \
          mingw-w64-x86_64-glew mingw-w64-x86_64-glfw
```

### 2. Clone the repository

```bash
git clone --recurse-submodules https://github.com/JohnKrysa/Project_openGL.git
cd Project_openGL/latest
```

> `--recurse-submodules` automatically downloads miniaudio.

### 3. Build

```bash
cmake -B build/release -G Ninja -DCMAKE_BUILD_TYPE=Release
cmake --build build/release
```

The executable `GridDodge.exe` will appear in the project root.

---

## Project structure

```
grid-dodge/
├── audio/          # Music files
├── levels/         # Level files (.txt)
├── deps/
│   └── miniaudio/  # Audio library (git submodule)
├── main.cpp
├── game.cpp
├── utils.cpp
├── audio.cpp / audio.h
├── globals.cpp
├── common.h
└── CMakeLists.txt
```

---

## Dependencies

| Library | Version | License |
|---------|---------|---------|
| [GLFW](https://www.glfw.org/) | 3.x | zlib |
| [GLEW](https://glew.sourceforge.net/) | 2.x | MIT |
| [miniaudio](https://miniaud.io/) | latest | MIT |
