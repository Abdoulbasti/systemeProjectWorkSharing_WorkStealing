struct scheduler;


typedef void (*taskfunc)(void*, struct scheduler *);

//Representation d'une tache
typedef struct Task {
    taskfunc f;
    void *closure;
} Task;


// Définition d'un nœud de la pile
typedef struct Node {
    Task taskToDo;
    struct Node* next;
} Node;

typedef struct Stack {
    Node* top;    // Tête de la pile
    int maxSize;  // Taille maximale de la pile
    int currentSize;  // Taille actuelle de la pile
} Stack;


static inline int
sched_default_threads()
{
    return sysconf(_SC_NPROCESSORS_ONLN);
}

struct scheduler {
    Stack *taskStack;               // Pile de tâches
    pthread_mutex_t lock;           // Mutex pour l'accès exclusif à la pile
    pthread_cond_t taskAvailable;   // Condition variable pour signaler la disponibilité de tâches
    pthread_cond_t idle;            // Condition variable pour signaler que l'ordonnanceur est oisif
    pthread_t* threads;             // Nombre de threads a créer.
    int activeThreads;              // Compteur de threads actifs
};



int sched_init(int nthreads, int qlen, taskfunc f, void *closure);
int sched_spawn(taskfunc f, void *closure, struct scheduler *s);


void initializeSchedulerForThread(struct scheduler* ordonnanceur, int nthread, int qlen);
Stack* createStack(int maxSize);
int getCurrentSize(Stack* stack);
int pop(Stack* stack, Task* task, pthread_mutex_t lock, pthread_cond_t taskAvailable, pthread_cond_t idle, int activeThreads);
void push(Stack* stack, Task tache, pthread_mutex_t lock, pthread_cond_t taskAvailable);
void taskPrint(void* closure, struct scheduler* s);
void cleanupScheduler(struct scheduler *s, int nthreads);
void* threadFunctionTask(struct scheduler* s);