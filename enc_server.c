#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>

#define BUFFERSIZE 150000
#define PIPEBUFFER 1000

// Error function used for reporting issues
void error(const char* msg) {
    perror(msg);
    exit(1);
}

// Set up the address struct for the server socket
void setupAddressStruct(struct sockaddr_in* address, int portNumber) {
    // Clear out the address struct
    memset((char*)address, '\0', sizeof(*address));

    // The address should be network capable
    address->sin_family = AF_INET;
    // Store the port number
    address->sin_port = htons(portNumber);
    // Allow a client at any address to connect to this server
    address->sin_addr.s_addr = INADDR_ANY;
}

int main(int argc, char* argv[]) {

    // Various variables for connecting, buffers, and the socket
    // Connect bool is for checking that the right client is connecting
    int connectionSocket, charsRead, charsWritten;
    char buffer[BUFFERSIZE];
    char pipe_buffer[PIPEBUFFER];
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);
    int connect_bool = 1;

    // Check usage & args
    if (argc < 2) {
        fprintf(stderr, "USAGE: %s port\n", argv[0]);
        exit(1);
    }

    // Create the socket that will listen for connections
    int listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("ERROR opening socket");
    }

    printf("Port Number is %d\n", atoi(argv[1]));
    // Set up the address struct for the server socket
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Associate the socket to the port
    if (bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0) {
        error("ERROR on binding");
    }

    // Start listening for connetions. Allow up to 5 connections to queue up
    listen(listenSocket, 5);

    // Accept a connection, blocking if one is not available until one connects
    while (1) {

        // Accept the connection request which creates a connection socket
        connectionSocket = accept(listenSocket, (struct sockaddr*)&clientAddress, &sizeOfClientInfo);
        if (connectionSocket < 0) {
            error("ERROR on accept");
        }

        // Fork a child process in oorder to have up to 5 different connections but not interfere
        // with each other
        pid_t childPid = fork();
        if (childPid == 0) {

            // We begin reading the socket buffer in chunks of 1000
            memset(buffer, '\0', BUFFERSIZE);
            char sockBuffer[PIPEBUFFER];
            int beginRead = 1;

            // @@ is the terminator, so we will keep reading until client sends this
            while (strstr(buffer, "@@") == NULL) {

                // Clear buffer so old text is not re-inputted
                memset(sockBuffer, '\0', PIPEBUFFER);

                // Read in the chunk from the socket
                charsRead = recv(connectionSocket, sockBuffer, sizeof(sockBuffer) - 1, 0);
                if (charsRead < 0) {
                    error("ERROR reading from socket");
                }

                // Check it is coming from DEC or ENC client, find ENC in first 3 char
                if (beginRead == 1) {
                    char sourceCheck[4];
                    strncpy(sourceCheck, sockBuffer, 3);
                    int cmp_enc = strcmp(sourceCheck, "ENC");
                    if (cmp_enc != 0) {
                        //If we entered this it means the connection is not from the encryption client
                        charsRead = send(connectionSocket, "++ERROR: source is wrong", 22, 0);
                        if (charsRead < 0) {
                            error("ERROR reading from socket");
                        }

                        // Now we break out in order to end the connection
                        connect_bool = 0;
                        break;
                    }
                    else {
                        beginRead = 0;
                    }
                }

                // Add the chunk to the main buffer message
                strcat(buffer, sockBuffer);
            }

            // Is it a valid connection? 1 yes, 0 no
            if (connect_bool == 1) {

                // Remove the terminal indicator of "@@"
                char* termBuff = strstr(buffer, "@@");
                *termBuff = '\0';

                // Since the first 3 characters are ENC we need to remove them
                char inptStr[strlen(buffer) - 2];
                memset(inptStr, '\0', strlen(buffer) - 2);
                strcpy(inptStr, &buffer[3]);

                // input string has both key + text so essentially 2 times the length
                long cipherLen = strlen(inptStr) / 2;

                // Create and null terminate a string for the original text, key, and encrypted text
                char originalText[cipherLen + 1];
                memset(originalText, '\0', cipherLen + 1);
                char key[cipherLen + 1];
                memset(key, '\0', cipherLen + 1);
                char encryptedText[cipherLen + 3];
                memset(encryptedText, '\0', cipherLen + 3);

                // Copy first half = the text
                //     second half = key
                strncpy(originalText, inptStr, cipherLen);
                strcpy(key, &inptStr[cipherLen]);

                // Each char is associated with aan int. Minus 65 we get it in a 0-25 range
                // Manually change 32-65 = -33 to space
                for (int i = 0; i < cipherLen; ++i) {
                    int origInt = originalText[i] - 65;
                    if (origInt == -33) {
                        // Space
                        origInt = 26;
                    }
                    int keyInt = key[i] - 65;
                    if (keyInt == -33) {
                        // Space
                        keyInt = 26;
                    }

                    if (((origInt + keyInt) % 27) == 26) {
                        encryptedText[i] = 32;
                    }
                    else {
                        encryptedText[i] = ((origInt + keyInt) % 27) + 65;
                    }
                }

                // Add @@ tot he end for terminator
                strcat(encryptedText, "@@");

                // Send back
                charsWritten = send(connectionSocket, encryptedText, strlen(encryptedText), 0);
                if (charsWritten < 0) {
                    error("ERROR writing to socket");
                }
                if (charsWritten < strlen(encryptedText)) {
                    error("SERVER: WARNING: Not all data written to socket!\n");
                }
            }
            // Terminate the child
            exit(0);
            break;
        }
        // Close the connection socket for this client and terminates child processes
        wait(NULL);
        close(connectionSocket);
    }
    // Close the listening socket
    close(listenSocket);
    return 0;
}
