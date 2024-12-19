rm -rf build
cmake -DCMAKE_BUILD_TYPE=Debug -DHY3_NO_VERSION_CHECK=TRUE -B build
bear -- cmake --build build -j16
