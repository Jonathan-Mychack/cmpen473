// Starter code for the page replacement project
#define _GNU_SOURCE 1
#define READ 0
#define READ_WRITE 1
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <sys/mman.h>
#include <signal.h>
#include "473_mm.h"

// Global Variables
void *start_of_vm;
int mem_size;
int num_pages;
int p_size;
int handling_policy;
int id_counter = 0x0000;
int clk = -2147483648;

typedef struct Node
{
    int id;
    int time_added;
    int page;
    int reference;
    int modified;
    int permissions;  //0 (defined READ) for R, 1 (defined READ_WRITE) for R+W
    int chance;
    struct Node *next;
} Node;

typedef struct CircularLinkedList
{
    Node *head;
    int length;
} CircularLinkedList;

CircularLinkedList *frames;

void init_list(CircularLinkedList *list)
{
    list->head = NULL;
    list->length = 0;
}

void add(CircularLinkedList *list, int page, int reference, int modified, int permissions, int chance)
{
    Node *newNode;
    newNode = (Node *)malloc(sizeof(Node));
    newNode->id = id_counter;
    id_counter += 0x1000;
    newNode->page = page;
    newNode->reference = reference;
    newNode->modified = modified;
    newNode->permissions = permissions;
    newNode->chance = chance;

    if (list->head == NULL)
    {
        newNode->next = newNode;
    }
    else
    {
        newNode->next = list->head;
        int count = 0;
        Node *temp;
        temp = list->head;
        while (count != list->length)
        {
            if (temp->id == 0x0000)
            {
                temp->next = newNode;
            }
            temp = temp->next;
            count++;
        }
    }
    
    list->head = newNode;
    list->length++;
}

void evict(CircularLinkedList *list, int page, int reference, int modified, int permissions, int chance)
{
    list->head->time_added = clk++;
    list->head->page = page;
    list->head->reference = reference;
    list->head->modified = modified;
    list->head->permissions = permissions;
    list->head->chance = chance;
}

