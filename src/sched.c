#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include "sched.h"
#include <pthread.h>
#include <semaphore.h>


struct scheduler* ordonnanceur;

pthread_t threads[3]; //Stockage du nombre de thread en forme de variable globale.

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
void push(Stack* stack, Task tache) {
    if (stack->currentSize >= stack->maxSize) {
        fprintf(stderr, "Pile pleine, impossible d'ajouter une nouvelle tâche\n");
        return;
    }
    Node* newNode = (Node*) malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        return;
    }
    newNode->taskToDo = tache;
    newNode->next = stack->top;
    stack->top = newNode;
    stack->currentSize++;
}

// Fonction pour retirer une tâche au sommet de la pile et la retourner
Task* pop(Stack* stack) {
    if (stack->top == NULL) {
        fprintf(stderr, "Pile vide\n");
        exit(EXIT_FAILURE);
        return NULL;
    }

    Node* temp = stack->top;
    Task poppedTask = temp->taskToDo;
    stack->top = temp->next;
    free(temp);
    stack->currentSize--;
    return &poppedTask;
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
void initializeSchedulerForThread(){
    ordonnanceur =(struct scheduler*) malloc(sizeof(struct scheduler));

    // Initialisation du mutex avec pthread_mutex_init
    if (pthread_mutex_init(&ordonnanceur->lock, NULL) != 0) {
        fprintf(stderr, "Failed to initialize the mutex\n");
        pthread_mutex_destroy(&ordonnanceur->lock);
        exit(1);
    }

    if (pthread_cond_init(&ordonnanceur->taskAvailable, NULL) != 0) {
        fprintf(stderr, "Failed to initialize taskAvailable condition variable\n");
        pthread_cond_destroy(&ordonnanceur->taskAvailable);
        //free(ordonnanceur->taskStack);
        exit(1);
    }

    if (pthread_cond_init(&ordonnanceur->idle, NULL) != 0) {
        fprintf(stderr, "Failed to initialize idle condition variable\n");
        pthread_cond_destroy(&ordonnanceur->idle);
        //free(ordonnanceur->taskStack);
        exit(1);
    }
}


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
int sched_spawn(taskfunc f, void *closure, struct scheduler *s) {
    /*if (s == NULL) {
        fprintf(stderr, "Erreur: ordonnanceur non initialisé\n");
        return -1;  // Retourne -1 pour indiquer une erreur
    }*/

    //Verifier que la pile est pleine.
    if (getCurrentSize(s->taskStack) == s->taskStack->maxSize) {
        fprintf(stderr, "Erreur: la capacité de l'ordonnaceur est atteint\n");
        return -1;  // Retourne -1 pour indiquer une erreur
    }
    Task tache;
    tache.f = f;
    tache.closure = closure;

    //Ajouter une tache à la pile(Action d'enfilement), gestion de l'acces concurante à la pile
    push(s->taskStack, tache);


    //Après enfilement de la tache on s'arrange pour que cette tache enfiler soit executer
    //L'acces concurante pour recuper une tache pour qu'il soit executer.
    
    //Fin du traitement
    return 1;
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
int sched_init(int nthreads, int qlen, taskfunc f, void *closure) {
    //initalisation des primitives de synchronisation.
    initializeSchedulerForThread();

    //Les threads qui sont créer ici on doit avoir acces à eu dans la fonction spawn

    //Création d'une nouvelle pile.
    ordonnanceur->taskStack = createStack(qlen);

    //Enfilement de la tache initial
    //On doit aussi proteger l'enfilement dans la pile
    sched_spawn(f, closure, ordonnanceur);
    //Cet premier enfilement va generer recursivement les fututre enfilement sched_spawn


    /*Après le premier enfilement, les futures enfilements sont creer par le programme 
    principal par les appels recursive sur sched_spawn qui s'execute avec l'exemple du programme quicksort.c*/


    //Pour verifier l'enfilement on doit afficher la taille de la pile
    //printf("La taille de la pile est %d\n", getCurrentSize(ordonnanceur->taskStack));



    //Au moment ou sched_init realise que tout qu'il n'y a plus de tache il termine(variable conditionnels ???)
    //L’ordonnanceur termine lorsque la pile est vide et tous les threads sont endormis.
    return 1;
}

// Fonctions tâches pour les tests
void taskPrint(void* arg, struct scheduler* s) {
    printf("Task: %s\n", (char*)arg);
}


void threadFunctionTask(Stack *stack, struct scheduler* s) {
    Task* task;

    //Tout le temps il va essayer de depiler une tache de la pile pour être executer.
    //Si cette tache n'est pas disponible pour cet thread le thread dors un momement et redemande ainsi de suite
    while (1) {
        // Vérifie que la défilement a réussi
        if ((task = pop(stack)) != NULL) { 
            task->f(task->closure, s);
        }
        else {
        }
    }

    return NULL;
}