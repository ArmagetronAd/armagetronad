#!/usr/bin/env bash
# batch/test_builds.sh - Canonical test build script for AI use
# 
# This script builds and tests Armagetron Advanced with multiple configurations,
# making it easy for AI agents and developers to verify changes across different
# build setups.
#
# Usage:
#   ./batch/test_builds.sh [config1] [config2] ... [configN]
#   ./batch/test_builds.sh all
#   TEST_ONLY=1 ./batch/test_builds.sh debug    # Skip build, just test existing build
#   FORCE_RECONFIGURE=1 ./batch/test_builds.sh  # Force re-run of configure
#   VERBOSE=1 ./batch/test_builds.sh            # Show full build output
#   COVERAGE=1 ./batch/test_builds.sh server_debug # build debug server, run tests, generate coverage report
#   MAKEFLAGS                                   # Flags passed on to make
#
# Available configurations (use 'list' or 'help' to see more):
#   client      - Explicit client build (no server)
#   server      - Dedicated server build
#   clean       - Clean all test build directories
#
# Exit codes:
#   0 - All configurations passed
#   1 - Command line error
#   >1 - Number of failed configurations
#

set -e

# Ensure we're in the root directory
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$SCRIPT_DIR/.."
ROOT="$(pwd)"

# Define configurations: name:configure_flags
DEBUG_CONFIGURATIONS=(
    "client_debug:DEBUGLEVEL=3 --enable-coverage"
    "server_debug:DEBUGLEVEL=3 --enable-master --enable-dedicated --disable-glout --enable-coverage"
)

CONFIGURATIONS=(
    "client:"
    "server:--enable-master --enable-dedicated --disable-glout"
    "${DEBUG_CONFIGURATIONS[@]}"
    "minimal:--disable-music --disable-authentication --disable-krawall --disable-respawn --disable-memmanager"
)

# -Wno-error=deprecated-declarations currently required because libxml deprecated some things
CXXFLAGS_COMMON='-fmessage-length=0 -Wno-error=deprecated-declarations'

# variations of code strictness flags, the goal is to move down the list
#PEDANTIC_FLAGS=''
PEDANTIC_FLAGS="CODELEVEL=2 CXXFLAGS=\"-Werror ${CXXFLAGS_COMMON}\""
# PEDANTIC_FLAGS='CODELEVEL=3 CXXFLAGS=\"-Werror ${CXXFLAGS_COMMON}\""
# PEDANTIC_FLAGS='CODELEVEL=4 CXXFLAGS=\"-Werror ${CXXFLAGS_COMMON}\""

#echo ${PEDANTIC_FLAGS}

# different compilers -> different directories; store for later
CXX_KEY=""
if [ ! -z "$CXX" ] && [ ! "$CXX" = "c++" ]; then 
    CXX_KEY=_${CXX};
fi

WORKSPACE_KEY=""
if [[ $ROOT == /work* ]]; then
    # looks like we are in a devcontainer
    WORKSPACE_KEY="_pod"
fi

# Common configure flags for all test builds
COMMON_FLAGS="${PEDANTIC_FLAGS} --prefix=/tmp/armagetronad_test --disable-sysinstall --disable-desktop --disable-etc --disable-useradd --enable-curl"

