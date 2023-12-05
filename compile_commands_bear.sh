rm -rf build
bear -- cmake --build build
cmake -DCMAKE_BUILD_TYPE=Debug -DHY3_NO_VERSION_CHECK=TRUE -B build
sed -i 's/-std=gnu++23/-std=gnu++2b/g' compile_commands.json
