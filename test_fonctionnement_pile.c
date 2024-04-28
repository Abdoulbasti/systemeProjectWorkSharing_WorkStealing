
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


// Définition de la structure de la pile
/*typedef struct Stack {
    Node* top; //Tete de la pile
} Stack ;*/
typedef struct Stack {
    Node* top;    // Tête de la pile
    int maxSize;  // Taille maximale de la pile
    int currentSize;  // Taille actuelle de la pile
} Stack;


// Implémentation des fonctions de la pile
Stack* createStack() {
    Stack* stack = (Stack*) malloc(sizeof(Stack));
    if (stack == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        return NULL;
    }
    stack->top = NULL;
    return stack;
}


// Fonction pour ajouter une tâche au sommet de la pile
void push(Stack* stack, Task tache) {
    Node* newNode = (Node*) malloc(sizeof(Node));
    if (newNode == NULL) {
        fprintf(stderr, "Erreur d'allocation mémoire\n");
        return;
    }
    newNode->taskToDo = tache;
    newNode->next = stack->top;
    stack->top = newNode;
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
    return poppedTask;
}

// Fonction pour obtenir la taille de la pile
int sizeStack(Stack* stack) {
    Node* current = stack->top;
    int count = 0;
    while (current != NULL) {
        count++;
        current = current->next;
    }
    return count;
}

// Fonctions tâches pour les tests
void taskPrint(void* arg, struct scheduler* s) {
    printf("Task: %s\n", (char*)arg);
}








// Programme principal pour tester la pile
int main() {
    Stack* stack = createStack();
    if (stack == NULL) {
        printf("Failed to create a stack.\n");
        return 1;
    }

    // Création des tâches
    Task t1 = {taskPrint, "Hello"};
    Task t2 = {taskPrint, "World"};
    Task t3 = {taskPrint, "Stack Test"};
    Task t4 = {taskPrint, "Attack of Titan"};
    Task t5 = {taskPrint, "Boruto"};

    // Ajouter des tâches à la pile
    push(stack, t1);
    push(stack, t2);
    push(stack, t3);
    push(stack, t4);
    push(stack, t5);

    // Taille de la pile après ajout
    printf("Stack size after pushes: %d\n", sizeStack(stack));

    // Exécution et retrait des tâches
    /*Task temp;
    while (sizeStack(stack) > 0) {
        temp = pop(stack);
        temp.f(temp.closure, NULL);  // Exécuter la tâche
    }*/
    Task temp1 = pop(stack);
    temp1.f(temp1.closure, NULL);  // Exécuter la tâche
    Task temp2 = pop(stack);
    temp2.f(temp2.closure, NULL);  // Exécuter la tâche

    // Taille de la pile après retrait
    printf("Stack size after pops: %d\n", sizeStack(stack));

    // Nettoyer
    free(stack);
    return 0;
}
