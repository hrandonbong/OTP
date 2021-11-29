# OTP
One Time Pad program that uses a client interface to send a message, encrypts it in a server, and if a client wants to, they can send the same messahe to a decryption server to
read the message in English.

These 5 programs combine multi-processing code with socket-based inter-process communication. The programs are accessible from the command line using standard Unix features.

## Setup
run ./compileall 
run ./enc_server #randomnumber 50000+
run ./dec_server #randomnumber 50000+ different from enc_server
run ./keygen > #keyFile
run ./enc_client #textFile keyFile enc_server > cipher file
#this will print out whatever cipher file you chose
#to decrypt
run ./dec_client #cipherFile keyFile enc_server > decrypt file
