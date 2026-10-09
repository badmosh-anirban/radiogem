# Build Instructions:

## How the Build System Works (CMake + Make + Python)

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
   - `pyradiolib.SX1262`
   - Status enums (`RADIOLIB_ERR_NONE`, `RADIOLIB_ERR_CHIP_NOT_FOUND`, etc.)
3. The C++ code directly calls `lgpio` to control SPI (bus 0, CE 0) and GPIO pins (Reset, Busy, DIO1).

---

- **Option A: Build via CMake (Recommended for Native + Python)**: If you prefer manually running `cmake .. && make`, you can do that and use the "_.so_" file directly.
- **Option B: Install via pip**: To install pyradiolib directly into your Python environment or virtual environment.

---

### Option A: Build via CMake

#### Step 1: Install prerequisites on your Raspberry Pi:

```bash
sudo apt update
sudo apt install -y cmake g++ liblgpio-dev python3-dev python3-pip
```

#### Step 2: Build via CMake (Builds both native test & Python module):

```bash
# git clone this repo
git clone https://github.com/badmosh-anirban/pyradiolib
cd pyradiolib

# Create build directory
mkdir -p build && cd build


# Run CMake pointing to your active Python executable
# cmake .. -DPYTHON_EXECUTABLE=$(which python) -DCMAKE_BUILD_TYPE=Release
# Build the pyradiolib shared module
# make pyradiolib -j$(nproc)

# The shared library (e.g. pyradiolib.cpython-313-*.so) is created in build/
# Copy or test it directly:
# python -c "import pyradiolib; print(dir(pyradiolib))"

# cd ..

# Configure and compile
cmake ..
make -j4
```

This builds two targets:

1. `native_test` — Standalone C++ verification binary (Note: I used this previously while debugging and this is no longer built by default, can only be built with an optional flag, see step 3).
2. `pyradiolib.so` — Python extension module.

_Because `pyradiolib` is written in C++ (wrapping RadioLib and `lgpio`), Python cannot run it as plain `.py` text files. It must be compiled into a binary shared object library (`pyradiolib.so`)._

#### Step 3: Test Native C++ First:

Before using Python, if you want verify your SPI wiring and module connection natively using c++:

```bash
# 1. Create a build directory
mkdir -p build && cd build

# 2. Run CMake enabling native test
cmake .. -DBUILD_NATIVE_TEST=ON

# 3. Compile native_test using make
make native_test -j$(nproc)


# 4. In the build/ directory:
./native_test --tx    # Run transmitter
# or
./native_test --rx    # Run receiver

# 5. Return to project root
cd ..
```

_or you may even skip this step._

#### Step 4: Run the Python Examples:

```bash
# Create and activate a virtual environment in the repo root:
python3 -m venv .venv --system-site-packages
source ./venv/bin/activate

# From repository root with PYTHONPATH pointing to build/:
export PYTHONPATH=$PYTHONPATH:$(pwd)/build

# Test transmitter:
python3 examples/basic_tx.py

# Test receiver:
python3 examples/basic_rx.py

# Test gpiozero coexistence:
python3 examples/gpiozero_coexist.py
```

_(You have to run the export command when your building using cmake)_

---

### Option B: Install via pip

In Python, [`setup.py`](setup.py) is the **build, packaging, and installation configuration script**.
[`setup.py`](setup.py) allows you to simply run: `pip install .`

Under the hood, `setup.py` automatically:

1. Calls `cmake` to configure the C++ project.
2. Compiles `RadioLib`, `PiHal.h`, `sx1262_wrapper.cpp`, and `bindings.cpp`.
3. Links against `lgpio`.
4. Installs the resulting `pyradiolib` module directly into your Python environment (`.venv` or system Python).

Without [`setup.py`](setup.py), after building with CMake, the `.so` file would only live in your `build/` folder, meaning you would have to either:

- Keep all your Python scripts inside `build/`, or
- Constantly set `export PYTHONPATH=...`

With `pip install .` (via [`setup.py`](setup.py)), `pyradiolib` is installed into Python's `site-packages`, so any script anywhere on the Raspberry Pi can simply run:

```python
from pyradiolib import SX1262
```

#### Step 1: Install prerequisites on your Raspberry Pi:

```bash
sudo apt update
sudo apt install -y cmake g++ liblgpio-dev python3-dev python3-pip
```

#### Step 2: Clone this repo:

```bash
git clone https://github.com/badmosh-anirban/pyradiolib

cd pyradiolib
```

#### Step 3: Create a virtual environment in the repo root and build the library

```bash
# Create and activate a virtual environment :
python3 -m venv .venv --system-site-packages
source ./venv/bin/activate

# Install directly
pip install .

# Or for editable/development mode (rebuilds when files change):
# pip install -e . --no-build-isolation
pip install -e .
```

Verify the installation:

````bash
python -c "import pyradiolib; radio = pyradiolib.SX1262(); print('pyradiolib installed successfully!')"

# or
Verify the installation:

