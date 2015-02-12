#include <math.h>
#include <string.h>
#include <arrayfunctions.h>
#include <mpivars.h>
#include <hypar.h>
#include <timeintegration.h>

int TimeError(void *ts)
{
  TimeIntegration *TS     = (TimeIntegration*) ts;
  HyPar           *solver = (HyPar*)           TS->solver;
  MPIVariables    *mpi    = (MPIVariables*)    TS->mpi;
  int             size    = solver->npoints_local_wghosts * solver->nvars;
  double          sum     = 0.0, global_sum = 0.0;

  static const double tolerance = 1e-15;

  if (!strcmp(solver->time_scheme,_GLM_GEE_)) {
    /* For GLM-GEE methods, calculate the norm of the estimated global error */
    GLMGEEParameters *params = (GLMGEEParameters*) solver->msti;
    double error[3] = {0,0,0}, *Uerr;
    if (!strcmp(params->ee_mode,_GLM_GEE_YEPS_)) Uerr = TS->U[params->r];
    else {
      Uerr = TS->U[0];
      _ArraySubtract1D_(Uerr,solver->u,TS->U[params->r],size);
      _ArrayScale1D_(Uerr,(1.0/(1.0-params->gamma)),size);
    }

    /* calculate solution norm for relative errors */
    double sol_norm[3] = {0.0,0.0,0.0};
    /* L1 */
    sum = ArraySumAbsnD   (solver->nvars,solver->ndims,solver->dim_local,
                           solver->ghosts,solver->index,solver->u);
    global_sum = 0; MPISum_double(&global_sum,&sum,1,&mpi->world);
    sol_norm[0] = global_sum/((double)solver->npoints_global);
    /* L2 */
    sum = ArraySumSquarenD(solver->nvars,solver->ndims,solver->dim_local,
                           solver->ghosts,solver->index,solver->u);
    global_sum = 0; MPISum_double(&global_sum,&sum,1,&mpi->world);
    sol_norm[1] = sqrt(global_sum/((double)solver->npoints_global));
    /* Linf */
    sum = ArrayMaxnD      (solver->nvars,solver->ndims,solver->dim_local,
                           solver->ghosts,solver->index,solver->u);
    global_sum = 0; MPIMax_double(&global_sum,&sum,1,&mpi->world);
    sol_norm[2] = global_sum;

    /* calculate L1 norm of error */
    sum = ArraySumAbsnD   (solver->nvars,solver->ndims,solver->dim_local,
                           solver->ghosts,solver->index,Uerr);
    global_sum = 0; MPISum_double(&global_sum,&sum,1,&mpi->world);
    error[0] = global_sum/((double)solver->npoints_global);
    /* calculate L2 norm of error */
    sum = ArraySumSquarenD(solver->nvars,solver->ndims,solver->dim_local,
                           solver->ghosts,solver->index,Uerr);
    global_sum = 0; MPISum_double(&global_sum,&sum,1,&mpi->world);
    error[1] = sqrt(global_sum/((double)solver->npoints_global));
    /* calculate Linf norm of error */
    sum = ArrayMaxnD      (solver->nvars,solver->ndims,solver->dim_local,
                           solver->ghosts,solver->index,Uerr);
    global_sum = 0; MPIMax_double(&global_sum,&sum,1,&mpi->world);
    error[2] = global_sum;

    if (   (sol_norm[0] > tolerance)
        && (sol_norm[1] > tolerance)
        && (sol_norm[2] > tolerance) ) {
      error[0] /= sol_norm[0];
      error[1] /= sol_norm[1];
      error[2] /= sol_norm[2];
    }

    /* write to file */
    if (!mpi->rank) {
      FILE *out;
      out = fopen("glm_err.dat","w");
      fprintf(out,"%1.16E  %1.16E  %1.16E  %1.16E\n",TS->dt,error[0],error[1],error[2]);
      fclose(out);
      printf("Estimated Global Errors (GLM-GEE Time-Integration):-\n");
      printf("L1         Error           : %1.16E\n",error[0]);
      printf("L2         Error           : %1.16E\n",error[1]);
      printf("Linfinity  Error           : %1.16E\n",error[2]);
    }
  }

  return(0);
}
