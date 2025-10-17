cmake -B build -S . -DCMAKE_BUILD_TYPE=Release -DENABLE_PGO=ON -DPGO_MODE=GENERATE
cmake --build build --config Release
./build/zex-tests zexdoc.com
./build/zex-tests zexall.com
./build/json-tests zexdoc.com