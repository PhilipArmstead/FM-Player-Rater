mkdir -p build
pushd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build .
cd Release
./FM-Player-Rater.exe
popd