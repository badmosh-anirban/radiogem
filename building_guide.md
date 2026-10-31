<h1 align="center">Build Instructions</h1>

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
       └──► Target 2: radiogem (Compiles src/sx1262_wrapper.cpp & src/bindings.cpp)
                │
                ▼ Links with: RadioLib + lgpio
                │
                ▼ Generates:
             radiogem.cpython-313-aarch64-linux-gnu.so
```

When Python executes `import radiogem`:

1. Python loads the `.so` shared library using dynamic linking (`dlopen`).
2. The PyBind11 initialization function `PYBIND11_MODULE(radiogem, m)` registers the classes:
   - `radiogem.SX1262`
   - Status enums (`RADIOLIB_ERR_NONE`, `RADIOLIB_ERR_CHIP_NOT_FOUND`, etc.)
3. The C++ code directly calls `lgpio` to control SPI (bus 0, CE 0) and GPIO pins (Reset, Busy, DIO1).

---

- **Option A: Build via CMake (Recommended for Native + Python)**: If you prefer manually running `cmake .. && make`, you can do that and use the "_.so_" file directly.
- **Option B: Install via pip**: To install radiogem directly into your Python environment or virtual environment.

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
git clone --recursive-submodules https://github.com/badmosh-anirban/radiogem
cd radiogem

# Create build directory
mkdir -p build && cd build


# Run CMake pointing to your active Python executable
# cmake .. -DPYTHON_EXECUTABLE=$(which python) -DCMAKE_BUILD_TYPE=Release
# Build the radiogem shared module
# make radiogem -j$(nproc)

# The shared library (e.g. radiogem.cpython-313-*.so) is created in build/
# Copy or test it directly:
# python -c "import radiogem; print(dir(radiogem))"

# cd ..

# Configure and compile
cmake ..
make -j4
```

This builds two targets:

1. `native_test` — Standalone C++ verification binary (Note: I used this previously while debugging and this is no longer built by default, can only be built with an optional flag, see step 3).
2. `radiogem.so` — Python extension module.

_Because `radiogem` is written in C++ (wrapping RadioLib and `lgpio`), Python cannot run it as plain `.py` text files. It must be compiled into a binary shared object library (`radiogem.so`)._

> [!IMPORTANT]
> '--recursive-submodules' flag is required as the RadioLib repo is set as a submodule, if you ignore this flag Git will only clone the parent repository and leave the submodule folders empty.
>
> If you have already cloned the repository and are looking at empty folders, open your terminal inside the project root directory and run:
>
> ```bash
> git submodule update --init --recursive
> ```

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
source .venv/bin/activate

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
4. Installs the resulting `radiogem` module directly into your Python environment (`.venv` or system Python).

Without [`setup.py`](setup.py), after building with CMake, the `.so` file would only live in your `build/` folder, meaning you would have to either:

- Keep all your Python scripts inside `build/`, or
- Constantly set `export PYTHONPATH=...`

With `pip install .` (via [`setup.py`](setup.py)), `radiogem` is installed into Python's `site-packages`, so any script anywhere on the Raspberry Pi can simply run:

```python
from radiogem import SX1262
```

#### Step 1: Install prerequisites on your Raspberry Pi:

```bash
sudo apt update
sudo apt install -y cmake g++ liblgpio-dev python3-dev python3-pip
```

#### Step 2: Clone this repo:

```bash
git clone --recursive-submodules https://github.com/badmosh-anirban/radiogem
cd radiogem
```

#### Step 3: Create a virtual environment in the repo root and build the library

```bash
# Create and activate a virtual environment :
python3 -m venv .venv --system-site-packages
source .venv/bin/activate

# Install directly
pip install .

# Or for editable/development mode (rebuilds when files change):
# pip install -e . --no-build-isolation
pip install -e .
```

(optionlly pass `--record=install_log.txt`; and later `cat install_log.txt | xargs rm -rf`)<br><br>
Verify the installation:

```bash
python -c "import radiogem; radio = radiogem.SX1262(); print('radiogem installed successfully!')"

# or
pip show radiogem

```
