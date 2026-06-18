#ifndef APP_TASKS_H
#define APP_TASKS_H

#ifdef __cplusplus
extern "C" {
#endif

void SCPITask(void *argument);
void ModBusTask(void *argument);
void WatchDogTask(void *argument);
void StateVectorTask(void *argument);
void SysTimerCallback(void *argument);

#ifdef __cplusplus
}
#endif

#endif /* APP_TASKS_H */
