/* Schedular class identifiers*/
#define DEFAULTSCHED 0
#define EXPDISTSCHED 1
#define LINUXSCHED 2

/* Scheduling Class variable*/
extern int sched_class;

/* Scheduling APIs */
extern int setschedclass(int sched_class);
extern int getschedclass();