mkdir -p build
cd build
cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=1 ..
mv compile_commands.json ..
cd ..
sed -i 's/-std=gnu++23/-std=gnu++2b/g' compile_commands.json
