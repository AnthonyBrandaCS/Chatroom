#include "server.h"

#include <string.h>

#define PORT 8000
#define MAX_CLIENTS 30
#define BUFF_SIZE 1024
#define DEFAULT_NAME "Someone"

void handleMessage(char buffer[], int bytes_read, client *clients, int idx)
{
    std::string message = DecodeWebSocket(buffer, bytes_read);

    std::cout << message << std::endl;

    if (message.find("\\name") != std::string::npos || message.find("\\NAME") != std::string::npos)
    {
        if (message[5] == ' ' && message[0] == '\\' && message.size() > 6)
        {
            std::cout << "NAME command found." << std::endl;
            std::string name = "";
            for (int i = 6; i < message.size(); i++)
            {
                name += message[i];
            }

            std::string name_msg = "\\name " + name;
            name_msg = EncodeWebSocket((char *)name_msg.c_str(), name_msg.size());
            send(clients[idx].fd, name_msg.c_str(), name_msg.size(), 0);

            if (clients[idx].name != DEFAULT_NAME)
            {
                name_msg = clients[idx].name + " has changed their name to " + name;
                name_msg = EncodeWebSocket((char *)name_msg.c_str(), name_msg.size());

                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i].fd > 0 && idx != i)
                    {
                        send(clients[i].fd, name_msg.c_str(), name_msg.size(), 0);
                    }
                }
            }
            else
            {
                name_msg = name + " has joined the chat.";
                name_msg = EncodeWebSocket((char *)name_msg.c_str(), name_msg.size());
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i].fd > 0 && idx != i)
                    {
                        send(clients[i].fd, name_msg.c_str(), name_msg.size(), 0);
                    }
                }
            }

            clients[idx].name = name;
        }
    }
    else if (message.find("\\exit") != std::string::npos || message.find("\\EXIT") != std::string::npos)
    {
        std::cout << "EXIT command found." << std::endl;
        std::string exit_msg = clients[idx].name + " has left the chat.";
        exit_msg = EncodeWebSocket((char *)exit_msg.c_str(), exit_msg.size());

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (clients[i].fd > 0 && idx != i)
            {
                send(clients[i].fd, exit_msg.c_str(), exit_msg.size(), 0);
            }
        }

        close(clients[idx].fd);
        clients[idx].fd = 0;
        clients[idx].name = "";
    }
    else
    {
        message = clients[idx].name + ": " + message;
        std::string new_message = EncodeWebSocket((char *)message.c_str(), message.size());
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            send(clients[i].fd, new_message.c_str(), new_message.size(), 0);
        }
    }
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
    client clients[MAX_CLIENTS] = {0};
    int biggest_fd;

    int server_activity;
    int bytes_read;

    char buffer[BUFF_SIZE] = {0};

    while (true)
    {
        // Clear the socket set
        FD_ZERO(&client_set);

        // Add server socket to set
        FD_SET(server, &client_set);
        biggest_fd = server;

        // Add existing clients back to socket set
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            // If current socket is valid, add to list
            if (clients[i].fd > 0)
            {
                FD_SET(clients[i].fd, &client_set);
            }

            // Update biggest_fd if necessary
            if (clients[i].fd > biggest_fd)
            {
                biggest_fd = clients[i].fd;
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
            int new_fd = accept(server, (sockaddr *)&addr, &addr_len);

            if (new_fd == -1)
            {
                std::cerr << "Failed to accept new client: " << strerror(errno) << std::endl;
                std::cout << "Trying to continue..." << std::endl;
            }
            else
            {
                // Add new socket to array of sockets
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    if (clients[i].fd == 0)
                    {
                        clients[i].fd = new_fd;
                        clients[i].name = DEFAULT_NAME;
                        SendWebSockAccept(new_fd);

                        break;
                    }
                }
            }
        }

        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            if (FD_ISSET(clients[i].fd, &client_set))
            {
                // Read the message and check number of bytes read
                bytes_read = recv(clients[i].fd, buffer, BUFF_SIZE, 0);

                buffer[bytes_read] = '\0';

                // If bytes read is 1 recv failed
                if (bytes_read == -1)
                {
                    std::cerr << "Failed to receive message from client " << i << ": " << strerror(errno) << std::endl;
                    // TODO: How should I handle this? Probably just kick the client.
                    // Should I try to send a message to the client when they get kicked here?
                }
                // recv returns 0 if the client has gracefully disconnected.
                else if (bytes_read == 0)
                {
                    close(clients[i].fd);
                    clients[i].fd = 0;
                }
                // Handle the recv succeed case
                else
                {
                    handleMessage(buffer, bytes_read, clients, i);
                }
            }
        }
    }

    return 0;
}