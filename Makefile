all: libHoardAllocator239.so
	
libHoardAllocator239.so: malloc-intercept.cpp HoardAllocator.cpp
	g++ --shared -std=c++11 -fPIC -g -o libHoardAllocator239.so malloc-intercept.cpp HoardAllocator.cpp
	
run: libHoardAllocator239.so
	LD_PRELOAD=./libHoardAllocator239.so xterm

runWithoutTrace: libHoardAllocator239.so
	LD_PRELOAD=./libHoardAllocator239.so HOARD_NO_TRACE=1 gedit
	
clean:
	rm -rf *o ./libHoardAllocator239.so