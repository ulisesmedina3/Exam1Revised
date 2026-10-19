#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>
#include <semaphore.h>



char* shm_name;
char* ptr = NULL;
int fd;

volatile sig_atomic_t cmd_create = 0;
volatile sig_atomic_t cmd_fill = 0;
volatile sig_atomic_t cmd_exit = 0;

void sig_handler(int signo) {
  if (signo == SIGUSR1) cmd_create = 1;
  if (signo == SIGINT) cmd_exit = 1;
}

int main(int argc, char* argv[]) {

  if (argc < 2) {
      printf("Encoder requires SHM name argument.\n");
      exit(1);
  }

  shm_name = argv[1];

  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);
  sigaction(SIGUSR2, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);

  signal(SIGUSR1, sig_handler);
  pause();
  while(!cmd_exit){

    if (cmd_create) {
        cmd_create = 0;
        printf("Decoder: Received SIGUSR1\n");
        fd = shm_open(shm_name, O_RDWR, 0666);
          if (fd == -1) {
          perror("[CHILD]:error on shm");
        }
        ptr = mmap(0, 50, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        


        for (int i = 0; i < 50; i++) {
          ptr[i] = (unsigned char)(((ptr[i] - 5) * 2));
          printf("[DECODED MESSAGE]: %s", ptr[i]);
        }

        kill(getppid(), SIGALRM);
      }
    if (cmd_exit) {
        cmd_exit = 0;
        printf("Decoder: Received SIGINT\n");
        shm_unlink(shm_name);
    }

  }

    return 0;
}