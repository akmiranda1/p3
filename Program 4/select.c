#include <stdio.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/types.h>
#include <string.h>
#include <netdb.h>

#define MAX_LINE 256
int main() {
fd_set fdset;
int sel_rtn;
char buf[MAX_LINE];
FD_CLR(STDIN_FILENO, &fdset);
FD_SET(STDIN_FILENO, &fdset);
struct timeval tv;


tv.tv_sec = 5;
tv.tv_usec = 0;

while (1) {
  sel_rtn = select(STDIN_FILENO+1, &fdset,NULL,NULL, &tv);
  if (FD_ISSET(STDIN_FILENO,&fdset)) {
    fgets(buf, sizeof(buf), stdin);
    printf("%s", buf);
    printf("Select return value: %d, Timeval:%ld.%ld", sel_rtn, tv.tv_sec, tv.tv_usec);
    printf(" STDIN_FILENO is in the file descriptor set\n");
    tv.tv_sec = 5;
    tv.tv_usec = 0;
  } else {
    printf("Select return value: %d, Timeval:%ld.%ld", sel_rtn, tv.tv_sec, tv.tv_usec);
    printf(" STDIN_FILENO is NOT in the file descriptor set\n");
    break;
  }
}
  return 0;
}