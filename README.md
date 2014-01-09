HoardAllocator239
=================
This is an implementation of Hoard: A Scalable Memory Allocator for Multithreaded Applications.

All ideas of this allocator were read from this article:
http://people.cs.umass.edu/~emery/pubs/berger-asplos2000.pdf

The reason for this task was a course in SPb NRU ITMO, Computer Technologies Department - http://www.ifmo.ru/


How to use:
=================
You can use make:
-----------------

To compile allocator:
 
    make

To run xterm with tracing all allocations:
    
    make run

To run gedit without tracing:

    make runWithoutTrace

How to compile and run:
-----------------------

1) compile:

    g++ --shared -std=c++11 -fPIC -g -o libHoardAllocator239.so malloc-intercept.cpp HoardAllocator.cpp

2) run with no trace:

    LD_PRELOAD=./libHoardAllocator239.so HOARD_NO_TRACE=1 krusader

3) run with trace:

    LD_PRELOAD=./libHoardAllocator239.so HOARD_NO_TRACE=0 krusader

4) run with trace:

    LD_PRELOAD=./libHoardAllocator239.so krusader
