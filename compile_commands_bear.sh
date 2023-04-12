rm -rf build
mkdir -p build
cd build
cmake ..
bear -- make
mv compile_commands.json ..
cd ..
sed -i 's/-std=gnu++23/-std=gnu++2b/g' compile_commands.json
