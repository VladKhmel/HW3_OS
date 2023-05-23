#include <sys/socket.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

#define MAX_MESSAGE_SIZE 1024
#define MAX_CLIENTS_COUNT 10
#define SUCCESS_MESSAGE "SUCCESS"
#define FAILURE_MESSAGE "FAILURE"
#define TREASURE_FOUND_MESSAGE "TREASURE FOUND"

struct Client {
    int socket;
    struct sockaddr_in address;
};
int clientsCount = 0;
struct Client clients[MAX_CLIENTS_COUNT];
int treasureIndex;
int checkedCount = 0;
bool sendMessage(int socket, char* message) {
    if (send(socket, message, strlen(message), 0) < 0) {
        perror("Send failed: ");
        return false;
    }
    return true;
}

bool receiveMessage(int socket, char* messageBuffer) {
    size_t size;
    if ((size = recv(socket, messageBuffer, MAX_MESSAGE_SIZE, 0)) < 0) {
        perror("Receive failed: ");
        return false;
    }
    messageBuffer[size] = '\0';
    return true;
}

void handleClient(int socket) {
    char messageBuffer[MAX_MESSAGE_SIZE];
    if(treasureIndex < MAX_CLIENTS_COUNT) {
        if (sendMessage(socket, SUCCESS_MESSAGE)) {
            if (!receiveMessage(socket, messageBuffer)) {
                return;
            }
            if (strcmp(messageBuffer, TREASURE_FOUND_MESSAGE) == 0) {
                treasureIndex = clientsCount;
                sendMessage(socket, TREASURE_FOUND_MESSAGE);
                for (int i = 0; i < clientsCount; i++) {
                    sendMessage(clients[i].socket, TREASURE_FOUND_MESSAGE);
                }
                return;
            }
        }
        sendMessage(socket, FAILURE_MESSAGE);
    } else {
        sendMessage(socket, TREASURE_FOUND_MESSAGE);
    }
}

void runServer(int port) {
    int serverSocket;
    struct sockaddr_in serverAddress;
    char messageBuffer[MAX_MESSAGE_SIZE];
    if ((serverSocket = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed: ");
        return;
    }

    memset(&serverAddress, 0, sizeof(serverAddress));
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(port);
    serverAddress.sin_addr.s_addr = htonl(INADDR_ANY);

    if (bind(serverSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        perror("Bind failed: ");
        return;
    }

    if (listen(serverSocket, MAX_CLIENTS_COUNT) < 0) {
        perror("Listen failed: ");
        return;
    }

    printf("Server started on port %d\n", port);

    while (true) {
        struct sockaddr_in clientAddress;
        int clientSocket;
        socklen_t clientAddressLen = sizeof(clientAddress);
        if ((clientSocket = accept(serverSocket, (struct sockaddr*)&clientAddress, &clientAddressLen)) < 0) {
            perror("Accept failed: ");
            continue;
        }
        clients[clientsCount] = (struct Client){.socket = clientSocket, .address = clientAddress};
        clientsCount++;
        handleClient(clientSocket);
        checkedCount++;
        if (checkedCount == MAX_CLIENTS_COUNT) {
            for (int i = 0; i < clientsCount; i++) {
                sendMessage(clients[i].socket, TREASURE_FOUND_MESSAGE);
            }
            break;
        }
    }

    printf("Treasure has been found\n");

    for (int i = 0; i < clientsCount; i++) {
        close(clients[i].socket);
    }
    close(serverSocket);
}

int main(int argc, char *argv[]) {

    if (argc < 2) {
        printf("Usage: %s port\n", argv[0]);
        return 1;
    }
    int port = atoi(argv[1]);
    if (port == 0) {
        printf("Invalid port number\n");
        return 1;
    }
    runServer(port);
    return 0;
}
