/*
 * File: tst_p2p_many_to_one.c
 *
 * Functionality:
 *  Simple point-to-point many-to-one test, with everyone in the comm sending blocking to
 *  process zero, which receives with MPI_ANY_SOURCE.
 *  Works with intra- and inter- communicators and up to now with any C (standard and struct) type.
 *
 * Author: Rainer Keller
 *
 * Date: Aug 8th 2003
 */
#include "mpi.h"
#include "mpi_test_suite.h"

#undef DEBUG
#define DEBUG(x)

static char * buffer = NULL;

int tst_p2p_many_to_one_probe_anysource_init (const struct tst_env * env)
{
  int comm_rank;
  MPI_Comm comm;

  DEBUG (printf ("(Rank:%d) env->comm:%d env->type:%d env->values_num:%d\n",
                 tst_global_rank, env->comm, env->type, env->values_num));

  buffer = tst_type_allocvalues (env->type, env->values_num);

  /*
   * Now, initialize the buffer
   */
  comm = tst_comm_getcomm (env->comm);
  MPI_CHECK (MPI_Comm_rank (comm, &comm_rank));

  tst_type_setstandardarray (env->type, env->values_num, buffer, comm_rank);
  return 0;
}

int tst_p2p_many_to_one_probe_anysource_run (const struct tst_env * env)
{
  int comm_size;
  int comm_rank;
  int hash_value;
  MPI_Comm comm;
  MPI_Datatype type;
  MPI_Status status;

  comm = tst_comm_getcomm (env->comm);
  type = tst_type_getdatatype (env->type);

  if (tst_comm_getcommclass (env->comm) == TST_MPI_INTRA_COMM)
    MPI_CHECK (MPI_Comm_size (comm, &comm_size));
  else
    MPI_CHECK (MPI_Comm_remote_size (comm, &comm_size));

  MPI_CHECK (MPI_Comm_rank (comm, &comm_rank));

  hash_value = tst_hash_value (env);
  DEBUG (printf ("(Rank:%d) comm:%d type:%d test:%d hash_value:%d comm_size:%d comm_rank:%d\n",
                 tst_global_rank,
                 env->comm, env->type, env->test, hash_value,
                 comm_size, comm_rank));

  if (comm_rank == 0)
    {
      int rank;

      for (rank = 0; rank < comm_size-1; rank++)
        {
          int source;
          int tag;
          MPI_CHECK (MPI_Probe (MPI_ANY_SOURCE, MPI_ANY_TAG, comm, &status));
          DEBUG (printf ("(Rank:%d) MPI_SOURCE:%d MPI_TAG:%d\n",
                         comm_rank, status.MPI_SOURCE, status.MPI_TAG));
          if (status.MPI_SOURCE <= 0 ||
              status.MPI_SOURCE >= comm_size ||
              status.MPI_TAG != hash_value)
            ERROR (EINVAL, "Error in status after MPI_Probe");

          source = status.MPI_SOURCE;
          tag = status.MPI_TAG;

          MPI_CHECK (MPI_Recv (buffer, env->values_num, type, source, tag, comm, &status));
          if (status.MPI_SOURCE != source ||
              status.MPI_TAG != tag)
            ERROR (EINVAL, "Error in status after MPI_Recv");
          tst_test_checkstandardarray (env, buffer, source);
        }
    }
  else
    MPI_CHECK (MPI_Send (buffer, env->values_num, type, 0, hash_value, comm));

  return 0;
}

int tst_p2p_many_to_one_probe_anysource_cleanup (const struct tst_env * env)
{
  tst_type_freevalues (env->type, buffer, env->values_num);
  return 0;
}
