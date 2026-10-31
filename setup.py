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

        # Detect suitable parallel build jobs (avoid OOM on 1GB Raspberry Pi)
        jobs = os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL", str(min(4, max(1, os.cpu_count() or 1))))

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
            ["cmake", "--build", ".", "--target", "pyradiolib"] + build_args,
            cwd=build_temp,
        )

setup(
    ext_modules=[CMakeExtension("pyradiolib")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
)