```bash
python -c "import pyradiolib; radio = pyradiolib.SX1262(); print('pyradiolib installed successfully!')"
````

# Compiling, Packaging, and Publishing to PyPI

This guide provides end-to-end instructions for compiling, packaging, and publishing your custom pyradiolib Python module to the Python Package Index (PyPI) so anyone in the world can install and use it via pip install

## 1. Binary Wheels vs. Source Distribution (`sdist`)

When publishing Python C++ extensions for Raspberry Pi, it is important to understand the difference between distributing an **sdist** and a **wheel**:

### Option 1: Source Distribution Only (`.tar.gz`) — Simplest & Recommended for v0.1.0

- You upload `pyradiolib-0.1.0.tar.gz`.
- When a user runs `pip install pyradiolib`:
  1. `pip` downloads the `.tar.gz`.
  2. `pip` creates an isolated environment, automatically installs `cmake` and `pybind11` (specified in our `pyproject.toml`).
  3. `pip` runs CMake and GCC on their Raspberry Pi, compiling `pyradiolib` natively for their exact architecture (whether it's Raspberry Pi 3, 4, 5, Zero, 32-bit or 64-bit).
  4. Takes ~1 to 2 minutes on Pi 4/5.
- **Advantage**: 100% compatible with all Raspberry Pi models and Linux OS versions without cross-compilation headaches.

### Option 2: Pre-compiled Binary Wheels (`.whl`) — Instant Installation (2 Seconds)

To avoid compiling from source on every user's Raspberry Pi, you can upload pre-compiled binary wheels!

PyPI requires Linux wheels to carry a **`manylinux`** tag (e.g. `manylinux_2_36_aarch64` or `manylinux2014_aarch64`) rather than generic `linux_aarch64`.

#### How to Convert Your `linux_aarch64.whl` to `manylinux` on Raspberry Pi:

You can convert your compiled wheel into a PyPI-compliant wheel in 30 seconds using **`auditwheel`**:

```bash
# 1. Install patchelf and auditwheel
sudo apt install -y patchelf
pip install auditwheel

# 2. Build the wheel and sdist
python -m build

# 3. Repair the wheel to conform to manylinux standards
# (This bundles liblgpio if needed and patches dynamic links to manylinux compatibility)
auditwheel repair dist/pyradiolib-*-linux_aarch64.whl -w wheelhouse/

# 4. Check the repaired wheel
twine check wheelhouse/* dist/*.tar.gz

# 5. Upload BOTH the manylinux wheel (for instant 2-second install) and sdist (.tar.gz fallback):
twine upload --repository testpypi wheelhouse/* dist/*.tar.gz
```

> [!TIP] > **Best of both worlds**: When you upload both the repaired `manylinux` `.whl` and the `.tar.gz`, users with matching Python/architecture (e.g. Python 3.13 on 64-bit Pi) get an **instant 2-second install**, while users on other Python versions (e.g. Python 3.11) automatically fall back to compiling the `.tar.gz`.

## Building PyPI Distribution Packages (`sdist` & `wheel`)

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

#### Step 3: Run Python Build (Source Distribution)

PyPI **rejects** raw Linux wheels (like `linux_aarch64` or `linux_armv7l`) unless built in a specialized `manylinux` container. Because `pyradiolib` dynamically links to the host system's hardware libraries (`liblgpio.so`), the **standard and recommended method** is to publish the **Source Distribution (`sdist`)**:

```bash
# Build ONLY the source distribution (.tar.gz):
python -m build --sdist
```

> [!TIP]
> If you previously ran `python -m build` (which also created a `.whl`), simply delete the wheel before uploading:
>
> ```bash
> rm -f dist/*.whl
> ```
>
> This leaves only `dist/pyradiolib-0.1.0.tar.gz`, which PyPI will happily accept!

#### Step 4: Verify the Source Distribution Contents

Check that `RadioLib/src` and `hal/PiHal.h` are correctly bundled inside the `.tar.gz`:

```bash
tar -tzvf dist/pyradiolib-*.tar.gz | grep -E "(RadioLib.h|PiHal.h|CMakeLists.txt)"
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
```

---

## 5. Publishing to PyPI: Step-by-Step

PyPI uses secure **API Tokens** for uploading packages.

### Step 1: Create Accounts

1. **TestPyPI** (Staging sandbox): Register at [https://test.pypi.org/account/register/](https://test.pypi.org/account/register/)
2. **PyPI** (Official production): Register at [https://pypi.org/account/register/](https://pypi.org/account/register/)

_(Note: Enable Two-Factor Authentication (2FA) on your account as PyPI requires 2FA to generate API tokens)._

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
   twine upload --repository testpypi dist/* --verbose
   ```
2. When prompted:

   - **Username**: `__token__` (literally type two underscores, "token", two underscores)
   - **Password**: Paste your TestPyPI API token (`pypi-...`)

3. Verify installation from TestPyPI on a Raspberry Pi:
   ```bash
   pip install --index-url https://test.pypi.org/simple/ --extra-index-url https://pypi.org/simple/ pyradiolib
   ```
   _(Note: `--extra-index-url https://pypi.org/simple/` is required so pip can find dependencies like `pybind11` on regular PyPI if needed)._

---

### Step 4: Official Production Upload to PyPI

Once tested and verified, upload to the official PyPI:

1. Upload the files:
   ```bash
   twine upload dist/*
   ```
2. When prompted:

   - **Username**: `__token__`
   - **Password**: Paste your official PyPI API token (`pypi-...`)

3. Once complete, your package will be live immediately at:
   [https://pypi.org/project/pyradiolib/](https://pypi.org/project/pyradiolib/)

4. **Anyone can now install your package via:**
   ```bash
   pip install pyradiolib
   ```

---

## How to Release Future Updates (v0.2.0, etc.)

When you add new features (e.g. FSK modulation, SX1268 support, CAD scanning):

1. **Update Version Number**:
   - In `pyproject.toml`:
     ```toml
     version = "0.2.0"
     ```
   - In `setup.py`:
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
