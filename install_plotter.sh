#!/bin/bash

set -e
QT_DIR=$(find ~/Qt -maxdepth 2 -type d \( -name "gcc_64" -o -name "clang_64" -o -name "mingw*" \))
REPO_URL="https://github.com/swallace23/317_realtime.git"
dir="$HOME/Documents/317_realtime"
new_install=false
if [ ! -d "$dir" ]; then
	echo "Cloning Repository"
	git clone "$REPO_URL" "$dir"
	cd "$dir"
	new_install=true

else
	cd "$dir"
fi
GIT_STATUS=$(git status)
if [[ $GIT_STATUS == *"up to date"* && "$new_install" = false ]]; then
	echo "========================================="
	echo "Project up to date. Closing installer."
	echo "========================================="
	exit 0
fi
if [ -d "build" ]; then
	rm -rf "build"
fi
git fetch
git pull
mkdir "build"
cd "build"
cmake -DCMAKE_PREFIX_PATH="$QT_DIR" ..
make
echo ""
echo "======================================"
echo "Plotter Installed."
echo "======================================"
