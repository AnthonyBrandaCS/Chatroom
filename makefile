make: server.cpp server_resources.cpp server_test.cpp
	g++ server.cpp server_resources.cpp server_test.cpp -o server -lcrypto -lz -ldl -static-libgcc