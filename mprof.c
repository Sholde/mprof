#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <mpi.h>

#include "mprof.h"

/**************************************
 *                                    *
 *       LOAD / UNLOAD FUNCTION       *
 *                                    *
 **************************************/

__attribute__((constructor)) void init(void)
{
  char *env = getenv(MPROF_ENV);

  if (env != NULL)
    {
      if (strcmp(env, "--verbose") == 0)
        {
          __verbose = 1;
        }

      if (strcmp(env, "--barrier") == 0)
        {
          __barrier = 1;
        }

      if (strcmp(env, "--finalize") == 0)
        {
          __finalize = 1;
        }
    }
}

__attribute__((destructor)) void finalize(void)
{
  ;
}

/**************************************
 *                                    *
 *         GETTER FUNCTION            *
 *                                    *
 **************************************/

static void get_MPI_Init(void)
{
  if (!real_MPI_Init)
    {
      real_MPI_Init = (int (*)(int *, char ***))dlsym(RTLD_NEXT, "MPI_Init");

      if (!real_MPI_Init)
        {
          abort();
        }
    }
}

static void get_MPI_Comm_rank(void)
{
  if (!real_MPI_rank)
    {
      real_MPI_rank = (int (*)(MPI_Comm, int *))dlsym(RTLD_DEFAULT, "MPI_Comm_rank");

      if (!real_MPI_rank)
        {
          abort();
        }
    }
}

static void get_MPI_Comm_size(void)
{
  if (!real_MPI_size)
    {
      real_MPI_size = (int (*)(MPI_Comm, int *))dlsym(RTLD_DEFAULT, "MPI_Comm_size");

      if (!real_MPI_size)
        {
          abort();
        }
    }
}

static void get_MPI_Send()
{
  if (!real_MPI_Send)
    {
      real_MPI_Send = (int (*)(const void *, int, MPI_Datatype, int, int, MPI_Comm))dlsym(RTLD_NEXT, "MPI_Send");

      if (!real_MPI_Send)
        {
          abort();
        }
    }
}

static void get_MPI_Recv()
{
  if (!real_MPI_Recv)
    {
      real_MPI_Recv = (int (*)(void *, int, MPI_Datatype, int, int, MPI_Comm, MPI_Status *))dlsym(RTLD_NEXT, "MPI_Recv");

      if (!real_MPI_Recv)
        {
          abort();
        }
    }
}

static void get_MPI_Barrier()
{
  if (!real_MPI_Barrier)
    {
      real_MPI_Barrier = (int (*)(MPI_Comm))dlsym(RTLD_NEXT, "MPI_Barrier");

      if (!real_MPI_Barrier)
        {
          abort();
        }
    }
}

static void get_MPI_Finalize()
{
  if (!real_MPI_Finalize)
    {
      real_MPI_Finalize = (int (*)())dlsym(RTLD_NEXT, "MPI_Finalize");

      if (!real_MPI_Finalize)
        {
          abort();
        }
    }
}

/**************************************
 *                                    *
 *            MPI FUNCTION            *
 *                                    *
 **************************************/

int MPI_Init(int *argc, char ***argv)
{
  // First call
  get_MPI_Init();
  
  // Call MPI_Init
  int ret = real_MPI_Init(argc, argv);

  //////////////////////////
  // Prepare all function //
  //////////////////////////

  get_MPI_Comm_rank();
  get_MPI_Comm_size();
  get_MPI_Send();
  get_MPI_Recv();
  get_MPI_Barrier();
  get_MPI_Finalize();

  // Call MPI_Comm_rank
  real_MPI_rank(MPI_COMM_WORLD, &__real_rank);
  
  // Call MPI_Comm_size
  real_MPI_size(MPI_COMM_WORLD, &__real_size);

  // Allocate list
  __list_of_process_send_to = malloc(sizeof(unsigned long long) * __real_size);
  memset(__list_of_process_send_to, 0, sizeof(unsigned long long) * __real_size);

  __list_of_process_recv_from = malloc(sizeof(unsigned long long) * __real_size);
  memset(__list_of_process_recv_from, 0, sizeof(unsigned long long) * __real_size);

  //
  if (__verbose)
    {
      fprintf(stderr, MPROF "Process %d enter in MPI_init\n", __real_rank);
    }
  
  //
  return ret;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  //
  if (__verbose)
    {
      fprintf(stderr, MPROF "Process %d enter in MPI_Send\n", __real_rank);
    }

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

      __total_time_wait_send += elapsed;
    }
  
  return ret;
}

int MPI_Recv(void *buf, int count, MPI_Datatype datatype, int source, int tag, MPI_Comm comm, MPI_Status *status)
{
  //
  if (__verbose)
    {
      fprintf(stderr, MPROF "Process %d enter in MPI_Recv\n", __real_rank);
    }

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

      __total_time_wait_recv += elapsed;
    }
  
  return ret;
}

int MPI_Barrier(MPI_Comm comm)
{
  //
  if (__verbose || __barrier)
    {
      fprintf(stderr, MPROF "Process %d enter in MPI_Barrier\n", __real_rank);
    }

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

      __total_time_wait_barrier += elapsed;
    }
  
  return ret;
}

