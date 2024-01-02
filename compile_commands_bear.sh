rm -rf build
cmake -DCMAKE_BUILD_TYPE=Debug -DHY3_NO_VERSION_CHECK=TRUE -B build
bear -- cmake --build build -j16
sed -i 's/-std=gnu++23/-std=gnu++2b/g' compile_commands.json
