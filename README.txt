Testing:
This project ran perfectly on my macbook pro with two terminal windows open using localhost.
It also ran when testing the server on flip1 and the client on flip2.

References:
Listed at the top of .py and .c files

HOW TO COMPILE:
1. Terminal A type the following to compile and run server (port number has to be between 1024 and 65535):

   make server
   ./ftserver <server port number>


2. Terminal B type the following to start client (server host is the host you are running ftserver on.
    for example if Terminal A is on flip1.engr.oregonstate.edu you would use that as the <server host>.
    The server port is the same as the port number used on Terminal A for the server. -l command will
    print the directory contents of the server. -g <filename> will transfer the file to the directory 
    that the client is running on if it exists.  <data port> needs to be a different port that the 
    server's port and again must be between 1024 and 65535):

    python ftclient.py <server host> <server port> -l | -g <filename> <data port>
    

3. To shut down the server type CTRL-C, this will terminate the server and automatically send \quit 
   the client causing it to also terminate




   



