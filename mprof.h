#ifndef _MPROF_H_
#define _MPROF_H_

#define MPROF "==mprof== "
#define MPROF_ENV "MPROF_OPTIONS"

// Store MPI variable
static int __real_size = -1;
static int __real_rank = -1;

// Store MPI function to get MPI variable
int (*real_MPI_size)(MPI_Comm comm, int *size) = NULL;
int (*real_MPI_rank)(MPI_Comm comm, int *rank) = NULL;

// Count send and recv
static unsigned long long __count_send = 0;
static unsigned long long __count_recv = 0;
static unsigned long long __count_send_local = 0;
static unsigned long long __count_recv_local = 0;

// Time for send and recv
static unsigned long long __max_time_wait_send = 0;
static unsigned long long __total_time_wait_send = 0;
static unsigned long long __max_time_wait_recv = 0;
static unsigned long long __total_time_wait_recv = 0;

static unsigned long long __global_time_send = 0;
static unsigned long long __global_time_recv = 0;

// Store MPI function to send and recv
int (*real_MPI_Send)(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) = NULL;
int (*real_MPI_Recv)(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) = NULL;

// Count barrier
static unsigned long long __count_process_hit_barrier = 0;
static unsigned long long __count_barrier_local = 0;
static unsigned long long __count_barrier = 0;

// Time for barrier
static unsigned long long __max_time_wait_barrier = 0;
static unsigned long long __total_time_wait_barrier = 0;

static unsigned long long __global_time_barrier = 0;

// Store MPI Barrier
int (*real_MPI_Barrier)(MPI_Comm comm) = NULL;

// Store MPI Init and Finalize
int (*real_MPI_Init)(int *argc, char ***argv) = NULL;
int (*real_MPI_Finalize)() = NULL;

// List of process
static unsigned long long *__list_of_process_send_to = NULL;
static unsigned long long *__list_of_process_recv_from = NULL;

// Handle warning
static unsigned long long __count_warning_local = 0;
static unsigned long long __count_warning = 0;

// DEBUG variable
static char __debug = 0;
static char __verbose = 0;
static char __barrier = 0;
static char __finalize = 0;
static char __warning = 0;

// Monitoring time
static struct timespec __start;
static struct timespec __end;
static unsigned long long __process_time;
static unsigned long long __app_time;

#endif // _MPROF_H_
