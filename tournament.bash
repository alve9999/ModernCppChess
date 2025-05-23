#!/bin/bash

# Ensure two arguments (executable paths) are passed
if [ "$#" -ne 3 ]; then
    echo "Usage: $0 <executable_path_1> <executable_path_2> <num cores>"
    exit 1
fi

# Extract the executable names from the paths
exec1_name=$(basename "$1")
exec2_name=$(basename "$2")

# Check if the executables are the same
if [ "$1" = "$2" ]; then
    # If they are the same, append "_1" and "_2" to the names
    exec1_name="${exec1_name}_1"
    exec2_name="${exec2_name}_2"
fi

rm -rf fastchess_log

# Run the chess engines with the appropriate names
fastchess -engine cmd="$1" name="$exec1_name" -engine cmd="$2" name="$exec2_name" \
    -openings file=Pohl.epd format=epd order=random -each tc=10+0.1 -sprt elo0=0 elo1=10 alpha=0.10 beta=0.10 model=normalized -rounds 15000 -repeat -recover -concurrency $3 -log file=fastchess_log engine=true

