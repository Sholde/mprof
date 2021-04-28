#ifndef _MPROF_H_
#define _MPROF_H_

#define MPROF "==mprof== "
#define MPROF_ENV "MPROF_OPTIONS"
#define SPACE_MSG "        "

//
static unsigned long long __count_mpi_call = 0;
static unsigned long long __count_mpi_call_local = 0;

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

// count bytes send and recv
static unsigned long long __count_bytes_send = 0;
static unsigned long long __count_bytes_recv = 0;
static unsigned long long __count_bytes_send_local = 0;
static unsigned long long __count_bytes_recv_local = 0;

//
static unsigned long long __count_contiguous_send_local = 0;
static unsigned long long __count_contiguous_send = 0;

static unsigned long long __send_previous_contiguous_addr = 0;
static unsigned long long __send_previous_shift = 0;

// Time for send and recv
static double __max_time_wait_send = 0.0;
static double __total_time_wait_send = 0.0;
static double __max_time_wait_recv = 0.0;
static double __total_time_wait_recv = 0.0;

static double __global_time_send = 0.0;
static double __global_time_recv = 0.0;

// Store MPI function to send and recv
int (*real_MPI_Send)(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) = NULL;
int (*real_MPI_Recv)(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) = NULL;

// Count barrier
static unsigned long long __count_process_hit_barrier = 0;
static unsigned long long __count_barrier_local = 0;
static unsigned long long __count_barrier = 0;

// Time for barrier
static double __max_time_wait_barrier = 0.0;
static double __total_time_wait_barrier = 0.0;

static double __global_time_barrier = 0.0;

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
static char __profile = 0;

// Monitoring time
static double __start = 0.0;
static double __end = 0.0;
static double __process_time = 0.0;
static double __app_time = 0.0;
static double __mpi_time_local = 0.0;
static double __mpi_time = 0.0;
static char __error_monitor_time = 0;

#endif // _MPROF_H_
