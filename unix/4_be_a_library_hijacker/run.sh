gcc -O3  -shared memcpy.c -fPIC -o libmem.so -ldl
gcc -O3  -shared time.c -fPIC -o libtime.so -ldl
g++ -O0 test.cpp 
FIXED_TIME=0 LD_PRELOAD="$PWD/libmem.so $PWD/libtime.so" ./a.out 2>&1 
