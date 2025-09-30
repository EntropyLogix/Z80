rm -rf build
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON -DPGO_MODE=GENERATE
cmake --build build --config Release
cp zexdoc.com ./build/
./build/Emulator zexdoc.com
cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON -DPGO_MODE=USE
cmake --build build --config Release