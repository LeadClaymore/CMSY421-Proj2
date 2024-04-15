#include <stdio.h>
#include <stdlib.h>
#include <linux/kernel.h>
#include <linux/uaccess.h>
#include <linux/errno.h>
#include <linux/syscalls.h>

struct QueueNode {
    unsigned char *msg;
    long len;

    struct QueueNode* prev;
    struct QueueNode* next;
};

struct Queue{ 
    unsigned char id;
    struct QueueNode* start;
    struct QueueNode* end;
    struct Queue* linkedQueue;
};

struct BSTNode {
    unsigned char key;
    struct Queue* queue;
    struct BSTNode* left;
    struct BSTNode* right;
};

struct BSTNode* publicRoot = NULL;

struct BSTNode* searchNodes(struct BSTNode* root, unsigned long toFind) {
    if (root == NULL || root->key == toFind){
        //return null or found BSTNode
        return root;
    } else if(root->key < toFind) {
        //if greater then toFind return right search
        return searchNodes(root->right, toFind);
    }
    //if neither return left search
    return searchNodes(root->left, toFind);
}

struct BSTNode* findMin(struct BSTNode* root) {
    //if its the end return the root
    if(root == NULL){
        return root;
    }
    //else return the left (smaller) child
    return findMin(root->left);
}

struct BSTNode* findMax(struct BSTNode* root) {
    //if its the end return the root
    if(root == NULL){
        return root;
    }
    //else return the right (larger) child
    return findMin(root->right);
}

struct BSTNode* findClose(struct BSTNode* root, unsigned long toFind) {
    if(root == NULL || root->key == toFind)
    {
        //already exists
        return NULL;
    }
    else if(root->key < toFind && root->right != NULL)
    {
        //we need to go deeper right
        return findClose(root->right, toFind);
    }
    else if(root->key > toFind && root->left != NULL)
    {
        //we need to go deeper left
        return findClose(root->left, toFind);
    }
    else if(root->key < toFind && root->right == NULL)
    {
        //found it
        return root;
    }
    else if(root->key > toFind && root->left == NULL)
    {
        //found it
        return root;
    }
    //case error
    return NULL;
}

void insertNode(struct BSTNode* root, unsigned long key, struct Queue* q) {
    struct BSTNode* newNode = (struct BSTNode*)malloc(sizeof(struct BSTNode));
    newNode->key = key;
    newNode->left = NULL;
    newNode->right = NULL;
    if(q == NULL) {
        newNode->queue = (struct Queue*)malloc(sizeof(struct Queue));
        newNode->queue->id = key;
        newNode->queue->end = NULL;
        newNode->queue->start = NULL;
        newNode->queue->linkedQueue = NULL;
    } else {
        newNode->queue = q;
    }

    struct BSTNode* closest = findClose(root, key);
    if(closest == NULL)
    {
        return;
    } else {
        if(closest->key > key)
        {
            closest->left = newNode;
            return;
        } else {
            closest->right = newNode;
            return;
        }
    }
}

void printTree(struct BSTNode* root) {
    if (root == NULL) {
        printf("NULL");
        return;
    }

    printf("%d", root->key);
    if (root->left != NULL || root->right != NULL) {
        printf("( ");
        printTree(root->left);
        printf(", ");
        printTree(root->right);
        printf(" ) ");
    }
}

void recursiveReInsert(struct BSTNode* root, struct BSTNode* reInsert, unsigned long keepNode) {
    if(reInsert == NULL)
    {
        return;
    }

    recursiveReInsert(root, reInsert->left, 1);
    recursiveReInsert(root, reInsert->right, 1);

    if(keepNode != 0){
        insertNode(root, reInsert->key, reInsert->queue);
        free(reInsert);
    } else {
        reInsert->left = NULL;
        reInsert->right = NULL;
        deleteAll(reInsert);
    }

}

struct BSTNode* deleteNode(struct BSTNode* root, unsigned long toDelete)
{
    if(root == NULL)
    {
        return NULL;
    } else if(root->left != NULL && root->left->key == toDelete)
    {
        struct BSTNode* deleted = root->left;
        root->left = NULL;
        return deleted;

    } else if(root->right != NULL && root->right->key == toDelete)
    {
        struct BSTNode* deleted = root->right;
        root->right = NULL;
        return deleted;
    } else if(root->key < toDelete) {
        return deleteNode(root->right, toDelete);
    } else if(root->key > toDelete) {
        return deleteNode(root->left, toDelete);
    }

    printf("error deleting %d", toDelete);
    return NULL;
}

void deleteAll(struct BSTNode* root)
{
    if(root != NULL) {
        deleteAll(root->left);
        deleteAll(root->right);
    }

    deleteQueue(root->queue);
    free(root);
}

void deleteQueueNode(struct QueueNode* q) {
    if(q->next != NULL) {
        deleteQueue(q->next);
    }
    free(q->msg);
    free(q);
}

void deleteQueue(struct Queue* q) {
    deleteQueueNode(q->start);
    deleteQueueNode(q->linkedQueue->start);
    free(q);
}

long mailbox_init(void) {
    publicRoot = NULL;
    return 0;
}

long mailbox_shutdown(void) {
    deleteAll(publicRoot);
    return 0;
}

