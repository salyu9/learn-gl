cmake -S . -B build/Release -DCMAKE_BUILD_TYPE="Release"
cmake --build build/Release --config Release
cmake --install build/Release --prefix build/publish/Release --config Release