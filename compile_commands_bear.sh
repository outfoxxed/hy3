rm -rf build
cmake -DCMAKE_BUILD_TYPE=Debug
bear -- cmake --build build
sed -i 's/-std=gnu++23/-std=gnu++2b/g' compile_commands.json
