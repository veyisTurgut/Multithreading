/**
 * @file 2017400210.cpp
 * @author Adalet Veyis Turgut
 * @date 2021-01-20
 */
#include "2017400210.h"

/*
    First check arguments.
    Then initialize semaphores and create teller threads.
    After that read the file, initialize the environment and create client teller threads one by one.
    Wait for client threads to terminate.
    Sleep 1 second just in case before termination of the program.
    */
int main(int argc, char *argv[])
{

    if (argc != 3)
    { /* Check the number of the arguments*/
        std::cout << "Please provide one input file and one output file!" << std::endl;
        exit(1); // terminate with error
    }

    std::ifstream in_file(argv[1]);
    if (!in_file)
    {
        std::cout << "Unable to open input file " << argv[1] << std::endl;
        exit(1); // terminate with error
    }
    output_file.open(argv[2]);
    if (!output_file)
    {
        std::cout << "Unable to open output file " << argv[2] << std::endl;
        exit(1); // terminate with error
    }

    std::string line;                   /*temp varibale to hold the lines of input file*/
    std::string theaterName;            /*name of the theater, has 3 possibilities*/
    std::getline(in_file, theaterName); /*first line is name of the theatre*/
    initializeSeats(theaterName);       /*since we have the name of the theatre after the previous line, we can initialize our vector with corresponding length*/
    std::getline(in_file, line);        /*second line is number of clients*/
    int numberOfClients = stoi(line);   /*convert string to integer*/

    pthread_attr_t attr;
    pthread_attr_init(&attr); /* Get the default attributes defined by pthread */
    output_file << "Welcome to the Sync-Ticket!" << std::endl;

    int a = 0, b = 1, c = 2;
    pthread_create(&tidA, &attr, teller, &a); /* Create a thread with argument a */
    pthread_create(&tidB, &attr, teller, &b); /* Create a thread with argument b */
    pthread_create(&tidC, &attr, teller, &c); /* Create a thread with argument c */
    sem_init(&sem, 0, 3);                     /* Initialize the semaphore "sem" with value 3 */
    sem_init(&semSeat, 0, 1);                 /* Initialize the semaphore "semSeat" with value 1, I could have used mutex too. But I wanted to use both mutex and binary semaphore to get familiarize with both of them.*/
    usleep(1000 * 20);                        /* Wait 20 ms for teller threads to prompt "Teller x has arrived" */

    for (int i = 0; i < numberOfClients; i++)
    {
        std::getline(in_file, line);                                          /* Read each line and store it in "line" variable. */
        std::string name = line.substr(0, line.find_first_of(','));           /* Substring from beginning to first comma represents the name of the client. */
        line = line.substr(line.find_first_of(',') + 1);                      /* We read the name in the previous line, we can trim that part. */
        int arrivalTime = std::stoi(line.substr(0, line.find_first_of(','))); /* Same procedure as above except since these values are integer, we use std::stoi to convert string to integer. */
        line = line.substr(line.find_first_of(',') + 1);
        int serviceTime = std::stoi(line.substr(0, line.find_first_of(',')));
        line = line.substr(line.find_first_of(',') + 1);
        int seatNumber = std::stoi(line);
        clientVector.push_back({name, arrivalTime, serviceTime, seatNumber}); /* We converted a line of file to variables. Now we should construct a client struct and put it on vector. */
    }

    std::vector<pthread_t> clientThreadIds(numberOfClients); /* Vector of Id's of client threads. */
    for (int i = 0; i < numberOfClients; i++)
    {
        pthread_create(&clientThreadIds[i], &attr, client, &clientVector[i]); /* Create client threads one by one by giving them their struct as parameter.*/
    }
    for (int i = 0; i < numberOfClients; i++)
    {
        pthread_join(clientThreadIds[i], NULL); /* Wait all threads to complete before terminating. */
    }
    usleep(1000 * 1000); /* Sleep 1 second. We waited all clients to finish, but tellers may still be active. Maximum time interval they will be alive is probably a few ms. We can wait them to complete before terminating. */
    output_file << "All clients received service." << std::endl;
}
/* 
    This is the function that teller threads run. 
    It first prompts welcome message firstly. 
    Then waits for a client.
    After client signalled its presence with "pthread_cond_wait", it assigns a suitable seat for client.
    Then sleeps for service time in ms. 
    Finally prints whether a seat is found or none.
    Increments the semaphore value by 1 so that next client can enter from queue.
*/
void *teller(void *param)
{
    int id = *(int *)param; /* 0 for A, 1 for B, 2 for C */
    usleep(10 * 1000 * id); /* No sleep for Teller A, 10 ms sleep for Teller B, 20 ms sleep for Teller C. */
    pthread_mutex_lock(&printLock);
    output_file << "Teller " << (char)(id + 65) /*ASCII values of A, B and C*/ << " has arrived." << std::endl;
    pthread_mutex_unlock(&printLock);
    while (1)
    {
        if (id == 0)                           /* Wait until a client comes. */
            pthread_cond_wait(&condA, &lockA); /* Signal teller A to continue.*/
        if (id == 1)                           /* Wait until a client comes. */
            pthread_cond_wait(&condB, &lockB); /* Signal teller B to continue.*/
        if (id == 2)                           /* Wait until a client comes. */
            pthread_cond_wait(&condC, &lockC); /* Signal teller C to continue.*/
        /* Client is here. */
        int assignedSeat = assingSeat(currentlyProcessingClients[id].seatNumber); /*Calculate assigned seat to the client. */
        usleep(currentlyProcessingClients[id].serviceTime * 1000);                /* Sleep for service time in ms. */
        if (assignedSeat == -1)
        { //All seats were occupied, could not reserve one.
            pthread_mutex_lock(&printLock);
            output_file << currentlyProcessingClients[id].name << " requests seat " << std::to_string(currentlyProcessingClients[id].seatNumber) << ", reserves None. Signed by Teller " << (char)(id + 65) << "." << std::endl;
            pthread_mutex_unlock(&printLock);
        }
        else
        { //Found a seat. Perfect.
            pthread_mutex_lock(&printLock);
            output_file << currentlyProcessingClients[id].name << " requests seat " << std::to_string(currentlyProcessingClients[id].seatNumber) << ", reserves seat " << std::to_string(assignedSeat) << ". Signed by Teller " << (char)(id + 65) << "." << std::endl;
            pthread_mutex_unlock(&printLock);
        }
        sem_post(&sem); /* Allow next client to come in. */
    }
    pthread_exit(0); // Terminate the thread. P.S: We wont come here anyways.
}

