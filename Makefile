all:
	mkdir -p build && cd build && cmake .. && make

test:
	cd build && ctest --output-on-failure

clean:
	rm -rf build gateway_node
