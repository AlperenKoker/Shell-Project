#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <semaphore.h>

//Define mutex and semaphore variable
pthread_mutex_t publishMutex;
pthread_mutex_t packagedMutex;
sem_t full;
//Define total count of packaged books global variable
int totalCountOfPackagedBooks = 0;
int totalCountOfPublisher = 0;
//Define publisher struct for information
typedef struct Publisher
{
    int id;
    int type;
    int countOfPublishedBooks;
    int maxCountOfPublishedBooks;
} Publisher;
//Define publisher type struct for information
typedef struct PublisherType
{
    int type;
    int countOfPublishedBooks;
    int sizeOfBuffer;
    int countOfPublisher;
    char **publishedBooks;
} PublisherType;
//Define param struct for send parameteter for publisher thread method
typedef struct Param
{
    Publisher *publisher;
    PublisherType *publisherType;
} Param;
//Define packager struct for information
typedef struct Packager
{
    int packagerID;
    int packageLimit;
    int countOfPackagedBooks;
    char **packagedBooks;
} Packager;
//Define param1 struct for send parameteter for packager thread method
typedef struct Param1
{
    int numberOfType;
    int totalNumberOfBooks;
    Packager *packager;
} Param1;
//Define buffer struct for information of publisher's buffer
typedef struct Buffer
{
    PublisherType *publisherType;
    struct Buffer *next;
} Buffer;
//Define linkedlist head for store buffers on global
Buffer *bufferHead;
//This method work is add buffers in linkedlist
void append(struct Buffer **head_ref, PublisherType *publisherType)
{
    /* 1. allocate node */
    struct Buffer *new_node = (struct Buffer *)malloc(sizeof(struct Buffer));

    struct Buffer *last = *head_ref; /* used in step 5*/

    /* 2. put in the data  */
    new_node->publisherType = publisherType;

    /* 3. This new node is going to be the last node, so make next  
          of it as NULL*/
    new_node->next = NULL;

    /* 4. If the Linked List is empty, then make the new node as head */
    if (*head_ref == NULL)
    {
        *head_ref = new_node;
        return;
    }

    /* 5. Else traverse till the last node */
    while (last->next != NULL)
        last = last->next;

    /* 6. Change the next of last node */
    last->next = new_node;
    return;
}
//This method work is find and return buffers' publisher type struct
PublisherType *findSpesificBUffer(Buffer **head, int index)
{
    //Define current pointer
    Buffer *current = *head;
    //Define variable for find desired index
    int count = 0;
    //Travel linkedlist
    while (current != NULL)
    {
        //Control desired index in linkedlist or not
        if (count == index)
        {
            return (current->publisherType);
        }
        count++;
        current = current->next;
    }
}
//Publisher threads' method
void *publishBook(void *param)
{
    //Cast void to Param for take data in param struct
    Param *parameter = (Param *)param;
    //Create books  according to max count of books for each publisher
    for (int i = 0; i <= parameter->publisher->maxCountOfPublishedBooks; i++)
    {
        //Control publisher is finished publish max or not
        if (parameter->publisher->countOfPublishedBooks + 1 <= parameter->publisher->maxCountOfPublishedBooks)
        {
            //Print published books
            char bookName[20];
            pthread_mutex_lock(&publishMutex);
            sprintf(bookName, "Book %d_%d", parameter->publisherType->type, parameter->publisherType->countOfPublishedBooks + 1);
            strcpy(parameter->publisherType->publishedBooks[parameter->publisherType->countOfPublishedBooks], bookName);
            parameter->publisherType->countOfPublishedBooks++;
            parameter->publisher->countOfPublishedBooks++;
            printf("Publisher %d of type %d \t %s is published and put into buffer %d.\n", parameter->publisher->id, parameter->publisher->type, bookName, parameter->publisherType->type);
            //Control buffer size is full or not is buffer is full resizing buffer
            if (parameter->publisherType->sizeOfBuffer == parameter->publisherType->countOfPublishedBooks)
            {
                printf("Publisher %d of type %d \t Buffer is full. Resizing the buffer.\n", parameter->publisher->id, parameter->publisher->type);
                char **buffer = malloc(sizeof(char *) * (parameter->publisherType->sizeOfBuffer * 2));
                for (int j = 0; j < parameter->publisherType->sizeOfBuffer; j++)
                {
                    buffer[j] = malloc(sizeof(char) * 20);
                    strcpy(buffer[j], parameter->publisherType->publishedBooks[j]);
                }
                for (int a = parameter->publisherType->sizeOfBuffer; a < parameter->publisherType->sizeOfBuffer * 2; a++)
                {
                    buffer[a] = malloc(sizeof(char) * 20);
                    strcpy(buffer[a], "empty");
                }
                parameter->publisherType->sizeOfBuffer *= 2;
                free(parameter->publisherType->publishedBooks);
                parameter->publisherType->publishedBooks = buffer;
            }
            sem_post(&full);
            pthread_mutex_unlock(&publishMutex);
        }
        else
        {
            //Publisher is finished publish books this publisher exiting system
            totalCountOfPublisher--;
            parameter->publisherType->countOfPublisher--;
            printf("Publisher %d of type %d \t Finished pusblishing 5 books. Exiting system.\n", parameter->publisher->id, parameter->publisher->type);
        }
    }
}
//Packager threads' method
void *packageBook(void *param)
{
    Param1 *parameter = (Param1 *)param;
    int randomIndex, control = 1, oldRandomValue = ((double)rand() / (double)RAND_MAX) * parameter->numberOfType;
    while (1)
    {
        //Control books in the buffer is empty or not if it is empty packager print last status and exit the system
        if (parameter->totalNumberOfBooks <= totalCountOfPackagedBooks)
        {
            if (parameter->packager->countOfPackagedBooks > 0)
            {
                printf("Packager %d \t There are no publishers left in the system. Only %d of %d number of books could be packaged. The package contains ", parameter->packager->packagerID, parameter->packager->countOfPackagedBooks, parameter->packager->packageLimit);
                for (int i = 0; i < parameter->packager->countOfPackagedBooks; i++)
                {
                    printf("%s ", parameter->packager->packagedBooks[i]);
                }
                printf(". Exiting system.\n");
                free(parameter->packager->packagedBooks);
            }
            break;
        }
        if (totalCountOfPublisher!=0)
        {
            sem_wait(&full);
        }
        pthread_mutex_lock(&packagedMutex);
        //Select random buffer
        randomIndex = ((double)rand() / (double)RAND_MAX) * parameter->numberOfType;
        pthread_mutex_lock(&publishMutex);
        PublisherType *publisherType = findSpesificBUffer(&bufferHead, oldRandomValue);
        //Control buffer is empty but publisher type is in the system or not
        if (control == 0 && publisherType->countOfPublisher != 0)
        {
            oldRandomValue = oldRandomValue;
        }
        else
        {
            oldRandomValue = randomIndex;
        }
        control = 0;
        //Packager choose one book in the buffer, package books
        for (int i = 0; i < publisherType->sizeOfBuffer; i++)
        {
            if (strcmp(publisherType->publishedBooks[i], "empty"))
            {
                for (int j = 0; j < parameter->packager->packageLimit; j++)
                {
                    if (strcmp(parameter->packager->packagedBooks[j], "empty") == 0)
                    {
                        strcpy(parameter->packager->packagedBooks[j], publisherType->publishedBooks[i]);
                        printf("Packager %d \t Put %s into the package\n", parameter->packager->packagerID, publisherType->publishedBooks[i]);
                        totalCountOfPackagedBooks++;
                        parameter->packager->countOfPackagedBooks++;
                        strcpy(publisherType->publishedBooks[i], "empty");
                        control = 1;
                        break;
                    }
                }
                if (control==1)
                {
                    break;
                }
                
            }
        }
        //Control packager package's size is full or not
        if (parameter->packager->countOfPackagedBooks == parameter->packager->packageLimit)
        {
            printf("Packeger %d \t Fİnished preparing one package. The package contain:\n ", parameter->packager->packagerID);
            for (int k = 0; k < parameter->packager->packageLimit; k++)
            {
                printf("%s,", parameter->packager->packagedBooks[k]);
                strcpy(parameter->packager->packagedBooks[k], "empty");
            }
            parameter->packager->countOfPackagedBooks = 0;
            printf("\n");
        }
        pthread_mutex_unlock(&publishMutex);
        pthread_mutex_unlock(&packagedMutex);
    }
}

