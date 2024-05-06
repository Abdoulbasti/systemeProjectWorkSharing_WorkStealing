#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "sched.h"
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

//--------- La pile ---------
typedef struct Noeud {
    taskfunc taskfunc;
    void *closure;
    struct Noeud* next;
} Noeud;

typedef struct Pile {
    Noeud* top;
    int maxSize;
    int currentSize;
} Pile;

struct scheduler {
    Pile *pile;
    int workingThreads;
    pthread_t* threads;
    pthread_mutex_t m_scheduler;
    pthread_cond_t c_pile;
    pthread_cond_t c_workingThreads;
};

int pop(Noeud **noeud, Pile* pile) {
    if (pile->top == NULL) {
        return -1;
    }
    *noeud = pile->top;
    pile->top = pile->top->next;
    pile->currentSize--;
    return 1;
}

int push(Pile* pile, taskfunc task, void *closure) {
    if (pile->currentSize == pile->maxSize) {
        perror("La pile est pleine");
        errno = EAGAIN;
        return -1;
    }
    Noeud* nouveauNoeud = (Noeud*)malloc(sizeof(Noeud));
    if (nouveauNoeud == NULL){
        return -1;
    }
    nouveauNoeud->taskfunc = task;
    nouveauNoeud->closure = closure;
    nouveauNoeud->next = pile->top;
    pile->top = nouveauNoeud;
    pile->currentSize++;
    return 1;
}

void* threadFunction(void* scheduler) {
    struct scheduler* scheduler_ = (struct scheduler*)scheduler;
    Noeud *task;
    while (1) {
        pthread_mutex_lock(&scheduler_->m_scheduler);
        while (scheduler_->pile->top == NULL) {
            pthread_cond_wait(&scheduler_->c_pile, &scheduler_->m_scheduler);
        }
        int ret = pop(&task, scheduler_->pile);
        pthread_mutex_unlock(&scheduler_->m_scheduler);
        if (ret >= 0){
            pthread_mutex_lock(&scheduler_->m_scheduler);
            scheduler_->workingThreads++;
            pthread_mutex_unlock(&scheduler_->m_scheduler);

            task->taskfunc(task->closure, scheduler_);

            pthread_mutex_lock(&scheduler_->m_scheduler);
            scheduler_->workingThreads--;
            pthread_mutex_unlock(&scheduler_->m_scheduler);
            pthread_cond_signal(&scheduler_->c_workingThreads);
            free(task);
        }
    }
}

int pile_init(struct scheduler *scheduler, int qlen){
    Pile *pile = (Pile*) malloc(sizeof(Pile));
    if (pile == NULL) {
        return -1;
    }

    pile->top = NULL;
    pile->maxSize = qlen;
    pile->currentSize = 0;

    scheduler->pile = pile;

    return 1;
}

int threads_init(struct scheduler *scheduler, int nthreads) {
    scheduler->threads = malloc(nthreads * sizeof(pthread_t));
    if (scheduler->threads == NULL) {
        return -1;
    }
    scheduler->workingThreads = 0;
    for (int i = 0; i < nthreads; i++) {
        if (pthread_create(&scheduler->threads[i], NULL, threadFunction, (void *)scheduler) != 0) {
            return -1;
        }
    }
    return 1;
}

int mutex_init(struct scheduler *scheduler){
    if (pthread_mutex_init(&scheduler->m_scheduler, NULL) < 0) {
        return -1;
    }
    if (pthread_cond_init(&scheduler->c_pile, NULL) < 0) {
        return -1;
    }
    if (pthread_cond_init(&scheduler->c_workingThreads, NULL) < 0) {
        return -1;
    }
    return 1;
}

int sched_init(int nthreads, int qlen, taskfunc f, void *closure){
    if (nthreads < 0) {
        nthreads = sched_default_threads();
        #ifdef DEBUG
        printf("On a %d threads.\n", nthreads);
        #endif
    }

    struct scheduler* ordonnanceur = (struct scheduler*) malloc(sizeof(struct scheduler));
    if (ordonnanceur == NULL ) {
        perror("Erreur d'allocation mémoire de l'ordonnanceur");
        return -1;
    }
    #ifdef DEBUG
    printf("Ordonnanceur alloué\n");
    #endif

    if (mutex_init(ordonnanceur) < 0){
        perror("Erreur initialisation des mutex");
        return -1;
    }
    #ifdef DEBUG
    printf("Mutex initialisé\n");
    #endif
    
    if (pile_init(ordonnanceur, qlen) < 0){
        perror("Erreur d'allocation mémoire de la pile");
        return -1;
    }
    #ifdef DEBUG
    printf("Pile initialisé\n");
    #endif

    if (threads_init(ordonnanceur, nthreads) < 0){
        perror("Erreur initialisation des threads");
        return -1;
    }
    #ifdef DEBUG
    printf("Threads initialisés\n");
    #endif

    if (sched_spawn(f, closure, ordonnanceur) < 0) {
        perror("Erreur tache initail");
        return -1;
    }
    #ifdef DEBUG
    printf("Tache initial ajouté\n");
    #endif

    pthread_mutex_lock(&ordonnanceur->m_scheduler);    
    while (ordonnanceur->workingThreads > 0 || ordonnanceur->pile->currentSize > 0) {
        pthread_cond_wait(&ordonnanceur->c_workingThreads, &ordonnanceur->m_scheduler);
    }

    return 1;
}

int sched_spawn(taskfunc f, void *closure, struct scheduler *s){
    pthread_mutex_lock(&s->m_scheduler);
    int res = push(s->pile, f, closure);
    pthread_mutex_unlock(&s->m_scheduler);
    pthread_cond_signal(&s->c_pile);
    if (res < 0) {
        return -1;
    }
    return 1;
}