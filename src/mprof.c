#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dlfcn.h>
#include <time.h>
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
          __debug = 1;
          __verbose = 1;
        }
      else if (strcmp(env, "--barrier") == 0)
        {
          __debug = 1;
          __barrier = 1;
        }
      else if (strcmp(env, "--finalize") == 0)
        {
          __debug = 1;
          __finalize = 1;
        }
      else if (strcmp(env, "--profile") == 0)
        {
          __debug = 1;
          __profile = 1;
        }
    }
}

__attribute__((destructor)) void finalize(void)
{
  ;
}

/**************************************
 *                                    *
 *          HELPER FUNCTION           *
 *                                    *
 **************************************/

//
static inline void update_global_variable(void)
{
  //
  unsigned long long buff_count_send = __count_send_local;
  unsigned long long buff_count_recv = __count_recv_local;
  unsigned long long buff_count_warning = __count_warning_local;

  //
  MPI_Reduce(&buff_count_send, &__count_send, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&buff_count_recv, &__count_recv, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  __count_barrier = __count_barrier_local;
  MPI_Reduce(&buff_count_warning, &__count_warning, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

  //
  unsigned long long buff_count_bytes_send = __count_bytes_send_local;
  unsigned long long buff_count_bytes_recv = __count_bytes_recv_local;

  //
  MPI_Reduce(&buff_count_bytes_send, &__count_bytes_send, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);
  MPI_Reduce(&buff_count_bytes_recv, &__count_bytes_recv, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

  //
  unsigned long long buff_count_send_contiguous = __count_send_contiguous_local;

  //
  MPI_Reduce(&buff_count_send_contiguous, &__count_send_contiguous, 1, MPI_UNSIGNED_LONG_LONG, MPI_SUM, 0, MPI_COMM_WORLD);

  //
  double buff_time_send = __total_time_wait_send;
  double buff_time_recv = __total_time_wait_recv;
  double buff_time_barrier = __total_time_wait_barrier;
  
  //
  MPI_Reduce(&buff_time_send, &__global_time_send, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&buff_time_recv, &__global_time_recv, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
  MPI_Reduce(&buff_time_barrier, &__global_time_barrier, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  //
  double buff_app_time = __process_time;
  MPI_Reduce(&buff_app_time, &__app_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);

  //
  char buff_error_monitor_time = __error_monitor_time;
  MPI_Reduce(&buff_error_monitor_time, &__error_monitor_time, 1, MPI_CHAR, MPI_MAX, 0, MPI_COMM_WORLD);

  //
  __mpi_time_local = __total_time_wait_send + __total_time_wait_recv + __total_time_wait_barrier;
  double buff_mpi_time = __mpi_time_local;
  MPI_Reduce(&buff_mpi_time, &__mpi_time, 1, MPI_DOUBLE, MPI_MAX, 0, MPI_COMM_WORLD);
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

//
static void fprintf_time(double time)
{
  if (time <= 0.000001)
    {
      fprintf(stderr, "%.f ns", (time * 1000000000));
    }
  else if (time <= 0.001)
    {
      fprintf(stderr, "%.f us", (time * 1000000));
    }
  else if (time <= 1.0)
    {
      fprintf(stderr, "%.f ms", (time * 1000));
    }
  else if (time <= 60)
    {
      fprintf(stderr, "%.3f s", (time));
    }
  else
    {
      fprintf(stderr, "%lld m %lld s", (unsigned long long)(time / 60), (unsigned long long)((unsigned long long)time % 60));
    }
}

static inline void fprintf_mprof()
{
  fprintf(stderr, "===============================================================================\n");
  fprintf(stderr, "================================= MPI PROFILER ================================\n");
  fprintf(stderr, "===============================================================================\n");
}

static inline void fprintf_global_summary()
{
  //
  fprintf(stderr, MPROF "GLOBAL SUMMARY:\n");
}

static inline void fprintf_global_app_time(double time)
{
  fprintf(stderr, MPROF "              running: ");
  fprintf_time(time);
  fprintf(stderr, "\n");
}

static inline void fprintf_global_mpi_time(double time)
{
  fprintf(stderr, MPROF "             mpi time: ");
  fprintf_time(time);
  fprintf(stderr, "\n");
}

static inline void fprintf_global_msg_send(unsigned long long count, unsigned long long bytes, double time)
{
  fprintf(stderr, MPROF "      message(s) sent: ");

  if (count)
    {
      fprintf_bignumber(count);
      fprintf(stderr, " msg take ");
      fprintf_bignumber(bytes);
      fprintf(stderr, " bytes - waiting ");
      fprintf_time(time);
      fprintf(stderr, " maximum\n");
    }
  else
    {
      fprintf(stderr, "No send\n");
    }
}

static inline void fprintf_global_msg_recv(unsigned long long count, unsigned long long bytes, double time)
{
  fprintf(stderr, MPROF "      message(s) recv: ");
  
  if (count)
    {
      fprintf_bignumber(count);
      fprintf(stderr, " msg take ");
      fprintf_bignumber(bytes);
      fprintf(stderr, " bytes - waiting ");
      fprintf_time(time);
      fprintf(stderr, " maximum\n");
    }
  else
    {
      fprintf(stderr, "No recv\n");
    }
}

static inline void fprintf_global_barrier(unsigned long long count, double time)
{
  fprintf(stderr, MPROF "    barrier(s) passed: ");
  
  if (count)
    {  
      fprintf_bignumber(count);
      fprintf(stderr, " - waiting ");
      fprintf_time(time);
      fprintf(stderr, " maximum\n");
    }
  else
    {
      fprintf(stderr, "No barrier\n");
    }
}

static inline void fprintf_warning(unsigned long long count, unsigned long long send_contiguous)
{
  fprintf(stderr, MPROF "           warning(s): ");
  fprintf_bignumber(count);

  if (send_contiguous)
    {
      fprintf(stderr, " - ");
      fprintf_bignumber(send_contiguous);
      fprintf(stderr, " contiguous send");
    }

  fprintf(stderr, "\n");
}

static inline void fprintf_local_summary(int rank)
{
  fprintf(stderr, MPROF "LOCAL SUMMARY (Process ");
  fprintf_bignumber(rank);
  fprintf(stderr, "):\n");
}

static inline void fprintf_local_process_time(double time)
{
  fprintf(stderr, MPROF "              running: ");
  fprintf_time(time);
  fprintf(stderr, "\n");
}

static inline void fprintf_local_mpi_time(double time)
{
  fprintf(stderr, MPROF "             mpi time: ");
  fprintf_time(time);
  fprintf(stderr, "\n");
}

static inline void fprintf_local_msg_send(unsigned long long count, unsigned long long bytes, double time, double max)
{
  fprintf(stderr, MPROF "      message(s) sent: ");

  if (count)
    {
      fprintf_bignumber(count);
      fprintf(stderr, " msg take ");
      fprintf_bignumber(bytes);
      fprintf(stderr, " bytes - waiting ");
      fprintf_time(time);
      fprintf(stderr, " (max: ");
      fprintf_time(max);
      fprintf(stderr, ")\n");
    }
  else
    {
      fprintf(stderr, "No send\n");
    }
}

static inline void fprintf_local_msg_recv(unsigned long long count, unsigned long long bytes, double time, double max)
{
  fprintf(stderr, MPROF "      message(s) recv: ");

  if (count)
    {
      fprintf_bignumber(count);
      fprintf(stderr, " msg take ");
      fprintf_bignumber(bytes);
      fprintf(stderr, " bytes - waiting ");
      fprintf_time(time);
      fprintf(stderr, " (max: ");
      fprintf_time(max);
      fprintf(stderr, ")\n");
    }
  else
    {
      fprintf(stderr, "No recv\n");
    }
}

static inline void fprintf_local_barrier(unsigned long long count, double time, double max)
{
  fprintf(stderr, MPROF "    barrier(s) passed: ");

  if (count)
    {
      fprintf_bignumber(count);
      fprintf(stderr, " - waiting ");
      fprintf_time(time);
      fprintf(stderr, " (max: ");
      fprintf_time(max);
      fprintf(stderr, ")\n");
    }
  else
    {
      fprintf(stderr, "No barrier\n");
    }
}

static inline void fprintf_process_blocked(unsigned long long count)
{
  fprintf(stderr, MPROF "  process(es) blocked: ");
  fprintf_bignumber(count);
  fprintf(stderr, " in barrier\n");
}

static inline void fprintf_process_in_wait(unsigned long long count)
{
  fprintf(stderr, MPROF "  process(es) in wait: ");
  fprintf_bignumber(count);
  fprintf(stderr, " in barrier\n");
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
  if (__debug)
    {
      if (__verbose)
        {
          fprintf(stderr, MPROF "Process %d enter in MPI_init\n", __real_rank);
        }
    }

  //
  __start = MPI_Wtime();
  
  //
  return ret;
}

int MPI_Send(const void *buf, int count, MPI_Datatype datatype, int dest, int tag, MPI_Comm comm)
{
  //
  char send_to_me = 0;

  if (dest == __real_rank)
    {
      send_to_me = 1;
      __count_warning_local++;
    }
  
  //
  int buff_size = 0;
  MPI_Type_size(datatype, &buff_size);
  __count_bytes_send_local += count * buff_size;

  //
  int len;
  char buff_type_name[64] = { 0 };
  MPI_Type_get_name(datatype, buff_type_name, &len);

  //
  char send_contiguous = 0;

  //
  if (__send_previous_contiguous_addr != NULL &&
      (unsigned long long)__send_previous_contiguous_addr + __send_previous_shift == (unsigned long long)buf)
    {
      //
      send_contiguous = 1;

      //
      __count_warning_local++;

      //
      __count_send_contiguous_local++;
    }
  
  __send_previous_contiguous_addr = (void *)buf;
  __send_previous_shift = count * buff_size;

  //
  if (__debug)
    {
      if (__verbose)
        {
          fprintf(stderr, MPROF "Process %d enter in MPI_Send, send %d %s to %d\n", __real_rank, count, buff_type_name, dest);
        }

      if (__profile)
        {
          if (send_to_me)
            {
              fprintf(stderr, MPROF "WARNING: Process %d send to himself\n", __real_rank);
            }

          if (send_contiguous)
            {
              fprintf(stderr, MPROF "PROFILE: Process %d send independently elements which are contiguous to Process %d\n"
                      MPROF         "            - Sending %d element(s) of %s\n", __real_rank, dest, count, buff_type_name);
            }
        }
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

  //
  end = MPI_Wtime();

  //
  if (end - start >= 0.0)
    {
      double elapsed = end - start;

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
  char recv_from_me = 0;

  if (source == __real_rank)
    {
      recv_from_me = 1;
      __count_warning_local++;
    }
  
  //
  int buff_size = 0;
  MPI_Type_size(datatype, &buff_size);
  __count_bytes_recv_local += count * buff_size;

  //
  int len;
  char buff_type_name[64] = { 0 };
  MPI_Type_get_name(datatype, buff_type_name, &len);

  //
  if (__debug)
    {
      if (__verbose)
        {
          int len;
          char buff[64] = { 0 };
          MPI_Type_get_name(datatype, buff, &len);
          
          fprintf(stderr, MPROF "Process %d enter in MPI_Send, recv %d %s from %d\n", __real_rank, count, buff, source);
        }

      if (__profile)
        {
          if (recv_from_me)
            {
              fprintf(stderr, MPROF "WARNING: Process %d recv from himself\n", __real_rank);
            }
        }
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

  //
  end = MPI_Wtime();

  //
  if (end - start >= 0.0)
    {
      double elapsed = end - start;

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
  if (__debug)
    {
      //
      if (__verbose || __barrier)
        {
          fprintf(stderr, MPROF "Process %d enter in MPI_Barrier\n", __real_rank);
        }
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

  //
  end = MPI_Wtime();

  //
  if (end - start >= 0.0)
    {
      double elapsed = end - start;

      if (elapsed > __max_time_wait_barrier)
        {
          __max_time_wait_barrier = elapsed;
        }

      __total_time_wait_barrier += elapsed;
    }
  
  return ret;
}

int MPI_Finalize(void)
{
  // Get time of MPI process
  __end = MPI_Wtime();

  if (__end - __start < 0.0)
    {
      __process_time = 0.0;
      __error_monitor_time = 1;
    }
  else
    {
      __process_time = __end - __start;
    }

  //
  if (__debug)
    {
      //
      if (__verbose || __finalize)
        {
          fprintf(stderr, MPROF "Process %d enter in MPI_Finalize\n", __real_rank);
        }
    }

  // Update global variable
  update_global_variable();

  // Global summary
  if (__real_rank == 0)
    {
      // mprof
      fprintf_mprof();

      //
      fprintf_global_summary();
      fprintf_global_app_time(__app_time);
      fprintf_global_mpi_time(__mpi_time);
      fprintf_global_msg_send(__count_send, __count_bytes_send, __global_time_send);
      fprintf_global_msg_recv(__count_recv, __count_bytes_recv, __global_time_recv);
      fprintf_global_barrier(__count_barrier, __global_time_barrier);

      // Warning
      if (__count_warning)
        {
          fprintf_warning(__count_warning, __count_send_contiguous);
        }

      //
      fprintf(stderr, MPROF "\n");
    }

  // Local summary

  // Array to store all information for all process to print onl with process 0
  unsigned long long *buff_count_send = NULL;
  unsigned long long *buff_count_recv = NULL;
  unsigned long long *buff_count_barrier = NULL;
  unsigned long long *buff_count_warning = NULL;

  unsigned long long *buff_count_bytes_send = NULL;
  unsigned long long *buff_count_bytes_recv = NULL;

  unsigned long long *buff_count_send_contiguous = NULL;

  double *buff_total_time_wait_send = NULL;
  double *buff_total_time_wait_recv = NULL;
  double *buff_total_time_wait_barrier = NULL;

  double *buff_max_time_wait_send = NULL;
  double *buff_max_time_wait_recv = NULL;
  double *buff_max_time_wait_barrier = NULL;

  unsigned long long *buff_list_send_to = NULL;
  unsigned long long *buff_list_recv_from = NULL;

  double *buff_process_time = NULL;
  double *buff_mpi_time = NULL;

  // Allocate array only for process 0
  if (__real_rank == 0)
    {
      buff_count_send = malloc(sizeof(unsigned long long) * __real_size);
      buff_count_recv = malloc(sizeof(unsigned long long) * __real_size);
      buff_count_barrier = malloc(sizeof(unsigned long long) * __real_size);
      buff_count_warning = malloc(sizeof(unsigned long long) * __real_size);

      buff_count_bytes_send = malloc(sizeof(unsigned long long) * __real_size);
      buff_count_bytes_recv = malloc(sizeof(unsigned long long) * __real_size);

      buff_count_send_contiguous = malloc(sizeof(unsigned long long) * __real_size);

      buff_total_time_wait_send  = malloc(sizeof(double) * __real_size);
      buff_total_time_wait_recv  = malloc(sizeof(double) * __real_size);
      buff_total_time_wait_barrier  = malloc(sizeof(double) * __real_size);
      
      buff_max_time_wait_send  = malloc(sizeof(double) * __real_size);
      buff_max_time_wait_recv  = malloc(sizeof(double) * __real_size);
      buff_max_time_wait_barrier  = malloc(sizeof(double) * __real_size);

      buff_list_send_to = malloc(sizeof(unsigned long long *) * __real_size * __real_size);
      buff_list_recv_from = malloc(sizeof(unsigned long long *) * __real_size * __real_size);

      buff_process_time = malloc(sizeof(double) * __real_size);
      buff_mpi_time = malloc(sizeof(double) * __real_size);
    }

  // Recup all information
  MPI_Gather(&__count_send_local, 1, MPI_UNSIGNED_LONG_LONG, buff_count_send, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  MPI_Gather(&__count_recv_local, 1, MPI_UNSIGNED_LONG_LONG, buff_count_recv, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  MPI_Gather(&__count_barrier_local, 1, MPI_UNSIGNED_LONG_LONG, buff_count_barrier, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  MPI_Gather(&__count_warning_local, 1, MPI_UNSIGNED_LONG_LONG, buff_count_warning, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

  MPI_Gather(&__count_bytes_send_local, 1, MPI_UNSIGNED_LONG_LONG, buff_count_bytes_send, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  MPI_Gather(&__count_bytes_recv_local, 1, MPI_UNSIGNED_LONG_LONG, buff_count_bytes_recv, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

  MPI_Gather(&__count_send_contiguous_local, 1, MPI_UNSIGNED_LONG_LONG, buff_count_send_contiguous, 1, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

  MPI_Gather(&__total_time_wait_send, 1, MPI_DOUBLE, buff_total_time_wait_send, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Gather(&__total_time_wait_recv, 1, MPI_DOUBLE, buff_total_time_wait_recv, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Gather(&__total_time_wait_barrier, 1, MPI_DOUBLE, buff_total_time_wait_barrier, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  MPI_Gather(&__max_time_wait_send, 1, MPI_DOUBLE, buff_max_time_wait_send, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Gather(&__max_time_wait_recv, 1, MPI_DOUBLE, buff_max_time_wait_recv, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Gather(&__max_time_wait_barrier, 1, MPI_DOUBLE, buff_max_time_wait_barrier, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  MPI_Gather(__list_of_process_send_to, __real_size, MPI_UNSIGNED_LONG_LONG, buff_list_send_to, __real_size, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);
  MPI_Gather(__list_of_process_recv_from, __real_size, MPI_UNSIGNED_LONG_LONG, buff_list_recv_from, __real_size, MPI_UNSIGNED_LONG_LONG, 0, MPI_COMM_WORLD);

  MPI_Gather(&__process_time, 1, MPI_DOUBLE, buff_process_time, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);
  MPI_Gather(&__mpi_time_local, 1, MPI_DOUBLE, buff_mpi_time, 1, MPI_DOUBLE, 0, MPI_COMM_WORLD);

  // Print only with process 0
  if (__real_rank == 0)
    {
      for (int i = 0; i < __real_size; i++)
        {
          // Print local
          fprintf_local_summary(i);

          // Test if we need to print info
          if (buff_count_send[i] || buff_count_recv[i] || buff_count_barrier[i] || buff_count_warning[i])
            {
              //
              fprintf_local_process_time(buff_process_time[i]);
              fprintf_local_mpi_time(buff_mpi_time[i]);
              fprintf_local_msg_send(buff_count_send[i], buff_count_bytes_send[i], buff_total_time_wait_send[i], buff_max_time_wait_send[i]);
              fprintf_local_msg_recv(buff_count_recv[i], buff_count_bytes_recv[i], buff_total_time_wait_recv[i], buff_max_time_wait_recv[i]);
              fprintf_local_barrier(buff_count_barrier[i], buff_total_time_wait_barrier[i], buff_max_time_wait_barrier[i]);

              // List process sent to
              fprintf(stderr, MPROF "      list(s) sent to:");

              if (buff_count_send[i])
                {
                  //
                  for (int j = 0; j < __real_size; j++)
                    {
                      if (buff_list_send_to[i * __real_size + j])
                        fprintf(stderr, " %d", j);
                    }
                }

              fprintf(stderr, "\n");

              // List process received from
              fprintf(stderr, MPROF "    list(s) recv from:");

              if (buff_count_recv[i])
                {
                  //
                  for (int j = 0; j < __real_size; j++)
                    {
                      if (buff_list_recv_from[i * __real_size + j])
                        fprintf(stderr, " %d", j);
                    }
                }

              fprintf(stderr, "\n");
      
              // Warning
              if (buff_count_warning[i])
                {
                  fprintf_warning(buff_count_warning[i], buff_count_send_contiguous[i]);
                }

              // Separate process
              fprintf(stderr, MPROF "\n");
            }
          else
            {
              fprintf(stderr, MPROF SPACE_MSG "No communication\n");
              fprintf(stderr, MPROF "\n");
            }

          // Try to unifrom the output
          fflush(stderr);
        }
    }

  // Release memory only for process 0
  if (__real_rank == 0)
    {
      free(buff_count_send);
      free(buff_count_recv);
      free(buff_count_barrier);
      free(buff_count_warning);

      free(buff_count_bytes_send);
      free(buff_count_bytes_recv);

      free(buff_count_send_contiguous);

      free(buff_total_time_wait_send);
      free(buff_total_time_wait_recv);
      free(buff_total_time_wait_barrier);

      free(buff_max_time_wait_send);
      free(buff_max_time_wait_recv);
      free(buff_max_time_wait_barrier);

      free(buff_list_send_to);
      free(buff_list_recv_from);

      free(buff_process_time);
      free(buff_mpi_time);
    }

  // Error summary
  if (__real_rank == 0)
    {
      // Error
      fprintf(stderr, MPROF "ERROR SUMMARY:\n");
      
      if (__count_process_hit_barrier != 0)
        {
          // Blocked in barrier
          fprintf_process_blocked(__count_process_hit_barrier);

          // In wait for barrier
          fprintf_process_in_wait(__real_size - __count_process_hit_barrier);
        }
      else if (__error_monitor_time != 0)
        {
          fprintf(stderr, MPROF "             time: errors occur when process time was monitored\n");
        }
      else
        {
          fprintf(stderr, MPROF SPACE_MSG "No errors\n");
        }
    }

  // Release list
  free(__list_of_process_send_to);
  free(__list_of_process_recv_from);

  // Call MPI_Finalize
  return real_MPI_Finalize();
}