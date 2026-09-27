# Complete Guide: Compiling, Packaging, and Publishing `pyradiolib` to PyPI

This guide provides end-to-end instructions for compiling, packaging, and publishing your custom **`pyradiolib`** Python module to the **Python Package Index (PyPI)** so anyone in the world can install and use it via:

```bash
pip install pyradiolib
```

---

## 1. Fundamental Answers to Your Key Questions

### Q1: Is the original RadioLib library required?

#### For the End-User (running `pip install pyradiolib`):
* **NO, the end-user does NOT need to install RadioLib or Arduino!**
* **Why?** RadioLib is a pure C++ library. When `pyradiolib` is compiled (either on your machine into a wheel or on the user's Raspberry Pi from source), CMake compiles `RadioLib` source files (`RadioLib/src/*.cpp`) and `src/sx1262_wrapper.cpp` into a single self-contained Python shared object (`pyradiolib*.so`).
* RadioLib C++ code is **statically baked into the binary**. The end user does not need to clone RadioLib, install Arduino, or have RadioLib files on their disk.

#### What DOES the End-User need on their Raspberry Pi?
The only external runtime and system dependencies needed are:
1. **Linux `lgpio` library**: RadioLib's Raspberry Pi Hardware Abstraction Layer (`hal/PiHal.h`) communicates with GPIO and SPI via the Linux kernel's `lgpio` library.
   ```bash
   sudo apt update
   sudo apt install -y liblgpio-dev python3-lgpio
   ```
2. **SPI Interface Enabled**:
   ```bash
   sudo raspi-config
   # Interface Options -> SPI -> Enable -> Reboot
   ```

#### For the Developer / Packaging Environment (when building):
* **YES, the RadioLib source files ARE required during compilation.**
* CMake needs `RadioLib/src/RadioLib.h` and `RadioLib/CMakeLists.txt` to compile the C++ classes.
* Therefore, when publishing to PyPI, the source distribution package (`.tar.gz`) must bundle the `RadioLib/src` directory (handled automatically via `MANIFEST.in`).

---

### Q2: Is the package name `pyradiolib` available on PyPI?
* **YES!** We verified the PyPI registry: `pyradiolib` is currently **unclaimed and completely available** on both PyPI and TestPyPI.
* You can claim and own the official `pyradiolib` name right now.

---

## 2. File Audit: What to Create, Keep, or Delete

### A. Files Created / Essential for Publishing
| File | Status | Purpose |
| :--- | :--- | :--- |
| **`MANIFEST.in`** | **Created** | **CRITICAL.** Tells `setuptools` to bundle non-Python files (`CMakeLists.txt`, `hal/`, `src/`, `RadioLib/src/`) into the source distribution (`.tar.gz`). Without this, `pip install` on another machine will fail with "missing CMakeLists.txt or RadioLib.h". |
| **`LICENSE`** | **Created** | MIT License file acknowledging Jan Gromeš (RadioLib author) and your Python port. Required for open-source PyPI distribution. |
| **`pyproject.toml`** | **Updated** | Modern PEP 517/621 package specification declaring build requirements (`setuptools`, `wheel`, `cmake`, `pybind11`), classifiers, and metadata. |
| **`setup.py`** | **Updated** | Custom `CMakeBuild` extension class that invokes CMake to compile the C++ extension module. Improved with dynamic CPU core detection (avoids out-of-memory errors on 1GB Raspberry Pi) and passes `-DBUILD_NATIVE_TEST=OFF`. |
| **`CMakeLists.txt`** | **Updated** | Configured to make the standalone `native_test` optional (`BUILD_NATIVE_TEST=OFF`), preventing unnecessary compilation during `pip install`. |

### B. Files That May Be Safely Deleted / Cleaned Up
Before publishing or pushing to a public repository, you should remove temporary and scratch files:
* **`error1.txt` & `error2.txt`**: Past build/debug terminal logs. Safe to delete.
* **`gemini1.md` & `gemini2.md`**: Scratchpad notes. Safe to delete or move to a private folder.
* **`build/`, `dist/`, `*.egg-info/`**: Local build directories. Safe to delete (they will be regenerated cleanly).
* **`.DS_Store`**: macOS folder metadata files. Safe to delete (`find . -name ".DS_Store" -delete`).
* **`native_test`**: Standalone native binary if compiled in the root. Safe to delete.

### C. Files to Keep or Move
* **`RadioLib/`**: Keep! Contains the upstream C++ library source in `RadioLib/src`.
* **`hal/PiHal.h`**: Keep! The hardware abstraction layer for Raspberry Pi GPIO/SPI.
* **`src/`**: Keep! Contains `bindings.cpp`, `sx1262_wrapper.cpp`, and `sx1262_wrapper.h`.
* **`examples/`**: Keep! Contains verification scripts and examples for users.
* **`telegram_lora_bot.py`**: An application-level script. You can either keep it as an example (e.g., move to `examples/telegram_lora_bot.py`) or leave it in the root for personal testing. It will not be bundled into the binary wheel.
* **`instruction.md`**: Internal project guidelines. You can keep it locally, but it will be excluded from the published package.

---

## 3. How the Build System Works (CMake + Make + Python)

Understanding how the parts fit together:

```text
[Python pip / build]
       │
       ▼
  setup.py (CMakeBuild class)
       │
       ▼
   CMakeLists.txt ─────────────► Finds Python3, pybind11, and lgpio
       │
       ├──► Target 1: RadioLib (Compiles RadioLib/src/*.cpp into static library)
       │
       └──► Target 2: pyradiolib (Compiles src/sx1262_wrapper.cpp & src/bindings.cpp)
                │
                ▼ Links with: RadioLib + lgpio
                │
                ▼ Generates:
             pyradiolib.cpython-313-aarch64-linux-gnu.so
```

When Python executes `import pyradiolib`:
1. Python loads the `.so` shared library using dynamic linking (`dlopen`).
2. The PyBind11 initialization function `PYBIND11_MODULE(pyradiolib, m)` registers the classes:
   * `pyradiolib.SX1262`
   * Status enums (`RADIOLIB_ERR_NONE`, `RADIOLIB_ERR_CHIP_NOT_FOUND`, etc.)
3. The C++ code directly calls `lgpio` to control SPI (bus 0, CE 0) and GPIO pins (Reset, Busy, DIO1).

---

## 4. Step-by-Step Compilation & Build Workflows

All commands below should be run inside your project directory (`/radiogem` or on your Raspberry Pi).

If you are using the virtual environment `.venv`:
```bash
source .venv/bin/activate
```

---

### Workflow A: Direct C++ Native Verification (Stage 1)
To verify the SX1262 hardware wiring directly in pure C++ without Python:

```bash
# 1. Create a build directory
mkdir -p build && cd build

# 2. Run CMake enabling native test
cmake .. -DBUILD_NATIVE_TEST=ON

# 3. Compile native_test using make
make native_test -j$(nproc)

# 4. Run the hardware test
./native_test

# 5. Return to project root
cd ..
```

---

### Workflow B: Manual CMake Compilation of Python Extension Module
To compile `pyradiolib*.so` directly using CMake without `pip`:

```bash
mkdir -p build && cd build

# Run CMake pointing to your active Python executable
cmake .. -DPYTHON_EXECUTABLE=$(which python) -DCMAKE_BUILD_TYPE=Release

# Build the pyradiolib shared module
make pyradiolib -j$(nproc)

# The shared library (e.g. pyradiolib.cpython-313-*.so) is created in build/
# Copy or test it directly:
python -c "import pyradiolib; print(dir(pyradiolib))"

cd ..
```

---

### Workflow C: Local pip Installation (Development Mode)
To install `pyradiolib` into your local Python environment directly from the source directory:

```bash
# Make sure your virtual environment is active
source .venv/bin/activate

# Install the package locally
pip install .

# Or for editable/development mode (rebuilds when files change):
pip install -e . --no-build-isolation
```

Verify the installation:
```bash
python -c "import pyradiolib; radio = pyradiolib.SX1262(); print('pyradiolib installed successfully!')"
```

---

### Workflow D: Building PyPI Distribution Packages (`sdist` & `wheel`)
This is the **official way** to generate the files that get uploaded to PyPI.

#### Step 1: Install Build Tools
In your active `.venv`:
```bash
pip install --upgrade pip build twine
```

#### Step 2: Clean previous build artifacts
```bash
rm -rf build/ dist/ *.egg-info
```

#### Step 3: Run Python Build
```bash
python -m build
```

This will create two files in the `dist/` directory:
1. **Source Distribution (`sdist`)**:
   `dist/pyradiolib-0.1.0.tar.gz`
   Contains all C++ sources, headers, `CMakeLists.txt`, and `setup.py`.
2. **Binary Wheel (`wheel`)**:
   `dist/pyradiolib-0.1.0-cp313-cp313-linux_aarch64.whl` (or your machine's platform tag).
   Contains the pre-compiled `.so` binary.

#### Step 4: Verify the Source Distribution Contents
Check that `RadioLib/src` and `hal/PiHal.h` are correctly bundled inside the `.tar.gz`:
```bash
tar -tzvf dist/pyradiolib-0.1.0.tar.gz | grep -E "(RadioLib.h|PiHal.h|CMakeLists.txt)"
```
You should see:
```text
pyradiolib-0.1.0/CMakeLists.txt
pyradiolib-0.1.0/hal/PiHal.h
pyradiolib-0.1.0/RadioLib/CMakeLists.txt
pyradiolib-0.1.0/RadioLib/src/RadioLib.h
```
If those lines appear, your source package is **100% complete and self-contained**!

#### Step 5: Validate Package Metadata with Twine
```bash
twine check dist/*
```
Output must read:
```text
Checking dist/pyradiolib-0.1.0.tar.gz: PASSED
Checking dist/pyradiolib-0.1.0-...whl: PASSED
```

---

## 5. Publishing to PyPI: Step-by-Step

PyPI uses secure **API Tokens** for uploading packages.

### Step 1: Create Accounts
1. **TestPyPI** (Staging sandbox): Register at [https://test.pypi.org/account/register/](https://test.pypi.org/account/register/)
2. **PyPI** (Official production): Register at [https://pypi.org/account/register/](https://pypi.org/account/register/)

*(Note: Enable Two-Factor Authentication (2FA) on your account as PyPI requires 2FA to generate API tokens).*

### Step 2: Generate an API Token
1. Go to your PyPI account: **Account Settings** -> **API tokens** -> **Add API token**.
2. Token name: `pyradiolib-publisher`.
3. Scope: Select **"Entire account (all projects)"** (since the project does not exist yet).
4. Click **Create token**.
5. Copy the generated token string (starts with `pypi-...`). **Save it securely!**

---

### Step 3: Test Upload to TestPyPI (Recommended First Step)
Always test on TestPyPI first to ensure descriptions, formatting, and installation work without affecting production.

1. Upload your package:
   ```bash
   twine upload --repository testpypi dist/*
   ```
2. When prompted:
   * **Username**: `__token__` (literally type two underscores, "token", two underscores)
   * **Password**: Paste your TestPyPI API token (`pypi-...`)

3. Verify installation from TestPyPI on a Raspberry Pi:
   ```bash
   pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ pyradiolib
   ```
   *(Note: `--extra-index-url https://pypi.org/simple/` is required so pip can find dependencies like `pybind11` on regular PyPI if needed).*

---

### Step 4: Official Production Upload to PyPI
Once tested and verified, upload to the official PyPI:

1. Upload the files:
   ```bash
   twine upload dist/*
   ```
2. When prompted:
   * **Username**: `__token__`
   * **Password**: Paste your official PyPI API token (`pypi-...`)

3. Once complete, your package will be live immediately at:
   [https://pypi.org/project/pyradiolib/](https://pypi.org/project/pyradiolib/)

4. **Anyone can now install your package via:**
   ```bash
   pip install pyradiolib
   ```

---

## 6. Binary Wheels vs. Source Distribution (`sdist`)

When publishing Python C++ extensions for Raspberry Pi, it is important to understand the difference between distributing an **sdist** and a **wheel**:

### Option 1: Source Distribution Only (`.tar.gz`) — Simplest & Recommended for v0.1.0
* You upload `pyradiolib-0.1.0.tar.gz`.
* When a user runs `pip install pyradiolib`:
  1. `pip` downloads the `.tar.gz`.
  2. `pip` creates an isolated environment, automatically installs `cmake` and `pybind11` (specified in our `pyproject.toml`).
  3. `pip` runs CMake and GCC on their Raspberry Pi, compiling `pyradiolib` natively for their exact architecture (whether it's Raspberry Pi 3, 4, 5, Zero, 32-bit or 64-bit).
  4. Takes ~1 to 2 minutes on Pi 4/5.
* **Advantage**: 100% compatible with all Raspberry Pi models and Linux OS versions without cross-compilation headaches.

### Option 2: Pre-compiled Binary Wheels (`.whl`) — Instant Installation
* You build `.whl` files on each target architecture and upload them to PyPI.
* When a user runs `pip install pyradiolib`:
  1. `pip` downloads the matching `.whl`.
  2. Installation completes in **2 seconds** without needing a compiler.
* **Target Architectures for Raspberry Pi**:
  * `linux_aarch64` (Raspberry Pi OS 64-bit on Pi 3, 4, 5, Zero 2 W)
  * `linux_armv7l` (Raspberry Pi OS 32-bit on Pi 2, 3, 4, Zero 2 W)

#### How to build wheels on an actual Raspberry Pi:
If you have a Raspberry Pi running 64-bit OS:
```bash
# On your Raspberry Pi:
source .venv/bin/activate
pip install build wheel
python -m build --wheel
# The resulting dist/pyradiolib-*-linux_aarch64.whl can be uploaded to PyPI!
```

---

## 7. How to Release Future Updates (v0.2.0, etc.)

When you add new features (e.g. FSK modulation, SX1268 support, CAD scanning):

1. **Update Version Number**:
   * In `pyproject.toml`:
     ```toml
     version = "0.2.0"
     ```
   * In `setup.py`:
     ```python
     version = "0.2.0",
     ```
2. **Document changes** in `ChangeLog.md`.
3. **Rebuild**:
   ```bash
   rm -rf build/ dist/ *.egg-info
   python -m build
   ```
4. **Upload to PyPI**:
   ```bash
   twine upload dist/*
   ```
   Users will automatically receive updates when they run:
   ```bash
   pip install --upgrade pyradiolib
   ```

---

## 8. Summary of Pre-Configured Files in This Workspace

Everything has been configured and prepared for you:
1. [MANIFEST.in](MANIFEST.in): Bundles all C++ headers (`RadioLib/src`, `hal/`, `src/`) and CMake scripts.
2. [pyproject.toml](pyproject.toml): Configured with modern PEP 517 build dependencies (`setuptools`, `wheel`, `cmake`, `pybind11`) and full package metadata.
3. [setup.py](setup.py): Automated parallel CMake builder with `-DBUILD_NATIVE_TEST=OFF`.
4. [CMakeLists.txt](CMakeLists.txt): Conditioned native test target.
5. [LICENSE](LICENSE): Standard MIT License.