long mailbox_create(unsigned long id) {
    if (publicRoot == NULL) {
        publicRoot = (struct BSTNode*)malloc(sizeof(struct BSTNode));
        publicRoot->key = id;
        publicRoot->left = NULL;
        publicRoot->right = NULL;
        publicRoot->queue = (struct Queue*)malloc(sizeof(struct Queue));
        publicRoot->queue->id = id;
        publicRoot->queue->end = NULL;
        publicRoot->queue->start = NULL;
        publicRoot->queue->linkedQueue = NULL;
        return 0;
    }
    insertNode(publicRoot, id, NULL);
    return 0;
}

long mailbox_destroy(unsigned long id) {
    if(publicRoot->key != id){
        recursiveReInsert(publicRoot, deleteNode(publicRoot, id), 0);
    } else {
        if(publicRoot->left != NULL)
        {
            struct BSTNode* temp = publicRoot;
            publicRoot = publicRoot->left;
            temp->left = NULL;
            recursiveReInsert(publicRoot, temp->right, 1);
            temp->right = NULL;
            deleteAll(temp);
        } else if(publicRoot->right != NULL) {
            struct BSTNode* temp = publicRoot;
            publicRoot = publicRoot->right;
            temp->right = NULL;
            recursiveReInsert(publicRoot, temp->left, 1);
            temp->left = NULL;
            deleteAll(temp);
        } else {
            deleteAll(publicRoot);
        }
    }
}

///*
long mailbox_add(unsigned long id, unsigned char __user *msg, long len) {
    struct BSTNode* activeQueue = searchNodes(publicRoot, id);
    if(activeQueue != NULL && activeQueue->queue != NULL) {
        if(activeQueue->queue->end == NULL)
        {
            activeQueue->queue->end = (struct QueueNode*)malloc(sizeof(struct QueueNode));
            //Allocate kernel-space memory for the message data
            activeQueue->queue->end->msg = kmalloc(len, GFP_KERNEL);
            
            // Copy the message from user space to kernel space
            if (copy_from_user(activeQueue->queue->end->msg, msg, len)) {
                kfree(activeQueue->queue->end->msg);  // Free the data memory
                kfree(activeQueue->queue->end);        // Free the node memory
                return -EFAULT;         // Return -EFAULT if copying fails
            }
            activeQueue->queue->start = activeQueue->queue->end; // TODO CHECK IF THIS WORKS
            activeQueue->queue->end->len = len;
            activeQueue->queue->end->next = NULL;
            activeQueue->queue->end->prev = NULL;
        } else {
            struct QueueNode* temp = activeQueue->queue->end;
            activeQueue->queue->end = (struct QueueNode*)malloc(sizeof(struct QueueNode));
            //Allocate kernel-space memory for the message data
            activeQueue->queue->end->msg = kmalloc(len, GFP_KERNEL);
            
            // Copy the message from user space to kernel space
            if (copy_from_user(activeQueue->queue->end->msg, msg, len)) {
                kfree(activeQueue->queue->end->msg);  // Free the data memory
                kfree(activeQueue->queue->end);        // Free the node memory
                activeQueue->queue->end = temp;
                return -EFAULT;         // Return -EFAULT if copying fails
            }
            activeQueue->queue->start = activeQueue->queue->end; // TODO CHECK IF THIS WORKS
            activeQueue->queue->end->len = len;
            activeQueue->queue->end->next = NULL;
            activeQueue->queue->end->prev = temp;
            temp->next = activeQueue->queue->end;
        }
        return 0;
    } else {
        return -1;
    }
}
//*/

long mailbox_delete(unsigned long id) {
    struct BSTNode* temp = searchNodes(publicRoot, id);
    if(temp != NULL) {
        struct QueueNode* tempQNode = temp->queue->end;
        if(tempQNode->prev != NULL) {
            temp->queue->end = tempQNode->prev;
            tempQNode->prev->next = NULL;
            free(tempQNode);
            return 0;
        } else {
            return -1;
        }
    } else {
        return -1;
    }
}

long mailbox_send(unsigned long from, unsigned long to){
    struct BSTNode* fromQueue = searchNodes(publicRoot, from);
    struct BSTNode* toQueue = searchNodes(publicRoot, to);

    //if neither are null and the fromQueue is linked to the toQueue
    if(fromQueue != NULL && toQueue != NULL && fromQueue->queue->linkedQueue == toQueue->queue)
    {

    }
}

long mailbox_recv(unsigned long id, unsigned char __user *msg, long len) {
    struct BSTNode* activeQueue = searchNodes(publicRoot, id);
    if(activeQueue != NULL && activeQueue->queue != NULL && activeQueue->queue->start != NULL && activeQueue->queue->start->msg != NULL) {
        if (copy_to_user(msg, activeQueue->queue->start->msg, bytes_to_copy)) {
            return -EFAULT;
        }
        struct QueueNode* temp = activeQueue->queue->start;
        activeQueue->queue->start = temp->next;
        activeQueue->queue->start->prev = NULL;
        deleteQueueNode(temp);
        return 0;
    } else {
        return -1;
    }
}

long message_count(unsigned long id) {
    struct BSTNode* activeQueue = searchNodes(publicRoot, id);
    if(activeQueue != NULL && activeQueue->queue != NULL && activeQueue->queue->start != NULL) {
        struct QueueNode * elem = activeQueue->queue->start;
        long count = 0;
        while(elem != NULL)
        {
            count++;
            elem = elem->next;
        }
    } else {
        return -1;
    }
}

long message_length(unsigned long id) {
    struct BSTNode* activeQueue = searchNodes(publicRoot, id);
    if(activeQueue != NULL && activeQueue->queue != NULL && activeQueue->queue->start != NULL) {
        return activeQueue->queue->start->len;
    } else {
        return -1;
    }
}
