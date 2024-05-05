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
    //pthread_cond_signal(taskAvailable); //Reveiller un des thread endormi pour qu'il travail
    pthread_cond_broadcast(taskAvailable);

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

/*int pop(Stack* stack, Task* task, pthread_mutex_t* lock, pthread_cond_t* taskAvailable, pthread_cond_t* idle, int* activeThreads) {
    pthread_mutex_lock(lock);
    while (stack->top == NULL) {
        (*activeThreads)--;// Le thread devient inactif car il attend
        
        //Un des threads va signaler qu'il n'y a plus de thread actif
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
}*/
/*int pop(Stack* stack, Task* task, pthread_mutex_t* lock, pthread_cond_t* taskAvailable, pthread_cond_t* idle, int* activeThreads) {
    pthread_mutex_lock(lock);
    while (stack->top == NULL) {
        (*activeThreads)--;
        if (*activeThreads == 0) {
            pthread_cond_broadcast(idle);  // Utiliser broadcast pour réveiller tous les threads en attente
        }
        pthread_cond_wait(taskAvailable, lock);
        (*activeThreads)++;
    }
    Node* temp = stack->top;
    *task = temp->taskToDo;
    stack->top = temp->next;
    stack->currentSize--;
    free(temp);
    pthread_mutex_unlock(lock);
    return 1;
}*/
int pop(Stack* stack, Task* task, pthread_mutex_t* lock, pthread_cond_t* taskAvailable, pthread_cond_t* idle, int* activeThreads, int* shutdown) {
    pthread_mutex_lock(lock);
    //while (stack->top == NULL && !(*shutdown)) {
    
    //Lorqu'il n'y a pas de tache à effectuer dans la pile
    /*while (stack->top == NULL) {
        (*activeThreads)--; //Nombre de thread actif mise à jour juste avant le wait
        
        //Action realiser par le dernier thread qui est aucourant que : 
        if (*activeThreads == 0) {
            pthread_cond_broadcast(idle);  // le dernier thread au courant réveille tous pour la terminaison, mise à jour de la variable pour arrêter le thread correctement
            pthread_cond_wait(taskAvailable, lock);// une fois le signale envoyer lui aussi il dort. A cet instant tout les threads sont entraint de dormir

            //ACTION POUR DERNIER THREAD A S'ENDORMIR POUR ARRET LE PROGRAMME
            if(*shutdown){
                pthread_exit(NULL);
            }
        }
        pthread_cond_wait(taskAvailable, lock); //Thread est entrain de dormir
        //ACTION LES AUTRES THREADS ENDORMI POUR ARRET LE PROGRAMME
        //Thread reveiller par un signal et contate qu (Une verification en amont que tout les threads dorment)
        if (*shutdown){
            pthread_exit(NULL);
        }
        (*activeThreads)++; //Nombre de threads actif mise à jour après qu'on depasse le wait
    }*/

    //Lorqu'il n'y a pas de tache à effectuer dans la pile
    if(stack->top == NULL) {
        (*activeThreads)--; //Nombre de thread actif mise à jour juste avant le wait
        
        //Action realiser par le dernier thread qui est aucourant que : 
        if (*activeThreads == 0) {
            //pthread_cond_broadcast(idle);  //le dernier thread au courant réveille tous pour la terminaison, mise à jour de la variable pour arrêter le thread correctement
            pthread_cond_signal(idle); //Le dernier thread envoie un signal pour mettre à jour la valeur de shutdown
            pthread_cond_wait(taskAvailable, lock);// une fois le signale envoyer lui aussi il dort. A cet instant tout les threads sont entraint de dormir

            //ACTION POUR DERNIER THREAD A S'ENDORMIR POUR ARRET LE PROGRAMME
            if(*shutdown){
                printf("Thread terminer correctement : dernier thread qui etait endormi\n");
                pthread_exit(NULL);
            }
        }
        pthread_cond_wait(taskAvailable, lock); //Thread est entrain de dormir
        //ACTION LES AUTRES THREADS ENDORMI POUR ARRET LE PROGRAMME
        //Thread reveiller par un signal et contate qu (Une verification en amont que tout les threads dorment)
        if (*shutdown){
            printf("Thread terminer correctement\n");
            pthread_exit(NULL);
        }
        (*activeThreads)++; //Nombre de threads actif mise à jour après qu'on depasse le wait
    }
    if (stack->top != NULL) {
        Node* temp = stack->top;
        *task = temp->taskToDo;
        stack->top = temp->next;
        stack->currentSize--;
        free(temp);
    }
    pthread_mutex_unlock(lock);
    //return stack->top != NULL ? 1 : -1;
    return 1;
}

