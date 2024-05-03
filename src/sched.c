#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "sched.h"
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>



/***********************************************************Fonction pour la gestion de la pile**********************************************************/
// Implémentation des fonctions de la pile
Stack* createStack(int maxSize) {
    Stack* stack = (Stack*) malloc(sizeof(Stack));
    if (stack == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        return NULL;
    }
    stack->top = NULL;
    stack->maxSize = maxSize;
    stack->currentSize = 0;
    return stack;
}



// Fonction pour ajouter une tâche au sommet de la pile
/*int push(Stack* stack, Task tache, pthread_mutex_t lock, pthread_cond_t taskAvailable) {
    pthread_mutex_lock(&lock);

    //Verifier s'il est possible d'ajouter un element dan sla pile
    if (getCurrentSize(stack) == stack->maxSize) {
        fprintf(stderr, "Pile pleine, impossible d'ajouter une nouvelle tâche\n");
        errno = EAGAIN;
        return -1;
        pthread_mutex_unlock(&lock);
    }else{
        Node* newNode = (Node*) malloc(sizeof(Node));
        if (newNode == NULL) {
            fprintf(stderr, "Erreur d'allocation mémoire\n");
            return;
        }
        newNode->taskToDo = tache;
        newNode->next = stack->top;
        stack->top = newNode;
        stack->currentSize++;
        pthread_cond_signal(&taskAvailable);
    }
    pthread_mutex_unlock(&lock);
}*/

int push(Stack* stack, Task tache, pthread_mutex_t* lock, pthread_cond_t* taskAvailable) {
    pthread_mutex_lock(lock);

    if (stack->currentSize == stack->maxSize) {
        fprintf(stderr, "Stack is full\n");
        errno = EAGAIN;
        pthread_mutex_unlock(lock);
        return -1;  // Libération du verrou avant de retourner
    }

    Node* newNode = (Node*) malloc(sizeof(Node));
    if (!newNode) {
        fprintf(stderr, "Failed to allocate node\n");
        pthread_mutex_unlock(lock);
        return -1;  // Libération du verrou avant de retourner
    }

    newNode->taskToDo = tache;
    newNode->next = stack->top;
    stack->top = newNode;
    stack->currentSize++;
    pthread_cond_signal(taskAvailable);

    pthread_mutex_unlock(lock);  // Libération du verrou avant de retourner
    return 1;  // Retourner après avoir libéré le verrou
}

// Fonction pour retirer une tâche au sommet de la pile et la retourner
//La tache retirer est stocker dans task
/*int pop(Stack* stack, Task* task, pthread_mutex_t lock, pthread_cond_t taskAvailable) {
    pthread_mutex_lock(&lock);
    while (stack->top == NULL) {
        //Mettre le thread appellant en attente jusqu'a ce qu'une tâche soit disponible
        pthread_cond_wait(&taskAvailable, &lock);
    }

    Node* temp = stack->top;
    *task = temp->taskToDo;
    stack->top = temp->next;
    free(temp);
    stack->currentSize--;
    pthread_mutex_unlock(&lock);
    return 1;
}*/

/*int pop(Stack* stack, Task* task, pthread_mutex_t lock, pthread_cond_t taskAvailable, pthread_cond_t idle, int activeThreads) {
    pthread_mutex_lock(&lock);

    while (stack->top == NULL) {
        activeThreads--;  // Le thread devient inactif car il attend
        if (activeThreads == 0) {
            pthread_cond_signal(&idle);  // Peut signaler la condition de terminaison
        }
        pthread_cond_wait(&taskAvailable, &lock);
        activeThreads++;  // Le thread redevient actif
    }

    Node* temp = stack->top;
    *task = temp->taskToDo;
    stack->top = temp->next;
    stack->currentSize--;
    free(temp);

    pthread_mutex_unlock(&lock);
    return 1;
}*/

int pop(Stack* stack, Task* task, pthread_mutex_t* lock, pthread_cond_t* taskAvailable, pthread_cond_t* idle, int* activeThreads) {
    pthread_mutex_lock(lock);
    while (stack->top == NULL) {
        (*activeThreads)--;// Le thread devient inactif car il attend
        if (*activeThreads == 0) {
            pthread_cond_signal(idle);// Peut signaler la condition de terminaison
        }
        pthread_cond_wait(taskAvailable, lock);
        (*activeThreads)++; // Le thread redevient actif
    }
    Node* temp = stack->top;
    *task = temp->taskToDo;
    stack->top = temp->next;
    stack->currentSize--;
    free(temp);
    pthread_mutex_unlock(lock);
    return 1;
}


int getCurrentSize(Stack* stack) {
    if (stack == NULL) {
        fprintf(stderr, "Erreur: pile non initialisée\n");
        return -1;  // Retourne -1 pour indiquer une erreur
    }
    return stack->currentSize;
}