# Parse arguments
if [ $# -eq 0 ] || [ "$1" = "all" ]; then
    SELECTED_CONFIGS=("${CONFIGURATIONS[@]}")
elif [ "$1" = "help" ] || [ "$1" = "-h" ] || [ "$1" = "--help" ]; then
    echo "Usage: $0 [config1|config2|...|debug|all|full|list|clean|help]"
    echo ""
    echo "Builds and tests Armagetron Advanced with multiple configurations."
    echo ""
    echo "Available configurations:"
    for config in "${CONFIGURATIONS[@]}"; do
        name="${config%%:*}"
        flags="${config#*:}"
        printf "  %-12s %s\n" "$name" "$flags"
    done
    echo ""
    echo "Special targets:"
    echo "  debug     - Builds default server_debug and client_debug for TDD and debugging"
    echo "  all       - Run all configurations"
    echo "  full      - Build all configurations in all available compilers (clang, gcc and c++)"
    echo "  list      - List available configurations"
    echo "  clean     - Remove all test build directories"
    echo "  help      - Show this help message"
    echo ""
    echo "Environment variables:"
    echo "  TEST_ONLY=1        - Skip building, only run tests on existing builds"
    echo "  BUILD_ONLY=1       - Skip testing, only build"
    echo "  FORCE_RECONFIGURE=1 - Force re-run of configure step"
    echo "  VERBOSE=1          - Show full build output (not just summary)"
    echo "  COVERAGE=1         - Generate human readable code coverage report (requires lcov)"
    echo "  COVERAGE=2         - Silently generate code coverage report, do not fail on error (for AI Agents)"
    echo "  JOBS=N             - Number of parallel jobs (default: auto)"
    exit 0
elif [ "$1" = "list" ]; then
    for config in "${CONFIGURATIONS[@]}"; do
        echo "${config%%:*}"
    done
    exit 0
elif [ "$1" = "clean" ]; then
    for BUILD_DIR in ${ROOT}/build/test_*; do
        if [ -d "$BUILD_DIR" ]; then
            echo "Removing $BUILD_DIR..."
            rm -rf "$BUILD_DIR"
        fi
    done
    exit 0
elif [ "$1" = "full" ]; then
    # set -x
    # determine default compiler
    DEFAULT_CXX=${CXX:-c++}
    # we compare compilers by their version output
    DEFAULT_V=`$DEFAULT_CXX -v 2>&1`
    # identify possible compilers
    for COMPILER in g++ clang++ c++ `ls /usr/bin/g++-* /usr/bin/clang++-* 2>/dev/null | sed -e s,/usr/bin/,,g`; do
        # see if they differ from the default; if yes, build with them
        COMPILER_V=`$COMPILER -v 2>&1` || continue
        if [ "$DEFAULT_V" = "$COMPILER_V" ]; then continue; fi
        echo "Building with $COMPILER..."
        CXX=$COMPILER ./batch/test_builds.sh all || exit $?
    done

    echo "Building with default..."
    ./batch/test_builds.sh all || exit $?
    exit 0
elif [ "$1" = "debug" ]; then
    # two debug configurations
    SELECTED_CONFIGS=("${DEBUG_CONFIGURATIONS[@]}")
else
    SELECTED_CONFIGS=()
    for arg in "$@"; do
        found=false
        for config in "${CONFIGURATIONS[@]}"; do
            if [ "${config%%:*}" = "$arg" ]; then
                SELECTED_CONFIGS+=("$config")
                found=true
                break
            fi
        done
        if [ "$found" = false ]; then
            echo "Error: Unknown configuration '$arg'"
            echo "Use '$0 list' to see available configurations or '$0 help' for full usage."
            exit 1
        fi
    done
fi

# Default number of parallel jobs: number of CPUs
AUTO_JOBS=$(nproc 2>/dev/null || echo 1)
# get total memory in bytes, mac and linux style, or fallback (16G)
MEM=$(sysctl -n hw.memsize  2>/dev/null || awk '/MemTotal/ {print $2*1000}' /proc/meminfo 2>/dev/null || echo 16000000000)
# echo MEM=$MEM
# compilation eats memory like nothing, assume 2G (trunk) or 1G (legacy)
MEM_PER_JOB=$((2 * 1000000000))
# leave 8G free for the system
JOBS_LIMIT=$(( ($MEM - 8 * 1000000000 ) / $MEM_PER_JOB ))
# but surely, every system can handle 4 jobs?
if [ $JOBS_LIMIT -lt 4 ]; then
    JOBS_LIMIT=4
fi

# clamp 
if [ $AUTO_JOBS -gt $JOBS_LIMIT ]; then
    AUTO_JOBS=$JOBS_LIMIT
fi

JOBS="${JOBS:-${AUTO_JOBS}}"

#echo JOBS=$JOBS
#exit

# Bootstrap if needed
if [ ! -x configure ] && [ ! -f configure ]; then
    echo "Bootstrapping autotools..."
    ./bootstrap.sh || {
        echo "Error: bootstrap.sh failed. Make sure autotools are installed."
        exit 1
    }
fi

# Counters
FAILURES=0
TOTAL=0
FAILED_CONFIGS=()

# Process each configuration
for config in "${SELECTED_CONFIGS[@]}"; do
    NAME="${config%%:*}"
    SPECIFIC_FLAGS="${config#*:}"

    # Build key: if this changes, we need to rebuild
    # flags should be self explanatory
    # the root directory is in there to force rebuild on container/host switches
    BUILD_KEY="$SPECIFIC_FLAGS $COMMON_FLAGS $ROOT"

    BUILD_DIR_BASE=test_${NAME}${WORKSPACE_KEY}${CXX_KEY}
    BUILD_DIR="$ROOT/build/${BUILD_DIR_BASE}"

    echo ""
    echo "============================================================"
    echo "Configuration: $NAME"
    if [ -n "$SPECIFIC_FLAGS" ]; then
        echo "Specific flags: $SPECIFIC_FLAGS"
    fi
    echo "Build directory: $BUILD_DIR"
    echo "============================================================"

    TOTAL=$((TOTAL + 1))

    STEPS=2
    if [ "$BUILD_ONLY" != "1" ]; then
        STEPS=3
    fi

    # Create build directory
    mkdir -p "$BUILD_DIR"

	if echo $config | grep _debug > /dev/null; then
        cd "${ROOT}/build"
        # link output directory to canonical build directory where VS code will be able to find it
        CANONICAL_BUILD_DIR_BASE="./test_vs_${NAME}"
        rm -rf "${CANONICAL_BUILD_DIR_BASE}" # it's a directory link, if we do not remove it, ln below will create a link inside of it
        ln -sf "${BUILD_DIR_BASE}" "${CANONICAL_BUILD_DIR_BASE}"
	fi

    cd "$BUILD_DIR" || continue

    # Clear out directory on relevant changes to build configuration
    BUILD_KEY_OLD=`cat build_key 2> /dev/null || true`
    if [ ! "$BUILD_KEY_OLD" = "$BUILD_KEY" ]; then
        #echo BUILD_KEY    =${BUILD_KEY}
        #echo BUILD_KEY_OLD=${BUILD_KEY_OLD}
        if [ -f Makefile ]; then
            echo "[0/$STEPS] Configuration changed, cleaning..."
        fi
        rm -rf *
        echo > build_key "$BUILD_KEY"
    fi

    # Configure
    if [ ! -f Makefile ] || [ "$FORCE_RECONFIGURE" = "1" ]; then
        if [ -r $ROOT/Makefile ]; then
            echo "[0.5/$STEPS] Cleaning up..."
            make -C $ROOT distclean
        fi
        echo "[1/$STEPS] Configuring..."
        if [ "$VERBOSE" = "1" ]; then
            echo "../../configure $SPECIFIC_FLAGS $COMMON_FLAGS"
            eval "../../configure $SPECIFIC_FLAGS $COMMON_FLAGS"
        else
            eval "../../configure $SPECIFIC_FLAGS $COMMON_FLAGS > /tmp/configure_${NAME}.log 2>&1" || {
                echo "Configure FAILED for $NAME"
                echo "Log:"
                cat /tmp/configure_${NAME}.log
                FAILURES=$((FAILURES + 1))
                FAILED_CONFIGS+=("$NAME")
                cd "$ROOT"
                continue
            }
        fi
    fi

    # Build (skip if TEST_ONLY)
    if [ "$TEST_ONLY" != "1" ]; then
        echo "[2/$STEPS] Building..."
        if [ "$VERBOSE" = "1" ]; then
            make $MAKEFLAGS -k -j"$JOBS" debug || {
                echo "Build FAILED for $NAME"
                FAILURES=$((FAILURES + 1))
                FAILED_CONFIGS+=("$NAME")
                cd "$ROOT"
                continue
            }
        else
            make -j"$JOBS" debug > /dev/null 2>&1 || {
                echo "Build FAILED for $NAME"
                echo "Rerun with output:"
                make $MAKEFLAGS -k -j"$JOBS" debug || true
                FAILURES=$((FAILURES + 1))
                FAILED_CONFIGS+=("$NAME")
                cd "$ROOT"
                continue
            }
        fi
    fi

    if [ "$BUILD_ONLY" != "1" ]; then
        # Run tests
        echo "[3/3] Testing..."
        TEST_PASSED=false

        # clear previous coverage data
        find src -name "*.gcda" -exec rm -f \{\} \;
        rm -f coverage/*.info
        
        # Run unit_tests directly
        if [ -x ./src/unit_tests ] && ./src/unit_tests -ni -o=/tmp/test_${NAME}.log; then
            TEST_PASSED=true
        fi
        
        if [ "$TEST_PASSED" = true ]; then
            if [ "$VERBOSE" = "1" ]; then
                cat /tmp/test_${NAME}.log
            fi
            # Verify coverage data files were generated, if we support the configuration
        	if test -f .coverage_available; then
                if find . -name "*.gcda" -o -name "*.gcno" | grep -q .; then
                    echo "✓ All tests PASSED, coverage data files (.gcda/.gcno) generated for $NAME"
                    if [ "$COVERAGE" != "" ]; then
                        rm -f coverage/*.info coverage/html/index.html
                        if ! make -j"$JOBS" process_coverage > /dev/null 2>&1; then
                            if [ "$COVERAGE" = "1" ]; then
                                if ! make -j"$JOBS" coverage; then
                                    echo "✗ Coverage processing did not work for $NAME"
                                    FAILURES=$((FAILURES + 1))
                                    FAILED_CONFIGS+=("$NAME")
                                fi
                            fi
                        fi
                        if [ -r coverage/html/index.html ]; then
                            echo "✓ Test coverage data reviewable at file://`pwd`/coverage/html/index.html"
                        fi
                    fi
                else
                    echo "✗ Tests passed, but coverage data files (.gcda/.gcno) NOT found for $NAME"
                    FAILURES=$((FAILURES + 1))
                    FAILED_CONFIGS+=("$NAME")
                fi
            else
                echo "✓ All tests PASSED for $NAME"
            fi
        else
            echo "✗ Tests FAILED for $NAME"
            echo "Test log:"
            cat /tmp/test_${NAME}.log
            FAILURES=$((FAILURES + 1))
            FAILED_CONFIGS+=("$NAME")
        fi
    fi

    cd "$ROOT"
done

# Print summary
echo ""
echo "============================================================"
echo "Test Build Summary"
echo "============================================================"
echo "Total configurations: $TOTAL"
echo "Passed: $((TOTAL - FAILURES))"
echo "Failed: $FAILURES"

if [ $FAILURES -gt 0 ]; then
    echo ""
    echo "Failed configurations:"
    for failed in "${FAILED_CONFIGS[@]}"; do
        echo "  - $failed"
    done
    echo ""
    echo "To see detailed logs, run with VERBOSE=1"
fi

echo "============================================================"

exit $FAILURES
