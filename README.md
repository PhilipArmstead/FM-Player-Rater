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
- **Build System**: GNU Make 4.0+
- **Package Manager**: pkg-config (for GTK4 detection)
- **GUI Framework**: GTK4 development libraries

### Linux

#### Debian/Ubuntu (and derivatives)

```bash
sudo apt-get install \
  build-essential \
  pkg-config \
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
  make \
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
  make \
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

**Note**: The Makefile includes Windows-specific platform detection (`-DARCH_WIN` flag).

## Building the Project

### On Any Platform (after dependencies are installed)

```bash
# Build in release mode (default)
make

# Or explicitly
make release

# Build in debug mode
make debug

# Build with debug info (optimized)
make relwithdebinfo

# Clean build artifacts
make clean

# Run the application
make run

# View build configuration
make info
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

- This project uses **Clang** by default (`CC=clang` in Makefile), but GCC works equally well
- The Makefile supports 64-bit builds across all platforms
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
