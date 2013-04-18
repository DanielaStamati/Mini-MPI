#include "mpi.h"
#include "mpi_err.h"


int shm_fd; /*shared memory fd*/
DECLSPEC struct mpi_comm *mpi_comm_world;

static mqd_t *mailbox;  //mailbox array

static sem_t **sems;    //used for blocking the send operation until 
                        //a receive occurs

static sem_t *create;   //used for preventing reads from the 
                        // MPI_COMM_WORLD structure until it is created

/*
** Used for starting the np processes
** @argv - arguments given to the program
** @i - the rank of the process to be started
*/

int start_process(char **argv, int i){

    int pid, ret;

    pid = fork();

    DIE(pid == -1, "fork");

    if(!pid) {
        /*child process*/
        ret = execv(argv[3],argv+3);
        DIE(ret == -1, "exec");
    } else {
        /*parent*/
        mpi_comm_world[i].pid = pid;
        mpi_comm_world[i].rank = i;
    }

    return 0;
}

/* 
** Creating the *MPI_COMM_WORLD array of tuples (pid, rank)
** @np - number of processes started
*/

int createMPI_COMM_WORLD(int np){

    int rc;

    /* creating shm */
    shm_fd = shm_open("/mcw", O_CREAT | O_RDWR, 0644);
    DIE(shm_fd == -1, "shm_open");

    /* resizing shm */
    rc = ftruncate(shm_fd, np*sizeof(struct mpi_comm) );
    DIE(rc == -1, "ftruncate");

    /* mapping the shm */
    mpi_comm_world = mmap(0, np*sizeof(struct mpi_comm), 
                            PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    DIE(mpi_comm_world == MAP_FAILED, "mmap");

    return EXIT_SUCCESS;
}

/*
** MPI_COMM_WORLD* destructor
** @np - number of processes running
*/
int releaseMPI_COMM_WORLD(int np){

    int rc;

    /* unmap shm */
    rc = munmap(mpi_comm_world, np*sizeof(struct mpi_comm));
    DIE(rc == -1, "munmap");

    /* close descriptor */
    rc = close(shm_fd);
    DIE(rc == -1, "close");

    /* unlink */
    rc = shm_unlink("/mcw");
    DIE(rc == -1, "unlink");

    return EXIT_SUCCESS;
}

/*
** mailbox* constructor
** @see -   mailbox* in global variables, each process has its' own mailbox
            the sender inserts data into the receiver's mailbox
** @np - number of processes running
*/

void createMailBoxes(int mpi_comm_size){

    int i;
    mailbox = malloc(mpi_comm_size*sizeof(mqd_t));

    char *name;

    for (i = 0; i<mpi_comm_size; i++){
        name = calloc(sizeof("/mq..."),sizeof(char));
        sprintf(name,"/mq%d",i);
        mailbox[i] = mq_open(name, O_CREAT | O_RDWR, 0666, NULL);
        DIE(mailbox[i] == (mqd_t)-1, "mq_open");
        free(name);
    }


}

/*
** mailbox* destructor
** @np - number of processes running
*/
static int destroyMailBoxes(int np){

    int i, rc;
    char *name;

    for (i = 0; i < np; ++i)
    {

        name = calloc(sizeof("/mq..."),sizeof(char));
        sprintf(name,"/mq%d",i);

        rc = mq_close(mailbox[i]);
        DIE(rc == -1, "mq_close");

        rc = mq_unlink(name);
        DIE(rc == -1, "mq_unlink");
        free(name);
    }

    free(mailbox);

    return EXIT_SUCCESS;
}


/* Creates the used semaphores
** @see -   sems (in global variables) - an array of semaphores used
            for preventing the send operation to finish until a receive
            occurred. Each message queue used in mailbox implementation has
            a corresponding semaphore. Once the sender sent a message, it
            tries to decrement the semaphore which results the sender to block.
            Unblocking occurs when the receiver received a message and increments 
            the semaphore.

** @see -   create (in global variables) - a semaphore used for preventing
            any reading from the MPI_COMM_WORLD structure until it is created

** @np -    the number of the running processes
*/

void createSems(int np){


    int i;

    char *name; //stores the semaphores name from sems
    char *createName = strdup("/create"); //stores the create semaphore name

    create = sem_open(createName, O_CREAT, 0644, 0);
    DIE(create == SEM_FAILED, "sem_open failed");

    sems = malloc(np*sizeof(sem_t*));

    for (i = 0; i<np; i++){
        name = calloc(sizeof("/sem..."),sizeof(char));
        sprintf(name,"/sem%d",i);
        sems[i] = sem_open(name, O_CREAT, 0644, 0); 
        DIE(sems[i] == SEM_FAILED, "sem_open failed");
        free(name);

    }

    free(createName);

}

/* semaphore destructor
** @np - number of processes
*/

void destroySems(int np){

    int i, rc;
    char *name;
    char *createName = strdup("/create");

    /*closing the create semaphore*/
    rc = sem_close(create);
    DIE(rc == -1, "sem_close");

    /*unlinking the create semaphore*/
    rc = sem_unlink(createName);
     DIE(rc == -1, "sem_unlink");

    for (i = 0; i<np; i++){

        rc = sem_close(sems[i]);
        DIE(rc == -1, "sem_close");
    }

    for (i = 0; i<np; i++){
        name = calloc(sizeof("/sem..."),sizeof(char));
        sprintf(name,"/sem%d",i);
        rc = sem_unlink(name);
        DIE(rc == -1, "sem_unlink");
        free(name);
    }

    free(createName);
}


int main (int argc, char **argv){

    int i, np, rc;

    //sanity checks
    DIE(argc < 4 || strcmp(argv[1], "-np"),
            "Usage: ./mpirun -np <process no> <exec name>");

    np = atoi(argv[2]);

    createMPI_COMM_WORLD(np);
    createMailBoxes(np);
    createSems(np);


    /*  starting processes  */
    for (i = 0; i < np; i++){
        start_process(argv, i);
    }


    for (i = 0; i < np; i++){
        printf("%d ",mpi_comm_world[i].pid);
        printf("%d\n",mpi_comm_world[i].rank);
    }

    for (i = 0; i < np; i++){
        sem_post(create);
    }

    int exit_status;
    /*  waiting for child processes */
    for (i = 0; i < np; i++){
        rc = waitpid(mpi_comm_world[i].pid, &exit_status, 0);
        DIE(rc == -1, "wait");
    }

    destroySems(np);

    /*  closing and unlinking the message queues */
    destroyMailBoxes(np);

    /*  closing the shared memory   */
    releaseMPI_COMM_WORLD(np);

    return EXIT_SUCCESS;
}
