cmake:
	cmake -G Ninja -B build
	cmake --build build
clean:
	rm -rf build/*
