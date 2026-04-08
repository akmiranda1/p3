/* Project 2 - Peer 2 Peer v1.0
 * Developed by Alma Miranda-Rodriguez, Thomas Orozco
 * 8 Mar 2026
 * Description: Establishes a peer2peer connection with a file registry with 
 * host name or ip address and port number. User selects a peer id to connect with 
 * registry. Once in the registry the user can do the following:
 * 	1) JOIN the registry as a peer (must do so prior to searching or publishin)
 *  2) PUBLISH the names of files in the peer's SharedFiles directory 
 *  3) SEARCH which peer has a file chosen by the user
 */

/* This code is an updated version of the sample code from "Computer Networks: A Systems
 * Approach," 5th Edition by Larry L. Peterson and Bruce S. Davis. Some code comes from
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
#include <fcntl.h>

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
 Lookup a host IP address and connect to it using service. Arguments match the first two arguments
 to getaddrinfo(3). Returns a connected socket descriptor or -1 on error. Caller is responsible 
 for closing the returned socket.
*/

int lookup_and_connect(const char *host, const char *service);

// Used from Beej's Guide to Networking Chapter 7.4: Handling Partial send()s

// makes sure that all the byte are sent to the socket
int sendall(int s, char *buf, int *len);

//request number of bytes that are received and total bite count then returns the number of bytes recesived
int recvall(int s, char *buf, int len);

// Takes file name strings and builds parsed packet to send
uint32_t directoryContents(char *files_in_dir, uint8_t *file_count);

// Takes the buffer received from the search reply to ouput item's peer id, ip address and port number;
struct sockaddr_in searchPeerResults(char *recvd_buffer, uint32_t *peer_id);

int main(int argc, char *argv[]) {
	char *host, *port;  		// Command line arguments
	char *peerID;    		// The variables to store values from the user
	char user_file_input[FILE_BUFSIZE];
	char user_input[20]; 	// Stores user input for peer2peer actions
	char action_value; // Single byte character 

	uint32_t network_byte_order; // Used for byte order conversion
	uint32_t payload[4]; 
	int buf_length = 0; //Sets the buffer size before sending packet
	int s; 					        //socket

/*Takes three user arguments ip address, port number,and Peer ID (positive number
  less than (2^32) -1)
*/
	if (argc == 4) {
	  host = argv[1];
	  port = argv[2];
		peerID = argv[3];
	  if (atoi(peerID) < 0 || atoi(peerID) >  (pow(2,32)-1)) { // Peer ID must be a positive number less than (2^32) -1
			fprintf( stderr, "%s is an invalid Peer ID\n", argv[1] );
		  exit(1);
		}
		if (atoi(port) < 2000 || atoi(port) >  65536) { // Peer ID must be a positive number less than (2^32) -1
			fprintf( stderr, "%s is an invalid port number\n", argv[2] );
		  exit(1);
		}
	} else {
		fprintf(stderr, "Too many or no arguments entered\n");
		exit(1);
	}

	if ((s = lookup_and_connect(host, port)) < 0) {	// Lookup IP and connect to server
		exit(1);
	}

/*Loops the server connection until the entire file has been received,
  the bytes received have been totaled, and the entire "<h1>" tag has been
  counted.
*/
	while(1) {
		char buf[BUF_SIZE] = {0}; 		// Storage for sending and receiving data
		char filebuf[FILE_BUFSIZE] = {0};
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
  
				// Prepares the buffer to send action, file count, list of file names
	  		memcpy(buf, &action_value, sizeof(action_value));
	      network_byte_order = htonl(*file_count);
	  		memcpy(buf+1, &network_byte_order, sizeof(network_byte_order));
	  		memcpy(buf+1+sizeof(network_byte_order), filebufout, file_string_count);
	  		buf_length = (sizeof(action_value)+ sizeof(file_count)+ file_string_count);
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
		 *  b) send a SEARCH request to the registry received packet must convert 
		 *  c) print the peer info from the SEARCH response or a message that the file was not found
		 */
		else if (strncmp("SEARCH\n",user_input,7) == 0) {
			char search_buf[10] = {0};
			struct sockaddr_in search_sa;
			char str[INET_ADDRSTRLEN];
			uint32_t search_peer_id = 0;

			action_value = SEARCH;

		  printf("Enter a command: "); //User input for file to search
		  fgets(user_file_input,sizeof(user_file_input),stdin);
			buf_length = (1+strlen(user_file_input));

		  strncpy(filebuf, user_file_input, strlen(user_file_input)-1);//-1 removes newline character
      filebuf[strlen(user_file_input)] = '\0';

			// Prepares the buffer to send action, and file name to be searched on the registry
			memcpy(buf, &action_value, sizeof(action_value));
			memcpy(buf+1, &filebuf, strlen(user_file_input)+1); // +1 inludes null terminator
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
	    	// Prints Peer information on successful data received
        search_sa = searchPeerResults(search_buf, &search_peer_id);
  			
				// Implemented inet_ntop from Beejs Guide to Network Programming.
  			
				inet_ntop(AF_INET, &(search_sa.sin_addr), str, INET_ADDRSTRLEN);

  			if (search_peer_id == 0 && search_sa.sin_port == 0 && strcmp(str, "0.0.0.0") == 0) {
					printf("File not indexed by registry\n");
				} else {
  			  printf("File found at\n Peer %u \n ", search_peer_id);
  			  printf("%s:%u\n", str, search_sa.sin_port); // prints "IPv4:[Port]"
				}

		  }
		}
		/* If user selects "FETCH"
		 *  a) read a ﬁle name from the terminal,
     *  b) send a SEARCH request to the registry for the ﬁle,
     *  c) receive the peer information from the registry,
     *  d) send a FETCH request to the identiﬁed peer,
     *  e) receive and save the ﬁle information
		 */
		else if (strncmp("FETCH\n",user_input,7) == 0) {
			// printf("%s", user_input);
			struct sockaddr_in search_sa;
			char search_buf[10] = {0};
			char addr_str[INET_ADDRSTRLEN];
			char prt_str[32];
			uint32_t search_peer_id = 0;

			action_value = SEARCH;

		  printf("Enter a command: "); //User input for file to search
		  fgets(user_file_input,sizeof(user_file_input),stdin);
			buf_length = (1+strlen(user_file_input));

		  strncpy(filebuf, user_file_input, strlen(user_file_input)-1);//-1 removes newline character
      filebuf[strlen(user_file_input)] = '\0';

			// Prepares the buffer to send action, and file name to be searched on the registry
			memcpy(buf, &action_value, sizeof(action_value));
			memcpy(buf+1, &filebuf, strlen(user_file_input)+1); // +1 inludes null terminator
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
	    	// Prints Peer information on successful data received
        search_sa = searchPeerResults(search_buf, &search_peer_id);
  			
				// Implemented inet_ntop from Beejs Guide to Network Programming.
  			
				inet_ntop(AF_INET, &(search_sa.sin_addr), addr_str, INET_ADDRSTRLEN);

				if (search_peer_id == 0 && search_sa.sin_port == 0 && strcmp(addr_str, "0.0.0.0") == 0) {
					printf("File not indexed by registry\n");
				} else {
  			  // There is a valid file and a connection to the identified peer is establish
					sprintf(prt_str, "%u" ,search_sa.sin_port);
					int peer_s = lookup_and_connect(addr_str,prt_str);

					// Uses the same buffer from the search but changes the action to fetch
					action_value = FETCH;
					memcpy(buf, &action_value, 1);
					
					// Sends fetch request to the peer
					sendall(peer_s, buf, &buf_length);
					
					recv(peer_s, &buf, 1, 0); // Receive first byte used to determine if file was sent
					size_t n_write = 0;
					if (buf[0] == 0) {
						// char file_name[strlen(user_file_input)-1];
						// memcpy(file_name, user_file_input, sizeof(file_name));
						// Open file here
						FILE *fd = fopen(user_file_input, "w+");
						
					  while(1) {
					    char fetch_buf[4] = {'/0'}; // Reinitializes the buffer as empty to recv
							int n = recv(peer_s, &fetch_buf, 4, 0);

					    if (n == 0) {
					    	break;
					    } else if (n < 0){
								perror("Error receiving data");
								return EXIT_FAILURE;
							}	else {
								// This needs to be changed to create a file with the searched filename
								// Then buffer is then used to write to the file.
								  n_write += fwrite(fetch_buf, sizeof(char), sizeof(fetch_buf), fd);
					    	// printf("%s", fetch_buf);
					    	// fflush(stdout);
					    }
					  }
						
						fclose(fd);
						// Close file here
					}
					close(peer_s); // Closes peer socket when file all entire file is received.
				}
			}
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

 	close(s);	// Closes the socket once user exits
	
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
		fprintf(stderr, "stream-talk-client: getaddrinfo: %s\n", gai_strerror(s));
		return -1;
	}

	// Iterate through the address list and try to connect 
	for ( rp = result; rp != NULL; rp = rp->ai_next ) {
		if ((s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol)) == -1 ) {
			continue;
		}

		if (connect(s, rp->ai_addr, rp->ai_addrlen) != -1) {
			break;
		}

		close(s);
	}
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
/* This func was modified from the orignal Beej example, in which it called recv() one time thinking
	 that it requested the all the bytes. In this version:
	 1. the loops continues until the number of bytes (len) that was requested was received
	    or recv returns 0
	 2. it stores all the total number of bytes that was resived
	 3. returns the number of bytes that was read from the buffer
*/

