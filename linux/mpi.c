/******************
* STAMATI Daniela *
*     331CC       *
*******************/

#include "mpi.h"
#include "mpi_err.h"

#define MSG_SIZE 9000

static int mpi_initialized  = 0;
static int mpi_comm_size    = 0;
static int mpi_finalized    = 0;

static int shm_fd; /*shared memory fd*/
DECLSPEC struct mpi_comm *mpi_comm_world;
static mqd_t *mailbox;
static sem_t **sems;
static sem_t *create;


static int openMPI_COMM_WORLD(){

    int rc;
    struct stat st;

    /* create shm */
    shm_fd = shm_open("/mcw", O_RDWR, 0644);
    DIE(shm_fd == -1, "shm_open");

    fstat(shm_fd,&st);

    mpi_comm_size = (int)st.st_size/sizeof(struct mpi_comm);
    
    mpi_comm_world = mmap(0, (int)st.st_size, 
                            PROT_WRITE | PROT_READ, MAP_SHARED, shm_fd, 0);
    DIE(mpi_comm_world == MAP_FAILED, "mmap");

    return MPI_SUCCESS;
}


static int openMailBox(){

    int i;
    mailbox = malloc(mpi_comm_size*sizeof(mqd_t));

    char *name;

    for (i = 0; i<mpi_comm_size; i++){
        name = calloc(strlen("/mq..."),sizeof(char));
        sprintf(name,"/mq%d",i);
        mailbox[i] = mq_open(name, O_RDWR, 0666, NULL);
        DIE(mailbox[i] == (mqd_t)-1, "mq_open");
        free(name);
    }

    return MPI_SUCCESS;

}

static int closeMailBox(){

    int rc, i;

    for (i=0; i<mpi_comm_size; i++){
        rc = mq_close(mailbox[i]);
        DIE(rc == -1, "mq_close");
    }

    return MPI_SUCCESS;

}

static int openSems(){


    int i;

    //semaphore used for blocking the send function until receive
    char *name;
    sems = malloc(mpi_comm_size*sizeof(sem_t*));

    for (i = 0; i<mpi_comm_size; i++){

        name = calloc(strlen("/sem..."),sizeof(char));
        sprintf(name,"/sem%d",i);
        sems[i] = sem_open(name, 0); 
        DIE(sems[i] == SEM_FAILED, "sem_open failed");
        free(name);

    }

    return MPI_SUCCESS;
}

static int closeSems(){

    int i, rc;
    char *name;

    for (i = 0; i<mpi_comm_size; i++){

        name= calloc(7,sizeof(char));
        rc = sem_close(sems[i]);
        DIE(rc == -1, "sem_close");
        free(name);
    }


    return MPI_SUCCESS;
}




int DECLSPEC MPI_Init(int *argc, char ***argv){

    int rc;
    ERR_HANDLER(mpi_initialized||mpi_finalized, MPI_ERR_OTHER);

    rc = openMPI_COMM_WORLD();
    ERR_HANDLER(rc!=MPI_SUCCESS, MPI_ERR_IO);

    rc = openMailBox();
    ERR_HANDLER(rc!=MPI_SUCCESS, MPI_ERR_IO);

    rc = openSems();
    ERR_HANDLER(rc!=MPI_SUCCESS, MPI_ERR_IO);



    mpi_initialized = 1;
    mpi_finalized = 0;

    return MPI_SUCCESS;
}


int DECLSPEC MPI_Comm_size(MPI_Comm comm, int *size){

    ERR_HANDLER(comm!=MPI_COMM_WORLD, MPI_ERR_COMM);
    ERR_HANDLER(!mpi_initialized||mpi_finalized, MPI_ERR_OTHER);

    *size = mpi_comm_size;
    return MPI_SUCCESS;
}


int DECLSPEC MPI_Initialized(int *flag){

    ERR_HANDLER(flag == NULL, MPI_ERR_IO);

    *flag = mpi_initialized;

    return MPI_SUCCESS;
}



int DECLSPEC MPI_Finalize(){

    ERR_HANDLER(!mpi_initialized||mpi_finalized, MPI_ERR_OTHER);
    int rc;

    mpi_finalized = 1;


    closeMailBox();

    rc = closeSems();
    ERR_HANDLER(rc!=MPI_SUCCESS, MPI_ERR_IO);

    return MPI_SUCCESS;
}



int DECLSPEC MPI_Finalized(int *flag){

    //sanity checks
    ERR_HANDLER(flag == NULL, MPI_ERR_IO);

    *flag = mpi_finalized;
    return MPI_SUCCESS;
}



