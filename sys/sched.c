#include <sched.h>

int sched_class = DEFAULTSCHED; // Default scheduling class

int setschedclass(int class_id) {
    sched_class = class_id;
}

int getschedclass() {
    return sched_class;
}
