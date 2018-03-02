// Author: James Whiteley IV
// References:
// https://stackoverflow.com/questions/113224/what-is-the-largest-tcp-ip-network-port-number-allowable-for-ipv4
// https://codereview.stackexchange.com/questions/13933/stupidly-simple-tcp-client-server
// https://vcansimplify.wordpress.com/2013/03/14/c-socket-tutorial-echo-server/
// http://beej.us/guide/bgnet/html/single/bgnet.html#listen
// https://stackoverflow.com/questions/4214314/get-a-substring-of-a-char
// https://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
// https://stackoverflow.com/questions/2014033/send-and-receive-a-file-in-socket-programming-in-linux-with-c-c-gcc-g
// Also reused code from my Project 1 submission 

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <dirent.h>

//#define HANDLE_LENGTH 11 //room for 10 chars plus \0
#define MESSAGE_LENGTH 501 //room for 500 chars plus \0

//error message for connections
void error(const char *message) {
    perror(message);
    exit(0);
}

//displays usage message
void usage(void) {
    printf("\nUSAGE: ./ftserver <server port number>");
    printf("\nValid port numbers in range 1024 -> 65535\n");
    exit(0);
}

//this will hold all the data used for the server socket connection
struct Server {
    int port;  //port number
    int data_port; // port number for data transfer socket
    int sockfd; //socket file descriptor
    int data_sockfd; //data transfer socket file descriptor
    char message[MESSAGE_LENGTH]; // used to store full command from client
    char cmd[3]; // used to store command from client (-g or -l)
    char filename[MESSAGE_LENGTH - 3]; //stores filename.txt 
    struct sockaddr_in serv_addr; //address of server
};


// validates that port number is an integer in a range, returns port number
int get_port(int count, char *argv[]) {
    int port;

    //make sure correct command line args used
    if (count != 2) {
        usage();
    }

    port = atoi(argv[1]);

    if (port < 1024 || port > 65535) {
        usage(); 
    }

    return port;
}



// starts server on given port and waits for client request
void start_server(struct Server *server){
    // create socket
    server->sockfd = socket(AF_INET, SOCK_STREAM, 0); 

    if (server->sockfd < 0) {
        error("Could not establish socket"); 
    }

    server->serv_addr.sin_family = AF_INET;
    server->serv_addr.sin_port = htons(server->port);
    server->serv_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket
    if (bind(server->sockfd, (struct sockaddr *) &server->serv_addr, sizeof(server->serv_addr)) < 0) {
        close(server->sockfd);
        error("ERROR binding socket"); 
    } 

    // listen for requests, 10 incoming connections allowed to wait in backlog queue
    if (listen(server->sockfd, 10) < 0) {
        close(server->sockfd);
        error("ERROR listening"); 
    }

}


// sends message through socket param 
void send_message(int socket, char *message){
    int success;
    //char message[] = "File not found";
    success = write(socket, message, strlen(message)); 
    if (success < 0) {
        printf("ERROR sending error message\n"); 
    }
}


// validates command from client, returns 0 if invalid, 1 for -l, 2 for -g <FILENAME>
int validate_command(struct Server *server, int comm_fd){
    int success;
    memset(server->message, '\0', sizeof(server->message)); //set everything to null terminator

    // read request from client
    success = read(comm_fd, server->message, MESSAGE_LENGTH - 1); 

    //something went wrong, close socket and exit
    if (success < 0) {
        close(server->sockfd);
        return 0;
    }

    //copy command
    memcpy(server->cmd, &server->message[0], 2);
    server->cmd[2] = '\0';

    //command is -g so get filename also, return 2
    if (strcmp(server->cmd, "-g") == 0) {
        memcpy(server->filename, &server->message[2], MESSAGE_LENGTH - 1);
        return 2;
    }
    // command is -l, return 1
    else if (strcmp(server->cmd, "-l") == 0) { 
        return 1;
    }

    //neither -l nor -g were entered
    //send error to client showing correct usage
    send_message(comm_fd, "Invalid Command, Usage: -l || -g <FILENAME>");
    return 0;
}



// sends request to client to get data port number 
// returns the port number
int get_data_port(int socket){
    int success;
    char message[] = "OK";
    char buffer[7]; //to hold port as string

    success = write(socket, message, strlen(message)); 
    if (success < 0) {
        printf("ERROR sending error message\n"); 
    } 
    else { // receive data port number
        memset(buffer, '\0', sizeof(buffer)); //set everything to null terminator

        // read request from client
        success = read(socket, buffer, sizeof(buffer)); 

        //something went wrong, close socket and exit
        if (success < 0) {
            close(socket);
            return 0;
        }

        // return port number as integer
        return atoi(buffer);
    }

    return 0;

}


// checks directory for a filename, returns 1 if valid, 0 if invalid 
int validate_filename(int sock, char *filename){
    // if invalid FILENAME: send file not found message to client, return 0
    if (access(filename, 0) == 0) {
        send_message(sock, "OK");
        return 1;
    }
    printf("File not found. Sending error message.\n");
    send_message(sock, "File not found");
    return 0;
}

