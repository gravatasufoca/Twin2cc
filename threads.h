
typedef void*(*threadfn)(void*);
int create_prio_thread(pthread_t *tid, threadfn func, void *arg, int newprio);

