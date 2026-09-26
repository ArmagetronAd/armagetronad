#!/bin/bash
# Filter out lcov-specific flags that llvm-cov gcov doesn't understand
# llvm-cov gcov expects: [options] SOURCEFILE
# But lcov passes: --ignore-errors ... --gcov-tool <path> <sourcefiles>
ARGS=()
SKIP_NEXT=false
for arg in "$@"; do
    if [ "$SKIP_NEXT" = true ]; then
        SKIP_NEXT=false
        continue
    fi
    case "$arg" in
        --ignore-errors|--gcov-tool)
            # Skip this flag and possibly its value
            if [ "$arg" = "--gcov-tool" ]; then
                SKIP_NEXT=true
            fi
            ;;
        *)
            ARGS+=("$arg")
            ;;
    esac
done
exec llvm-cov gcov "${ARGS[@]}"