int DECLSPEC MPI_Comm_rank(MPI_Comm comm, int *rank){

    int pid = getpid();
    int i;

    ERR_HANDLER(!mpi_initialized||mpi_finalized, MPI_ERR_OTHER);
    ERR_HANDLER(comm!=MPI_COMM_WORLD, MPI_ERR_COMM);

    for(i=0; i<mpi_comm_size; i++){

        if (mpi_comm_world[i].pid == pid){
            *rank = mpi_comm_world[i].rank;
            return MPI_SUCCESS;
        }
    }

    return MPI_ERR_IO;

}



int DECLSPEC MPI_Send(void *buf, int count, MPI_Datatype datatype, int dest,
              int tag, MPI_Comm comm){

    int rc;
    int size;
    int rank;

    /* sanity checks */
    ERR_HANDLER(!mpi_initialized||mpi_finalized, MPI_ERR_OTHER);
    ERR_HANDLER(comm!=MPI_COMM_WORLD, MPI_ERR_COMM);
    ERR_HANDLER(datatype!=MPI_CHAR && datatype!=MPI_INT && datatype!=MPI_DOUBLE,
                 MPI_ERR_TYPE);
    ERR_HANDLER(dest<0 || dest >= mpi_comm_size, MPI_ERR_RANK);
    /*.............*/


    /* getting the data size */
    if(datatype==MPI_CHAR)
        size = sizeof(char);
    else if(datatype==MPI_INT)
        size = sizeof(int);
    else
        size = sizeof(double);
    /*.....................*/

    char buff[count*size+3*sizeof(int)];

    /* attaching to the message the status info */
    int sz = count*size;
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    memcpy(buff,&rank, sizeof(int));
    memcpy(buff+sizeof(int),&tag, sizeof(int));
    memcpy(buff+2*sizeof(int), &sz, sizeof(int));
    memcpy(buff+3*sizeof(int), buf, sz);
    /*........................................*/

    //printf("String is %d\n",strlen(buff)-3*sizeof(int)-strlen(buf));

    /* sending data */
    rc = mq_send(mailbox[dest], buff, count*size+3*sizeof(int), DEF_PRIO);
    DIE(rc == -1, "mq_send");
    /*.............*/

    /*blocking the send opperation until, on the other end,
    * a receive occurs */
    rc = sem_wait(sems[dest]);
    DIE(rc == -1, "sem wait");

    return MPI_SUCCESS;
}




int DECLSPEC MPI_Recv(void *buf, int count, MPI_Datatype datatype,
              int source, int tag, MPI_Comm comm, MPI_Status *status){

    int rc, size;
    int rank;
    char *buff;

    struct mq_attr attr;

    /* sanity checks */
    ERR_HANDLER(!mpi_initialized||mpi_finalized, MPI_ERR_OTHER);
    ERR_HANDLER(comm!=MPI_COMM_WORLD, MPI_ERR_COMM);
    ERR_HANDLER(datatype!=MPI_CHAR && datatype!=MPI_INT && datatype!=MPI_DOUBLE,
                 MPI_ERR_TYPE);
    ERR_HANDLER(source!=MPI_ANY_SOURCE, MPI_ERR_RANK);
    ERR_HANDLER(tag!=MPI_ANY_TAG, MPI_ERR_TAG);
    /*..............*/


    /*getting the datatype size*/
    if(datatype==MPI_CHAR)
        size = sizeof(char);
    else if(datatype==MPI_INT)
        size = sizeof(int);
    else
        size = sizeof(double);
    /*....................*/


    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    mq_getattr(mailbox[rank],&attr);

    buff = calloc(attr.mq_msgsize, sizeof(char));
    DIE(buff==NULL, "calloc");


    /*receiving data*/
    rc = mq_receive(mailbox[rank], buff, MSG_SIZE, 0);
    DIE(rc == -1, "mq_receive");
    /*.............*/


    /*getting the status info*/
    if(status!=MPI_STATUS_IGNORE){
        status->MPI_SOURCE = ((int*)buff)[0];
        status->MPI_TAG = ((int*)buff)[1];
        status->_size = ((int*)buff)[2];
    }
    /*.....................*/

    //extracting the actual data
    memcpy(buf, buff+3*sizeof(int),((int*)buff)[2]);
    //printf("%s\n",buf);

        sem_post(sems[rank]);
    return MPI_SUCCESS;
}





int DECLSPEC MPI_Get_count(MPI_Status *status, MPI_Datatype datatype, int *count){

    int size;

    ERR_HANDLER(datatype!=MPI_CHAR && datatype!=MPI_INT && datatype!=MPI_DOUBLE,
                 MPI_ERR_TYPE);
    ERR_HANDLER(!mpi_initialized||mpi_finalized, MPI_ERR_OTHER);


    /*getting the datatype size*/
    if(datatype==MPI_CHAR)
        size = sizeof(char);
    else if(datatype==MPI_INT)
        size = sizeof(int);
    else
        size = sizeof(double);
    /*....................*/


    *count = status->_size/size;

    return MPI_SUCCESS;
}
