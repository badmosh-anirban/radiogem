#!/usr/bin/env bash
# =============================================================================
# scripts/build_wheel_mac.sh
#
# Build a Raspberry Pi-compatible (linux/arm64) radiogem wheel on an Apple
# Silicon Mac using Docker.  The resulting .whl is placed in dist/.
#
# Prerequisites:
#   - Docker Desktop installed and running
#   - RadioLib submodule initialised:  git submodule update --init --recursive
#
# Usage:
#   ./scripts/build_wheel_mac.sh              # build wheel
#   ./scripts/build_wheel_mac.sh --upload     # build + upload to PyPI
# =============================================================================
set -euo pipefail

REPO_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
IMAGE_NAME="radiogem-builder"
PLATFORM="linux/arm64"
DIST_DIR="${REPO_ROOT}/dist"

# ── Colour helpers ────────────────────────────────────────────────────────────
bold=$'\e[1m'; reset=$'\e[0m'; green=$'\e[32m'; yellow=$'\e[33m'; red=$'\e[31m'
info()  { echo "${bold}${green}[radiogem-build]${reset} $*"; }
warn()  { echo "${bold}${yellow}[radiogem-build]${reset} $*"; }
die()   { echo "${bold}${red}[radiogem-build] ERROR:${reset} $*" >&2; exit 1; }

# ── Checks ────────────────────────────────────────────────────────────────────
command -v docker &>/dev/null || die "Docker not found. Install Docker Desktop from https://docker.com"
docker info &>/dev/null       || die "Docker daemon is not running. Start Docker Desktop first."

arch="$(uname -m)"
[[ "$arch" == "arm64" ]] || warn "Your Mac is ${arch}, not arm64. Build will use QEMU emulation and will be slow."

[[ -f "${REPO_ROOT}/RadioLib/CMakeLists.txt" ]] || \
    die "RadioLib submodule missing.\nRun: git submodule update --init --recursive"

# ── Build Docker image ────────────────────────────────────────────────────────
info "Building Docker image '${IMAGE_NAME}' for ${PLATFORM} …"
info "(First run downloads ~600 MB base image; subsequent runs use the cache)"
echo ""

docker build \
    --platform "${PLATFORM}" \
    --file "${REPO_ROOT}/Dockerfile.wheel" \
    --tag "${IMAGE_NAME}" \
    "${REPO_ROOT}"

# ── Extract the wheel from the image ─────────────────────────────────────────
mkdir -p "${DIST_DIR}"

info ""
info "Extracting wheel to ${DIST_DIR}/ …"

# Spin up a temporary container just to copy /dist/* out
CONTAINER_ID="$(docker create --platform "${PLATFORM}" "${IMAGE_NAME}")"
docker cp "${CONTAINER_ID}:/dist/." "${DIST_DIR}/"
docker rm "${CONTAINER_ID}" >/dev/null

# ── Report ────────────────────────────────────────────────────────────────────
echo ""
info "Built wheels:"
ls -lh "${DIST_DIR}"/radiogem-*.whl 2>/dev/null || warn "No wheel found in ${DIST_DIR}"
echo ""

# ── Optional upload to PyPI ───────────────────────────────────────────────────
if [[ "${1:-}" == "--upload" ]]; then
    command -v twine &>/dev/null || die "twine not found: pip install twine"
    info "Uploading to PyPI …"
    twine upload "${DIST_DIR}"/radiogem-*.whl
    info "Done! 🎉"
else
    info "To install on your Pi, copy the wheel and run:"
    echo ""
    echo "    scp ${DIST_DIR}/radiogem-*.whl pi@<your-pi-ip>:~/"
    echo "    ssh pi@<your-pi-ip> 'pip install ~/radiogem-*.whl'"
    echo ""
    info "To upload to PyPI:"
    echo ""
    echo "    ./scripts/build_wheel_mac.sh --upload"
    echo "    # or: twine upload ${DIST_DIR}/radiogem-*.whl"
    echo ""
fi

