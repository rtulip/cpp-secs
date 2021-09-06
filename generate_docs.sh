[ ! -f "Doxyfile" ] && echo "Run this script from the cpp-secs directory" && exit 1
doxygen
