
# Configuring Meson Command-Line Options for Debug Build and Compiler Selection

## 1. Specify the Build Type as Debug
Use the `-Dbuildtype` option to set the build type to `debug`. This enables debug symbols and disables optimizations for easier debugging.

```bash
meson setup builddir -Dbuildtype=debug
```
additionally, if you want to create a release with debug info, substitute `debug` with `debugoptimized`.

## 2. Specify a Custom Compiler

### a) On Unix-Like Systems
Use environment variables `CC` and `CXX` to set the compiler.

- Example for GCC:
  ```bash
  CC=gcc CXX=g++ meson setup builddir
  ```

- Example for Clang:
  ```bash
  CC=clang CXX=clang++ meson setup builddir
  ```

### b) On Windows (Command Prompt)
Use the `set` command to configure environment variables for the current session:

```cmd
set CC=clang
set CXX=clang++
meson setup builddir
```
  
To make these variables persistent, use `setx`:

```cmd
setx CC clang
setx CXX clang++
meson setup builddir
```

### c) On Windows (PowerShell)
Use the `$env` prefix to set environment variables for the current PowerShell session:

```powershell
$env:CC = "clang"
$env:CXX = "clang++"
meson setup builddir
```

### d) Cross-Platform Solution with a Native File
Create a `native.ini` file specifying the desired compilers:

```ini
[binaries]
c = 'clang'
cpp = 'clang++'
```

Pass this file to Meson using the `--native-file` option:

```bash
meson setup builddir --native-file=native.ini
```

## 3. Specify Additional Build Options
Add custom options using the `-D` flag to configure specific features:

```bash
meson setup builddir -Dsome_option=true
```

## 4. Reconfigure or Regenerate the Build
If the build directory is already configured, you can reconfigure with:

```bash
meson configure builddir -Dbuildtype=debug
```

## 5. Example of a Complete Setup
- **On Unix (Debug build with Clang):**
  ```bash
  CC=clang CXX=clang++ meson setup builddir -Dbuildtype=debug -Dsome_option=true
  ```

- **On Windows (Using the Native File Method):**
  ```bash
  meson setup builddir -Dbuildtype=debug --native-file=native.ini
  ```

## 6. Start the Build Process
Use the `ninja` build system (default for Meson):

```bash
ninja -C builddir
```
