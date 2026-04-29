/* Project 4 - Registry Server Peer 2 Peer v1.0
 * Developed by Alma Miranda-Rodriguez, Thomas Orozco
 * 12 Apr 2026
 * Description: Establishes a peer2peer connection with a file registry with 
 * host name or ip address and port number. User selects a peer id to connect with 
 * registry. Once in the registry the user can do the following:
 *  1) JOIN the registry as a peer (must do so prior to searching or publishin)
 *  2) PUBLISH the names of files in the peer's SharedFiles directory 
 *  3) SEARCH which peer has a file chosen by the user
 */

#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>
#include <netinet/in.h>
#include <arpa/inet.h>


#define LOCALHOST "127.0.0.1"
#define MAX_LINE 1256
#define MAX_PENDING 5
#define JOIN 0x0
#define PUBLISH 0x1
#define SEARCH 0x2
#define MAX_FILES 10
#define MAX_FILENAME_LEN 100
#define PEERPORT 2
#define FILECOUNT 3


struct peer_entry {
  uint32_t id; // ID of peer
  int socket_descriptor; // Socket descriptor for connection to peer
  char files[MAX_FILES][MAX_FILENAME_LEN]; // Files published by peer
  struct sockaddr_in address; // Contains IP address and port number
};

// Implemented bind_and_listen() and find_max_fd() from lab 9
/*
 * Create, bind and passive open a socket on a local interface for the provided service.
 * Argument matches the second argument to getaddrinfo(3).
 *
 * Returns a passively opened socket or -1 on error. Caller is responsible for calling
 * accept and closing the socket.
 */
int bind_and_listen( const char *service );

/*
 * Return the maximum socket descriptor set in the argument.
 * This is a helper function that might be useful to you.
 */
int find_max_fd(const fd_set *fs);

// Removes all data from the peer struct;
void zero_peer(struct peer_entry *empty_peer);

int main(int argc, char* argv[]) {
  struct peer_entry peer_db[MAX_PENDING];
  char* reg_port;
  bool verbose = false;
  char wrk_buf[MAX_LINE];
	int s_accept = 0;

  //initialize peer database to empty 
  for (int i = 0; i < MAX_PENDING; i++){
    zero_peer(&peer_db[i]);
  }

  //Parses program arguments
  if (argc > 3 && argc < 2 ) {
    fprintf(stderr,"Insufficent arguments\n");
    exit(1);
  } else if (argc == 3) {
      if (strcmp(argv[2], "-t") == 0) {
      verbose = true;
    } else if (strcmp(argv[2], "-t") != 0 || strcmp(argv[2], "") == 0) { 
      fprintf(stderr,"Invalid argument, use -t for testing output\n");
      exit(1);
    }
  }

  if (argc > 1 && (atoi(argv[1]) < 2000 || atoi(argv[1]) > 65536)) {
    fprintf(stderr,"Invalid port range: must be inclusive number 2000 through 65536\n");
    exit(1);
  } else {
    reg_port = argv[1];
  }
  
  /*Initializes empty working and master FD_SETs*/
  fd_set mstrcpy_sockets; //Sockets FD SET master copy
  FD_ZERO(&mstrcpy_sockets); // Zeros FD SET list

  fd_set wrkcpy_sockets; //Sockets FD SET working copy
  FD_ZERO(&wrkcpy_sockets); // Zeros FD SET list
  
  // Initializing fdset with the listening socket assigning it as max socket
  int listen_socket = bind_and_listen(reg_port);
  FD_SET(listen_socket, &mstrcpy_sockets);
  int max_socket = listen_socket;

  /*Keeps server active until program closes or error occurs*/
  while (1) {
    
    wrkcpy_sockets = mstrcpy_sockets;
    int sel_num = select(max_socket+1, &wrkcpy_sockets, NULL, NULL, NULL);

    if (sel_num < 0) {
      perror("ERROR in select call");
      exit(1);
    }

    for (int s = 3; s <= max_socket; ++s) {

      if(!FD_ISSET(s, &wrkcpy_sockets)) {
        continue;
      }

      if (s == listen_socket) {
        s_accept = accept(s, NULL, NULL);
        FD_SET(s_accept, &mstrcpy_sockets);
        max_socket = find_max_fd(&mstrcpy_sockets);
      } else {
          uint8_t action_byte;
          uint32_t peer_bytes;
          uint32_t file_count;
          uint32_t network_byte_l;
          // uint16_t network_byte_s;
          char file_list[MAX_FILES*MAX_FILENAME_LEN] = {0};
          char addr_str[INET_ADDRSTRLEN];
          struct sockaddr_in addr;
          socklen_t len = sizeof(addr);    
          int n_rcvd = 0;
          int valid_peer;
          bool peer_joined = false;
          

          int ret = getpeername(s, (struct sockaddr*)&addr, &len);
          inet_ntop(AF_INET, &(addr.sin_addr), addr_str, INET_ADDRSTRLEN);

          n_rcvd =  recv(s, wrk_buf, sizeof(wrk_buf), 0);

          if (n_rcvd < 0) {
            close(s);
            perror("ERROR in recv() call");
			      return -1;  

          } else if (n_rcvd > 0) {
                
              memcpy(&action_byte, wrk_buf, sizeof(action_byte));
              // network_byte_s = ntohs(action_byte);
              // action_byte = network_byte_s;


              /* First byte determines what action the server will take*/
              if (action_byte == JOIN) {

                int empty_db_slot = MAX_PENDING + 1;
                bool peer_joined = false;
                memcpy(&peer_bytes, wrk_buf+sizeof(action_byte), sizeof(peer_bytes));
                network_byte_l = ntohl(peer_bytes);
                peer_bytes = network_byte_l;
            
                for (int i = 0; i < MAX_PENDING; i++){
                  if (peer_db[i].id == -1 && empty_db_slot > MAX_PENDING) {
                    empty_db_slot = i;
                  }
                  if (peer_db[i].id == peer_bytes) {
                    peer_joined = true;
                  }
                }

                if (peer_joined == false) {
                  peer_db[empty_db_slot].id = peer_bytes;
                  peer_db[empty_db_slot].socket_descriptor = s;
                  peer_db[empty_db_slot].address = addr;
                }
          
                if (verbose == true) {
                  if (peer_joined == true) {
                    printf("Peer %d cannot JOIN again\n", peer_bytes);
                    // fflush(stdout);
                  } else {
                    printf("TEST] JOIN %d\n", peer_bytes);
                    // printf("Peer %d has joined\n%s:%d", peer_bytes, addr_str, addr.sin_port);
                    fflush(stdout);
                  }
                
                }
          } else if (action_byte == PUBLISH) {
              char temp_addr_str[INET_ADDRSTRLEN];
              memcpy(&file_count, wrk_buf+sizeof(action_byte), sizeof(file_count));
              network_byte_l = ntohl(file_count);
              file_count = network_byte_l;


              for (int i = 0; i < MAX_PENDING; i++){
                inet_ntop(AF_INET, &(peer_db[i].address.sin_addr), temp_addr_str, INET_ADDRSTRLEN);
                if (peer_db[i].socket_descriptor == s && (strcmp(temp_addr_str,addr_str) == 0) && (peer_db[i].address.sin_port == addr.sin_port) ) {
                  peer_joined = true;
                  valid_peer = i;
                  break;
                }
              }

              if (peer_joined == true){
                memcpy(&file_list, wrk_buf + sizeof(action_byte) + sizeof(file_count), sizeof(file_list));
                int char_count_start = 0;
                int temp_file_counter = 0;
                for (int k = 0; k < sizeof(file_list); k ++) {
                  if (file_list[k] == '\0') {
                    strcpy(peer_db[valid_peer].files[temp_file_counter], file_list+char_count_start);
                    temp_file_counter++;
                    char_count_start = k+1;
                  }

                  if (temp_file_counter == file_count) {
                    break;
                  }
                }
                if (verbose == true) {
                  printf("TEST] PUBLISH %d", file_count);
                  fflush(stdout);
                  for (int i = 0; i < file_count; i++) {
                    printf(" %s", peer_db[valid_peer].files[i]);
                    fflush(stdout);
                  }
                  printf("\n");
                  fflush(stdout);
                }
              }
          } else if (action_byte == SEARCH) {

            // if (verbose == true);
          }     
            
          } else if (n_rcvd == 0) {
            char temp_addr_str[INET_ADDRSTRLEN];
            for (int i = 0; i < MAX_PENDING; i++){
                inet_ntop(AF_INET, &(peer_db[i].address.sin_addr), temp_addr_str, INET_ADDRSTRLEN);
                if (peer_db[i].socket_descriptor == s && (strcmp(temp_addr_str,addr_str) == 0) && (peer_db[i].address.sin_port == addr.sin_port) ) {
                  peer_joined = true;
                  valid_peer = i;
                  break;
                }
              }

            zero_peer(&peer_db[valid_peer]);
            close(s);
            FD_CLR(s, &mstrcpy_sockets);
          }             
      }
    }
  }
  return 0;
}

