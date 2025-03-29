#define NAME_LEN 16

enum procinfo_state {
  PROC_SLEEPING,
  PROC_RUNNABLE,
  PROC_RUNNING,
  PROC_ZOMBIE,
};

struct procinfo {
  int pid;
  char proc_name[NAME_LEN];
  enum procinfo_state state;
  int parent_pid;
  char parent_name[NAME_LEN];
};