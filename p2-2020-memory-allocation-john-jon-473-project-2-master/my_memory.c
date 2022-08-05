////////
// Names: John Anderson & Jonathan Mychack
// Last Updated: 11-8-20 @ 9:41 PM
////////

// Include files
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define  N_OBJS_PER_SLAB  64
#define MIN_SIZE 1024
#define MAX_SIZE 1048576
#define H_F_SIZE 4
#define TRUE 1
#define FALSE 0

int bidCounter = 0; 
int NodeSlab_counter = 0;
int returnCounter = 0; // counts the number of returns of my_malloc

// Global variables
int allocation_type;
int memory_size;
void* memory_location;

// Functional prototypes
void setup( int malloc_type, int mem_size, void* start_of_memory );
void *my_malloc( int size );
void my_free( void *ptr );

// General Purpose Functions
int power(int base, int exp) 
{
    if (exp == 0)
        return 1;
    else if (exp % 2)
        return base * power(base, exp - 1);
    else 
    {
        int temp = power(base, exp / 2);
        return temp * temp;
    }
}

// converts address to form of output
// for debugging purposes
// ex 0x7ffff7b76010 goes to 4
void * addressReadable(void * input)
{
    return (input - memory_location);
}

///// Default Linked List Structure and Functions //////////////////////////////////////////////////////////////////
typedef struct Node
{
    int bid[11];
    char rel_pos;
    void *start_address;
    void *end_address;
    struct Node *next;
    struct Node *prev;
} Node;

typedef struct
{
    struct Node *head;
    struct Node *tail;
    int length;
} LinkedList;

///// Slab Linked List and Functions ///////////////////////////////////////////////////////////////////////////////
// slab 
// each slab needs 64 start address and end address
typedef struct NodeSlab
{
    int NodeSlab_ID;
    int bid[11]; // each NodeSlab needs a buddy chunk
    int slab_size;
    int obj_size;
    unsigned long value;
    void *start_address; // start address of slab
    void *end_address;
    void *BMStartEndAddr[64][2]; // stores start and end addr of each chunk

    struct NodeSlab *next;
    struct NodeSlab *prev;
} NodeSlab; // ? NodeHole

typedef struct
{
    struct NodeSlab *head; // doesn't need all values in NodeSlab 
    struct NodeSlab *tail; // TODO: ? create separate node
    int length;
} LinkedListSlab;

LinkedListSlab *availableHolesSlab[11];
LinkedListSlab *notAvailableHolesSlab[11];

///// Descriptor Table Linked List and Functions ///////////////////////////////////////////////////////////////////
typedef struct NodeDT
{
    int slab_size; // size of slab
    int obj_size; // size of object aka type
    int numObjs;
    int numUsed;
    struct NodeDT *next;
    struct NodeDT *prev;
    LinkedListSlab *bitmapList;
} NodeDT; // NodeDT for each slab type

typedef struct     
{
    struct NodeDT *head;
    struct NodeDT *tail;
    int length;
} LinkedListDT;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

// 11 indices
// 0 - 1KB; 1 - 2KB; 2 - 4KB; 3 - 8KB; 4 - 16KB
// 5 - 32KB; 6 - 64KB; 7 - 128KB; 8 - 256KB; 9 - 512KB; 10 - 1MB

LinkedList *availableHoles[11];
LinkedList *notAvailableHoles[11];

void init_list(LinkedList *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;
}

void init_slabList(LinkedListSlab *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;   
}

void init_descTable(LinkedListDT *list)
{
    list->head = NULL;
    list->tail = NULL;
    list->length = 0;   
}

// adds a node to the end of the linked list
void add(LinkedList *list, char *rel_pos, void *start_address, void *end_address)
{
    Node *newNode = malloc(sizeof(Node));
    for (int i = 0; i < 11; i++)
    {
        newNode->bid[i] = -1;
    }
    newNode->rel_pos = rel_pos;
    newNode->start_address = start_address;
    newNode->end_address = end_address;
    newNode->next = NULL;
    newNode->prev = NULL;

    if (list->head == NULL)
    {
        list->head = newNode;
        list->tail = newNode;
    }
    else
    {
        list->tail->next = newNode;
        newNode->prev = list->tail;
        list->tail = newNode;
    }
    
    list->length++;
}


// type is memory chunk size
void addSlab(LinkedListSlab *list, int type, int Nodeslab_ID, void *start_address, void *end_address)
{
    NodeSlab *newNode = (NodeSlab*)malloc(sizeof(NodeSlab));
    for (int i = 0; i < 11; i++)
    {
        newNode->bid[i] = -1;
    }
    for(int i = 0; i < 64; i++)
    {
        
        newNode->BMStartEndAddr[i][0] = ((start_address + i*(type+4)));
        
         

        newNode->BMStartEndAddr[i][1] = ((start_address + i*(type+4)+type));
         
    }
     
    newNode->start_address = start_address;
    newNode->end_address = end_address;
    newNode->next = NULL;
    newNode->prev = NULL;
    newNode->NodeSlab_ID = Nodeslab_ID;
    newNode->obj_size = type;


    if (list->head == NULL)
    {
        list->head = newNode;
        list->tail = newNode;
    }
    else
    {
        list->tail->next = newNode;
        newNode->prev = list->tail;
        list->tail = newNode;
    }
    
    list->length++;
}

// deletes the selected node from the linked list
void delete(LinkedList *list, Node *node)
{
    if (list->length == 0)
    {
        printf("List is already empty");
    }
    else if (list->length == 1)
    {
        list->head = NULL;
        list->tail = NULL;
        free(node);
        list->length--;
    }
    else if (node->start_address == list->tail->start_address)
    {
        Node *prevNode;
        prevNode = node->prev;
        prevNode->next = NULL;
        list->tail = prevNode;
        free(node);
        list->length--;
    }
    else if (node->start_address == list->head->start_address)
    {
        Node *nextNode;
        nextNode = node->next;
        nextNode->prev = NULL;
        list->head = nextNode;
        free(node);
        list->length--;
    }
    else
    {
        Node *temp;
        temp = list->head;
        while (temp != NULL)
        {
            if (temp->start_address == node->start_address)
            {
                Node *nextNode, *prevNode;
                nextNode = node->next;
                prevNode = node->prev;
                prevNode->next = nextNode;
                nextNode->prev = prevNode;
                free(node);
                list->length--;
            }
            temp = temp->next;
        }
    }
}