int find_max_fd(const fd_set *fs) {
	int ret = 0;
	for(int i = FD_SETSIZE-1; i>=0 && ret==0; --i){
		if( FD_ISSET(i, fs) ){
			ret = i;
		}
	}
	return ret;
}


int bind_and_listen( const char *service ) {
	struct addrinfo hints;
	struct addrinfo *rp, *result;
	int s;

	/* Build address data structure */
	memset( &hints, 0, sizeof( struct addrinfo ) );
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_flags = AI_PASSIVE;
	hints.ai_protocol = 0;

	/* Get local address info */
	if ( ( s = getaddrinfo( NULL, service, &hints, &result ) ) != 0 ) {
		fprintf( stderr, "stream-talk-server: getaddrinfo: %s\n", gai_strerror( s ) );
		return -1;
	}

	/* Iterate through the address list and try to perform passive open */
	for ( rp = result; rp != NULL; rp = rp->ai_next ) {
		if ( ( s = socket( rp->ai_family, rp->ai_socktype, rp->ai_protocol ) ) == -1 ) {
			continue;
		}

		if ( !bind( s, rp->ai_addr, rp->ai_addrlen ) ) {
			break;
		}

		close( s );
	}
	if ( rp == NULL ) {
		perror( "stream-talk-server: bind" );
		return -1;
	}
	if ( listen( s, MAX_PENDING ) == -1 ) {
		perror( "stream-talk-server: listen" );
		close( s );
		return -1;
	}
	freeaddrinfo( result );

	return s;
}

void zero_peer(struct peer_entry *empty_peer) {
  empty_peer->id = -1;
  empty_peer->socket_descriptor = -1;
  inet_pton(AF_INET, "0.0.0.0", &empty_peer->address.sin_addr);
  char erase_file_name[MAX_FILENAME_LEN] = {0};
  for (int i = 0; i < MAX_FILES; i++) {
    memcpy(&empty_peer->files[i],erase_file_name, sizeof(erase_file_name));
  }
}
