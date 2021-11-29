# OTP
One Time Pad program that uses a client interface to send a message, encrypts it in a server, and if a client wants to, they can send the same messahe to a decryption server to
read the message in English.

These 5 programs combine multi-processing code with socket-based inter-process communication. The programs are accessible from the command line using standard Unix features.

## Setup 1
Note: plain text files have been provided in order for you to test <br />
run ./compileall <br />
run ./enc_server #randomnumber 50000+ <br />
run ./dec_server #randomnumber 50000+ different from enc_server <br />
run ./keygen > #keyFile <br />
run ./enc_client #textFile keyFile enc_server > cipher file <br />
#this will print out whatever cipher file you chose <br />
#to decrypt <br />
run ./dec_client #cipherFile keyFile enc_server > decrypt file

## Setup 2
Note: plain text files have been provided in order for you to test <br />
run ./compileall <br />
run ./p5testsscript #randomNumber1 #randomNumber2 > myTestResults