//adds a buddy id to the inputted node
void give_bid(LinkedList *list, Node *node, int bid)
{
    Node *temp;
    temp = list->head;
    while (temp != NULL)
    {
        if (temp->start_address == node->start_address)
        {
            break;
        }
        temp = temp->next;
    }
    for (int i = 0; i < 11; i++)
    {
        if (temp->bid[i] == -1)
        {
            temp->bid[i] = bid;
            break;
        }
    }
}

//removes the given buddy id from the inputted node (typically the most recent)
void remove_bid(LinkedList *list, Node *node, int bid)
{
    Node *temp;
    temp = list->head;
    while (temp != NULL)
    {
        if (temp->start_address == node->start_address)
        {
            break;
        }
        temp = temp->next;
    }
    for (int i = 0; i < 11; i++)
    {
        if (temp->bid[i] == bid)
        {
            temp->bid[i] = -1;
        }
    }
}

//gets the most recent buddy id from the inputted node
int get_bid(LinkedList *list, Node *node)
{
    Node *temp;
    temp = list->head;
    while (temp != NULL)
    {
        if (temp->start_address == node->start_address)
        {
            break;
        }
        temp = temp->next;
    }
    for (int i = 0; i < 11; i++)
    {
        if (temp->bid[i] == -1 && i != 0)
        {
            return(temp->bid[i-1]);
        }
        else if (i == 10 && temp->bid[i] != -1)
        {
            return(temp->bid[i]);
        }        
    }
    printf("No valid bid found\n");
}

//prints notable information from the content of the inputted list
void print_list(LinkedList *list)
{
    if (list->length == 0)
    {
        printf("List is empty\n\n");
    }
    else
    {
        Node *temp;
        temp = list->head;
        int node_count = 0;
        while (temp != NULL)
        {
            printf("Node %i:\nbid = %i\nstart_address = %i\nend_address = %i\nLinkedList length = %i\n\n", node_count, temp->bid, temp->start_address, temp->end_address, list->length);
            node_count++;
            temp = temp->next;
        }
    }
}

void deleteSlab(LinkedListSlab *list, NodeSlab *node)
{
    if (list->length == 0)
    {
        printf("List is already empty");
    }

    else if (list->length == 1)
    {
        list->head = NULL;
        list->tail = NULL;
        free(node);
        list->length--;
    }
    else if (node->NodeSlab_ID == list->tail->NodeSlab_ID)
    {
        NodeSlab *prevNode;
        prevNode = node->prev;
        prevNode->next = NULL;
        list->tail = prevNode;
        free(node);
        list->length--;
    }
    else if (node->NodeSlab_ID == list->head->NodeSlab_ID)
    {
        NodeSlab *nextNode;
        nextNode = node->next;
        nextNode->prev = NULL;
        list->head = nextNode;
        free(node);
        list->length--;
    }

    else
    {
        NodeSlab *temp;
        temp = list->head;
        while (temp != NULL)
        {
            if (temp->NodeSlab_ID == node->NodeSlab_ID)
            {
                NodeSlab *nextNode, *prevNode;
                nextNode = node->next;
                prevNode = node->prev;
                prevNode->next = nextNode;
                nextNode->prev = prevNode;
                free(node);
                list->length--;
            }
            temp = temp->next;
        }
    }
}

LinkedListDT *slabDescTable;

//adds a buddy id to the inputted node
void give_bid_slab(LinkedListSlab *list, NodeSlab *node, int bid)
{
    NodeSlab *temp;
    temp = list->head;
    while (temp != NULL)
    {
        if (temp->NodeSlab_ID == node->NodeSlab_ID)
        {
            break;
        }
        temp = temp->next;
    }
    for (int i = 0; i < 11; i++)
    {
        if (temp->bid[i] == -1)
        {
            temp->bid[i] = bid;
            break;
        }
    }
}

//removes the given buddy id from the inputted node (typically the most recent)
void remove_bid_slab(LinkedListSlab *list, NodeSlab *node, int bid)
{
    NodeSlab *temp;
    temp = list->head;
    while (temp != NULL)
    {
        if (temp->NodeSlab_ID == node->NodeSlab_ID)
        {
            break;
        }
        temp = temp->next;
    }
    for (int i = 0; i < 11; i++)
    {
        if (temp->bid[i] == bid)
        {
            temp->bid[i] = -1;
        }
    }
}

//gets the most recent buddy id from the inputted node
int get_bid_slab(LinkedListSlab *list, NodeSlab *node)
{
    NodeSlab *temp;
    temp = list->head;
    while (temp != NULL)
    {
        if (temp->NodeSlab_ID == node->NodeSlab_ID)
        {
            break;
        }
        temp = temp->next;
    }
    for (int i = 0; i < 11; i++)
    {
        if (temp->bid[i] == -1 && i != 0)
        {
            return(temp->bid[i-1]);
        }
        else if (i == 10 && temp->bid[i] != -1)
        {
            return(temp->bid[i]);
        }        
    }
    printf("No valid bid found\n");
}

