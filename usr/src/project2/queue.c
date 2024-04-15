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
                kfree(activeQueue->queue->end->msg);
                kfree(activeQueue->queue->end);
                return -EFAULT;
            }
            activeQueue->queue->start = activeQueue->queue->end;
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
                kfree(activeQueue->queue->end->msg);
                kfree(activeQueue->queue->end);
                activeQueue->queue->end = temp;
                return -EFAULT;
            }
            activeQueue->queue->start = activeQueue->queue->end;
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

/*
Clayton Gorman Ross
CMSC 421, spring 2024

Project Overview
Introduction: Briefly describe the purpose of the project and its main functionality.
This was meant to implement an queue stored within a BST, the queue was meant to hold a message and its message
the important part of the project is it was in kernel space

Objective: Clearly state what the project is intended to achieve, highlighting the implementation of the Graph data structure and the associated system calls. This will tell me if you truly understand the project.
I do not truely understand the graph structure but what I believe was meant to happen was a queue was made in kernel space, in this example stored in a BST, and it was meant to pass data to and from userspace

Contact: Provide contact information or steps for getting help if someone is having trouble with the project.
contact TA or go to office hours

Installation and Setup
Environment Setup: Detail the steps for setting up the development environment, including any dependencies or tools required.
using the kernel provided in project 0 modifiy it to use the neccicary syscalls

Building the Kernel: Provide instructions on how to build and install your modified Linux kernel.
from what I understand the steps from project 0:
make mrproper
make xconfig
make bindeb-pkg
to be honest I did not test it

Kernel Modifications
System Calls Implemented: List the new system calls you've added to the kernel, with a brief description of each.

long mailbox_init(void): Initializes the queue system, setting up the initial state of the graph. You may initialize the graph by adding the root node. Return 0 on success.
long mailbox_shutdown(void): Shuts down the queue system, deleting all existing queues and any messages contained therein. Returns 0 on success.
long mailbox_create(unsigned long id): Creates a new queue with the given id if it does not already exist (no duplicates are allowed). Returns 0 on success or an appropriate error on failure. If  a negative id or an id of (2­­64 - 1) is passed, this is considered an invalid ID and an appropriate error shall be returned.
long mailbox_destroy(unsigned long id): Deletes the queue identified by id if it exists. If the queue has any messages stored in it, they should be deleted. Returns 0 on success or an appropriate error code on failure.
long mailbox_connect(unsigned long from, unsigned long to, unsigned long weight): Connects two queues. Queues connected in this way can send messages from from to to, but not vice versa. A queue may not be connected to itself, be connected to the same queue twice, nor may a connection have a weight less than 1 or equal to (264 – 1). Returns 0 on success or an appropriate error code on failure. 
long mailbox_disconnect(unsigned long from, unsigned long to): Disconnects two queues that have a connection. Returns 0 on success or an appropriate error code on failure.
long mailbox_add(unsigned long id, unsigned char __user *msg, long len): Adds a new message to the queue identified by id if it exists. An appropriate return code is issued if the queue does not exist.
long mailbox_delete(unsigned long id): Deletes the oldest added message in the mailbox identified by id if it exists. This is a FIFO data structure - hence why we are deleting the oldest message. HINT: Keep in mind all the things you have to do for this one - it's not just a matter of just deleting a node in the queue. What other data within your mailbox data structure (not the graph data structure, just one single mailbox) gets affected by the deletion other than just the message node in the FIFO queue? An appropriate return code is issued if the queue is empty or does not exist.
long mailbox_send(unsigned long from, unsigned long to): Sends a new message from the queue identified by from to the queue identified by to if they both exist and are connected. You must check, in an efficient manner, whether the two nodes are connected. Returns 0 on success or an appropriate error code on failure.
long mailbox_recv(unsigned long id, unsigned char __user *msg, long len): Reads the first message that is in the mailbox identified by id if it exists, storing either the entire length of the message or len bytes to the user-space pointer msg, whichever is less. The entire message is then removed from the mailbox (even if len was less than the total length of the message). Returns the number of bytes copied to the user space pointer on success or an appropriate error code on failure. 
long message_count(unsigned long id): Print the number of messages in the queue to the system log identified by id if it exists. Returns an appropriate error code on failure.
long message_length(unsigned long id): Retrieves the length (in bytes) of the oldest message pending in the mailbox identified by id, if it exists. Returns the number of bytes in the oldest pending message in the mailbox on success, or an appropriate error code on failure.

Data Structures: Describe the data structures or modifications to existing structures you implemented. 
a bst that contaions queues that contain a message each

Locking Mechanisms: Explain the locking mechanisms used, including the rationale behind the choice of each mechanism.
did not use any prob should have

User-space Driver Programs
Driver Overview: Offer an overview of the user-space driver programs, explaining their purpose in testing the system calls.
idk

Building Drivers: Include a step-by-step guide to building the driver programs using the provided Makefile.
no idea does not exist

Running Tests: Describe how to run each test program, including the expected outputs and any command-line arguments or flags.
no idea does not exist

Testing Strategy
General Strategy: Outline your approach to testing the system calls, including how you handle normal operations and error cases.
did not test the program

Test Cases: Provide a list of test cases, explaining what each one covers and why it’s important.
no idea does not exist

Troubleshooting
Common Issues: List any common issues that might arise and how to resolve them.
no idea did not test

References 
Please state all outside resources used during the project. This would include websites or textbooks used. Also, please include output of any llm used.
for parts of the userspace to kernel space code I used chatGPT for the outline of what to do and c functions to use
