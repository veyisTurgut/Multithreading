#include <iostream> //std::cout
#include <fstream>  //file open close
#include <pthread.h>
#include <semaphore.h>
#include <string>
#include <vector>
#include <unistd.h> //sleep

int assingSeat(int reservedSeat);              /* declaration of the function which handles seat reservations */
void initializeSeats(std::string theaterName); /* declaration of the function which initializes the seat array */
void *client(void *param);                     /* the declaration of function executed by the client threads */
void *teller(void *param);                     /* the declaration of function executed by the teller threads */

/*I represented client as a struFct.*/
struct Client
{
    std::string name;
    int arrivalTime;
    int serviceTime;
    int seatNumber;
};

sem_t sem, semSeat;                                /* Names of the semaphores.*/
int numberOfSeats = 60;                            /* 60 or 80 or 200. Initialize to 60 at compile time. We can resize it if needed at run time. */
std::vector<Client> currentlyProcessingClients(3); /* Struct of the client. First index is handled by Teller A, second index is handled by Teller B, last index is handled by Teller C. */
pthread_t tidA, tidB, tidC;                        /* Defines the identifier for the thread we will create. */
std::vector<Client> clientVector;                  /* All the clients will be put in here after read from file. */
std::ofstream output_file;
std::vector<bool> isSeatAvailable(numberOfSeats, false); /* A vector to check the availability of a seat. 0 for empty, 1 for occupied. */

/*  
    Declaration of thread condition variables.
    The reason I use condition variables is that I want to lock teller threads until a client thread comes. 
*/
pthread_cond_t condA = PTHREAD_COND_INITIALIZER;
pthread_cond_t condB = PTHREAD_COND_INITIALIZER;
pthread_cond_t condC = PTHREAD_COND_INITIALIZER;

/*  Declaring and initializing mutexes.*/
pthread_mutex_t lockA = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockB = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t lockC = PTHREAD_MUTEX_INITIALIZER;
/* Whenever I print something to file, threads may intervene. So, writing to file is critic. */
pthread_mutex_t printLock = PTHREAD_MUTEX_INITIALIZER;