int recvall(int s, char *buf, int len) {
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
		// *byte_count += n;
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
  return file_size;
}

/**************************************************************************
 * Takes the raw bytes reveived from the registry, parses them in to print
 * out peer id, ip address and port number. If data received is all zeros
 * there is no information on the registry.
 */

struct sockaddr_in searchPeerResults(char *recvd_buffer, uint32_t *peer_id) {
  struct sockaddr_in sa;
  // char str[INET_ADDRSTRLEN];

 	uint32_t network_byte_order;
  uint8_t  search_buf[10] = {0};
  uint32_t search_peer_id;
  uint32_t search_port_num;

  memcpy(search_buf, recvd_buffer, 10);

  // Copies the raw bytes from received to packet to print
  // peer id, ip adress and port number
  memcpy(&network_byte_order, search_buf, 4);
  search_peer_id = ntohl(network_byte_order); //peer id
	*peer_id = search_peer_id;

  memcpy(&(sa.sin_addr), search_buf+4, 4);    //ip address

  memcpy(&search_port_num, search_buf+8, 2);  //port number
  sa.sin_port = ntohs(search_port_num);

  // // Implemented inet_ntop from Beejs Guide to Network Programming.
  // inet_ntop(AF_INET, &(sa.sin_addr), str, INET_ADDRSTRLEN);

  // if (search_peer_id == 0 && sa.sin_port == 0 && strcmp(str, "0.0.0.0") == 0) {
	// 	printf("File not indexed by registry\n");
	// } else {
  //   printf("File found at\n Peer %u \n ", search_peer_id);
  //   printf("%s:%u\n", str, sa.sin_port); // prints "IPv4:[Port]"
	// }
	return sa;
}

