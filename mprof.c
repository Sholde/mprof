#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <mpi.h>

//
static int __real_size = -1;
static int __real_rank = -1;

int (*real_MPI_size)(MPI_Comm comm, int *size) = NULL;
int (*real_MPI_rank)(MPI_Comm comm, int *rank) = NULL;

//
static unsigned long long __count_send = 0;
static unsigned long long __count_recv = 0;
static unsigned long long __count_send_local = 0;
static unsigned long long __count_recv_local = 0;
static double __max_time_wait_send = 0;
static double __max_time_wait_recv = 0;

int (*real_MPI_Send)(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm) = NULL;
int (*real_MPI_Recv)(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status) = NULL;

//
static unsigned long long __count_process_hit_barrier = 0;
static unsigned long long __count_barrier_local = 0;
static unsigned long long __count_barrier = 0;
static double __max_time_wait_barrier = 0;

int (*real_MPI_Barrier)(MPI_Comm comm) = NULL;

//
int (*real_MPI_Init)(int *argc, char ***argv) = NULL;
int (*real_MPI_Finalize)() = NULL;

//
static unsigned long long *__list_of_process_send_to = NULL;
static unsigned long long *__list_of_process_recv_from = NULL;

/******************
 *    FUNCTION    *
 ******************/

__attribute((constructor)) void init(void)
{
  ;
}

int MPI_Init(int *argc, char ***argv)
{
  // First call
  if (!real_MPI_Init)
    {
      real_MPI_Init = (int (*)(int *, char ***))dlsym(RTLD_NEXT, "MPI_Init");

      if (!real_MPI_Init)
        {
          abort();
        }
    }

  // Call MPI_Init
  int ret = real_MPI_Init(argc, argv);

  //////////////////////////
  // Prepare all function //
  //////////////////////////

  // MPI_Comm_rank
  if (!real_MPI_rank)
    {
      real_MPI_rank = (int (*)(MPI_Comm, int *))dlsym(RTLD_DEFAULT, "MPI_Comm_rank");

      if (!real_MPI_rank)
        {
          abort();
        }
    }

  // Call MPI_Comm_rank
  real_MPI_rank(MPI_COMM_WORLD, &__real_rank);

  // MPI_Comm_size
  if (!real_MPI_size)
    {
      real_MPI_size = (int (*)(MPI_Comm, int *))dlsym(RTLD_DEFAULT, "MPI_Comm_size");

      if (!real_MPI_size)
        {
          abort();
        }
    }

  // Call MPI_Comm_size
  real_MPI_size(MPI_COMM_WORLD, &__real_size);

  // MPI_Send
  if (!real_MPI_Send)
    {
      real_MPI_Send = (int (*)(const void *, int, MPI_Datatype, int, int, MPI_Comm))dlsym(RTLD_NEXT, "MPI_Send");

      if (!real_MPI_Send)
        {
          abort();
        }
    }

  // MPI_Recv
  if (!real_MPI_Recv)
    {
      real_MPI_Recv = (int (*)(void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *))dlsym(RTLD_NEXT, "MPI_Recv");

      if (!real_MPI_Recv)
        {
          abort();
        }
    }

  // MPI_Barrier
  if (!real_MPI_Barrier)
    {
      real_MPI_Barrier = (int (*)(MPI_Comm))dlsym(RTLD_NEXT, "MPI_Barrier");

      if (!real_MPI_Barrier)
        {
          abort();
        }
    }

  // MPI_Finalize
  if (!real_MPI_Finalize)
    {
      real_MPI_Finalize = (int (*)())dlsym(RTLD_NEXT, "MPI_Finalize");

      if (!real_MPI_Finalize)
        {
          abort();
        }
    }

  // Allocate list
  __list_of_process_send_to = malloc(sizeof(unsigned long long) * __real_size);
  memset(__list_of_process_send_to, 0, sizeof(unsigned long long) * __real_size);

  __list_of_process_recv_from = malloc(sizeof(unsigned long long) * __real_size);
  memset(__list_of_process_recv_from, 0, sizeof(unsigned long long) * __real_size);

  //
  return ret;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  //
  __count_send_local++;

  //
  __list_of_process_send_to[dest] = 1;
  

  //
  double start, end;

  //
  start = MPI_Wtime();
  
  // Call MPI_Send
  int ret = real_MPI_Send(buf, count, datatype, dest, tag, comm);

  end = MPI_Wtime();

  //
  double elapsed = end - start;

  //
  if (elapsed > 0.0)
    {
      if (elapsed > __max_time_wait_send)
        {
          __max_time_wait_send = elapsed;
        }
    }
  
  return ret;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  //
  __count_recv_local++;

  //
  __list_of_process_recv_from[source] = 1;

  //
  double start, end;

  //
  start = MPI_Wtime();

  // Call MPI_Recv
  int ret = real_MPI_Recv(buf, count, datatype, source, tag, comm, status);

  end = MPI_Wtime();

  //
  double elapsed = end - start;

  //
  if (elapsed > 0.0)
    {
      if (elapsed > __max_time_wait_recv)
        {
          __max_time_wait_recv = elapsed;
        }
    }
  
  return ret;
}

