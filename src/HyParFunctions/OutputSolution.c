#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <basic.h>
#include <arrayfunctions.h>
#include <mpivars.h>
#include <hypar.h>

/* Function declarations */
void IncrementFilename        (char*);
static int OutputSolutionSerial   (void*,void*);
static int OutputSolutionParallel (void*,void*);

int OutputSolution(void *s, void *m)
{
  HyPar         *solver = (HyPar*)       s;
  MPIVariables  *mpi    = (MPIVariables*)m;
  _DECLARE_IERR_;

  /* if WriteOutput() is NULL, then return */
  if (!solver->WriteOutput) return(0);
  if (!strcmp(solver->output_mode,"serial")) 
    return(OutputSolutionSerial(solver,mpi));
  else
    return(OutputSolutionParallel(solver,mpi));
}

/*
  Function to write out the solution to file in serial mode.
  It will allocate the global domain on rank 0, so do not
  use for big problems for which the entire global domain 
  will not fit on one node. This approach is also not very
  scalable.
*/
int OutputSolutionSerial(void *s, void *m)
{
  HyPar         *solver = (HyPar*)       s;
  MPIVariables  *mpi    = (MPIVariables*)m;
  int           d;
  _DECLARE_IERR_;

  /* root process: allocate global output arrays */
  double *ug, *xg;
  if (!mpi->rank) {
    int size_global;

    size_global = 1;
    for (d=0; d<solver->ndims; d++) size_global *= solver->dim_global[d];
    ug = (double*) calloc (size_global*solver->nvars,sizeof(double));
    _ArraySetValue_(ug,size_global*solver->nvars,0.0);

    size_global = 0;
    for (d=0; d<solver->ndims; d++) size_global += solver->dim_global[d];
    xg = (double*) calloc (size_global,sizeof(double));
    _ArraySetValue_(xg,size_global,0.0); CHECKERR(ierr);

  } else {

    /* null pointers on non-root processes */
    ug = xg = NULL;

  }

  /* Assemble the local output arrays into the global output arrays */
  IERR MPIGatherArraynD(solver->ndims,mpi,ug,solver->u,solver->dim_global,
                          solver->dim_local,solver->ghosts,solver->nvars);  CHECKERR(ierr);
  int offset_global, offset_local;
  offset_global = offset_local = 0;
  for (d=0; d<solver->ndims; d++) {
    IERR MPIGatherArray1D(mpi,(mpi->rank?NULL:&xg[offset_global]),
                            &solver->x[offset_local+solver->ghosts],
                            mpi->is[d],mpi->ie[d],solver->dim_local[d],0); CHECKERR(ierr);
    offset_global += solver->dim_global[d];
    offset_local  += solver->dim_local [d] + 2*solver->ghosts;
  }

  if (!mpi->rank) {
    /* write output file to disk */
    char filename[_MAX_STRING_SIZE_] = "";
    strcat(filename,solver->op_filename);
    printf("Writing solution file %s.\n",filename);
    IERR solver->WriteOutput(solver->ndims,solver->nvars,solver->dim_global,xg,ug,
                               filename,solver->index); CHECKERR(ierr);
    if (!strcmp(solver->op_overwrite,"no"))  IncrementFilename(solver->op_filename);

    /* Clean up output arrays */
    free(xg);
    free(ug);
  }

  return(0);
}

