#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CACHE_SIZE 4

typedef struct SSInfo {
    int ssid;
    char path[255];
} SSInfo;

typedef struct node_info* Node;

struct node_info{
    Node Prev;
    Node Next;
    SSInfo SS; 
};

Node HEAD;

Node UPDATEtoFront(Node node){
    if(node == HEAD){
        return HEAD;
    }
    if(node->Next == NULL){
        node->Prev->Next = NULL;
    }else{
        node->Prev->Next = node->Next;
        node->Next->Prev = node->Prev;
    }
    node->Next = HEAD;
    node->Prev = NULL;
    HEAD->Prev = node;
    HEAD = node;
    return HEAD;
}

Node Search(char* key){
    Node current = HEAD;
    while(current != NULL){
        if(strcmp(current->SS.path, key) == 0){
            UPDATEtoFront(current);
            return current;
        }
        current = current->Next;
    }
    return NULL;
}

Node ADD(SSInfo ssinfo) {
    Node newNode = (Node)malloc(sizeof(struct node_info));
    newNode->SS = ssinfo;
    newNode->Next = HEAD;
    newNode->Prev = NULL;

    if (HEAD != NULL) {
        HEAD->Prev = newNode;
    }

    HEAD = newNode;

    // Check and remove the least recently used element if cache size exceeds CACHE_SIZE
    Node current = HEAD;
    int count = 0;

    while (current != NULL) {
        count++;
        current = current->Next;
    }

    if (count > CACHE_SIZE) {
        current = HEAD;

        while (current->Next != NULL) {
            current = current->Next;
        }

        if (current->Prev != NULL) {
            current->Prev->Next = NULL;
        }

        free(current);
    }

    return newNode;
}


void printCache(){
    Node current = HEAD;
    while(current != NULL){
        // printf("%s\n",current->SS.path);
        printf("%d\n",current->SS.ssid);
        current = current->Next;
    }
}

int main() {
    HEAD=NULL;

    while(1){
        char path[255];
        int num;
        scanf("%s",path);
        scanf("%d",&num);
        SSInfo ss;
        ss.ssid = num;
        strcpy(ss.path,path);
        Node temp;
        temp=Search(path);
        if(temp != NULL){
            printf("%s\n",temp->SS.path);
            printf("%d\n",temp->SS.ssid);
            printf("Found\n");
        }
        else{
            printf("added\n");
            ADD(ss);
        }
        // if(HEAD!=NULL)  
        //     printf("%s\n",HEAD->SS.path);
        printCache(HEAD);
    }

    return 0;
}
