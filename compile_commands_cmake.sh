cmake -DCMAKE_BUILD_TYPE=Debug -DCMAKE_EXPORT_COMPILE_COMMANDS=1 -B build
mv build/compile_commands.json .
sed -i 's/-std=gnu++23/-std=gnu++2b/g' compile_commands.json
