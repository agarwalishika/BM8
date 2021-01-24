
const char * usage =
"                                                               \n"
"daytime-server:                                                \n"
"                                                               \n"
"Simple server program that shows how to use socket calls       \n"
"in the server side.                                            \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   daytime-server <port>                                       \n"
"                                                               \n"
"Where 1024 < port < 65536.             \n"
"                                                               \n"
"In another window type:                                       \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where daytime-server  \n"
"is running. <port> is the port number you used when you run   \n"
"daytime-server.                                               \n"
"                                                               \n"
"Then type your name and return. You will get a greeting and   \n"
"the time of the day.                                          \n"
"                                                               \n";


#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

int QueueLength = 5;

// Processes time request
void processTimeRequest( int socket );

  int
main( int argc, char ** argv )
{
  // Print usage if not enough arguments
  if ( argc < 2 ) {
    fprintf( stderr, "%s", usage );
    exit( -1 );
  }

  // Get the port from the arguments
  int port = atoi( argv[1] );

  // Set the IP address and port for this server
  // this is setting one end of the connection
  struct sockaddr_in serverIPAddress; 
  memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
  serverIPAddress.sin_family = AF_INET;
  serverIPAddress.sin_addr.s_addr = INADDR_ANY;
  serverIPAddress.sin_port = htons((u_short) port);

  // Allocate a socket
  int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
  if ( masterSocket < 0) {
    perror("socket");
    exit( -1 );
  }

  // Set socket options to reuse port. Otherwise we will
  // have to wait about 2 minutes before reusing the sae port number
  int optval = 1; 
  int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
      (char *) &optval, sizeof( int ) );

  // Bind the socket to the IP address and port
  // if the ip address is in use, then bind will give you an error
  int error = bind( masterSocket,
      (struct sockaddr *)&serverIPAddress,
      sizeof(serverIPAddress) );
  if ( error ) {
    perror("bind");
    exit( -1 );
  }

  // Put socket in listening mode and set the 
  // size of the queue of unprocessed connections
  error = listen( masterSocket, QueueLength);
  if ( error ) {
    perror("listen");
    exit( -1 );
  }

  //the server runs forever
  while ( 1 ) {

    // Accept incoming connections
    struct sockaddr_in clientIPAddress;
    int alen = sizeof( clientIPAddress );
    //accept will block until a connection (input) arrives
    //this is where the server will stop
    int slaveSocket = accept( masterSocket,
        (struct sockaddr *)&clientIPAddress,
        (socklen_t*)&alen);

    //masterSocket and slaveSocket are both file descriptors
    //the slaveSocket is a pipe, you are sending data from one
    //side and the masterSocket is reading it

    //masterSocket recieves the incoming data -> server
    //slaveSocket is for writing data to the masterSocket -> client


    if ( slaveSocket < 0 ) {
      perror( "accept" );
      exit( -1 );
    }

    // Process request.
    processTimeRequest( slaveSocket );

    // Close socket
    //this will close the client stuff and will print out "connection closed"
    close( slaveSocket );
  }

}

  void
processTimeRequest( int fd )
{
  //fd is actually outputting to client, so really client_fd
  // Buffer used to store the name received from the client
  const int MaxName = 1024;
  char name[ MaxName + 1 ];
  int nameLength = 0;
  int n;

  // Send prompt to the client telnet thing
  const char * prompt = "\nType your name:";
  write( fd, prompt, strlen( prompt ) );

  // Currently character read
  unsigned char newChar;

  // Last character read
  unsigned char lastChar = 0;

  //
  // The client should send <name><cr><lf>
  // Read the name of the client character by character until a
  // <CR><LF> is found.
  //

  //reading character by character
  while ( nameLength < MaxName &&
      ( n = read( fd, &newChar, sizeof(newChar) ) ) > 0 ) {

    //once I type enter, then it will break here, thats why this loop ends
    // reads the \r\n
    if ( lastChar == '\015' && newChar == '\012' ) {
      // Discard previous <CR> from name
      nameLength--;
      break;
    }

    name[ nameLength ] = newChar;
    nameLength++;

    lastChar = newChar;
  }

  // Add null character at the end of the string
  name[ nameLength ] = 0;

  //print to the current server stdout
  printf( "name=%s\n", name );

  // Get time of day
  time_t now;
  time(&now);
  char	*timeString = ctime(&now);

  // Send name and greetings to the client screen
  const char * hi = "\nHi ";
  const char * timeIs = " the time is:\n";
  write( fd, hi, strlen( hi ) );
  write( fd, name, strlen( name ) );
  write( fd, timeIs, strlen( timeIs ) );

  // Send the time of day 
  write(fd, timeString, strlen(timeString));

  // Send last newline
  const char * newline="\n";
  write(fd, newline, strlen(newline));

}