// creates a TCP data connection with client on <DATA_PORT>
void establish_data_connection(struct Server *server){

    // create socket
    server->data_sockfd = socket(AF_INET, SOCK_STREAM, 0); 

    if (server->data_sockfd < 0) {
        error("Could not establish socket"); 
    }

    server->serv_addr.sin_family = AF_INET;
    server->serv_addr.sin_port = htons(server->data_port);
    server->serv_addr.sin_addr.s_addr = INADDR_ANY;

    // bind socket
    if (bind(server->data_sockfd, (struct sockaddr *) &server->serv_addr, sizeof(server->serv_addr)) < 0) {
        close(server->data_sockfd);
        error("ERROR binding socket"); 
    } 

    // listen for requests, 10 incoming connections allowed to wait in backlog queue
    if (listen(server->data_sockfd, 10) < 0) {
        close(server->data_sockfd);
        error("ERROR listening"); 
    }


}

// sends server's directory to client on TCP data connection
void send_directory(int socket){
    DIR *d;
    struct dirent *dir;
    int success;
    int  i;
    char **names;
    char complete[] = "complete";

    sleep(2);

    // init pointer array of pointers 
    names = malloc(100 * sizeof(char *));
    for (i = 0; i < 100; i++) {
        names[i] = malloc(100 * sizeof(char)); 
        memset(names[i], '\0', 100); // set all to null terminator
    }

    // collect each file name into names array 
    i = 0;
    d = opendir(".");
    if (d) {
        while ((dir = readdir(d)) != NULL) {
            strcpy(names[i], dir->d_name);
            //printf("%s\n", dir->d_name);
            i++;
        }
        closedir(d);
    }

    int num_files = i;
    //send file names to client
    for (i = 0; i < num_files; i++) {
        success = write(socket, names[i], 100); 
        //printf("Sending file names...\n");
        //printf("%s\n", names[i]);
        if (success < 0) {
            printf("ERROR sending error message\n"); 
        }
    }

    success = write(socket, complete, sizeof(complete)); 
    if (success < 0) {
        printf("ERROR sending error message\n"); 
    }

    // free dynamic array
    for (i = 0; i < 100; i++) {
        free(names[i]);
    }
    free(names);
}

// sends contents of file to client, sends "transfer complete" message 
void send_file(int socket, char *file){
    int success;
    FILE *fd;
    int bytes_read;
    int bytes_written;
    char complete[] = "__complete__";
    char buffer[1000];
    void *ptr;
    memset(buffer, '\0', sizeof(buffer));

    sleep(2);

    fd = fopen(file, "r");
    if (fd == NULL) {return;}
    else {
        while(1) {

            // read file into buffer
            bytes_read = fread(buffer, sizeof(char), sizeof(buffer) - 1, fd);

            // nothing else to send
            if (bytes_read == 0) {break;}

            ptr = buffer;
            while(bytes_read >  0) {
                bytes_written = write(socket, ptr, sizeof(buffer)); 
                if (bytes_written < 0) {
                    printf("ERROR sending error message\n"); 
                    return;
                }
                bytes_read -= bytes_written;
                ptr += bytes_written;
            } 
            memset(buffer, '\0', sizeof(buffer));
        }

    }

    // send final transfer complete message
    success = write(socket, complete, sizeof(complete)); 
    if (success < 0) {
        printf("ERROR sending error message\n"); 
    }


}


int main(int argc, char *argv[]) {

    //used to differentiate command client sends  
    int cmd;

    //validate and get port number
    int port = get_port(argc, argv);

    struct Server *server; //init server struct
    server = malloc(sizeof(struct Server));

    //store port number
    server->port = port;

    // start server and wait on port for client request
    start_server(server); 


    // establish TCP control connection with client, waits for command from client
    while(1) {

        printf("Waiting for client request on port %d...\n", server->port);
        int comm_fd;
        //accept connection with client
        comm_fd = accept(server->sockfd, (struct sockaddr *) NULL, NULL);    

        if (comm_fd < 0) { //connection didn't work
            printf("Connection error with client\n");
        }
        else {  //connection worked

            //check which command was sent -g or -l
            cmd = validate_command(server, comm_fd);

            // get data port number from client
            server->data_port = get_data_port(comm_fd);
            //printf("%d\n", server->data_port);

            int data_fd; 
            //start a new socket for data transfer
            establish_data_connection(server);
            data_fd = accept(server->data_sockfd, (struct sockaddr *) NULL, NULL);    

            // -l was command
            if (cmd == 1) {
                printf("List directory requested on port %d\n", server->data_port);
                send_directory(data_fd);
            }

            // -g was command
            else if (cmd == 2) {
                // check if file is on server
                printf("File %s requested on port %d\n", server->filename, server->data_port);
                if (validate_filename(comm_fd, server->filename)) {
                    printf("Sending file '%s' on port %d\n", server->filename, server->data_port);
                    //TAG
                    send_file(data_fd, server->filename);
                } 
            }

            //close data client connection
            close(data_fd); 
            close(server->data_sockfd); 
        }

    }


    return 0;
}