// Fonction pour exécuter une tâche
/*void executeTask(Task task, struct scheduler *s) {
    if (task.f == NULL || task.closure == NULL) {
        fprintf(stderr, "Erreur: la fonction de tâche n'est pas définie.\n");
        return;
    }
    if (s == NULL) {
        fprintf(stderr, "Erreur: probleme avec le scheduler.\n");
        return;
    }

    // Appel de la fonction de tâche avec son contexte, utiliser s pour gerer correctement la synchronisation
    task.f(task.closure, s);
}*/


//Cette fonction est forcement appeler avant la création des threads
void initializeSchedulerForThread(struct scheduler* ordonnanceur, int nthread, int qlen){
    // Initialisation du mutex avec pthread_mutex_init
    if (pthread_mutex_init(&ordonnanceur->lock, NULL) != 0) {
        perror("Failed to initialize the mutex");
        pthread_mutex_destroy(&ordonnanceur->lock);
        exit(1);
    }

    if (pthread_cond_init(&ordonnanceur->taskAvailable, NULL) != 0) {
        perror("Failed to initialize taskAvailable condition variable");
        pthread_cond_destroy(&ordonnanceur->taskAvailable);
        //free(ordonnanceur->taskStack);
        exit(1);
    }

    if (pthread_cond_init(&ordonnanceur->idle, NULL) != 0) {
        perror("Failed to initialize idle condition variable");
        pthread_cond_destroy(&ordonnanceur->idle);    
        exit(1);
    }

    ordonnanceur->threads = (pthread_t*) malloc(sizeof(pthread_t)*nthread);

    // Initialiser tous les threads comme actifs
    ordonnanceur->activeThreads = nthread;

    //Création d'une nouvelle pile.
    ordonnanceur->taskStack = createStack(qlen);
}

/*void initializeSchedulerForThread(struct scheduler* sched, int nthreads, int qlen) {
    if (pthread_mutex_init(&sched->lock, NULL) != 0 ||
        pthread_cond_init(&sched->taskAvailable, NULL) != 0 ||
        pthread_cond_init(&sched->idle, NULL) != 0) {
        fprintf(stderr, "Failed to initialize synchronization primitives\n");
        exit(1);
    }
    sched->taskStack = createStack(qlen);
    sched->activeThreads = nthreads;
    sched->threads = (pthread_t*) malloc(sizeof(pthread_t) * nthreads);
}*/



/***************************************************************AUTRES*******************************************************/
/*La fonction sched_spawn sert à enfiler une nouvelle tâche.
int sched_spawn(taskfunc f, void *closure, struct scheduler *s);
Elle prend en paramètre une tâche (f, closure) et un ordonnanceur s. Elle a pour effet d’insérer
la tâche dans l’ensemble des tâches à exécuter par l’ordonnanceur s. Cette fonction ne peut être
exécutée qu’à partir d’une tâche qui s’exécute sur s.
Si le nombre de tâches en file est déjà supérieur ou égal à la capacité de l’ordonnanceur (la
valeur du paramètre qlen passé à sched_init), cette fonction peut soit enfiler la tâche quand
même, soit retourner -1 avec errno valant EAGAIN. En d’autres termes, l’ordonnanceur n’est pas
obligé d’enfiler plus de tâches que prévu, mais il n’est pas non plus obligé de détecter ce cas.
La fonction sched_spawn retourne immédiatement (dès qu’elle s’est arrangée pour que la
tâche soit effectuée).*/
/*int sched_spawn(taskfunc f, void *closure, struct scheduler *s) {
    //Ajouter une tache à la pile(Action d'enfilement), gestion de l'acces concurante à la pile
    Task tacheEnfilee = {f, closure};
    push(s->taskStack, tacheEnfilee, s->lock, s->taskAvailable);
    //printf("TESTING\n");
    //printf("Taille de la pile %d\n", getCurrentSize(s->taskStack));

    //Après enfilement de la tache on s'arrange pour defiler une tache et l'executer
    //Task taskToExecute;
    //pop(s->taskStack, &taskToExecute, s->lock, s->taskAvailable);

    //printf("TESTING\n");
    //printf("Taille de la pile %d\n", getCurrentSize(s->taskStack));
    //taskToExecute.f(taskToExecute.closure, s);
    return 1;
}*/

int sched_spawn(taskfunc f, void *closure, struct scheduler *s) {
    //Ajouter une tache à la pile(Action d'enfilement), gestion de l'acces concurante à la pile
    Task tacheEnfilee = {f, closure};
    int ret = push(s->taskStack, tacheEnfilee, &s->lock, &s->taskAvailable);
    if (ret == -1) {  perror("spawn error");}
    return ret;
}


void simpleTask(void *closure, struct scheduler *s) {
    printf("Executing task with arg: %s\n", (char*)closure);
}



