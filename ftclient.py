# Author: James Whiteley IV
# References:
# https://docs.python.org/2/howto/sockets.html
# https://stackoverflow.com/questions/7749341/very-basic-python-client-socket-example
# https://programminghistorian.org/lessons/working-with-text-files
# Reused some code from previous Project 1
import socket
import sys
import time 
import os.path

#show user the proper format, exit program
def usage():
   print "USAGE: python ftclient.py <server host> <server port> -l | -g <file name> <data port>"
   print "Valid port numbers in range 1024 -> 65535"
   print "<file name> must be of .txt extension"
   sys.exit(1)


def validate_args(args):
    ''' validates and returns formatted arguments '''
     
    # make sure correct command line args used
    if len(args) not in (5,6): 
        usage()
   
    host = args[1]
    serv_port = int(args[2])
    command = args[3]
    data_port = int(args[-1])
    filename = "" 

    # check valid .txt filename
    if len(args) is 6:
        if ".txt" not in args[4]:
            usage()
        else:
            filename = args[4]

    # check server port num
    if serv_port not in range(1024, 65536):
        usage()

    # check valid command 
    if command not in ("-l", "-g"):
        usage()

    # check data port num
    if data_port not in range(1024, 65536):
        usage()

    # make sure server and data port are different
    if int(args[-1]) == int(args[2]):
        usage()

    return host, serv_port, command, data_port, filename

          
def connect(host, serv_port):
      ''' connect to server on port, returns fd '''
      client_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM) #create socket
      client_socket.connect((host, serv_port))
      return client_socket

def recv_directory(socket, host, data_port):  
    ''' reads directory contents from server and displays on screen '''
    
    print "Receiving directory structure from {}:{}".format(host, data_port)
    status = socket.recv(100)
    while status != "complete\0":
        print status
        status = socket.recv(100)


def recv_file(socket, data_socket, host, data_port, filename):
    '''
        reads contents of file from server and saves as file in current directory.        
        If file already exists, print "duplicate file name".
        After transfer completes successfully print "transfer complete".
    '''

    status = socket.recv(100)
    if status == "File not found":
        print "{}:{} says File not found".format(host, data_port)
    else:
        print "Receiving '{}' from {}:{}".format(filename, host, data_port)

        #check if file already exits
        if os.path.isfile(filename): 
            print "{} already exits, would you like to overwrite it?"
            choice = raw_input("y/n: ")
            if choice != 'y':
                return #return, don't do anything

        #open file for writing
        f = open(filename, 'w') 
        status = data_socket.recv(1000)
        while "__complete__" not in status:
            f.write(status)
            status = data_socket.recv(1000)
        print "File Transfer Complete"
        f.close()





if __name__ == "__main__":

  # get and store validated command line args
  host, serv_port, command, data_port, filename = validate_args(sys.argv)

  # connect to server
  sock = connect(host, serv_port)

  # send command and filename
  sock.send(command + filename)
  status = sock.recv(100)

  # send data_port number
  if status == "OK":
    sock.send(str(data_port))
  else:
    sock.close()
    exit(1)

  #time.sleep(2) # wait for server to start socket on data port
  data_sock = connect(host, data_port)

  # either recv directory or recv file
  if command == '-l':
    recv_directory(data_sock, host, data_port)
  elif command == '-g':
    recv_file(sock, data_sock, host, data_port, filename)
    
  data_sock.close()
  sock.close()

