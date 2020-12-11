#include "server.h"

#include <bitset>

std::string getSockKey(std::string server_request)
{
    const std::string Sec_WebSocket_Key = "Sec-WebSocket-Key: ";
    size_t header_index = server_request.find(Sec_WebSocket_Key);
    header_index += Sec_WebSocket_Key.size();

    std::string acceptance = "";

    for (int i = header_index; i < header_index + 24; i++)
    {
        acceptance += server_request[i];
    }

    return acceptance;
}

std::string getSockAccept(std::string client_key)
{
    std::string working_key = client_key + "258EAFA5-E914-47DA-95CA-C5AB0DC85B11";

    unsigned char buffer[SHA_DIGEST_LENGTH] = {0};
    SHA1((const unsigned char *)working_key.c_str(), working_key.size(), buffer);

    return base64_encode(buffer, SHA_DIGEST_LENGTH);
}

void SendWebSockAccept(int client)
{
    char buffer[1024] = {0};
    recv(client, buffer, 1024, 0);

    std::string server_request(buffer);

    send(client, "HTTP/1.1 101 Switching Protocols\r\n", 34, 0);
    send(client, "Upgrade: websocket\r\n", 20, 0);
    send(client, "Connection: Upgrade\r\n", 21, 0);

    std::string socket_accept = "Sec-WebSocket-Accept: " + getSockAccept(getSockKey(server_request));
    socket_accept += "\r\n\r\n";

    send(client, socket_accept.c_str(), socket_accept.size(), 0);
}

std::string DecodeWebSocket(const char buffer[], const size_t length)
{

    std::vector<std::bitset<8>> data;

    for (int i = 0; i < length; i++)
    {
        data.push_back(std::bitset<8>(buffer[i]));
    }

    data[0].set(4, 0);
    data[0].set(5, 0);
    data[0].set(6, 0);
    data[0].set(7, 0);
    uint8_t opcode = static_cast<uint8_t>(data[0].to_ulong());

    switch (opcode)
    {
    case 1:
        // TODO:
        // Indicate that it is a text message
        break;
    default:
        // TODO:
        // Cover other cases
        break;
    }

    bool mask = data[1].test(7);
    data[1].set(7, 0);

    if (!mask)
    {
        std::cout << "Discard data, mask not set." << std::endl;
        return "";
    }

    uint64_t msg_length = static_cast<uint8_t>(data[1].to_ulong());

    int start_index = 2;
    if (msg_length == 126)
    {
        // TODO:
        // This should also be unit tested.
        std::cout << "Attempting to set 16 bit length." << std::endl;
        std::bitset<16> tmp(data[2].to_string() + data[3].to_string());
        msg_length = static_cast<uint64_t>(tmp.to_ullong());
        start_index = 4;
    }
    else if (msg_length == 127)
    {
        // TODO:
        // This should also be unit tested.
        std::cout << "Attempting to set 64 bit length." << std::endl;
        std::bitset<64> tmp(data[2].to_string() + data[3].to_string() + data[4].to_string() + data[5].to_string() + data[6].to_string() + data[7].to_string() + data[8].to_string() + data[9].to_string());
        msg_length = static_cast<uint64_t>(tmp.to_ullong());
        start_index = 10;
    }

    std::vector<std::bitset<8>> mask_bits;
    mask_bits.push_back(data[start_index]);
    mask_bits.push_back(data[start_index + 1]);
    mask_bits.push_back(data[start_index + 2]);
    mask_bits.push_back(data[start_index + 3]);
    start_index += 4;

    std::string decoded_message = "";
    for (int i = start_index, j = 0; i < length; i++, j++)
    {
        decoded_message += (int)(data[i].to_ulong() ^ mask_bits[j % 4].to_ulong());
    }

    return decoded_message;
}

std::string EncodeWebSocket(char message[], size_t length)
{

    std::string encoded_message;
    encoded_message += (char)0b10000001;

    std::bitset<8> size(length);
    encoded_message += static_cast<char>(size.to_ulong());

    for (int i = 0; i < length; i++)
    {
        std::bitset<8> current(message[i]);

        encoded_message += (int)current.to_ulong();
    }

    return encoded_message;
}