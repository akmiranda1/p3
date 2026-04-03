//Project 3 start moc code;
 * man pages, mostly getaddrinfo(3). */
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <string.h>
#include <unistd.h>
#include <math.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <errno.h>
#define BUF_SIZE 1200 // max buffer size to receive the data
#define FILE_BUFSIZE 1200
#define ACTION_SIZE 1
#define PEERID_SIZE 4
#define FILE_COUNT_SIZE 4
#define JOIN 0x0
#define PUBLISH 0x1
#define SEARCH 0x2
#define FETCH 0x3
/*
 Lookup a host IP address and connect to it using service. Arguments match the 
first two arguments
 to getaddrinfo(3). Returns a connected socket descriptor or -1 on error. Caller is 
responsible 
 for closing the returned socket.
*/
int lookup_and_connect(const char *host, const char *service);
// Used from Beej's Guide to Networking Chapter 7.4: Handling Partial send()s
// makes sure that all the byte are sent to the socket
int sendall(int s, char *buf, int *len);
//request number of bytes that are received and total bite count then returns the 
number of bytes recesived
int recvall(int s, char *buf, int len, int *byte_count);
// Takes file name strings and builds parsed packet to send
uint32_t directoryContents(char *files_in_dir, uint8_t *file_count);
// Takes the buffer received from the search reply to ouput item's peer id, ip 
address and port number;
void searchPeerResults(char *recvd_buffer);
int main(int argc, char *argv[]) {
char *host, *port;  
char *peerID;    
// Command line arguments
// The variables to store values from the user
char user_file_input[FILE_BUFSIZE];
char user_input[20]; 
// Stores user input for peer2peer actions
char action_value; // Single byte character 
uint32_t network_byte_order; // Used for byte order conversion
uint32_t payload[4]; 
int buf_length = 0; //Sets the buffer size before sending packet
int s; 
        //socket
/*Takes three user arguments ip address, port number,and Peer ID (positive number
  less than (2^32) -1)
*/
if (argc == 4) {
  host = argv[1];
  port = argv[2];
peerID = argv[3];
  if (atoi(peerID) < 0 || atoi(peerID) >  (pow(2,32)-1)) { // Peer ID must be 
a positive number less than (2^32) -1
fprintf( stderr, "%s is an invalid Peer ID\n", argv[1] );
  exit(1);
}
if (atoi(port) < 2000 || atoi(port) >  65536) { // Peer ID must be a 
positive number less than (2^32) -1
fprintf( stderr, "%s is an invalid port number\n", argv[2] );
  exit(1);
}
} else {
fprintf(stderr, "Too many or no arguments entered\n");
exit(1);
}
if ((s = lookup_and_connect(host, port)) < 0) {// Lookup IP and connect to 
server
exit(1);
}
/*Loops the server connection until the entire file has been received,
  the bytes received have been totaled, and the entire "<h1>" tag has been
  counted.
*/
while(1) {
char buf[BUF_SIZE] = {0}; 
receiving data
char filebuf[FILE_BUFSIZE] = {0};
// Storage for sending and 
// Take user input from the command terminal
printf("Enter a command: ");
fgets(user_input,sizeof(user_input),stdin);
/* If user selects "JOIN"
 *  a) send a JOIN Request to the registry
 */
if (strncmp("JOIN\n", user_input, 5) == 0) {
  action_value = JOIN;
*payload = strtol(peerID,NULL,10);
      buf_length = (ACTION_SIZE+PEERID_SIZE);
// Prepares the buffer to send action, peer id 
memcpy(buf, &action_value, sizeof(action_value));
    network_byte_order = htonl(*payload);
memcpy(buf+1, &network_byte_order, sizeof(network_byte_order));
  if(sendall(s, buf, &buf_length) < 0){
  perror("sendall failed");
  close(s);
  exit(1);
  }
}
/* If user selects "PUBLISH"
 *  send a PUBLISH request to the regisgtry
 */
else if (strncmp("PUBLISH\n",user_input,8)== 0) {
      uint32_t file_string_count;
uint8_t file_count[4] = {0};
action_value = PUBLISH;
file_string_count =  directoryContents(buf, file_count);
if (file_string_count > 0) {
char filebufout[file_string_count];
        memcpy(filebufout, buf, file_string_count);
// Prepares the buffer to send action, file count, list of 
file names
memcpy(buf, &action_value, sizeof(action_value));
      network_byte_order = htonl(*file_count);
memcpy(buf+1, &network_byte_order, sizeof(network_byte_order));
memcpy(buf+1+sizeof(network_byte_order), filebufout, 
file_string_count);
buf_length = (sizeof(action_value)+ sizeof(file_count)+ 
file_string_count);
    if(sendall(s, buf, &buf_length) < 0){
    perror("sendall failed");
    close(s);
    exit(1);
    }
} else {
printf("No files in SharedFiles direcory\n");
}
}
/* If user selects "SEARCH"
 *  a) read a file name from the terminal
 *  b) send a SEARCH request to the registry received packet must 
convert 
 *  c) print the peer info from the SEARCH response or a message that 
the file was not found
 */
else if (strncmp("SEARCH\n",user_input,7) == 0) {
// printf("%s", user_input);
char search_buf[10] = {0};
action_value = SEARCH;
  printf("Enter a command: "); //User input for file to search
  fgets(user_file_input,sizeof(user_file_input),stdin);
buf_length = (1+strlen(user_file_input));
  strncpy(filebuf, user_file_input, strlen(user_file_input)-1);//-1 
removes newline character
      filebuf[strlen(user_file_input)] = '\0';
// Prepares the buffer to send action, and file name to be 
searched on the registry
memcpy(buf, &action_value, sizeof(action_value));
memcpy(buf+1, &filebuf, strlen(user_file_input)+1); // +1 inludes 
null terminator
  if(sendall(s, buf, &buf_length) < 0){
    perror("sendall failed");
    close(s);
    exit(1);
  }
int nread = recv(s, search_buf, 10, 0);
    if(nread < 0){  //Error checker
perror("recv error");
close(s);
exit(1);
    }
    if(nread > 0){
// Printes Peer information on successful data received
        searchPeerResults(search_buf);
  }
}
//if the user selects "FETCH" we can just use the heart of the serch code for this one. 
else if (strncmp("FETCH\n", user_input, 6) ==0){
    char fetch_buf[10] = {0};
    char ip[16]
    unit32_t peer_id;
    unit16_t port;
    int peer_fd, n;
    int nread = recv(s, search_buf, 10, 0)
    FIlE*fp = fopen(filename, "wb");
    fwright(buffer, 1, nread, fp);

// read file from user
print ("enter a file: ");
if(fgets(user_file_input, sizeof(user_file_input), stdin));
user_file_input[strcspn(user_file_input, "\n")] = '\0'

//using SHEARCH
action_value = SEARCH;
buf_length = 1 + strlen(user_file_input) + 1;

memcopy(buf, &action_value, 1);
memcpy(buf, + 1, user_file_input, strlen(user_file_input) + 1);
sendall(s, buf, &buf_length);

recvall(s, search_reply, 10, &n);

if(!parse_search_reply(search_reply, &peer_id, ip, &port)){
  printf("File not found\n");
  return;
}

//we connect to peer
snprintf(port_str, sizeof(port_str),5000, port);
peer_fd = lookup_and_connect(ip, port_str);

//using fetch maybe?
action_value = FETCH;
buf_length = 1 + strlen(user_file_input) + 1;
memcopy(buf, &action_value, 1);
memcpy(buf, + 1, user_file_input, strlen(user_file_input) + 1);
sendall(s, buf, &buf_length);

}

 /*If user selects "EXIT"
* a) close the peer application.
  */
else if (strncmp("EXIT\n", user_input, 5) == 0) {
break;
} else {
printf("error invalid comands\n");
}
}
close(s); // Closes the socket once user exits
return 0;
}
// established a TCP connection to the given host and service.
int lookup_and_connect(const char *host, const char *service) {
struct addrinfo hints;
struct addrinfo *rp, *result;
int s;
// Translate host name into peer's IP address 
memset(&hints, 0, sizeof(hints));
hints.ai_family = AF_UNSPEC;
hints.ai_socktype = SOCK_STREAM;
hints.ai_flags = 0;
hints.ai_protocol = 0;
if ((s = getaddrinfo(host, service, &hints, &result)) != 0) {
fprintf(stderr, "stream-talk-client: getaddrinfo: %s\n", 
gai_strerror(s));
return -1;
}
// Iterate through the address list and try to connect 
for ( rp = result; rp != NULL; rp = rp->ai_next ) {
if ((s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1 ) {
}
continue;
}
if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
break;
}
close(s);
if (rp == NULL) {
perror("stream-talk-client: connect");
return -1;
}
freeaddrinfo(result);
return s;
}
// Code from Beej Guide to Networking
int sendall(int s, char *buf, int *len)
{
    int total = 0;        // how many bytes we've sent
    int bytesleft = *len; // how many we have left to send
    int n;
    while(total < *len) {
        n = send(s, buf+total, bytesleft, 0);
        if (n == -1) { break; }
        total += n;
        bytesleft -= n;
    }
    *len = total; // return number actually sent here
    return n==-1?-1:0; // return -1 on failure, 0 on success
} 
// Code from Beej Guide to Networking
/* This func was modified from the orignal Beej example, in which it called recv() 
one time thinking
 that it requested the all the bytes. In this version:
 1. the loops continues until the number of bytes (len) that was requested 
was received
    or recv returns 0
 2. it stores all the total number of bytes that was resived
 3. returns the number of bytes that was read from the buffer
*/
int recvall(int s, char *buf, int len, int *byte_count) {
  int total = 0;
while (total < len) {
int n = recv(s, buf + total, len - total, 0);
if (n < 0) {
return -1;
}
if (n == 0) {
break;
}
total +=n;
*byte_count += n;
}
 return total;
}
/*******************************************************************
 * Opens the peer's "SharedFiles" directory and writes the filenames
 * to a buffer and counts the number of files in the directory. Returns
 * the number of bytes (including null terminators) in the buffer,
 *
 */
