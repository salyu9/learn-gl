cmake -S . -B build/Debug -DCMAKE_BUILD_TYPE="Debug"
cmake --build build/Debug --config Debug
cmake --install build/Debug --prefix build/publish/Debug --config Debug