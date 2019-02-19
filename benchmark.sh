#!/bin/bash
set -e

if [[ $# -ne 1 ]]; then
    echo "Pass in name of module to profile"
    exit 1
fi

python setup.py build_ext --inplace
python -m timeit "import $1; $1.sieve(20000)"
