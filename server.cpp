// g++ server.cpp server_resources.cpp server_test.cpp -o server -lcrypto -lz -ldl -static-libgcc

#include "server.h"
#include <string.h>
#include <bitset>

#define PORT 8000
#define MAX_CLIENTS 30
#define BUFF_SIZE 1024

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

  std::cout << std::endl
            << std::endl
            << server_request << std::endl
            << std::endl;

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

int main()
{

  if (!unit_test())
  {
    return -1;
  }

  std::cout << std::endl
            << std::endl
            << "Unit tests passed. Starting the server..." << std::endl
            << std::endl;

  int server = socket(AF_INET, SOCK_STREAM, 0);
  sockaddr_in addr;
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = INADDR_ANY;
  addr.sin_port = htons(PORT);
  socklen_t addr_len = sizeof(addr);

  if (server == -1 ||
      bind(server, (sockaddr *)&addr, addr_len) ||
      listen(server, SOMAXCONN))
  {
    std::cerr << "Failed to open socket." << std::endl;
    close(server);
    return -1;
  }

  fd_set client_set;
  int client_fd[MAX_CLIENTS] = {0};
  int biggest_fd;

  int server_activity;
  int current_fd;
  int bytes_read;

  char buffer[BUFF_SIZE] = {0};
  int i;

  while (true)
  {
    // Clear the socket set
    FD_ZERO(&client_set);

    // Add server socket to set
    FD_SET(server, &client_set);
    biggest_fd = server;

    // Add existing clients back to socket set
    for (i = 0; i < MAX_CLIENTS; i++)
    {
      current_fd = client_fd[i];

      // If current socket is valid, add to list
      if (current_fd > 0)
      {
        FD_SET(current_fd, &client_set);
      }

      // Update biggest_fd if necessary
      if (current_fd > biggest_fd)
      {
        biggest_fd = current_fd;
      }
    }

    // Wait for activity on one of the clients
    // Timeout is NULL so wait indefinitely
    server_activity = select(biggest_fd + 1, &client_set, NULL, NULL, NULL);

    if (server_activity < 0 && errno != EINTR)
    {
      std::cerr << "Select has failed: " << strerror(errno) << std::endl;
      std::cout << "Trying to continue..." << std::endl;
    }

    // If server socket is set, then a new client is joining
    if (FD_ISSET(server, &client_set))
    {
      current_fd = accept(server, (sockaddr *)&addr, &addr_len);

      if (current_fd == -1)
      {
        std::cerr << "Failed to accept new client: " << strerror(errno) << std::endl;
        std::cout << "Trying to continue..." << std::endl;
      }
      else
      {
        // Add new socket to array of sockets
        for (i = 0; i < MAX_CLIENTS; i++)
        {
          if (client_fd[i] == 0)
          {
            client_fd[i] = current_fd;
            SendWebSockAccept(current_fd);

            break;
          }
        }
      }
    }

    for (i = 0; i < MAX_CLIENTS; i++)
    {
      current_fd = client_fd[i];

      if (FD_ISSET(current_fd, &client_set))
      {
        // Clear the buffer
        buffer[0] = '\0';

        // Read the message and check number of bytes read
        bytes_read = recv(current_fd, buffer, BUFF_SIZE, 0);

        // If bytes read is 1 recv failed
        if (bytes_read == -1)
        {
          std::cerr << "Failed to receive message from client " << current_fd << ": " << strerror(errno) << std::endl;
          // TODO: How should I handle this? Probably just kick the client.
          // Should I try to send a message to the client when they get kicked here?
        }
        // recv returns 0 if the client has gracefully disconnected.
        else if (bytes_read == 0)
        {
          std::cout << "Client " << i << " has disconnected." << std::endl;

          close(current_fd);
          client_fd[i] = 0;
        }
        // Handle the recv succeed case
        else
        {
          std::string message = DecodeWebSocket(buffer, bytes_read);

          std::cout << message << std::endl;

          for (int j = 0; j < MAX_CLIENTS; j++)
          {
            if (i != j)
            {
              current_fd = client_fd[j];
              std::string new_message = EncodeWebSocket((char *)message.c_str(), message.size());
              send(current_fd, new_message.c_str(), new_message.size(), 0);
            }
          }

          buffer[0] = '\0';
        }
      }
    }
  }

  return 0;
}