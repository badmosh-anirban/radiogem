import os
import sys
import subprocess
from setuptools import setup, Extension
from setuptools.command.build_ext import build_ext


def _run_live(cmd, cwd):
    """Run *cmd* in *cwd*, streaming stdout+stderr line-by-line to the
    terminal in real time so the user can follow progress (e.g. CMake's
    '[  3%] Building …' lines).  Raises CalledProcessError on failure."""
    # Use line-buffered mode so each compiler status line prints immediately.
    process = subprocess.Popen(
        cmd,
        cwd=cwd,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,   # merge stderr into stdout
        bufsize=1,                  # line-buffered
        universal_newlines=True,
    )
    for line in process.stdout:
        print(line, end="", flush=True)
    process.wait()
    if process.returncode != 0:
        raise subprocess.CalledProcessError(process.returncode, cmd)


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

        # ------------------------------------------------------------------
        # Detect suitable parallel build jobs.
        # cc1plus uses >200 MB RSS per job when compiling pybind11/RadioLib
        # TUs; too many parallel jobs OOM-kill the compiler on low-RAM boards.
        # Override at any time with:  CMAKE_BUILD_PARALLEL_LEVEL=2 pip install .
        # ------------------------------------------------------------------
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
            default_jobs = 1
            ram_tier = f"low-RAM ({total_ram_mb} MB ≤ 2048 MB)"
        elif total_ram_mb and total_ram_mb <= 4096:
            default_jobs = min(2, cpu_count)
            ram_tier = f"mid-RAM ({total_ram_mb} MB ≤ 4096 MB)"
        elif total_ram_mb:
            default_jobs = min(4, cpu_count)
            ram_tier = f"high-RAM ({total_ram_mb} MB)"
        else:
            default_jobs = min(2, cpu_count)
            ram_tier = "RAM unknown (defaulting to safe limit)"

        env_override = os.environ.get("CMAKE_BUILD_PARALLEL_LEVEL")
        if env_override:
            jobs = env_override
            jobs_reason = f"CMAKE_BUILD_PARALLEL_LEVEL env override → {jobs} job(s)"
        else:
            jobs = str(default_jobs)
            jobs_reason = (
                f"auto-detected: {ram_tier}, {cpu_count} CPU(s) → {jobs} job(s)"
            )

        print(f"\n{'='*60}", flush=True)
        print(f"  radiogem build configuration", flush=True)
        print(f"{'='*60}", flush=True)
        print(f"  Jobs  : {jobs_reason}", flush=True)
        print(f"  Tip   : set CMAKE_BUILD_PARALLEL_LEVEL=N to override", flush=True)
        print(f"{'='*60}\n", flush=True)

        cmake_args = [
            f"-DCMAKE_LIBRARY_OUTPUT_DIRECTORY={extdir}",
            f"-DPYTHON_EXECUTABLE={sys.executable}",
            f"-DPython3_EXECUTABLE={sys.executable}",
            "-DCMAKE_BUILD_TYPE=Release",
            "-DBUILD_NATIVE_TEST=OFF",
        ]

        build_args = ["--config", "Release", "--", f"-j{jobs}"]

        build_temp = os.path.abspath(self.build_temp)
        os.makedirs(build_temp, exist_ok=True)

        print("-- Running CMake configure …", flush=True)
        _run_live(["cmake", ext.sourcedir] + cmake_args, cwd=build_temp)

        print(f"\n-- Running CMake build (j={jobs}) …", flush=True)
        _run_live(
            ["cmake", "--build", ".", "--target", "radiogem"] + build_args,
            cwd=build_temp,
        )

setup(
    ext_modules=[CMakeExtension("radiogem")],
    cmdclass={"build_ext": CMakeBuild},
    zip_safe=False,
)
