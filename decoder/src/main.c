
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
int fd = -1;

volatile sig_atomic_t cmd_create = 0;
volatile sig_atomic_t cmd_fill = 0;
volatile sig_atomic_t cmd_exit = 0;

void sig_handler(int signo) {
  if (signo == SIGUSR1) cmd_create = 1;
  if (signo == SIGINT) cmd_exit = 1;
}

int main(int argc, char* argv[]) {

  if (argc < 2) {
    printf("Decoder requires SHM name argument.\n");
    exit(1);
  }

  shm_name = argv[1];

  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);

  sigaction(SIGINT, &sa, NULL);

  while (!cmd_exit){
    if (!cmd_create && !cmd_exit) {
      pause();
    }

    if (cmd_create && !cmd_exit) {
      cmd_create = 0;

      printf("Decoder: Received SIGUSR1\n");

      fd = shm_open(shm_name, O_RDWR, 0666);

      if (fd == -1) {
        perror("[CHILD]: error on shm");
        exit(1);
      }

      ptr = mmap(0, 50, PROT_READ | PROT_WRITE,
                 MAP_SHARED, fd, 0);

      if (ptr == MAP_FAILED) {
        perror("Decoder: mmap failed");
        close(fd);
        exit(1);
      }

      for (int i = 0; i < 50 && ptr[i] != '\0'; i++) {
        ptr[i] = (unsigned char)(
            ((unsigned char)ptr[i] / 2) + 5);
      }

      printf("[DECODED MESSAGE]: %s\n", ptr);
      fflush(stdout);

      kill(getppid(), SIGALRM);
    }
  }


  printf("Decoder: Received SIGINT\n");

  if (ptr != NULL && ptr != MAP_FAILED) {
    munmap(ptr, 50);
  }

  if (fd != -1) {
    close(fd);
  }

  shm_unlink(shm_name);

  return 0;
}