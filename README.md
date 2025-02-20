# http -- a prototype of a webserver for learning [io_uring](https://github.com/axboe/liburing)

**Author**: Peter Foelsche |
**Date**: February 2025 |
**Location**: Austin, TX, USA |
**Email**: [peter_foelsche@outlook.com](mailto:peter_foelsche@outlook.com)

## Introduction

This prototype was only intended for me to understand the asynchronous io library called [io_uring](https://github.com/axboe/liburing) library and to make it safe to use in C++ without running the risk of leaking memory or system handles.
The prototype currently only accepts connection requests and reads input from the client.
The main point established is how to nicely wrap the io_uring API into C++ leveraging RAII and Exception Handling.
To build just do `g++ -std=c++14 *.cpp -luring -O3 -DNDEBUG` and run the created executable by typing `./a.out` into the same shell and enter `http://localhost:8080/` in a browser running on the same machine. The process will do nothing else than printing out the text it received from the browser.
The cpp files in this directory are:
### http.cpp
Contains the `main()` function, the event loop and the handler code for asynchronous `accept()`- and `read()`-requests.
### io_data.h, io_data.cpp
A abstract class serving for io_requests and the calling of handlers from the request.
### io_uring_queue_init.h, io_uring_queue_init.cpp
A RAII wrapper class for the function of the same name.
A factory for `read()`- and `accept()`-requests and their implementation.
### io_uring_wait_cqe.h
A RAII wrapper class for the function of the same name
### socket.h
A RAII wrapper class for the function of the same name