int getCurrentSize(Stack* stack) {
    if (stack == NULL) {
        fprintf(stderr, "Erreur: pile non initialisée\n");
        return -1;  // Retourne -1 pour indiquer une erreur
    }
    return stack->currentSize;
}


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
    ordonnanceur->shutdown = 0;
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
int sched_spawn(taskfunc f, void *closure, struct scheduler *s) {
    //Ajouter une tache à la pile(Action d'enfilement), gestion de l'acces concurante à la pile
    Task tacheEnfilee = {f, closure};
    int ret = push(s->taskStack, tacheEnfilee, &s->lock, &s->taskAvailable);
    if (ret == -1) {  perror("spawn error");}
    
    return ret;
}


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
        //printf("TEst\n");
    }

    // Enfilement de la tâche initiale et gestion des futures tâches.
    
    if (sched_spawn(f, closure, ordonnanceur) == -1) {
        cleanupScheduler(ordonnanceur, nthreads);
        return -1;
    }
    //printf("TEst\n");

    //printf("Active thread %d\n", ordonnanceur->activeThreads);
    //printf("taille de la pile %d\n", getCurrentSize(ordonnanceur->taskStack));

    // Attendre que l'ordonnanceur soit inactif avant de terminer.
    /*pthread_mutex_lock(&ordonnanceur->lock);
    while (ordonnanceur->activeThreads > 0 || getCurrentSize(ordonnanceur->taskStack) > 0) {
        pthread_cond_wait(&ordonnanceur->idle, &ordonnanceur->lock);
    }
    pthread_mutex_unlock(&ordonnanceur->lock);*/
    //printf("Active thread %d\n", ordonnanceur->activeThreads);
    //printf("taille de la pile %d\n", getCurrentSize(ordonnanceur->taskStack));
    
    /*pthread_mutex_lock(&ordonnanceur->lock);
    while (ordonnanceur->activeThreads > 0 || getCurrentSize(ordonnanceur->taskStack) > 0) {
        pthread_cond_wait(&ordonnanceur->idle, &ordonnanceur->lock);
    }
    pthread_mutex_unlock(&ordonnanceur->lock);*/



    //EXECUTION PAR LE PROCESSUS PRINCIPAL

    
    // Attente active
    /*pthread_mutex_lock(&ordonnanceur->lock);
    while (ordonnanceur->activeThreads > 0 || getCurrentSize(ordonnanceur->taskStack) > 0) {
        pthread_cond_wait(&ordonnanceur->idle, &ordonnanceur->lock);//Variable conditionnelle : ordonnanceur est oisif
    }
    ordonnanceur->shutdown = 1;  // Indiquer la fermeture
    pthread_cond_broadcast(&ordonnanceur->taskAvailable);  // Réveiller tous les threads en attente
    pthread_mutex_unlock(&ordonnanceur->lock);*/
    //printf("Test\n");

    //printf("Test\n");
    pthread_mutex_lock(&ordonnanceur->lock);
    //Attendre de recevoir le signal pour la mise à jour de shutdown
    pthread_cond_wait(&ordonnanceur->idle, &ordonnanceur->lock);
    ordonnanceur->shutdown = 1;  // Indiquer la fermeture

    //Pas de thread active(ils dorment tous) et Pile vide : Condition d'arrêt de l'ordonnanceur
    if(ordonnanceur->activeThreads == 0 && getCurrentSize(ordonnanceur->taskStack) == 0){
        pthread_cond_broadcast(&ordonnanceur->taskAvailable);  // Réveiller tous les threads qui dorment
    }
    pthread_mutex_unlock(&ordonnanceur->lock);
    


    // Attendre et joindre tous les threads arrêtés
    for (int i = 0; i < nthreads; i++) {
        pthread_join(ordonnanceur->threads[i], NULL);
    }
    
    //exit(1);
    cleanupScheduler(ordonnanceur, nthreads);
    
    //printf("Test\n");

    return 1;
}



void cleanupScheduler(struct scheduler *s, int nthreads) {
    // Cancel and join all threads
    //Probleme ici
    /*for (int i = 0; i < nthreads; i++) {
        pthread_cancel(s->threads[i]);
        pthread_join(s->threads[i], NULL);
    }*/


    // Clean up mutex and condition variables
    pthread_mutex_destroy(&s->lock);
    pthread_cond_destroy(&s->taskAvailable);
    pthread_cond_destroy(&s->idle);
    

    // Free the stack and the scheduler structure
    free(s->taskStack);
    free(s->threads);
    free(s);

    //printf("Test\n");
}


void* threadFunctionTask(void* ss) {
    Task task;
    struct scheduler* s = (struct scheduler*)ss;
    while (1) {
        //int pop(Stack* stack, Task* task, pthread_mutex_t* lock, pthread_cond_t* taskAvailable, pthread_cond_t* idle, int* activeThreads)
        if (pop(s->taskStack, &task, &s->lock, &s->taskAvailable, &s->idle, &s->activeThreads, &s->shutdown) == 1) {
            task.f(task.closure, s);
            //printf("Executing function...\n");
            free(task.closure); //Liberation des ressources
        }
    }

    return NULL;  // Cette ligne n'est jamais atteinte
}