// creates a new descriptor
// creates a new slab
void addDT(LinkedListDT *list, int obj_size, int slab_size, void*start_address, void* end_address)
{
    NodeDT *newNode = (NodeDT*)malloc(sizeof(NodeDT));
    
    newNode->next = NULL;
    newNode->prev = NULL;
    newNode->slab_size = slab_size;
    newNode->obj_size = obj_size;
    newNode->numObjs += 64;
    newNode->numUsed += 1;
    newNode->bitmapList = (LinkedListSlab*)malloc(sizeof(LinkedListSlab));
    init_slabList(newNode->bitmapList);
    
    if (list->head == NULL)
    {
        list->head = newNode;
        list->tail = newNode;
    }
    else
    {
        list->tail->next = newNode;
        newNode->prev = list->tail;
        list->tail = newNode;
    }
    
    list->length++;

    addSlab(slabDescTable->tail->bitmapList, obj_size, NodeSlab_counter++, start_address, end_address);
    
    slabDescTable->tail->bitmapList->tail->value = 0;
    
}

void deleteDT(LinkedListDT *list, NodeDT *node)
{
    if (list->length == 0)
    {
        printf("List is already empty");
    }
    else if (list->length == 1)
    {
        list->head = NULL;
        list->tail = NULL;
        free(node);
        list->length--;
    }
    else if (node->obj_size == list->tail->obj_size)
    {
        NodeDT *prevNode;
        prevNode = node->prev;
        prevNode->next = NULL;
        list->tail = prevNode;
        free(node);
        list->length--;
    }
    else if (node->obj_size == list->head->obj_size)
    {
        NodeDT *nextNode;
        nextNode = node->next;
        nextNode->prev = NULL;
        list->head = nextNode;
        free(node);
        list->length--;
    }
    else
    {
        NodeDT *temp;
        temp = list->head;
        while (temp != NULL)
        {
            if (temp->obj_size == node->obj_size)
            {
                NodeDT *nextNode, *prevNode;
                nextNode = node->next;
                prevNode = node->prev;
                prevNode->next = nextNode;
                nextNode->prev = prevNode;
                free(node);
                list->length--;
            }
            temp = temp->next;
        }
    }
}

// searches for first free memory chunk in all the slabs created
// for a given type
// needs to search all previous slabs in the case that a prior chunk is freed
// if there are no openings in any of the slabs, we need to call initSlab()
// to create a new slab for that object


// returns -1 if type (obj_size) is not found
//               create new NodeDT and NodeSlab, allocate in that node
//               initDescriptor
//               
// returns  0 if type (obj_size) is found but all chunks are full
//               create a new slab for that type
//               create_new_slab
//
// returns  1 if type (obj_size) is found and there is an opening
//               allocate at that chunk
int freeChunkExists(int type)
{
    // to do: I need to find the index i in slabDescTable whose
    // type matches the index type, so I can find the first free bit in
    // the bitmapList for the correct index to allocate the chunk needed
    // for the respective object size
    NodeDT *temp;
    temp = slabDescTable->head;
    int done = 0;
    while(temp != NULL && !done)
    {
        if(temp->obj_size == type) // found the right object type
        {
            done=1;
            NodeSlab *temp2;
            temp2 = temp->bitmapList->head;
            
            if(temp2 == NULL)
            {
                return -1;
            }
            while(temp2 != NULL)
            {
                if(temp2->value != 0xFFFFFFFFFFFFFFFF)
                {
        
                    return 1;
                }
                temp2 = temp2->next; // else, keep searching
            }
            return 0;
        }
        temp = temp->next;
    }
    return -1;
}

void* allocateChunkInSlab(LinkedListDT *list, int type)
{

    // there is an opening in a bitmap
    // no need to create a new slab
    
    // Note:
    // should have start address of slab, since the slab will 
    // already be allocated
    // can calculate the start address of each memory chunk based off
    // the start address of the slab and the size of the memory chunks
    

    // slabDescTable[i] is the type so a size 600 would have an index
    // in the slabDescTable, but it also may have more than 1 slab
    // and more than one bitmap
    // bit map List is a linked list of the bitmaps for each object type
    int BMindex; // first free index in bitMap
    int slab; // first free slab in slabDescTable
    int objectIndex;

    // to do: I need to find the index i in slabDescTable whose
    // type matches the index type, so I can find the first free bit in
    // the bitmapList for the correct index to allocate the chunk needed
    // for the respective object size
    NodeDT *temp;
    temp = slabDescTable->head;
    int done = 0;
    while(temp != NULL && !done)
    {
        if(temp->obj_size == type) // found the right object type
        {
            NodeSlab *temp2;
            temp2 = temp->bitmapList->head;
            int found = 0;
            int index;
            done = 1;
            while(temp2 != NULL)
            {
                for(int i = 63; i >-1; i--)
                {
                    // 11111....111110
                    if((temp2->value >> i) & 1 == 0)
                    {
                        
                        found = 1;
                        temp2->value |= 1UL<<i;
                         
                        // update value of bit map
                        index = i;
                        temp2->BMStartEndAddr[i][0] = temp2->start_address + i*(temp2->obj_size+4);
                        temp2->BMStartEndAddr[i][1] = temp2->start_address + (i+1)*(temp2->obj_size+4)-4;
                        return temp2->BMStartEndAddr[i][0];
                        
                        // found an open memory chunk at index i

                        // set start address end address here
                    }
                    
                }
                temp2 = temp2->next; // else, keep searching
            }

            if(!found)
            {
                //
                printf("semantic error has occurred\n we couldn't find an opening, although one should exist \n");
            }

        }
        temp = temp->next;
    }
}
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////
//
// Function     : setup
// Description  : initialize the memory allocation system
//
// Inputs       : malloc_type - the type of memory allocation method to be used [0..3] where
//                (0) Buddy System
//                (1) Slab Allocation