/* 
    This is the function that client threads run. 
    Takes the struct of clients as argument and assigns it to a teller thread. 
    At most 3 client threads can run simulteanously. 
*/
void *client(void *param)
{
    Client c = *(Client *)param;              /* Parameter is the client struct. */
    usleep(c.arrivalTime * 1000 + 20 * 1000); /* Sleep for arrival time in ms. + 20 ms is there because tellers sleep at first. */
    sem_wait(&sem);                           /* 3 clients can be serviced at most simultaneously. Wait for an available teller. */
    /* Great! Now, we can place ourself into the "currentlyProcessingClients" vector so that teller thread can assign us a seat. */
    /* Trylock tries to lock mutex, returns 0 upon successfull lock. */
    if (pthread_mutex_trylock(&lockA) == 0)
    {
        /* Teller A was available. */
        currentlyProcessingClients[0] = c; /* Put this client in vector so that Teller A can know the fields of this client. */
        pthread_cond_signal(&condA);       /* Wake up the Teller A. */
        pthread_mutex_unlock(&lockA);      /* We are serviced. Release the lock.*/
        usleep(1000 * c.serviceTime);      /* Sleep while being serviced. */
    }
    else if (pthread_mutex_trylock(&lockB) == 0)
    {
        /* Teller B was available. */
        currentlyProcessingClients[1] = c; /* Put this client in vector so that Teller B can know the fields of this client. */
        pthread_cond_signal(&condB);       /* Wake up the Teller B. */
        pthread_mutex_unlock(&lockB);      /* We are serviced. Release the lock.*/
        usleep(1000 * c.serviceTime);      /* Sleep while being serviced. */
    }
    else if (pthread_mutex_trylock(&lockC) == 0)
    {
        /* Teller C was available. */
        currentlyProcessingClients[2] = c; /* Put this client in vector so that Teller C can know the fields of this client. */
        pthread_cond_signal(&condC);       /* Wake up the Teller B. */
        pthread_mutex_unlock(&lockC);      /* We are serviced. Release the lock.*/
        usleep(1000 * c.serviceTime);      /* Sleep while being serviced. */
    }
    pthread_exit(0); /* Terminate the thread. */
}

/*  This function tries to find a suitable seat to a client.
    It tries to give the seat that client wishes at first. 
    If it's not empty, starts form first seat and checks for an empty seat one by one.
    If still can't find a seat, returns -1.
*/
int assingSeat(int reservedSeat)
{
    int assignedSeat = -1;
    /* At most 1 teller can reserve seat. Otherwise tellers may intervene and try to reserve same seat for different clients.*/
    sem_wait(&semSeat);
    if (reservedSeat <= numberOfSeats && !isSeatAvailable[reservedSeat - 1])
    {
        /* If seat number is less than capacity and it is empty, then occupy it. Everything is cool. */
        assignedSeat = reservedSeat;
    }
    else
    {
        /* Seat is occupied or greater than capacity. Search an empty seat one by one starting from first seat. */
        for (int i = 0; i < numberOfSeats; i++)
        {
            if (!isSeatAvailable[i])
            {
                /* We found a seat, great! */
                assignedSeat = i + 1;
                break;
            }
        }
    }
    if (assignedSeat != -1)
    {
        /* Mark the seat as full so no one else can reserve it again. */
        isSeatAvailable[assignedSeat - 1] = true;
    }

    sem_post(&semSeat); /*We can get out of the critical section.*/
    return assignedSeat;
}

/* Before any one of the threads even created, we need the capacity of the theatre. This function determines the size of the seats. */
void initializeSeats(std::string theaterName)
{
    /*  
        These if else statements are very simple and straight-forward. 
        Aim is to find out the number of seats by reading the name of the theatre.
    */
    if (theaterName.length() >= 12 && theaterName.substr(0, 12) == "OdaTiyatrosu")
        numberOfSeats = 60;
    else if (theaterName.length() >= 18 && theaterName.substr(0, 18) == "UskudarStudyoSahne")
        numberOfSeats = 80;
    else if (theaterName.length() >= 10 && theaterName.substr(0, 10) == "KucukSahne")
        numberOfSeats = 200;
    else
    {
        /* If we came here, input file must be wrong. */
        std::cout << "There is no theater with this name!" << std::endl;
        exit(1); // terminate with error
    }
    /*  
        I initialied the number of seats as 60 at compile time.
        We can resize the vector easily at run time, if theatre is different than we assumed at compile time.
    */
    isSeatAvailable.resize(numberOfSeats, false);
}