int main(int argc, char *argv[])
{
    //determine needed variable
    int numberOfTypePub, numberOfPub, numberOfPack, numberOfBook, numberOfPutBook, initalizeBufferSize, rc;

    //control arguments
    if (argc != 10)
    {
        printf("Invalid argument\n");
        exit(1);
    }

    if (!isdigit(*argv[2]))
    {

        printf("Invalid argument\n");
        exit(1);
    }

    if (!isdigit(*argv[3]))
    {

        printf("Invalid argument\n");
        exit(1);
    }

    if (!isdigit(*argv[4]))
    {

        printf("Invalid argument\n");
        exit(1);
    }

    if (!isdigit(*argv[6]))
    {

        printf("Invalid argument\n");
        exit(1);
    }

    if (!isdigit(*argv[8]))
    {

        printf("Invalid argument\n");
        exit(1);
    }

    if (!isdigit(*argv[9]))
    {

        printf("Invalid argument\n");
        exit(1);
    }
    //initalize variables
    numberOfTypePub = atoi(argv[2]);
    numberOfPub = atoi(argv[3]);
    numberOfPack = atoi(argv[4]);
    numberOfBook = atoi(argv[6]);
    numberOfPutBook = atoi(argv[8]);
    initalizeBufferSize = atoi(argv[9]);
    totalCountOfPublisher=numberOfPub*numberOfTypePub;
    //initalize mutex and semaphore variables
    pthread_mutex_init(&publishMutex, NULL);
    pthread_mutex_init(&packagedMutex, NULL);
    sem_init(&full, 0, numberOfPack);
    //Define publisher threads array
    pthread_t publisherThreads[numberOfTypePub * numberOfPub];
    int indexOfThread = -1;
    for (int i = 1; i <= numberOfTypePub; i++)
    {
        //Define buffer for each publisher type
        char **buffer;
        //initalize buffer
        buffer = malloc(sizeof(char *) * initalizeBufferSize);
        for (int a = 0; a < initalizeBufferSize; a++)
        {
            buffer[a] = malloc(sizeof(char) * 20);
            strcpy(buffer[a], "empty");
        }
        //Define publisherType
        PublisherType *publisherType = (PublisherType *)malloc(sizeof(PublisherType));
        publisherType->type = i;
        publisherType->countOfPublishedBooks = 0;
        publisherType->sizeOfBuffer = initalizeBufferSize;
        publisherType->publishedBooks = buffer;
        publisherType->countOfPublisher = numberOfPub;
        //Add all buffers in linkedlist
        append(&bufferHead, publisherType);
        for (int j = 1; j <= numberOfPub; j++)
        {
            //Define publisher
            Publisher *publisher = (Publisher *)malloc(sizeof(Publisher));
            publisher->type = i;
            publisher->id = j;
            publisher->countOfPublishedBooks = 0;
            publisher->maxCountOfPublishedBooks = numberOfBook;
            //Define param
            Param *param = (Param *)malloc(sizeof(Param));
            param->publisher = publisher;
            param->publisherType = publisherType;

            indexOfThread++;
            //Create publisher threads
            rc = pthread_create(&(publisherThreads[indexOfThread]), NULL, publishBook, (void *)param);
            //Control creating threads is successful or not
            if (rc)
            {
                printf("ERROR; return code from pthread_create() is %d\n", rc);
                exit(-1);
            }
        }
    }
    //Define packager threads array
    pthread_t packagerThreads[numberOfPack];
    for (int i = 0; i < numberOfPack; i++)
    {
        //Define packaged books for each packager
        char **packagedBooks;
        //initalize paclaged books
        packagedBooks = malloc(sizeof(char *) * numberOfPutBook);
        for (int a = 0; a < numberOfPutBook; a++)
        {
            packagedBooks[a] = malloc(sizeof(char) * 20);
            strcpy(packagedBooks[a], "empty");
        }
        //Define packager
        Packager *packager = (Packager *)malloc(sizeof(Packager));
        packager->packagerID = i + 1;
        packager->packagedBooks = packagedBooks;
        packager->countOfPackagedBooks = 0;
        packager->packageLimit = numberOfPutBook;
        //Define param1
        Param1 *param1 = (Param1 *)malloc(sizeof(Param1));
        param1->numberOfType = numberOfTypePub;
        param1->packager = packager;
        param1->totalNumberOfBooks = numberOfTypePub * numberOfPub * numberOfBook;
        //Create packager threads
        rc = pthread_create(&(packagerThreads[i]), NULL, packageBook, (void *)param1);
        //Control creating threads is successful or not
        if (rc)
        {
            printf("ERROR; return code from pthread_create() is %d\n", rc);
            exit(-1);
        }
    }
    //Wait finishing all publisher threads' process
    for (int i = 0; i < numberOfTypePub * numberOfPub; i++)
    {
        pthread_join(publisherThreads[i], NULL);
    }
    pthread_mutex_destroy(&publishMutex);
    //Wait finishing all packager threads' process
    for (int i = 0; i < numberOfPack; i++)
    {
        pthread_join(packagerThreads[i], NULL);
    }
    //Destroy packkager threads' mutex and semaphore
    sem_destroy(&full);
    pthread_mutex_destroy(&packagedMutex);
}