void setup( int malloc_type, int mem_size, void* start_of_memory )
{
    allocation_type = malloc_type;
    memory_size = mem_size;
    memory_location = start_of_memory;
    //lastEndAddress = memory_location-1;
    

    if (malloc_type == 0)  //Buddy System
    {
        for (int i = 0; i < 11; i++)
        {
            availableHoles[i] = (LinkedList*)malloc(sizeof(LinkedList));
            notAvailableHoles[i] = (LinkedList*)malloc(sizeof(LinkedList));

            init_list(availableHoles[i]);
            init_list(notAvailableHoles[i]);
        }
        add(availableHoles[10], NULL, memory_location + 4, (memory_location + memory_size) - H_F_SIZE - 1+4);
        give_bid(availableHoles[10], availableHoles[10]->tail, bidCounter);
        bidCounter++;
        //if mem_size isn't 1MB, we have to dynamically allocate the amount of indices present in the linked list, the indicies are be log(mem_size)-9 in reverse
    }

    else  // Slab Allocation
    {
        for (int i = 0; i < 11; i++)
        {
            availableHolesSlab[i] = (LinkedListSlab*)malloc(sizeof(LinkedListSlab));
            notAvailableHolesSlab[i] = (LinkedListSlab*)malloc(sizeof(LinkedListSlab));

            init_slabList(availableHolesSlab[i]);
            init_slabList(notAvailableHolesSlab[i]);
        }
        slabDescTable = (LinkedListDT*)malloc(sizeof(LinkedListDT));
        init_descTable(slabDescTable);
        addSlab(availableHolesSlab[10], 0, NodeSlab_counter++, memory_location + 8, (memory_location + memory_size) - H_F_SIZE - 1+4);
        give_bid(availableHolesSlab[10], availableHolesSlab[10]->tail, bidCounter);
        bidCounter++;
    }
    
}

////////////////////////////////////////////////////////////////////////////
//
// Function     : my_malloc
// Description  : allocates memory segment using specified allocation algorithm
//
// Inputs       : size - size in bytes of the memory to be allocated
// Outputs      : -1 - if request cannot be made with the maximum mem_size requirement

