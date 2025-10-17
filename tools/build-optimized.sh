rm -rf ../tests/build
cmake -B ../tests/build -S ../tests/ -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON -DPGO_MODE=GENERATE
cmake --build ../tests/build --config Release
../tests/build/zex-tests ../tests/zexdoc.com
cmake -B ../tests/build -S ../tests/ -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON -DPGO_MODE=USE
cmake --build ../tests/build --config Release