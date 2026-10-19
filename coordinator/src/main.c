#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <unistd.h>

volatile sig_atomic_t cmd_create = 0;
volatile sig_atomic_t cmd_fill = 0;
volatile sig_atomic_t cmd_exit = 0;
volatile sig_atomic_t cmd_alrm = 0;

void sig_handler(int signo) {
  if (signo == SIGUSR1) cmd_create = 1;
  if (signo == SIGUSR2) cmd_fill = 1;
  if (signo == SIGINT) cmd_exit = 1;
  if (signo == SIGALRM) cmd_alrm = 1;
}

int main(int argc, char* argv[]) {
  if (argc < 2) {
    printf("Coordinator requires SHM name argument.\n");
    exit(1);
  }

  char* shm_name = argv[1];

  struct sigaction sa;
  sa.sa_handler = sig_handler;
  sigemptyset(&sa.sa_mask);
  sa.sa_flags = 0;
  sigaction(SIGUSR1, &sa, NULL);
  sigaction(SIGUSR2, &sa, NULL);
  sigaction(SIGINT, &sa, NULL);
  sigaction(SIGALRM, &sa, NULL);


  pid_t child1_pid = fork();

  if (child1_pid == 0) {
    int error = execlp("../../decoder/build/decoder", "decoder", argv[1], NULL);
    if (error == -1) {
      perror("error lanzando decoder:");
      exit(1);
    }
  }

  pid_t child2_pid = fork();

  if (child2_pid == 0) {
    int error = execlp("../../encoder/build/encoder", "encoder", argv[1], NULL);
    if (error == -1) {
      perror("error lanzando encoder:");
      exit(1);
    }
  }

  sleep(1);
  kill(child2_pid, SIGUSR1);
  kill(child2_pid, SIGUSR2);
  pause();

  if(cmd_alrm){
    cmd_alrm = 0;
    kill(child1_pid, SIGUSR1);
  }

  pause();
  if(cmd_alrm){
    cmd_alrm = 0;
    kill(child1_pid, SIGINT);
    kill(child2_pid, SIGINT);
  }

  wait(NULL);
  wait(NULL);
 
  return 0;
}