#!/bin/bash

# Assigning input arguments to variables
filesdir=$1
searchstr=$2

# Check if the number of arguments is less than 2
if [ "$#" -ne 2 ]; then
    echo "Error: Two arguments are required."
    echo "Usage: $0 <directory_path> <search_string>"
    exit 1
fi

# Check if filesdir is a directory
if [ ! -d "$filesdir" ]; then
    echo "Error: The specified directory does not exist or is not a directory."
    exit 1
fi

# Counting the number of files
num_files=$(find "$filesdir" -type f | wc -l)

# Counting the number of matching lines
num_lines=$(grep -r "$searchstr" "$filesdir" | wc -l)

# Printing the result
echo "The number of files are $num_files and the number of matching lines are $num_lines"