/*
  Function to write the solution file in parallel. It will use
  the specified number of I/O ranks in groups, just like in 
  parallel input of initial solution.  
*/
int OutputSolutionParallel(void *s, void *m)
{
  HyPar         *solver = (HyPar*)        s;
  MPIVariables  *mpi    = (MPIVariables*) m;
  int           i,proc,d;
  _DECLARE_IERR_;

  int ndims = solver->ndims;
  int nvars = solver->nvars;
  int ghosts = solver->ghosts;
  int *dim_local = solver->dim_local;

  static int count = 0;

  if (!mpi->rank) printf("Writing solution file %s.xxxx (parallel mode).\n",solver->op_filename);

  /* calculate size of the local grid on this rank */
  int sizex = 0;     for (d=0; d<ndims; d++) sizex += dim_local[d];
  int sizeu = nvars; for (d=0; d<ndims; d++) sizeu *= dim_local[d];

  /* allocate buffer arrays to write grid and solution */
  double *buffer = (double*) calloc (sizex+sizeu, sizeof(double));

  /* copy the grid to buffer */
  int offset1 = 0, offset2 = 0;
  for (d = 0; d < ndims; d++) {
    _ArrayCopy1D_((solver->x+offset1+ghosts),(buffer+offset2),dim_local[d]);
    offset1 += (solver->dim_local[d]+2*ghosts);
    offset2 +=  solver->dim_local[d];
  }

  /* copy the solution */
  int index[ndims];
  IERR ArrayCopynD(ndims,solver->u,(buffer+sizex),solver->dim_local,ghosts,0,index,solver->nvars); CHECKERR(ierr);

  if (mpi->IOParticipant) {

    /* if this rank is responsible for file I/O */
    double *write_buffer = NULL;
    int     write_size_x, write_size_u, write_total_size;
    int     is[ndims], ie[ndims], size;

    /* open the file */
    FILE *out;
    int  bytes;
    char filename[_MAX_STRING_SIZE_];
    MPIGetFilename(solver->op_filename,&mpi->IOWorld,filename);

    if (!strcmp(solver->op_overwrite,"no")) {
      if ((!count) && (!solver->restart_iter)) {
        /* open a new file, since this function is being called the first time 
           and this is not a restart run*/
        out = fopen(filename,"wb");
        if (!out) {
          fprintf(stderr,"Error in OutputSolutionParallel(): File %s could not be opened for writing.\n",filename);
          return(1);
        }
      } else {
        /* append to existing file */
        out = fopen(filename,"ab");
        if (!out) {
          fprintf(stderr,"Error in OutputSolutionParallel(): File %s could not be opened for appending.\n",filename);
          return(1);
        }
      }
    } else {
      /* write a new file / overwrite existing file */
      out = fopen(filename,"wb");
      if (!out) {
        fprintf(stderr,"Error in OutputSolutionParallel(): File %s could not be opened for writing.\n",filename);
        return(1);
      }
    }
    count++;

    /* Write own data and free buffer */
    bytes = fwrite(buffer,sizeof(double),(sizex+sizeu),out);
    if (bytes != (sizex+sizeu)) {
      fprintf(stderr,"Error in OutputSolutionParallel(): Failed to write data to file %s.\n",filename);
      return(1);
    }
    free(buffer);

    /* receive and write the data for the other processors in this IO rank's group */
    for (proc=mpi->GroupStartRank+1; proc<mpi->GroupEndRank; proc++) {
      /* get the local domain limits for process proc */
      IERR MPILocalDomainLimits(ndims,proc,mpi,solver->dim_global,is,ie);
      /* calculate the size of its local data and allocate write buffer */
      write_size_x = 0;      for (d=0; d<ndims; d++) write_size_x += (ie[d]-is[d]);
      write_size_u = nvars;  for (d=0; d<ndims; d++) write_size_u *= (ie[d]-is[d]);
      write_total_size = write_size_x + write_size_u;
      write_buffer = (double*) calloc (write_total_size, sizeof(double));
      /* receive the data */
      MPI_Request req = MPI_REQUEST_NULL;
      MPI_Irecv(write_buffer,write_total_size,MPI_DOUBLE,proc,1449,mpi->world,&req);
      MPI_Wait(&req,MPI_STATUS_IGNORE);
      /* write the data */
      bytes = fwrite(write_buffer,sizeof(double),write_total_size,out);
      if (bytes != write_total_size) {
        fprintf(stderr,"Error in OutputSolutionParallel(): Failed to write data to file %s.\n",filename);
        return(1);
      }
      free(write_buffer);
    }

    /* close the file */
    fclose(out);

  } else {

    /* all other processes, just send the data to the rank responsible for file I/O */
    MPI_Request req = MPI_REQUEST_NULL;
    MPI_Isend(buffer,(sizex+sizeu),MPI_DOUBLE,mpi->IORank,1449,mpi->world,&req);
    MPI_Wait(&req,MPI_STATUS_IGNORE);
    free(buffer);

  }

  return(0);
}
