Football Manager Player Rater
===

## About this tool

This application shows you player data from your running Football Manager 24 save and presents quick ratings so you can
compare players and weigh up future purchases.

This has only been tested with
the [Steam version of Football Manager 24](https://store.steampowered.com/app/2252570/Football_Manager_2024/), v24.4.2.

### Features

Coming soon.

## Dependencies

### Common Requirements (All Platforms)

- **C Compiler**: GCC 9+ or Clang 10+ (C99 standard)
- **Build System**: CMake 3.20+
- **Package Manager**: pkg-config (for GTK4 detection)
- **GUI Framework**: GTK4 development libraries

### Linux

#### Debian/Ubuntu (and derivatives)

```bash
sudo apt-get install \
  build-essential \
  pkg-config \
  cmake \
  libgtk-4-dev \
  libglib2.0-dev
```

**Packages**:

- `build-essential` - GCC compiler, make, libc development files
- `pkg-config` - Package configuration helper
- `libgtk-4-dev` - GTK4 development headers and libraries
- `libglib2.0-dev` - GLib development headers (GTK4 dependency)

#### Red Hat/CentOS/Fedora

```bash
sudo dnf install \
  gcc \
  cmake \
  pkg-config \
  gtk4-devel \
  glib2-devel
```

**Packages**:

- `gcc` - GNU C Compiler
- `make` - Build automation tool
- `pkg-config` - Package configuration helper
- `gtk4-devel` - GTK4 development headers and libraries
- `glib2-devel` - GLib development headers (GTK4 dependency)

#### Arch Linux

```bash
sudo pacman -S \
  base-devel \
  pkg-config \
  cmake \
  gtk4
```

**Packages**:

- `base-devel` - Core development tools (GCC, make, libc)
- `pkg-config` - Package configuration helper
- `gtk4` - GTK4 libraries and headers

### macOS

#### Using Homebrew (Recommended)

```bash
# Install Homebrew if not already installed
/bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"

# Install dependencies
brew install \
  gcc \
  cmake \
  pkg-config \
  gtk4
```

**Packages**:

- `gcc` - GNU C Compiler (or use system Clang via Xcode)
- `make` - Build automation tool
- `pkg-config` - Package configuration helper
- `gtk4` - GTK4 libraries and headers

#### Using MacPorts (Alternative)

```bash
sudo port install \
  gcc12 \
  pkgconfig \
  cmake \
  gtk4
```

### Windows

#### MSYS2/MinGW64 (Recommended)

1. Download and install [MSYS2](https://www.msys2.org/)
2. Open MinGW64 terminal and run:

```bash
pacman -S \
  mingw-w64-x86_64-toolchain \
  mingw-w64-x86_64-pkg-config \
  mingw-w64-x86_64-gtk4
```

**Packages**:

- `mingw-w64-x86_64-toolchain` - GCC compiler and build tools
- `mingw-w64-x86_64-pkg-config` - Package configuration helper
- `mingw-w64-x86_64-gtk4` - GTK4 libraries and headers for Windows 64-bit

**Alternative: MinGW-w64**

- Standalone MinGW-w64 distribution with pkg-config and GTK4 support

**Note**: The build files include Windows-specific platform detection (`-DARCH_WIN` flag).

## Building the Project

### On Any Platform (after dependencies are installed)

```bash
# Create a build directory and configure (Release is default)
mkdir -p build; cd build
cmake -DCMAKE_BUILD_TYPE=Release ..

# Build
cmake --build .

# Build a specific configuration (Debug)
cmake -DCMAKE_BUILD_TYPE=Debug ..
cmake --build .

# Build with multiple cores
cmake --build . -- -j$(nproc)

# Clean build artifacts
cmake --build . --target clean

# Run the application (from build dir), or build a "run" target if provided
./$(pwd)/build/Release/FM-Player-Rater   # or adjust path to your configuration
# or from the build tree:
cmake --build . --target run

# Show build info (via CMake custom target)
cmake --build . --target info
```

## Runtime Requirements

- **GTK4 Runtime Libraries** (usually included with development packages)
- **X11 or Wayland** (Linux display server)
- **GLib2 Runtime** (included with GTK4)

### X11 on Linux (if not already running Wayland)

Most modern distributions include X11. For headless systems or when needed:

- `libx11-6` (Debian/Ubuntu)
- `libx11` (Fedora/RHEL)
- `libx11` (Arch)

## Notes

- This project prefers **Clang** by default where available, but GCC works equally well — set the `CC` environment
  variable or `CMAKE_C_COMPILER` to override.
- The build setup supports 64-bit builds across all platforms
- Platform-specific flags are set automatically during build:
	- Linux: `-DARCH_LINUX`
	- macOS: `-DARCH_MACOS`
	- Windows: `-DARCH_WIN`
- On Windows, ensure you're using the **MinGW64 shell** (not CMD.exe or PowerShell)
- GTK4 requires a modern C library and platform-specific graphics libraries

## Troubleshooting

**"pkg-config not found"**

- Install pkg-config package for your platform (see above)

**"gtk/gtk.h: No such file or directory"**

- Install GTK4 development headers (`libgtk-4-dev` or equivalent)

**"ld.exe: cannot find -lgtk-4" (Windows)**

- Ensure you're using the MinGW64 shell with GTK4 pacman package installed

**Build fails with compiler errors**

- Ensure your C compiler supports C99 standard (GCC 9+, Clang 10+)
