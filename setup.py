import os
import sys
import subprocess
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext

class CMakeExtension(Extension):
    def __init__(self, name, sourcedir=""):
        Extension.__init__(self, name, sources=[])
        self.sourcedir = os.path.abspath(sourcedir)

class CMakeBuild(build_ext):
    def build_extension(self, ext):
        extdir = os.path.abspath(os.path.dirname(self.get_ext_fullpath(ext.name)))

        # Required for auto-detection of auxiliary "native" libs
        if not extdir.endswith(os.path.sep):
            extdir += os.path.sep

        # Detect suitable parallel build jobs.
        # Default to 1 on systems with <= 2 GB RAM to avoid OOM kills (the
        # C++ compiler frontend cc1plus uses >200 MB per job when compiling
        # large pybind11 translation units).  Override by setting the
        # CMAKE_BUILD_PARALLEL_LEVEL environment variable.
        total_ram_mb = 0
        try:
            with open("/proc/meminfo") as f:
                for line in f:
                    if line.startswith("MemTotal:"):
                        total_ram_mb = int(line.split()[1]) // 1024
                        break
        except Exception:
            pass

        cpu_count = os.cpu_count() or 1
        if total_ram_mb and total_ram_mb <= 2048:
            # Low-RAM device (e.g. Raspberry Pi Zero / 1 / 2 with 512 MB–1 GB):
            # compile one file at a time to prevent OOM kills.
            default_jobs = 1
        elif total_ram_mb and total_ram_mb <= 4096:
            default_jobs = min(2, cpu_count)
        else:
            default_jobs = min(4, cpu_count)

        jobs = os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", str(default_jobs))

        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DPython3_EXECUTABLE={sys.executable}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_NATIVE_TEST=OFF"
        ]

        build_args = ["--config", "Release", "--", f"-j{jobs}"]

        build_temp = os.path.abspath(self.build_temp)
        os.makedirs(build_temp, exist_ok=True)

        subprocess.check_call(
            ["cmake", ext.sourcedir] + cmake_args, cwd=build_temp
        )
        subprocess.check_call(
            ["cmake", "--build", ".", "--target", "radiogem"] + build_args,
            cwd=build_temp,
        )

setup(
    ext_modules=[CMakeExtension("radiogem")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
)