int MPI_Barrier(MPI_Comm comm)
{
  //
  if (__real_rank == 0)
    {
      //
      __count_process_hit_barrier++;

      int buff;
      
      for (int i = 1; i < __real_size; i++)
        {
          real_MPI_Recv(&buff, 1, MPI_INT, i, 0, comm, MPI_STATUS_IGNORE);
          __count_process_hit_barrier++;

        }
      __count_process_hit_barrier = 0;
    }
  else
    {
      real_MPI_Send(&__real_rank, 1, MPI_INT, 0, 0, comm);
    }
  
  //
  __count_barrier_local++;
  
  //
  double start, end;

  //
  start = MPI_Wtime();

  // Call true MPI_Barrier
  int ret = real_MPI_Barrier(comm);

  end = MPI_Wtime();

  //
  double elapsed = end - start;

  //
  if (elapsed > 0.0)
    {
      if (elapsed > __max_time_wait_barrier)
        {
          __max_time_wait_barrier = elapsed;
        }
    }
  
  return ret;
}

int MPI_Finalize(void)
{
  //
  unsigned long long buff_send = __count_send_local;
  unsigned long long buff_recv = __count_recv_local;
  unsigned long long buff_barrier = __count_barrier_local;
  
  //
  MPI_Reduce(&buff_send, &__count_send, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&buff_recv, &__count_recv, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&buff_barrier, &__count_barrier, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

  if (__real_rank == 0)
    {
      // mprof
      fprintf(stderr, "===============================================================================\n");
      fprintf(stderr, "================================= MPI PROFILER ================================\n");
      fprintf(stderr, "===============================================================================\n");

      //
      fprintf(stderr, "GLOBAL SUMMARY:\n");

      // Send
      fprintf(stderr, "\t" "%lld message send\n", __count_send);
      
      //Recv
      fprintf(stderr, "\t" "%lld message recv\n", __count_recv);
      
      // Barrier
      fprintf(stderr, "\t" "%lld barrier passed\n", __count_barrier);

      //
      fprintf(stderr, "\n");
    }

  for (int i = 0; i < __real_size; i++)
    {
      if (i == __real_rank)
        {
          //
          fprintf(stderr, "LOCAL SUMMARY (Process %d):\n", __real_rank);

          // Send
          fprintf(stderr, "\t" "%lld message send", __count_send_local);

          if (__count_send_local)
            {
              //
              fprintf(stderr, " to [");

              //
              for (int j = 0; j < __real_size; j++)
                {
                  if (__list_of_process_send_to[j])
                    fprintf(stderr, " %d", j);
                }

              //
              fprintf(stderr, " ]");

              //
              fprintf(stderr, " (max: %f sec)\n", __max_time_wait_send);
            }
      
          // Recv
          fprintf(stderr, "\t" "%lld message recv", __count_recv_local);

          if (__count_recv_local)
            {
              //
              fprintf(stderr, " from [");

              //
              for (int j = 0; j < __real_size; j++)
                {
                  if (__list_of_process_recv_from[j])
                    fprintf(stderr, " %d", j);
                }

              //
              fprintf(stderr, " ]");

              //
              fprintf(stderr, " (max: %f sec)\n", __max_time_wait_recv);
            }
      
          // Barrier
          fprintf(stderr, "\t" "%lld barrier passed (max: %f sec)\n", __count_barrier_local, __max_time_wait_barrier);

          //
          fprintf(stderr, "\n");
        }

      // Call MPI_Barrier to print in order
      real_MPI_Barrier(MPI_COMM_WORLD);
    }

  if (__real_rank == 0)
    {
      // Error
      fprintf(stderr, "ERROR SUMMARY:\n");
      
      if (__count_process_hit_barrier != 0)
        {
          fprintf(stderr, "\t" "%lld processs waiting in barrier\n", __count_process_hit_barrier);
        }
      else
        {
          fprintf(stderr, "\t" "No error\n");
        }
    }

  // Release list
  free(__list_of_process_send_to);
  free(__list_of_process_recv_from);

  
  // Call MPI_Finalize
  return real_MPI_Finalize();
}

__attribute((destructor)) void finalize(void)
{
  ;
}
