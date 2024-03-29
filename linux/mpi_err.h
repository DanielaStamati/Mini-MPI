#ifndef MPI_ERR_H_
#define MPI_ERR_H_

#define  MPI_SUCCESS			0
#define	MPI_ERR_BUFFER			1
#define	MPI_ERR_COUNT			2
#define	MPI_ERR_TYPE			3
#define	MPI_ERR_TAG			4
#define	MPI_ERR_COMM			5
#define	MPI_ERR_RANK			6
#define	MPI_ERR_REQUEST			7
#define	MPI_ERR_ROOT			7
#define	MPI_ERR_GROUP			8
#define	MPI_ERR_OP			9
#define	MPI_ERR_TOPOLOGY		10
#define	MPI_ERR_DIMS			11
#define	MPI_ERR_ARG			12
#define	MPI_ERR_UNKNOWN			13
#define	MPI_ERR_TRUNCATE		14
#define	MPI_ERR_OTHER			15
#define	MPI_ERR_INTERN			16
#define	MPI_ERR_IN_STATUS		17
#define	MPI_ERR_PENDING			18
#define	MPI_ERR_ACCESS			19
#define	MPI_ERR_AMODE			20
#define	MPI_ERR_ASSERT			21
#define	MPI_ERR_BAD_FILE		22
#define	MPI_ERR_BASE			23
#define	MPI_ERR_CONVERSION		24
#define	MPI_ERR_DISP			25
#define	MPI_ERR_DUP_DATAREP		26
#define	MPI_ERR_FILE_EXISTS		27
#define	MPI_ERR_FILE_IN_USE		28
#define	MPI_ERR_FILE			29
#define	MPI_ERR_INFO_KEY		30
#define	MPI_ERR_INFO_NOKEY		31
#define	MPI_ERR_INFO_VALUE		32
#define	MPI_ERR_INFO			33
#define	MPI_ERR_IO			34
#define	MPI_ERR_KEYVAL			35
#define	MPI_ERR_LOCKTYPE		36
#define	MPI_ERR_NAME			37
#define	MPI_ERR_NO_MEM			38
#define	MPI_ERR_NOT_SAME		39
#define	MPI_ERR_NO_SPACE		40
#define	MPI_ERR_NO_SUCH_FILE		41
#define	MPI_ERR_PORT			42
#define	MPI_ERR_QUOTA			43
#define	MPI_ERR_READ_ONLY		44
#define	MPI_ERR_RMA_CONFLICT		45
#define	MPI_ERR_RMA_SYNC		46
#define	MPI_ERR_SERVICE			47
#define	MPI_ERR_SIZE			48
#define	MPI_ERR_SPAWN			49
#define	MPI_ERR_UNSUPPORTED_DATAREP	50
#define	MPI_ERR_UNSUPPORTED_OPERATION	51
#define	MPI_ERR_WIN			52
#define	MPI_ERR_LASTCODE		53
#define	MPI_ERR_SYSRESOURCE		-2

#define ERR_HANDLER(cond, err)              \
    do                                      \
        if (cond){                          \
            printf("%d\n",err);             \
            return err;                     \
        }                                   \
    while(0)

#define DIE(assertion, call_description)    \
    do {                                    \
        if (assertion) {                    \
            fprintf(stderr, "(%s, %d): ",   \
                    __FILE__, __LINE__);    \
            perror(call_description);       \
            exit(EXIT_FAILURE);             \
        }                                   \
    } while(0)


#endif
