all: http.exe
CXXFLAGS=-std=c++17 -g -DNDEBUG

clean:
	rm -f http.exe

http.exe:http.cpp io_data.cpp io_data.h io_uring_queue_init.cpp io_uring_queue_init.h io_uring_wait_cqe.h socket.h
	$(CXX) $(CXXFLAGS) -o $@ http.cpp io_data.cpp io_uring_queue_init.cpp -luring