void *my_malloc( int size )
{

    if (memory_size < MIN_SIZE || memory_size > MAX_SIZE)  //check if memory size out of bounds
    {
        return(-1);
    }

    if(allocation_type == 1) // slab
    {

        // calculating size for our slab
        // may not need a slab, depending on if a slab already
        // exists
        int slabSizeMin = 64*(size + 4)+4;
        
        int newSize = MAX_SIZE;

        // first thing we need to do is round up to a power of two
        while(newSize >= slabSizeMin && newSize>=1024)
        {
            newSize /= 2;
        }
        newSize*=2; // this is the slab size rounded to a power of two

        if(newSize > 1024*1024/2) // since we need two buddies
        {
            return -1;
        }
        
        int slab_alloc_type = freeChunkExists(size);

        int notFound = TRUE;
        int index = 0;
        while(notFound)
        {
            if(pow(2, index+10) == newSize)
            {
                notFound = FALSE; // found the index
            }
            index++;
        }
        index--; // index we are looking for a memory chunk at

        if(slab_alloc_type == -1) // obj is not found in DescTable
        {
            // allocate a hole for a slab

            if(availableHolesSlab[index]->head != NULL)
            {
                // we have our chunk ready at the current index
                NodeSlab *temp;
                temp = availableHolesSlab[index]->head;
                int lowest_address = 2147483647;

                while (temp != NULL)
                {
                    if ((int)temp->start_address < lowest_address)
                    {
                        lowest_address = (int)temp->start_address;
                    }
                    temp = temp->next;
                }
                
                temp = availableHolesSlab[index]->head;
                while (temp != NULL)
                {
                    if ((int)temp->start_address == lowest_address)
                    {
                        break;
                    }
                    temp = temp->next;
                }
                void *startAddress = temp->start_address;
                void *endAddress = startAddress + newSize -5; // end of memory chunk 
                                                              // excluding footer
                addDT(slabDescTable, size, newSize, startAddress, endAddress);

                // delete the available hole
                
                addSlab(notAvailableHolesSlab[index], size, slabDescTable->tail->bitmapList->tail->NodeSlab_ID, startAddress, endAddress);

                for (int i = 0; i < 11; i++)
                {
                    slabDescTable->tail->bitmapList->tail->bid[i] = temp->bid[i];
                    notAvailableHolesSlab[index]->tail->bid[i] = temp->bid[i];
                }

                deleteSlab(availableHolesSlab[index], temp);
                slabDescTable->tail->bitmapList->tail->value = 1UL<<63;
                 
                
                return (temp->start_address);
                //return slabDescTable->tail->bitmapList->head->BMStartEndAddr[0][0];

            }

            else // go up indices and break them down
            {
                while(availableHolesSlab[index]->head == NULL) // do this while we have no chunk at the correct index
                {
                    int found = FALSE;
                    int newIndex = index+1;

                    while(!found)
                    {
                        if (newIndex == 10 && availableHolesSlab[newIndex]->head == NULL)
                        {
                            return(-1);
                        }

                        if(availableHolesSlab[newIndex]->head != NULL) // we found an index with available memory
                                                                // we need to break it down into two smaller
                                                                // chunks
                        {
                            found = TRUE; // stops the while loop

                            NodeSlab *temp;
                            temp = availableHolesSlab[newIndex]->head;
                            int lowest_address = 2147483647;
                            while (temp != NULL)
                            {
                                if ((int)temp->start_address < lowest_address)
                                {
                                    lowest_address = (int)temp->start_address;
                                }
                                temp = temp->next;
                            }
                            
                            temp = availableHolesSlab[newIndex]->head;
                            while (temp != NULL)
                            {
                                if ((int)temp->start_address == lowest_address)
                                {
                                    break;
                                }
                                temp = temp->next;
                            }
                            
                            void* startAddrL = temp->start_address;
                            void* endAddrL = (void *)((int)startAddrL + (int)pow(2,newIndex+10-1));

                            
                            void* startAddrR = endAddrL;
                            void* endAddrR = (void *)((int)startAddrR + (int)pow(2,newIndex+10-1));

                            /* delete larger memory location node and create two smaller ones with
                            values calculated above*/
                             
                            addSlab(availableHolesSlab[newIndex-1], size, NodeSlab_counter++, startAddrL, endAddrL);
                            addSlab(availableHolesSlab[newIndex-1], size, NodeSlab_counter++, startAddrR, endAddrR);
                            for (int i = 0; i < 11; i++)
                            {
                                availableHolesSlab[newIndex-1]->tail->bid[i] = temp->bid[i];
                                availableHolesSlab[newIndex-1]->tail->prev->bid[i] = temp->bid[i];
                            }
                            deleteSlab(availableHolesSlab[newIndex], temp);
                            give_bid_slab(availableHolesSlab[newIndex-1], availableHolesSlab[newIndex-1]->tail->prev, bidCounter);
                            give_bid_slab(availableHolesSlab[newIndex-1], availableHolesSlab[newIndex-1]->tail, bidCounter);
                            bidCounter++;
                        
                        }
                        newIndex++;
                    }
                    newIndex--;
                }
                // found a free slab
                // we have our chunk ready at the current index
             
                NodeSlab *temp;
                temp = availableHolesSlab[index]->head;
                int lowest_address = 2147483647;
                while (temp != NULL)
                {
                    if ((int)temp->start_address < lowest_address)
                    {
                        lowest_address = (int)temp->start_address;
                    }
                    temp = temp->next;
                }
                
                temp = availableHolesSlab[index]->head;
                while (temp != NULL)
                {
                    if ((int)temp->start_address == lowest_address)
                    {
                        break;
                    }
                    temp = temp->next;
                }                                                   
                

                void *startAddress = temp->start_address;
                temp->end_address = startAddress + newSize -5;
                void *endAddress = temp->end_address; // end of memory chunk 
               
                addDT(slabDescTable, size, newSize, startAddress, endAddress);
                // delete the available hole
                addSlab(notAvailableHolesSlab[index], size, slabDescTable->tail->bitmapList->tail->NodeSlab_ID, startAddress, endAddress);
                deleteSlab(availableHolesSlab[index], temp);
                
                slabDescTable->tail->bitmapList->tail->value = 1UL<<63;

                
                 return slabDescTable->tail->bitmapList->tail->BMStartEndAddr[0][0];
            }
        }

        // type exists in slab descriptor table
        // and there is a free chunk in a slab
        // no need to create a new slab
        else if(slab_alloc_type == 1)
        {
            // search through the slab descriptor table for the correct slab
            NodeDT *temp0;
            temp0 = slabDescTable->head;
            int NodeDTFound = 0;
           
            while((temp0 != NULL) && !NodeDTFound)
            {
                if(temp0->obj_size == size)
                {
                    // found the correct descriptor
                    NodeSlab * temp1;
                    NodeDTFound = 1;
                
                    temp1 = temp0->bitmapList->head;
                    int lowest_address = 2147483647;
                   
                    while (temp1 != NULL)
                    {
                        if ((int)temp1->start_address < lowest_address && (temp1->value != 0xFFFFFFFFFFFFFFFF))
                        {
                            lowest_address = (int)temp1->start_address;
                        }
                        temp1 = temp1->next;
                    }
                    
                    temp1 = temp0->bitmapList->head;
                    while (temp1 != NULL)
                    {
                        if ((int)temp1->start_address == lowest_address)
                        {
                            break;
                        }
                        temp1 = temp1->next;
                    }
                    
                    int found = 0;
                    while(temp1 != NULL && !found) 
                    {
                        for(unsigned long i = 0; i < 64; i++)
                        {
                            
                            if(((temp1->value >> (63UL-i)) & 1UL) == 0UL)
                            {
                                
                                temp1->value |= 1UL<<(63-i);
                                found = 1;
                                return temp1->BMStartEndAddr[i][0];
                            }
                            else
                            {
                                continue;
                            }
                            
                        }
                        temp1= temp1->next;
                    }
                }


                temp0 = temp0->next;
            }
            printf("Error!!!! No free chunk exists\n\n\n\n\n");
            return -1;
        }

        // type is found, but the prior slabs are full
        // allocating a new slab for the given type
        else if(slab_alloc_type == 0)
        {
            
            int newIndex;
            NodeDT *temp0;
            temp0 = slabDescTable->head;
            

            NodeSlab *temp1;
            void *startAddress;
            void *endAddress;
            int done = 0;
           
            while(temp0 != NULL && !done)
            {
                if(temp0->obj_size == size)
                {
                    // found the correct descriptor
                    temp0->numObjs +=64;
                    temp0->numUsed += 1; // for type
                    done = 1;
                    if(availableHolesSlab[index]->head != NULL)
                    {
                        // we have our chunk ready at the current index
                        NodeSlab *temp;
                        temp = availableHolesSlab[index]->head;
                        int lowest_address = 2147483647;
                        while (temp != NULL)
                        {
                            if ((int)temp->start_address < lowest_address)
                            {
                                lowest_address = (int)temp->start_address;
                            }
                            temp = temp->next;
                        }
                        
                        temp = availableHolesSlab[index]->head;
                        while (temp != NULL)
                        {
                            if ((int)temp->start_address == lowest_address)
                            {
                                break;
                            }
                            temp = temp->next;
                        }
                        startAddress = temp->start_address;
                        endAddress = startAddress + newSize -5; // end of memory chunk 

                        addSlab(temp0->bitmapList, size,temp->NodeSlab_ID, startAddress, endAddress); 
                        // adds NodeSlab to bitmapList

                        for (int i = 0; i < 11; i++)
                        {
                            temp0->bitmapList->tail->bid[i] = temp->bid[i];
                        }

                        deleteSlab(availableHolesSlab[index], temp);
                        temp0->bitmapList->tail->value = 1UL<<63; // add to first opening
                
                        return startAddress;
                    
                    } 
                    
                    else
                    {
                        while(availableHolesSlab[index]->head == NULL) // do this while we have no chunk at the correct index
                        {
                            int found = FALSE;
                            newIndex = index+1;
                            
                            while(!found)
                            {
                                if (newIndex == 10 && availableHolesSlab[newIndex]->head == NULL)
                                {
                                    return(-1);
                                }

                                if(availableHolesSlab[newIndex]->head != NULL) // we found an index with available memory
                                                                               // we need to break it down into two smaller
                                                                               // chunks
                                {
                                    found = TRUE; // stops the while loop

                                    NodeSlab *temp;
                                    temp = availableHolesSlab[newIndex]->head;
                                    int lowest_address = 2147483647;
                                    while (temp != NULL)
                                    {
                                        if ((int)temp->start_address < lowest_address)
                                        {
                                            lowest_address = (int)temp->start_address;
                                        }
                                        temp = temp->next;
                                    }
                                    
                                    temp = availableHolesSlab[newIndex]->head;
                                    while (temp != NULL)
                                    {
                                        if ((int)temp->start_address == lowest_address)
                                        {
                                            break;
                                        }
                                        temp = temp->next;
                                    }
                                    
                                    void* startAddrL = temp->start_address;
                                    void* endAddrL = (void *)((int)startAddrL + (int)pow(2,newIndex+10-1));

                                    
                                    void* startAddrR = endAddrL;
                                    void* endAddrR = (void *)((int)startAddrR + (int)pow(2,newIndex+10-1));

                                    /* delete larger memory location node and create two smaller ones with
                                    values calculated above*/
                                    addSlab(availableHolesSlab[newIndex-1], size, NodeSlab_counter++, startAddrL, endAddrL);
                                    addSlab(availableHolesSlab[newIndex-1], size, NodeSlab_counter++, startAddrR, endAddrR);
                                    for (int i = 0; i < 11; i++)
                                    {
                                        availableHolesSlab[newIndex-1]->tail->bid[i] = temp->bid[i];
                                        availableHolesSlab[newIndex-1]->tail->prev->bid[i] = temp->bid[i];
                                    }
                                    deleteSlab(availableHolesSlab[newIndex], temp);
                                    give_bid_slab(availableHolesSlab[newIndex-1], availableHolesSlab[newIndex-1]->tail->prev, bidCounter);
                                    give_bid_slab(availableHolesSlab[newIndex-1], availableHolesSlab[newIndex-1]->tail, bidCounter);
                                    bidCounter++;
                                
                                }
                                newIndex++;
                            }
                            newIndex--;
                        }
                        // found a free slab
                        // we have our chunk ready at the current index

                        NodeSlab *temp2;
                        temp2 = availableHolesSlab[index]->head;
                        int lowest_address = 2147483647;
                        while (temp2 != NULL)
                        {
                            if ((int)temp2->start_address < lowest_address)
                            {
                                lowest_address = (int)temp2->start_address;
                            }
                            temp2 = temp2->next;
                        }
                        
                        temp2 = availableHolesSlab[index]->head;
                        while (temp2 != NULL)
                        {
                            if ((int)temp2->start_address == lowest_address)
                            {
                                break;
                            }
                            temp2 = temp2->next;
                        }
                        startAddress = temp2->start_address;
                        temp2->end_address = startAddress + newSize -5;
                        endAddress = temp2->end_address; // end of memory chunk 
                                                         // excluding footer

                        // delete the available hole
                        addSlab(notAvailableHolesSlab[index], size, temp2->NodeSlab_ID, startAddress, endAddress);
                        addSlab(temp0->bitmapList, size, temp2->NodeSlab_ID, startAddress, endAddress);
                       
                        for (int i = 0; i < 11; i++)
                        {
                            notAvailableHolesSlab[index]->tail->bid[i] = temp2->bid[i];
                        }

                        deleteSlab(availableHolesSlab[index], temp2);
                        
                        
                        
                        temp0->bitmapList->tail->value = 1UL<<63;
 
                        return notAvailableHolesSlab[index]->tail->BMStartEndAddr[0][0];            
                    }
                          
                }
                else
                {
                    temp0 = temp0->next;
                }
            }
        }
    }
        

///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////End Of Slab Above///////////////////////////////////////////////////////////////////   
///////////////////////////Vanilla Buddy Below/////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////////// 
///////////////////////////////////////////////////////////////////////////////////////////////////////////////

    else
    {
        // newSize will be different for buddy vs slab since we need to allocate a whole
        // slab for slab alloc, if it's a new type
        int newSize = MAX_SIZE;

        // first thing we need to do is round up to a power of two
        while(newSize >= size+8 && newSize>=1024)
        {
            newSize /= 2;
        }
        newSize*=2;

        if(newSize > 1024*1024/2)
        {
            return -1;
        }


        // calculating index
        int notFound = TRUE;
        int index = 0;
        while(notFound)
        {
            if(pow(2, index+10) == newSize)
            {
                notFound = FALSE; // found the index
            }
            index++;
        }
        index--; // index we are looking for a memory chunk at

        if(availableHoles[index]->head == NULL) // go up indexes and break them down
        {
            while(availableHoles[index]->head == NULL) // do this while we have no chunk at the correct index
            {
                int found = FALSE;
                int newIndex = index+1;

                while(!found)
                {
                    if (newIndex == 10 && availableHoles[newIndex]->head == NULL)
                    {
                        return(-1);
                    }

                    if(availableHoles[newIndex]->head != NULL) // we found an index with available memory
                                                            // we need to break it down into two smaller
                                                            // chunks
                    {
                        found = TRUE; // stops the while loop

                        Node *temp;
                        temp = availableHoles[newIndex]->head;
                        int lowest_address = 2147483647;
                        while (temp != NULL)
                        {
                            if ((int)temp->start_address < lowest_address)
                            {
                                lowest_address = (int)temp->start_address;
                            }
                            temp = temp->next;
                        }
                        
                        temp = availableHoles[newIndex]->head;
                        while (temp != NULL)
                        {
                            if ((int)temp->start_address == lowest_address)
                            {
                                break;
                            }
                            temp = temp->next;
                        }
                        
                        void* startAddrL = temp->start_address;
                        void* endAddrL = (void *)((int)startAddrL + (int)pow(2,newIndex+10-1));

                        void* startAddrR = endAddrL;
                        void* endAddrR = (void *)((int)startAddrR + (int)pow(2,newIndex+10-1));

                        /* delete larger memory location node and create two smaller ones with
                        values calculated above*/
                        add(availableHoles[newIndex-1], "left", startAddrL, endAddrL);
                        add(availableHoles[newIndex-1], "right", startAddrR, endAddrR);
                        for (int i = 0; i < 11; i++)
                        {
                            availableHoles[newIndex-1]->tail->bid[i] = availableHoles[newIndex]->head->bid[i];
                            availableHoles[newIndex-1]->tail->prev->bid[i] = availableHoles[newIndex]->head->bid[i];
                        }
                        delete(availableHoles[newIndex], temp);
                        give_bid(availableHoles[newIndex-1], availableHoles[newIndex-1]->tail->prev, bidCounter);
                        give_bid(availableHoles[newIndex-1], availableHoles[newIndex-1]->tail, bidCounter);
                        bidCounter++;
                    
                    }
                    newIndex++;
                }
                newIndex--;
            }

            // AT THIS POINT WE HAVE A MEMORY CHUNK AT OUR CALCULATED INDEX
            
            void *startAddress = availableHoles[index]->head->start_address;
            void *endAddress = startAddress + newSize -5; // end of memory chunk 
                                                        // excluding footer
            //lastEndAddress = endAddress;
            //lastSize = newSize;
            
            //int budID = get_bid(availableHoles[index], availableHoles[index]->head);
            char relative_pos = availableHoles[index]->head->rel_pos;
            add(notAvailableHoles[index], relative_pos, startAddress, endAddress);
            for (int i = 0; i < 11; i++)
            {
                notAvailableHoles[index]->tail->bid[i] = availableHoles[index]->head->bid[i];
            }
            delete(availableHoles[index], availableHoles[index]->head);
            //give_bid(notAvailableHoles[index], notAvailableHoles[index]->tail, budID);
            return notAvailableHoles[index]->tail->start_address;
        }

        else // we have our memory chunk
        {
            Node *temp;
            temp = availableHoles[index]->head;
            int lowest_address = 2147483647;
            while (temp != NULL)
            {
                if ((int)temp->start_address < lowest_address)
                {
                    lowest_address = (int)temp->start_address;
                }
                temp = temp->next;
            }
            
            temp = availableHoles[index]->head;
            while (temp != NULL)
            {
                if ((int)temp->start_address == lowest_address)
                {
                    break;
                }
                temp = temp->next;
            }

            void *startAddress = temp->start_address;
            void *endAddress = startAddress + newSize -5; // end of memory chunk 
                                                          // excluding footer
            //int budID = get_bid(availableHoles[index], availableHoles[index]->head);
            char relative_pos = temp->rel_pos;
            add(notAvailableHoles[index], relative_pos, startAddress, endAddress);
            for (int i = 0; i < 11; i++)
            {
                notAvailableHoles[index]->tail->bid[i] = temp->bid[i];
            }
            delete(availableHoles[index], temp);
            //give_bid(notAvailableHoles[index], notAvailableHoles[index]->tail, budID);
            return notAvailableHoles[index]->tail->start_address;
        }
    }
}