//
static void fprintf_bignumber(unsigned long long n)
{
  if (n < 1000)
    {
      fprintf(stderr, "%lld", n);
      return;
    }
  
  fprintf_bignumber(n / 1000);
  fprintf(stderr, ",%03lld", n % 1000);
}

int MPI_Finalize(void)
{
  //
  if (__verbose || __finalize)
    {
      fprintf(stderr, MPROF "Process %d enter in MPI_Finalize\n", __real_rank);
    }

  //
  unsigned long long buff_count_send = __count_send_local;
  unsigned long long buff_count_recv = __count_recv_local;

  //
  MPI_Reduce(&buff_count_send, &__count_send, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&buff_count_recv, &__count_recv, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  __count_barrier = __count_barrier_local;
  
  //
  double buff_time_send = __total_time_wait_send;
  double buff_time_recv = __total_time_wait_recv;
  double buff_time_barrier = __total_time_wait_barrier;
  
  //
  MPI_Reduce(&buff_time_send, &__global_time_send, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&buff_time_recv, &__global_time_recv, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&buff_time_barrier, &__global_time_barrier, 1, MPI_DOUBLE, MPI_SUM, 0, MPI_COMM_WORLD);

  // Global summary
  if (__real_rank == 0)
    {
      // mprof
      fprintf(stderr, "===============================================================================\n");
      fprintf(stderr, "================================= MPI PROFILER ================================\n");
      fprintf(stderr, "===============================================================================\n");

      //
      fprintf(stderr, MPROF "GLOBAL SUMMARY:\n");

      // Send
      fprintf(stderr, MPROF "         message sent: ");
      fprintf_bignumber(__count_send);
      fprintf(stderr, " - waiting %f sec in total\n", __global_time_send);

      // Send
      fprintf(stderr, MPROF "         message recv: ");
      fprintf_bignumber(__count_recv);
      fprintf(stderr, " - waiting %f sec in total\n", __global_time_recv);

      // Send
      fprintf(stderr, MPROF "    barrier(s) passed: ");
      fprintf_bignumber(__count_barrier);
      fprintf(stderr, " - waiting %f sec in total\n", __global_time_barrier);

      //
      fprintf(stderr, MPROF "\n");
    }

  // Local summary
  for (int i = 0; i < __real_size; i++)
    {
      if (i == __real_rank)
        {
          //
          fprintf(stderr, MPROF "LOCAL SUMMARY (Process ");
          fprintf_bignumber(__real_rank);
          fprintf(stderr, "):\n");

          // Send
          fprintf(stderr, MPROF "         message sent: ");
          fprintf_bignumber(__count_send_local);
          fprintf(stderr, " - waiting %f sec (max: %f sec)\n", __total_time_wait_send, __max_time_wait_send);

          // Recv
          fprintf(stderr, MPROF "         message recv: ");
          fprintf_bignumber(__count_recv_local);
          fprintf(stderr, " - waiting %f sec (max: %f sec)\n", __total_time_wait_recv, __max_time_wait_recv);

          // Barrier
          fprintf(stderr, MPROF "    barrier(s) passed: ");
          fprintf_bignumber(__count_barrier_local);
          fprintf(stderr, " - waiting %f sec (max: %f sec)\n", __total_time_wait_barrier, __max_time_wait_barrier);

          // List process sent to
          fprintf(stderr, MPROF "      list(s) sent to:");

          if (__count_send_local)
            {
              //
              for (int j = 0; j < __real_size; j++)
                {
                  if (__list_of_process_send_to[j])
                    fprintf(stderr, " %d", j);
                }
            }

          fprintf(stderr, "\n");

          // List process received from
          fprintf(stderr, MPROF "    list(s) recv from:");

          if (__count_recv_local)
            {
              //
              for (int j = 0; j < __real_size; j++)
                {
                  if (__list_of_process_recv_from[j])
                    fprintf(stderr, " %d", j);
                }
            }

          fprintf(stderr, "\n");
      
          // Separate process
          fprintf(stderr, MPROF "\n");
        }

      // Try to unifrom the output
      fflush(stderr);

      // Call MPI_Barrier to print in order
      real_MPI_Barrier(MPI_COMM_WORLD);
    }

  // Error summary
  if (__real_rank == 0)
    {
      // Error
      fprintf(stderr, MPROF "ERROR SUMMARY:\n");
      
      if (__count_process_hit_barrier != 0)
        {
          // Barrier
          fprintf(stderr, MPROF "  process(es) blocked: ");
          fprintf_bignumber(__count_process_hit_barrier);
          fprintf(stderr, " in barrier\n");
          fprintf(stderr, MPROF "  process(es) in wait: ");
          fprintf_bignumber(__real_size - __count_process_hit_barrier);
          fprintf(stderr, " in barrier\n");
        }
      else
        {
          fprintf(stderr, MPROF "         No errors\n");
        }
    }

  // Release list
  free(__list_of_process_send_to);
  free(__list_of_process_recv_from);

  // Call MPI_Finalize
  return real_MPI_Finalize();
}
