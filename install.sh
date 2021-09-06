cmake_version="$(cmake --version | head -n1 | cut -d" " -f3)"
cmake_required="3.17.0"

if [ "$(printf '%s\n' "$requiredver" "$currentver" | sort -V | head -n1)" = "$requiredver" ]; then 
        echo "CMake $cmake_version is sufficient"
 else
        echo "CMake $cmake_required or higher required. Version $cmake_version found."
        echo "Find the Latest version of CMake here: https://cmake.org/download/"
        exit 1
fi

sudo apt-get update
sudo apt-get install mesa-utils
sudo apt-get install freeglut3-dev
sudo apt-get install libboost-all-dev
sudo apt-get install doxygen