#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "sched.h"
#include <pthread.h>
#include <semaphore.h>
#include <errno.h>

//------- La pile -------
//
//          TOP
//          | ^
//     prev | | next
//          v |
//          BOT
//

typedef struct Noeud {
    taskfunc taskfunc;
    void *closure;
    struct Noeud* prev;
    struct Noeud* next;
} Noeud;

typedef struct Deques {
    Noeud* top;
    Noeud* bottom;
    int maxSize;
    int currentSize;
    pthread_mutex_t m_thread;
} Deques;

typedef struct thread {
    Deques* pile;
    pthread_t thread;
} thread;

struct scheduler {
    int nbThread;
    thread* threads;
    int nbOisifs;
    pthread_mutex_t m_scheduler;
    pthread_cond_t c_scheduler;
};

int popLastElt(Deques* pile){
    if (pile->top == NULL || pile->bottom == NULL){
        pile->top = NULL;
        pile->bottom = NULL;
        return 1;
    }
    return 0;
}

int pushFirstElt(Deques* pile, Noeud* noeud){
    if (pile->top == NULL) {
        pile->top = noeud;
        pile->bottom = noeud;
        pile->top->next = NULL;
        pile->top->prev = NULL;
        return 1;
    }
    return 0;
}

int popUp(Noeud **noeud, Deques* pile) {
    pthread_mutex_lock(&pile->m_thread);
    if (pile->top == NULL) {
        pthread_mutex_unlock(&pile->m_thread);
        return -1;
    }
    *noeud = pile->top;
    pile->top = pile->top->prev;
    pile->currentSize--;
    if (!popLastElt(pile)){
        pile->top->next = NULL;
    }
    pthread_mutex_unlock(&pile->m_thread);
    return 1;
}

int popBottom(Noeud **noeud, Deques* pile) {
    pthread_mutex_lock(&pile->m_thread);
    if (pile->bottom == NULL) {
        pthread_mutex_unlock(&pile->m_thread);
        return -1;
    }
    *noeud = pile->bottom;
    pile->bottom = pile->bottom->next;
    pile->currentSize--;
    if (!popLastElt(pile)) {
        pile->bottom->prev = NULL;
    }
    pthread_mutex_unlock(&pile->m_thread);
    return 1;
}

int pushBottom(Deques* pile, taskfunc task, void *closure) {
    pthread_mutex_lock(&pile->m_thread);
    if (pile->currentSize == pile->maxSize) {
        perror("La pile est pleine");
        errno = EAGAIN;
        pthread_mutex_unlock(&pile->m_thread);
        return -1;
    }
    Noeud* nouveauNoeud = (Noeud*)malloc(sizeof(Noeud));
    if (nouveauNoeud == NULL){
        pthread_mutex_unlock(&pile->m_thread);
        return -1;
    }
    nouveauNoeud->taskfunc = task;
    nouveauNoeud->closure = closure;
    nouveauNoeud->next = pile->bottom;
    pile->currentSize++;
    if (!pushFirstElt(pile, nouveauNoeud)){
        nouveauNoeud->next = pile->bottom;
        pile->bottom->prev = nouveauNoeud;
        pile->bottom = nouveauNoeud;
    }
    pthread_mutex_unlock(&pile->m_thread);
    return 1;
}

int getIdThread(struct scheduler* scheduler){
    pthread_t self = pthread_self();

    for (int i = 0; i < scheduler->nbThread; i++) {
        if (pthread_equal(scheduler->threads[i].thread, self)) {
            return i;
        }
    }
    return -1;
}

int steal(struct scheduler* scheduler, Noeud **task){
    unsigned int seed = time(NULL) + *((int *) pthread_self());
    int k = rand_r(&seed) % scheduler->nbThread;
    
    for (int i = 0; i< scheduler->nbThread; i++){
        int res = popUp(task, scheduler->threads[(k+i)%scheduler->nbThread].pile);
        if (res >= 0){
            return 1;
        }
    }
    
    return -1;
}

void* threadFunction(void* scheduler) {
    struct scheduler* scheduler_ = (struct scheduler*)scheduler;
    int id = getIdThread(scheduler_);
    if (id < 0){
        exit(-1);
    }
    Noeud *task;
    pthread_mutex_lock(&scheduler_->m_scheduler);
    pthread_mutex_unlock(&scheduler_->m_scheduler);
    while (1) {
        int res = popBottom(&task, scheduler_->threads[id].pile);
        if (res < 0){
            pthread_mutex_lock(&scheduler_->m_scheduler);
            scheduler_->nbOisifs++;
            pthread_mutex_unlock(&scheduler_->m_scheduler);
            pthread_cond_signal(&scheduler_->c_scheduler);
            while (steal(scheduler_, &task) < 0){
                sleep(1);
            }
            pthread_mutex_lock(&scheduler_->m_scheduler);
            scheduler_->nbOisifs--;
            pthread_mutex_unlock(&scheduler_->m_scheduler);
        }
        task->taskfunc(task->closure, scheduler_);
        
    }
}

int pile_init(Deques **pile, int qlen){
    *pile = (Deques*) malloc(sizeof(Deques));
    if (pile == NULL) {
        return -1;
    }

    if (pthread_mutex_init(&(*pile)->m_thread, NULL) < 0) {
        return -1;
    }

    (*pile)->top = NULL;
    (*pile)->bottom = NULL;
    (*pile)->maxSize = qlen;
    (*pile)->currentSize = 0;

    return 1;
}


int mutex_init(struct scheduler *scheduler){
    if (pthread_mutex_init(&scheduler->m_scheduler, NULL) < 0) {
        return -1;
    }
    if (pthread_cond_init(&scheduler->c_scheduler, NULL) < 0) {
        return -1;
    }
    return 1;
}

int threads_init(struct scheduler *scheduler, int nthreads, int qlen) {
    scheduler->threads = malloc(nthreads * sizeof(thread));
    if (scheduler->threads == NULL) {
        return -1;
    }
    for (int i = 0; i < nthreads; i++) {
        if (pile_init(&scheduler->threads[i].pile, qlen) < 0){
            return -1;
        }
        if (pthread_create(&scheduler->threads[i].thread, NULL, threadFunction, (void *)scheduler) != 0) {
            return -1;
        }
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
    ordonnanceur->nbThread = nthreads;
    ordonnanceur->nbOisifs = 0;
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
    printf("Mutex initialisés\n");
    #endif

    pthread_mutex_lock(&ordonnanceur->m_scheduler); //bloc thread

    if (threads_init(ordonnanceur, nthreads, qlen) < 0){
        perror("Erreur initialisation des threads");
        return -1;
    }
    #ifdef DEBUG
    printf("Threads initialisés\n");
    #endif

    if (pushBottom(ordonnanceur->threads[0].pile, f, closure) < 0){
        perror("Erreur tache initial");
        return -1;
    }
    #ifdef DEBUG
    printf("Tache initial ajouté\n");
    #endif

    pthread_mutex_unlock(&ordonnanceur->m_scheduler); //lancer thread

    while (ordonnanceur->nbOisifs < nthreads) {
        pthread_cond_wait(&ordonnanceur->c_scheduler, &ordonnanceur->m_scheduler);
    }

    return 1;
}

int sched_spawn(taskfunc f, void *closure, struct scheduler *s){
    int id = getIdThread(s);
    if (id < 0){
        return -1;
    }
    if (pushBottom(s->threads[id].pile, f, closure) < 0) {
        return -1;
    }
    return 1;
}