/*sched_init La fonction sched_init lance l’ordonnanceur.
int sched_init(int nthreads, int qlen,
taskfunc f, void *closure);
Elle prend en paramètre :
– le nombre nthreads de threads que va créer l’ordonnanceur; si ce paramètre vaut zéro,
le nombre de threads sera égal au nombre de cœurs de votre machine (que vous pouvez
déterminer à l’aide d’un appel à sysconf(_SC_NPROCESSORS_ONLN));
– le nombre minimum qlen de tâches simultanées que l’ordonnanceur devra supporter; si
l’utilisateur essaie d’enfiler plus de qlen tâches, l’ordonnanceur pourra retourner une erreur (comme décrit ci-dessous);
– la tâche initiale (f, closure).
La fonction sched_init retourne lorsqu’il n’y a plus de tâches à effectuer, et alors son résultat
vaut 1. Elle retourne -1 si l’ordonnanceur n’a pu être initialisé.*/
/*int sched_init(int nthreads, int qlen, taskfunc f, void *closure) {
    if(nthreads == 0) {
        nthreads = sched_default_threads();
        printf("le nombre de threads systeme %d\n",nthreads);
    }

    struct scheduler* ordonnanceur = (struct scheduler*) malloc(sizeof(struct scheduler));
    
    //initalisation des primitives de synchronisation.
    initializeSchedulerForThread(ordonnanceur, nthreads, qlen);


    //Creation et execution continue des threads qui essaye chacun de prendre une tache pour l'executer.
    for (int i = 0; i < nthreads; i++) {
        pthread_create(&ordonnanceur->threads[i], NULL, threadFunctionTask, ordonnanceur);
    }
    

    //Cet premier enfilement va generer recursivement les fututre enfilement sched_spawn
    sched_spawn(f, closure, ordonnanceur);

    
    //si tout les threads endormis et la pile est vide :
    //    effectuer les netoyages
    //    retourner 1 : terminer l'ordonnanceur
    
    pthread_mutex_lock(&ordonnanceur->lock);
    while (ordonnanceur->activeThreads > 0 || getCurrentSize(ordonnanceur->taskStack) > 0) {
        pthread_cond_wait(&ordonnanceur->idle, &ordonnanceur->lock);
    }
    pthread_mutex_unlock(&ordonnanceur->lock);

    //Cleanup
    cleanupScheduler(ordonnanceur, nthreads);
    return 1;
}*/

int sched_init(int nthreads, int qlen, taskfunc f, void *closure) {
    if (nthreads == 0) {
        nthreads = sched_default_threads();
        printf("Le nombre de threads système est %d\n", nthreads);
    }

    struct scheduler* ordonnanceur = (struct scheduler*) malloc(sizeof(struct scheduler));
    if (ordonnanceur == NULL ) {
        perror("Failed to allocate memory for scheduler");
        return -1;
    }


    // Initialise les primitives de synchronisation et la pile de tâches.
    initializeSchedulerForThread(ordonnanceur, nthreads, qlen);
    int n = nthreads;
    // Création et démarrage des threads.
    for (int i = 0; i < n; i++) {
        if (pthread_create(&ordonnanceur->threads[i], NULL, threadFunctionTask, (void*)ordonnanceur) != 0) {
            perror("Failed to create a thread");
            cleanupScheduler(ordonnanceur, nthreads);  // Nettoyer les threads déjà créés et sortir
            return -1;
        }
    }

    // Enfilement de la tâche initiale et gestion des futures tâches.
    if (sched_spawn(f, closure, ordonnanceur) == -1) {
        cleanupScheduler(ordonnanceur, nthreads);
        return -1;
    }

    // Attendre que l'ordonnanceur soit inactif avant de terminer.
    pthread_mutex_lock(&ordonnanceur->lock);
    while (ordonnanceur->activeThreads > 0 || getCurrentSize(ordonnanceur->taskStack) > 0) {
        pthread_cond_wait(&ordonnanceur->idle, &ordonnanceur->lock);
    }
    pthread_mutex_unlock(&ordonnanceur->lock);

    cleanupScheduler(ordonnanceur, nthreads);
    return 1;
}

// Fonctions tâches pour les tests
/*void taskPrint(void* closure, struct scheduler* s) {
    printf("Task: %s\n", (char*)closure);
}*/




void cleanupScheduler(struct scheduler *s, int nthreads) {
    // Cancel and join all threads
    for (int i = 0; i < nthreads; i++) {
        pthread_cancel(s->threads[i]);
        pthread_join(s->threads[i], NULL);
    }

    // Clean up mutex and condition variables
    pthread_mutex_destroy(&s->lock);
    pthread_cond_destroy(&s->taskAvailable);
    pthread_cond_destroy(&s->idle);

    // Free the stack and the scheduler structure
    free(s->taskStack);
    free(s->threads);
    free(s);
}


void* threadFunctionTask(void* ss) {
    Task task;
    struct scheduler* s = (struct scheduler*)ss;
    while (1) {
        //int pop(Stack* stack, Task* task, pthread_mutex_t* lock, pthread_cond_t* taskAvailable, pthread_cond_t* idle, int* activeThreads)
        if (pop(s->taskStack, &task, &s->lock, &s->taskAvailable, &s->idle, &s->activeThreads) == 1) {
            task.f(task.closure, s);
            //printf("Executing function...\n");
            free(task.closure); //Liberation des ressources
        }
    }

    return NULL;  // Cette ligne n'est jamais atteinte
}