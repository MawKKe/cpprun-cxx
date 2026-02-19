out/cpprun: cpprun.cpp
	mkdir -p out
	c++ -std=c++17 -Wall -Wextra -pedantic -g -o $@ cpprun.cpp

clean:
	rm -rf out

test: out/cpprun hello.cpp
	env CPPRUN_VERBOSE=1 ./out/cpprun hello.cpp -O3 -std=c++17 -- foo bar baz

ctest:
	cmake -B out -S . -GNinja -DENABLE_TESTING=ON -DCMAKE_BUILD_TYPE=Debug
	cmake --build out --verbose
	ctest --test-dir out --output-on-failure --no-tests=error

.PHONY: clean test ctest
