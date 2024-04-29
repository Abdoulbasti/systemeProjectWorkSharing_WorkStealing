
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <time.h>
#include <assert.h>
#include <pthread.h>

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



struct scheduler {
    Stack *taskStack;            // Pile de tâches
    pthread_mutex_t lock;        // Mutex pour l'accès exclusif à la pile
    pthread_cond_t taskAvailable;// Condition variable pour signaler la disponibilité de tâches
    pthread_cond_t idle;         // Condition variable pour signaler que l'ordonnanceur est oisif
    int maxTasks;                // Nombre maximum de tâches que la pile peut contenir
    int activeThreads;           // Compteur de threads actifs
};



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
Task pop(Stack* stack) {
    if (stack->top == NULL) {
        fprintf(stderr, "Pile vide\n");
        exit(EXIT_FAILURE);
    }

    Node* temp = stack->top;
    Task poppedTask = temp->taskToDo;
    stack->top = temp->next;
    free(temp);
    stack->currentSize--;
    return poppedTask;
}

int getCurrentSize(Stack* stack) {
    if (stack == NULL) {
        fprintf(stderr, "Erreur: pile non initialisée\n");
        return -1;  // Retourne -1 pour indiquer une erreur
    }
    return stack->currentSize;
}

// Fonctions tâches pour les tests
void taskPrint(void* arg, struct scheduler* s) {
    printf("Task: %s\n", (char*)arg);
}






struct scheduler ordonnanceur;

int main(int argc, char const *argv[])
{
    ordonnanceur.maxTasks  = 9;
    printf("Entier : %d tasks\n", ordonnanceur.maxTasks);
    return 0;
}



// Programme principal pour tester la pile
/*int main() {
    Stack* stack = createStack(1000);  // Création de la pile avec une taille maximale de 3

    // Création des tâches
    Task t1 = {taskPrint, "Première tâche"};
    Task t2 = {taskPrint, "Deuxième tâche"};
    Task t3 = {taskPrint, "Troisième tâche"};
    Task t4 = {taskPrint, "Quatrième tâche"}; // Cette tâche ne devrait pas être ajoutée

    // Ajouter des tâches à la pile
    push(stack, t1);
    push(stack, t2);
    push(stack, t3);
    push(stack, t4);  // Test de dépassement de la capacité

    // Exécution des tâches
    while (stack->currentSize > 0) {
        Task poppedTask = pop(stack);
        poppedTask.f(poppedTask.closure, NULL);  // Exécuter la tâche
    }

    // Nettoyer
    free(stack);
    return 0;
}*/