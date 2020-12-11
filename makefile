make: server.cpp server_resources.cpp server_test.cpp websocket.cpp
	g++ server.cpp server_resources.cpp server_test.cpp websocket.cpp -o server -lcrypto -lz -ldl -static-libgcc