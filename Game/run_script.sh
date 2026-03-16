#!/usr/bin/env bash
set -u

workspace_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
default_script="$workspace_dir/script/three_cubes.py"
target_script="$default_script"

if [[ $# -gt 0 ]]; then
    arg_script="$1"
    if [[ -f "$arg_script" ]]; then
        target_script="$arg_script"
    elif [[ -f "$workspace_dir/$arg_script" ]]; then
        target_script="$workspace_dir/$arg_script"
    elif [[ -f "$workspace_dir/script/$arg_script" ]]; then
        target_script="$workspace_dir/script/$arg_script"
    else
        echo "Script not found: $1" >&2
        echo >&2
        echo "Usage:" >&2
        echo "  bash Game/run_script.sh" >&2
        echo "  bash Game/run_script.sh three_cubes.py" >&2
        echo "  bash Game/run_script.sh script/three_cubes.py" >&2
        echo "  bash Game/run_script.sh /path/to/your_script.py" >&2
        exit 1
    fi
fi

if [[ ! -f "$target_script" ]]; then
    echo "Script not found: $target_script" >&2
    exit 1
fi

python_cmd=""

if command -v python3.13 >/dev/null 2>&1; then
    python_cmd="python3.13"
elif command -v python3 >/dev/null 2>&1; then
    if python3 -c 'import sys; raise SystemExit(0 if sys.version_info[:2] == (3, 13) else 1)' >/dev/null 2>&1; then
        python_cmd="python3"
    fi
fi

if [[ -z "$python_cmd" ]] && command -v python >/dev/null 2>&1; then
    if python -c 'import sys; raise SystemExit(0 if sys.version_info[:2] == (3, 13) else 1)' >/dev/null 2>&1; then
        python_cmd="python"
    fi
fi

if [[ -z "$python_cmd" ]]; then
    echo "Python 3.13 was not found." >&2
    echo "Install Python 3.13 or update this launcher." >&2
    exit 1
fi

echo "Using Python: $python_cmd"
echo "Running script: $target_script"

(
    cd "$workspace_dir" || exit 1
    "$python_cmd" "$target_script"
)
