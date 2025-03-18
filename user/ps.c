#include "kernel/types.h"
#include "kernel/stat.h"
#include "user.h"
#include "kernel/procinfo.h"


const char* procinfo_state_to_str(enum procinfo_state state) {
  switch (state) {
    case PROC_SLEEPING:
      return "Sleeping";
    case PROC_RUNNABLE:
      return "Runnable";
    case PROC_RUNNING:
      return "Running";
    case PROC_ZOMBIE:
      return "Zombie";
    default:
      return "Unknown";
  }
};

int main() {
  int ret;
  int bufsize = 16;
  struct procinfo *plist;
  while(1) {
    plist = malloc(sizeof(struct procinfo) * bufsize);
    if (plist == 0) {
      fprintf(1, "Ошибка выделения памяти");
      exit(1);
    }
    ret = ps_listinfo(plist, bufsize);
    if (ret == -1) {
      fprintf(1, "буфер размера %d слишком маленький. \n", bufsize);
      free(plist);
      bufsize *= 2;
      continue;
    } else if (ret == -2) {
       fprintf(1, "Некорректный адресс буфера = %d\n", ret);
       free(plist);
       exit(1);
    } break;
  }
  printf("PID\tNAME\t\tSTATE\t\tPPID\tPNAME\n");
  for (int i = 0; i < ret; i++) {
    printf("%d\t%s\t%s\t%d\t%s\n",
       plist[i].pid,
       plist[i].proc_name,
       procinfo_state_to_str(plist[i].state),
       plist[i].parent_pid,
       plist[i].parent_name);
  }

  free(plist);
  exit(0);
}