#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

// Set up the address struct
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname){

  // Clear out the address struct
  memset((char*) address, '\0', sizeof(*address));

  // The address should be network capable
  address->sin_family = AF_INET;
  // Store the port number
  address->sin_port = htons(portNumber);

  // Get the DNS entry for this host name
  struct hostent* hostInfo = gethostbyname(hostname);
  if (hostInfo == NULL) {
    fprintf(stderr, "CLIENT: ERROR, no such host\n");
    exit(1);
  }

  // Copy the first IP address from the DNS entry to sin_addr.s_addr
  memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

int main(int argc, char *argv[]) {

  // Variables for reading in the text from both the input file and key
  char* fileBuffer = NULL;
  char* keyBuffer = NULL;
  char* buffer = NULL;
  long fileLength;
  long keyLength;
  long bufferLength;

  // File variables for the text file and key
  FILE *fileDecrypt;
  FILE *fileKey;

  // Open file
  fileDecrypt = fopen(argv[1], "r+");

  // File reader for decrypting
  if (fileDecrypt) {
    fseek (fileDecrypt, 0, SEEK_END);
    fileLength = ftell(fileDecrypt);
    fseek (fileDecrypt, 0, SEEK_SET);
    fileBuffer = malloc(fileLength+1);

    // If the file reading was successful, we read whats in it
    if (fileBuffer) {
      fread (fileBuffer, 1, fileLength, fileDecrypt);
    }
    fclose (fileDecrypt);
  }

  // Remove the new line char at the end cuz we are combing the key with text
  if (strstr(fileBuffer, "\n") != NULL) {
    char* textNewLine = strstr(fileBuffer, "\n");
    *textNewLine = '\0';
    fileLength = fileLength - 1;
  }

  // For loop goes through all of the input characters and verifies they aren't bad
  for (int i = 0; i < fileLength; ++i) {
    int decChecker = fileBuffer[i] - 65;

    // If the check int is below 0 and not -33 (space) then it is bad
    if (decChecker < 0 && decChecker != -33) {
      fprintf(stderr, "dec_client error: input contains bad characters\n");
      exit(1);

    // Or if the int is above 25
    } else if (decChecker > 25) {
      fprintf(stderr, "dec_client error: input contains bad characters\n");
      exit(1);
    }
  }

  // Same code as above, open the key file and read it in
  fileKey = fopen(argv[2], "r+");
  if (fileKey) {
    fseek (fileKey, 0, SEEK_END);
    keyLength = ftell(fileKey);
    fseek (fileKey, 0, SEEK_SET);
    keyBuffer = malloc(keyLength+1);

    // If the file reading was successful, we read whats in it
    if (keyBuffer) {
      fread (keyBuffer, 1, keyLength, fileKey);
    }
    fclose (fileKey);
  }

  // Remove the new line char at the end cuz we are combing the key with text
  if (strstr(keyBuffer, "\n") != NULL) {
    char* keyNewLine = strstr(keyBuffer, "\n");
    *keyNewLine = '\0';
    keyLength = keyLength - 1;
  }

  // Check to see if the key is at least the length of the file
  if (keyLength < fileLength){
    fprintf(stderr, "ERROR: key ‘%s’ is too short\n",argv[2]);
    exit(1);
  }

  // Buffer length will be double the input file size cuz text + key. We need to add ENC to the beginning 
  // and @@ to the end, hence +5 for length, and +6 for buffer + \n
  bufferLength = (fileLength * 2) + 5;
  buffer = malloc((fileLength * 2) + 6);

  // So we know it is the dec text we are sending to the decrypt server
  strcpy(buffer, "DEC");                    //Decrypt indicator
  strcat(buffer, fileBuffer);               // Cipher text we are sending in
  strncat(buffer, keyBuffer, fileLength);   // key used to decrypt
  strcat(buffer, "@@");                     // terminator

  // Create ints for reading/writing into the socket
  // Create the socket address struct
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress_dec;

  // Check usage & args - that there are enough arguments
  if (argc < 2) {
    fprintf(stderr, "USAGE: %s hostname port\n", argv[0]);
    exit(1);
  }

  // Create a socket
  socketFD = socket(AF_INET, SOCK_STREAM, 0);
  if (socketFD < 0){
    fprintf(stderr, "CLIENT: ERROR opening socket");
    exit(1);
  }

   // Set up the server address struct
  setupAddressStruct(&serverAddress_dec, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress_dec, sizeof(serverAddress_dec)) < 0){
    fprintf(stderr, "CLIENT: ERROR connecting");
    exit(1);
  }

  // Write to the server checking to make sure all the chars were written
  charsWritten = send(socketFD, buffer, strlen(buffer), 0);
  if (charsWritten < 0){
    fprintf(stderr, "CLIENT: ERROR writing to socket");
  }
  if (charsWritten < strlen(buffer)){
    fprintf(stderr, "CLIENT: WARNING: Not all data written to socket!\n");
  }

  // Read in chunks of 1000 set up
  memset(buffer, '\0', bufferLength);
  char socket_buffer_dec[1000];

  // Reading the response from the server, terminate while loop at @@
  while (strstr(buffer, "@@") == NULL){

    // If an improper connection was established ++ would be sent back with an error message
    if (strstr(buffer, "++") != NULL) {
      fprintf(stderr, "Error: could not contact dec_server on port %s\n", argv[3]);
      exit(2);
    }

    // clear the socket buffer, we are reading 1000 characters at a time until @@
    memset(socket_buffer_dec, '\0', 1000);

    // Read socket chunk
    charsRead = recv(socketFD, socket_buffer_dec, sizeof(socket_buffer_dec) - 1, 0);
    if (charsRead < 0){
      fprintf(stderr, "ERROR reading from socket\n");
    }

    // Add the chunk to the main buffer message
    strcat(buffer, socket_buffer_dec);
  }

  // Remove the terminating characters
  char* terminalLoc_dec = strstr(buffer, "@@");
  *terminalLoc_dec = '\0';

  // Add a newline char at the end
  strcat(buffer, "\n");

  // Print to stdout
  fprintf(stdout,"%s", buffer);

  // Close the socket
  close(socketFD);
  return 0;
}