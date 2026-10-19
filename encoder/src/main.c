#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <time.h>
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
  if (signo == SIGUSR2) cmd_fill = 1;
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

  srand(time(NULL) ^ getpid());
  unsigned char* messages[] = {"OPERATING", "SYSTEMS", "PROCESSES", "MEMORY",
                               "SIGNALS"};
  unsigned char* selected_msg = messages[rand() % 5];


      

  while (!cmd_exit) {
    signal(SIGUSR1, sig_handler);
    signal(SIGUSR2, sig_handler);
    
    pause();
    if (cmd_create) {
      usleep(500000);
      cmd_create = 0;
      fd = shm_open(shm_name, O_CREAT | O_RDWR, 0666);
      ftruncate(fd, 50);
      ptr = mmap(0, 50, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    }
    if (cmd_fill) {
      cmd_fill = 0;
      printf("Encoder: Received SIGUSR2\n");
      fflush(stdout);
      if (ptr != MAP_FAILED && ptr != NULL) {
        memset(ptr, 0, 50);
        for (int i = 0; selected_msg[i] != '\0'; i++) {
          ptr[i] = (unsigned char)(((selected_msg[i] - 5) * 2));
        }
        kill(getppid(), SIGALRM);
      }
    }
  }
  if (ptr != NULL && ptr != MAP_FAILED) munmap(ptr, 50);
  return 0;
}