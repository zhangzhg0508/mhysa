#include <stdlib.h>
#include <basic.h>
#include <arrayfunctions.h>
#include <mpivars.h>
#include <physicalmodels/fppowersystem.h>
#include <hypar.h>

int FPPowerSystemPostStep(double *u,void* s,void *m,double t)
{
  HyPar         *solver = (HyPar*)         s;
  FPPowerSystem *params = (FPPowerSystem*) solver->physics;
  int           ierr    = 0;

  int *dim    = solver->dim_local;
  int ghosts  = solver->ghosts;
  int ndims   = solver->ndims;
  int *index  = (int*) calloc (ndims,sizeof(int));

  double local_sum = 0;
  int done = 0; ierr = ArraySetValue_int(index,ndims,0); CHECKERR(ierr);
  while (!done) {
    int p = ArrayIndex1D(ndims,dim,index,NULL,ghosts);
    double dx = 1.0 / solver->GetCoordinate(0,index[0],dim,ghosts,solver->dxinv);;
    double dy = 1.0 / solver->GetCoordinate(1,index[1],dim,ghosts,solver->dxinv);;
    local_sum     += (u[p] * dx * dy);
    done = ArrayIncrementIndex(ndims,dim,index);
  }
  double local_integral = local_sum;
  double global_integral = 0; ierr = MPISum_double(&global_integral,&local_integral,1); CHECKERR(ierr);
  params->pdf_integral = global_integral;

  free(index);
  return(0);
}