void handler(int signal, siginfo_t *info, void *context)
{
    void *address = info->si_addr;
    int offset_from_start = (int)address - (int)start_of_vm;
    int page_number = offset_from_start / p_size;
    int page_offset = (int)address - ((int)start_of_vm + (page_number * p_size));

    ucontext_t *ucontext = context;
    unsigned long long error = ucontext->uc_mcontext.gregs[REG_ERR];
    unsigned long long read_or_write = (error >> 1) & 1;

    int instruction_type = read_or_write;
    
    Node *temp;
    temp = frames->head;
    int count = 0;
    int found = 0;
    while (count != frames->length)
    {
        if (temp->page == page_number)
        {
            found = 1;
        }
        count++;
        temp = temp->next;
    }

    if (found != 1)  //if page not found
    {
        count = 0;
        int added = 0;
        while (count != frames->length)
        {
            if (frames->head->page == -1)
            {
                added = 1;
                if (instruction_type == 0)
                {
                
                    evict(frames, page_number, 1, 0, READ, 0);
                    mm_logger(page_number, 0, -1, 0, frames->head->id|page_offset);
                    mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ);
                    break;
                }
                else
                {
                    evict(frames, page_number, 1, 1, READ_WRITE, 0);
                    mm_logger(page_number, 1, -1, 0, frames->head->id|page_offset);
                    mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ|PROT_WRITE);
                    break;
                }
            }
            frames->head = frames->head->next;
            count++;
        }

        //frames->head = frames->head->next;  //set the head back to where it's supposed to be once all of the null entries are filled
        if (!added)  //check if the current page has been added in the prior section, if it has, skip this part
        {
            if (handling_policy == 1)  //FIFO, case that the faulted page is not in a frame
            {
                temp = frames->head;
                count = 0;
                int earliest_time = 2147483647;
                while (count != frames->length)
                {
                    if (temp->time_added < earliest_time)
                    {
                        earliest_time = temp->time_added;
                    }

                    temp = temp->next;
                    count++;
                }

                count = 0;
                while (count != frames->length)
                {
                    int done = 0;
                    if (frames->head->time_added == earliest_time)
                    {
                        done = 1;
                        int evicted_page = frames->head->page;
                        int write_back = frames->head->modified;
                        mprotect(frames->head->page*p_size + start_of_vm, p_size, PROT_NONE);
                        if (instruction_type == 0)
                        {
                            evict(frames, page_number, 1, 0, READ, -1);
                            mm_logger(page_number, 0, evicted_page, write_back, frames->head->id|page_offset);
                            mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ);
                        }
                        else
                        {
                            evict(frames, page_number, 1, 1, READ_WRITE, -1);
                            mm_logger(page_number, 1, evicted_page, write_back, frames->head->id|page_offset);
                            mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ|PROT_WRITE);
                        }
                    }
                    
                    frames->head = frames->head->next;
                    count++;
                    if (done == 1)
                    {
                        break;
                    }
                }
            }
            else  //Third Chance, case that the faulted page is not in a frame
            {
                frames->head = frames->head->next;
                
                int isRead = 0, isWrite = 0;
                int isModified = 0;
                int evicted_page = -1, write_back = -1;
                
                if(read_or_write == 0)
                    isRead = 1;
                else
                    isWrite = 1;

                int done = 0;
                while(!done)
                {
                    // evict
                    if(((frames->head->modified == 0) && (frames->head->reference == 0)) || (frames->head->chance == 2)) 
                    {
                        done = 1;
                        evicted_page = frames->head->page;
                        write_back = frames->head->modified;
                        mprotect(frames->head->page*p_size + start_of_vm, p_size, PROT_NONE);
                        if(isWrite)
                        {
                            isModified = 1;
                            evict(frames, page_number, 1, isModified, READ_WRITE, 0);
                            mm_logger(page_number, 1, evicted_page, write_back, frames->head->id|page_offset);
                            mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ|PROT_WRITE);
                            break;
                        }
                    
                        if(isRead)
                        {
                            evict(frames, page_number, 1, isModified, READ, 0);
                            mm_logger(page_number, 0, evicted_page, write_back, frames->head->id|page_offset);
                            mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ);
                            break;
                        }
                    }
                    
                    // don't evict, yet
                    else if(frames->head->chance == 1 || frames->head->chance == 0) 
                    {                        
                        frames->head->reference = 0;
                        frames->head->chance++; 
                        mprotect(frames->head->page*p_size + start_of_vm, p_size, PROT_NONE);
                    }
                    frames->head = frames->head->next;
                }
            }
        }
    }
    else  //if page found
    {
        if (handling_policy == 1)  //FIFO, case that the faulted page is in a frame
        {
            temp = frames->head;  //we utilize temp because in this case we don't want to move the actual head (see project help video)
            count = 0;
            while (count != frames->length)
            {
                if (temp->page == page_number)
                {
                    temp->reference = 1;
                    if (instruction_type == 0)  //if the instruction is a read
                    {
                        if (temp->permissions == READ)  //maintain current permissions
                        {
                            mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ);
                        }
                        else  //maintain current permissions
                        {
                            mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ|PROT_WRITE);
                        }

                        mm_logger(page_number, 3, -1, 0, temp->id|page_offset);
                    }
                    else if (instruction_type == 1 && temp->permissions == READ)  //if the instruction is a write on a page with read only perms
                    {
                       temp->permissions = READ_WRITE;
                       temp->modified = 1;
                       mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ|PROT_WRITE);
                       mm_logger(page_number, 2, -1, 0, temp->id|page_offset);
                    }
                    else if (instruction_type == 1 && temp->permissions == READ_WRITE)  //if the instruction is a write on a page with read-write perms
                    {
                        mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ|PROT_WRITE);
                        mm_logger(page_number, 4, -1, 0, temp->id|page_offset);
                    }
                }
                
                temp = temp->next;
                count++;
            }
        }
        else  //Third Chance, case that the faulted page is in a frame
        {
            temp = frames->head;  //we utilize temp because in this case we don't want to move the actual head (see project help video)
            count = 0;
                
            int isRead = 0, isWrite = 0;
            int isModified = 0;
            int evicted_page = -1, write_back = -1;
            
            if(read_or_write == 0)
                isRead = 1;
            else
                isWrite = 1;

            while(count != frames->length)
            {
                if (temp->page == page_number)
                {
                    temp->reference = 1;
                    temp->chance = 0;
                    
                    if(isRead)
                    {
                        if (temp->permissions == READ)  //maintain current permissions
                        {
                            mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ);
                        }
                        else  //maintain current permissions
                        {
                            mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ|PROT_WRITE);
                        }
                        mm_logger(page_number, 3, -1, 0, temp->id|page_offset);
                    }
                    else if(isWrite && temp->permissions == READ)
                    {
                        temp->permissions = READ_WRITE;
                        temp->modified = 1;
                        mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ|PROT_WRITE);
                        mm_logger(page_number, 2, -1, 0, temp->id|page_offset);
                    }
                    else if(isWrite && temp->permissions == READ_WRITE)
                    {
                        mprotect(page_number*p_size + start_of_vm, p_size, PROT_READ|PROT_WRITE);
                        mm_logger(page_number, 4, -1, 0, temp->id|page_offset);
                    }
                }

                temp = temp->next;
                count++;
            }
        }
    }
}

void mm_init(void* vm, int vm_size, int n_frames, int page_size, int policy)
{
    start_of_vm = vm;
    mem_size = vm_size;
    num_pages = n_frames;
    p_size = page_size;
    handling_policy = policy;

    struct sigaction sa;
    sa.sa_flags = SA_SIGINFO;
    sa.sa_sigaction = &handler;
    
    sigaction(SIGSEGV, &sa, NULL);

    mprotect(start_of_vm, mem_size, PROT_NONE);

    frames = (CircularLinkedList *)malloc(sizeof(CircularLinkedList));
    init_list(frames);
    for (int i = 0; i < num_pages; i++)
    {
        add(frames, -1, -1, -1, -1, -1);
    }

    Node *temp;
    temp = frames->head;
    int count = 0;
    int reverse_id = 0x0000;
    while (count != frames->length)
    {
        temp->id = reverse_id;
        reverse_id += 0x1000;
        count++;
        temp = temp->next;
    }
}
