all: http.exe
CXXFLAGS=-std=c++17 -O3 -DNDEBUG -flto=auto -MMD -MP

OBJECTS=http.o io_data.o io_uring_queue_init.o

DEPS = $(OBJECTS:.o=.d)

clean:
	rm -f http.exe $(OBJECTS) $(DEPS)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

http.exe:$(OBJECTS)
	$(CXX) $(CXXFLAGS) -o $@ $(OBJECTS) -luring

-include $(DEPS)