////////////////////////////////////////////////////////////////////////////
//
// Function     : my_free
// Description  : deallocated the memory segment being passed by the pointer
//
// Inputs       : ptr - pointer to the memory segment to be free'd
// Outputs      : 

void my_free( void *ptr )
{
    if (allocation_type == 0)  //Buddy System
    {
        Node *temp;
        int index;
        int original_index;
        for (index = 0; index < 11; index++)
        {
            temp = notAvailableHoles[index]->head;
            while (temp != NULL)
            {
                if (temp->start_address == ptr)
                {
                    break;
                }
                temp = temp->next;
            }
            if (temp != NULL && temp->start_address == ptr)
            {
                break;
            }
        }
        original_index = index;

        int condition = 1;
        while (condition == 1 && index < 11)
        {
            Node *temp_buddy = availableHoles[index]->head;
            while (temp_buddy != NULL)
            {
                int bid;
                if (index == original_index)
                {
                    bid = get_bid(notAvailableHoles[index], temp);
                }
                else
                {
                    bid = get_bid(availableHoles[index], temp);
                }
                
                int buddy_bid = get_bid(availableHoles[index], temp_buddy);
                if (bid == buddy_bid && temp != temp_buddy)
                {
                    break;
                }
                temp_buddy = temp_buddy->next;
            }

            if (temp_buddy == NULL)
            {
                if (index == original_index)
                {
                    add(availableHoles[index], temp->rel_pos, temp->start_address, temp->end_address);
                    for (int i = 0; i < 11; i++)
                    {
                        availableHoles[index]->tail->bid[i] = temp->bid[i];
                    }
                    delete(notAvailableHoles[index], temp);
                    condition = 0;
                }
                else
                {
                    condition = 0;
                }
            }
            else  //there is a buddy
            {
                void *newStartAddr;
                void *newEndAddr;
                if ((int)temp->start_address < (int)temp_buddy->start_address)
                {
                    newStartAddr = temp->start_address;
                    newEndAddr = temp_buddy->end_address;
                }
                else
                {
                    newStartAddr = temp_buddy->start_address;
                    newEndAddr = temp->end_address;
                }

                    add(availableHoles[index+1], NULL, newStartAddr, newEndAddr);
                    for (int i = 0; i < 11; i++)
                    {
                        availableHoles[index+1]->tail->bid[i] = temp->bid[i];
                    }
                    if (index == original_index)
                    {
                        delete(notAvailableHoles[index], temp);
                    }
                    else
                    {
                        delete(availableHoles[index], temp);
                    }
                    delete(availableHoles[index], temp_buddy);
                    int new_bid = get_bid(availableHoles[index+1], availableHoles[index+1]->tail);
                    remove_bid(availableHoles[index+1], availableHoles[index+1]->tail, new_bid);
                    temp = availableHoles[index+1]->tail;
                    index++;
            }
        }
    }
    else  //Slab Allocator
    {
        NodeDT *temp_DT;
        temp_DT = slabDescTable->head;
        NodeSlab *temp_slab;
        int chunk_index = -1;
        while (temp_DT != NULL)  //find the chunk that the input parameter refers to
        {
            temp_slab = temp_DT->bitmapList->head;
            while (temp_slab != NULL)
            {
                for (int i = 0; i < 64; i++)
                {
                    if (temp_slab->BMStartEndAddr[i][0] == ptr)
                    {
                        chunk_index = i;
                        break;
                    }
                }
                if (temp_slab->BMStartEndAddr[chunk_index][0] == ptr)
                {
                    break;
                }
                temp_slab = temp_slab->next;
            }
            if (temp_slab != NULL && temp_slab->BMStartEndAddr[chunk_index][0] == ptr)
            {
                break;
            }
            temp_DT = temp_DT->next;
        }

        if (temp_slab == NULL)
        {
            printf("Slab not found");
        }

        long mask = 0;
        for (int i = 0; i < 64; i++)  //create a map to bitwise AND with the bitmap to deallocate a specific chunk
        {
            if (i != chunk_index && i != 63)
            {
                mask |= 1;
                mask <<= 1;
            }
            else if (i != chunk_index && i == 63)
            {
                mask |= 1;
            }
            else if (i == chunk_index && i != 63)
            {
                mask <<= 1;
            }
        }

        temp_slab->value &= mask;

        if (temp_slab->value == 0)  //the slab has no chunks being used after the deallocation
        {
            NodeSlab *temp_slab_buddy;

            int bid = get_bid_slab(temp_DT->bitmapList, temp_slab);
            int buddy_bid = -1;

            for (int i = 0; i < 11; i++)
            {
                temp_slab_buddy = availableHolesSlab[i]->head;
                while (temp_slab_buddy != NULL)
                {
                    if (get_bid_slab(availableHolesSlab[i], temp_slab_buddy) == bid)
                    {
                        buddy_bid = bid;
                        break;
                    }
                    temp_slab_buddy = temp_slab_buddy->next;
                }
                if (bid == buddy_bid)
                {
                    break;
                }
            }

            int iteration = 0;
            int condition = 1;
            while (condition == 1)
            {
                if (temp_slab_buddy == NULL)  //if no buddy exists
                {
                    int index;
                    if (iteration == 0)  //if there is no buddy, we need to remove the freed slab from notAvailableHolesSlab
                    {
                        NodeSlab *temp;
                        for (int i = 0; i < 11; i++)  //get the index that the slab exists on for notAvailableHolesSlab
                        {
                            temp = notAvailableHolesSlab[i]->head;
                            while (temp != NULL)
                            {
                                if (get_bid_slab(notAvailableHolesSlab[i], temp) == bid)
                                {
                                    index = i;
                                    break;
                                }
                                temp = temp->next;
                            }
                            if (temp == temp_slab)
                            {
                                break;
                            }
                        }

                        addSlab(availableHolesSlab[index], temp_slab->obj_size, temp_slab->NodeSlab_ID, temp_slab->start_address, temp_slab->end_address);
                        deleteSlab(notAvailableHolesSlab[index], temp);
                        deleteSlab(temp_DT->bitmapList, temp_slab);
                        if (temp_DT->bitmapList->head == NULL)
                        {
                            deleteDT(slabDescTable, temp_DT);
                        }
                    }

                    condition = 0;
                }
                else  //there exists a buddy
                {
                    int index;
                    NodeSlab *temp;
                    if (iteration == 0)  //on the first iteration, the freed slab will be on notAvailableHolesSlab
                    {
                        for (int i = 0; i < 11; i++)
                        {
                            temp = notAvailableHolesSlab[i]->head;
                            while (temp != NULL)
                            {
                                if (get_bid_slab(notAvailableHolesSlab[i], temp) == bid)
                                {
                                    index = i;
                                    break;
                                }
                                temp = temp->next;
                            }
                            if (temp == temp_slab)
                            {
                                break;
                            }
                        }
                    }
                    else  //past first iteration, freed slab will be on availableHolesSlab
                    {
                        for (int i = 0; i < 11; i++)
                        {
                            temp = availableHolesSlab[i]->head;
                            while (temp != NULL)
                            {
                                if (get_bid_slab(availableHolesSlab[i], temp) == bid)
                                {
                                    index = i;
                                    break;
                                }
                                temp = temp->next;
                            }
                            if (temp == temp_slab)
                            {
                                break;
                            }
                        }
                    }

                    NodeSlab *temp_buddy;
                    temp_buddy = availableHolesSlab[index]->head;
                    while (temp_buddy != NULL)
                    {
                        if (get_bid_slab(availableHolesSlab[index], temp_buddy) == buddy_bid && temp_buddy->start_address != temp->start_address)
                        {
                            break;
                        }
                        temp_buddy = temp_buddy->next;
                    }

                    void *newStartAddr;
                    void *newEndAddr;
                    if (temp->start_address < temp_buddy->start_address)
                    {
                        newStartAddr= temp->start_address;
                        newEndAddr = temp_buddy->end_address;
                    }
                    else
                    {
                        newStartAddr = temp_buddy->start_address;
                        newEndAddr = temp->end_address;
                    }
                    
                    int type;
                    int id;
                    if (iteration == 0)
                    {
                        type = temp->obj_size;
                        id = temp->NodeSlab_ID;
                        deleteSlab(notAvailableHolesSlab[index], temp);
                        if (temp_DT->bitmapList->head == NULL)
                        {
                            deleteDT(slabDescTable, temp_DT);
                        }
                    }
                    else
                    {
                        type = temp->obj_size;
                        id = temp->NodeSlab_ID;
                        deleteSlab(availableHolesSlab[index], temp);
                    }
                    
                    deleteSlab(availableHolesSlab[index], temp_buddy);
                    addSlab(availableHolesSlab[index+1], type, id, newStartAddr, newEndAddr);

                    iteration++;
                }
            }
        }
    }
}