uint32_t directoryContents(char *files_in_dir, uint8_t *file_count) {
  DIR *dirp;
  char files_to_publish[1200];
  char file_name[100];
  uint32_t file_size = 0;
  uint32_t tempfile_count = 0;
  struct dirent *test;
  dirp = opendir("./SharedFiles");
  if (dirp == NULL) {
    printf("Error: %s\n", strerror(errno));
    return 0;
  } else {
    errno = 0;
    while ( (test = readdir(dirp)) != NULL) {
        if (test->d_type != DT_DIR) {
          // printf("%s %ld\n",test->d_name, strlen(test->d_name));
          memcpy(&file_name, test->d_name, strlen(test->d_name)+1);
          memcpy(files_to_publish + file_size, file_name, strlen(test->d_name)+1);
          file_size += strlen(test->d_name)+1;
          tempfile_count ++;
        }
    };
    if (errno != 0) {
        printf("Error: %s\n", strerror(errno));
        return 0;
    }
  }
  closedir(dirp);
  memcpy(files_in_dir,files_to_publish, file_size);
  memcpy(file_count, &tempfile_count, 4);
}
  return file_size;
/**************************************************************************
 * Takes the raw bytes reveived from the registry, parses them in to print
 * out peer id, ip address and port number. If data received is all zeros
 * there is no information on the registry.
 */
void searchPeerResults(char *recvd_buffer) {
  struct sockaddr_in sa;
  char str[INET_ADDRSTRLEN];
uint32_t network_byte_order;
  uint8_t  search_buf[10] = {0};
  uint32_t search_peer_id;
  uint32_t search_port_num;
  memcpy(search_buf, recvd_buffer, 10);
  // Copies the raw bytes from received to packet to print
  // peer id, ip adress and port number
  memcpy(&network_byte_order, search_buf, 4);
  search_peer_id = ntohl(network_byte_order); //peer id
  memcpy(&(sa.sin_addr), search_buf+4, 4);    //ip address
  memcpy(&search_port_num, search_buf+8, 2);      //port number
  sa.sin_port = ntohs(search_port_num);
  // Implemented inet_ntop from Beejs Guide to Network Programming.
  inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);
  if (search_peer_id == 0 && sa.sin_port == 0 && strcmp(str, "0.0.0.0") == 0) {
printf("File not indexed by registry\n");
} else {
    printf("File found at\n Peer %u \n ", search_peer_id);
    printf("%s:%u\n", str, sa.sin_port); // prints "IPv4:[Port]"
	}

}