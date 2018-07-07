
//****************
// TASK
//****************
typedef struct task {
  void (*execute)(void);
  int16_t millis_between;
  int32_t millis_last_executed;
  bool paused;
} Task;

#define TASK_PAUSED (true)

// negative number for millis_between is one-shot
void task_init(Task* t, void (*func)(void), int16_t _millis_between, bool _paused) {
  t->execute = func;
  t->millis_between = _millis_between;
  t->millis_last_executed = 0;
  t->paused = _paused;
}
void task_init(Task* t, void (*func)(void), int16_t _millis_between) {
  task_init(t, func, _millis_between, false);
}
void task_pause(Task* t) {
  t->paused = true;
}
void task_continue(Task* t, int16_t _millis_between) {
  t->paused = false;
  t->millis_between = _millis_between;
}
void task_continue(Task* t) {
  t->paused = false;
}
bool task_is_running(Task* t) {
  return !t->paused;
}

//****************
// TASK MANAGER
//****************
#define MAX_TASKS 20

typedef struct taskmanager {
  uint8_t num_tasks;
  Task* task_list[MAX_TASKS];
} TaskManager;

void manager_init(TaskManager* tm) {
  tm->num_tasks = 0;
}
int8_t manager_register_task(TaskManager* tm, Task* t) {
  int8_t index = -1;
  if(tm->num_tasks+1 < MAX_TASKS) {
    index = tm->num_tasks;
    tm->task_list[tm->num_tasks] = t;
    tm->num_tasks += 1;
  }
  return index;
}
void manager_update(TaskManager* tm) {
  if(tm->num_tasks > 0) {
    int32_t now = millis();
    for(uint8_t i = 0; i < tm->num_tasks; i++) {
      Task* t = tm->task_list[i];
      if(!t->paused) {
        if(now - t->millis_last_executed >= t->millis_between) {
          t->millis_last_executed = now;
          t->execute();
          if(t->millis_between < 0) t->paused = true;
        }
      }
    }
  }
}
