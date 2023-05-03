#include "llvm_mode/Currency-instr.h"
#include <cstring>
#include <sys/ipc.h>
#include <sys/types.h>

static struct Thread_info *t_info;

void adjust_t_info(struct Thread_info *t_info);
void adjust_t_info1(struct Thread_info *t_info);


int main() {
  int shmid_info;
  key_t key_info = ftok("/tmp", 111);
  if ((shmid_info = shmget(key_info, sizeof(struct Thread_info),
                           IPC_CREAT | 0666)) < 0) {
    fprintf(stderr, "Error: getting shared memory key_psie id: 1324\n");
  }
  if ((struct Thread_info *)-1 ==
      (t_info = (struct Thread_info *)shmat(shmid_info, NULL, 0))) {
    fprintf(stderr, "Error: attaching shared memory id.\n");
    exit(1);
  }
}

void adjust_t_info(struct Thread_info *t_info) {
  memset(t_info, 0, sizeof(struct Thread_info));
  
}