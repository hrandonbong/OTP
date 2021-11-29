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
   //------------ My code -----------------------//
  // Variables for reading in the text from both the input file and key
  char* textBuffer = NULL;
  char* keyBuffer = NULL;
  char* buffer = NULL;
  long textLength;
  long keyLength;
  long bufferLength;

  // File variables for the text file and key
  FILE *file_read;
  FILE *keyRead;

  // Open the given file
  file_read = fopen(argv[1], "r+");

  // Reading our text file
  if (file_read) {
    fseek (file_read, 0, SEEK_END);
    textLength = ftell(file_read);
    fseek (file_read, 0, SEEK_SET);
    textBuffer = malloc(textLength+1);

    if (textBuffer) {
      fread (textBuffer, 1, textLength, file_read);
    }
    fclose (file_read);
  }

  // There will be a \n at the end of the text buffer so we will remove it cuz we are adding the key to it
  // since we removed /n we decrement the file length
  if (strstr(textBuffer, "\n") != NULL) {
    char* new_line = strstr(textBuffer, "\n");
    *new_line = '\0';
    textLength = textLength - 1;
  }

  // Makes sure we are in the 0 to 25 range or -33 for spaces
  for (int i = 0; i < textLength; ++i) {
    int check_int = textBuffer[i] - 65;

    // If the check int is below 0 and not -33 (space) then it is bad
    if (check_int < 0 && check_int != -33) {
      fprintf(stderr, "enc_client ERROR: input contains bad characters\n");
      exit(1);

    // Or if the int is above 25
    } else if (check_int > 25) {
      fprintf(stderr, "enc_client ERROR: input contains bad characters\n");
      exit(1);
    }
  }

  // reading our key file with the same code above
  keyRead = fopen(argv[2], "r+");
  if (keyRead) {
    fseek (keyRead, 0, SEEK_END);
    keyLength = ftell(keyRead);
    fseek (keyRead, 0, SEEK_SET);
    keyBuffer = malloc(keyLength+1);

    if (keyBuffer) {
      fread (keyBuffer, 1, keyLength, keyRead);
    }
    fclose (keyRead);
  }

  // Remove the new line char at the end cuz we are combing the key with text
  if (strstr(keyBuffer, "\n") != NULL) {
    char* new_line_k = strstr(keyBuffer, "\n");
    *new_line_k = '\0';
    keyLength = keyLength - 1;
  }

  // Is key length shorter than our text length?
  if (keyLength < textLength){
    fprintf(stderr, "Error: key ‘%s’ is too short\n",argv[2]);
    exit(1);
  }

  // Buffer length is double the input file length. We need to add ENC to the beginning 
  // and @@ to the end, hence +5 for length, and +6 for buffer + \n
  bufferLength = (textLength * 2) + 5;
  buffer = malloc((textLength * 2) + 6);
  
  // Creating our cipher text
  strcpy(buffer, "ENC");                    // Copy in ENC first
  strcat(buffer, textBuffer);               // Add the text
  strncat(buffer, keyBuffer, textLength);   // Add the key
  strcat(buffer, "@@");                     // Add the @@ to end the buffer
  //------------ My code -----------------------//


  // Original code from example provided
  int socketFD, portNumber, charsWritten, charsRead;
  struct sockaddr_in serverAddress;

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
  setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

  // Connect to server
  if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
      fprintf(stderr, "CLIENT: ERROR connecting");
  }

  // Write to the server
  charsWritten = send(socketFD, buffer, strlen(buffer), 0);

  if (charsWritten < 0){
      fprintf(stderr, "CLIENT: ERROR writing to socket");
  }
  if (charsWritten < strlen(buffer)){
    fprintf(stderr, "CLIENT: WARNING: Not all data written to socket!\n");
  }

  // Now we sent info to enc server so buffer needs to be emptied
  memset(buffer, '\0', bufferLength);
  char socketBuffer[1000]; // Reading in 1000 sized chunks

  // Until we find @@ we will continue to call the server 
  while (strstr(buffer, "@@") == NULL){

    // If an improper connection was established ++ would be sent back with an error message
    if (strstr(buffer, "++") != NULL) {
      fprintf(stderr, "Error: could not contact enc_server on port %s\n", argv[3]);
      exit(2);
    }

    // First clear the socket buffer
    memset(socketBuffer, '\0', 1000);

    // Read in the chunk from the socket
    charsRead = recv(socketFD, socketBuffer, sizeof(socketBuffer) - 1, 0);
    if (charsRead < 0){
        fprintf(stderr, "ERROR reading from socket\n");
    }

    // Add the chunk to the main buffer message
    strcat(buffer, socketBuffer);
  }

  // Remove the terminating characters
  char* terminateLoc = strstr(buffer, "@@");
  *terminateLoc = '\0';

  // Add a newline char at the end
  strcat(buffer, "\n");

  // Print to stdout console or file
  fprintf(stdout,"%s", buffer);

  // Close the socket
  close(socketFD);
  return 0;
}