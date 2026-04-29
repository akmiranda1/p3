#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>
#include <stdbool.h>
#include <stdlib.h>

char *itoa(int, char*, int);



int main() {
  // char string[10] = {'h','e','l','l','o'};
  uint32_t string = 0x0061;

  uint32_t count = 0;
  // char string[10] = {'a'};

  char string2[256] = {0};
  int network_byte_order;

  char *test;
  

  sprintf(string2, "%d", 11235678);

  test = (char*)string2; 


    // network_byte_order = htonl(string);
    // string2 = *network_byte_order;
    // memcpy(string2+count, &network_byte_order, sizeof(network_byte_order));
  // for (count = 0 ; count < strlen(string); count++) {
  //   network_byte_order = htonl(*(string+count));
  //   memcpy(string2+count, &network_byte_order, sizeof(network_byte_order));
  // }


  printf ("%s", string2);

  return 0;
}