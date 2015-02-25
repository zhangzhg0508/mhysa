/*
  Function to compute and print (to file) matrix representations
  of the IFunction and RHSFunction.

  Implemented only for single-processor simulations.
*/

#ifdef compute_rhs_operators

#include <basic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arrayfunctions.h>
#include <mpivars.h>
#include <petscinterface.h>
#include <hypar.h>

#undef __FUNCT__
#define __FUNCT__ "PetscComputeRHSOperators"

#define absolute(a) ((a)<0?-(a):(a))

int PetscComputeRHSOperators(TS ts,double t,void *ctxt)
{
  PETScContext  *context = (PETScContext*) ctxt;
  HyPar         *solver  = (HyPar*)        context->solver;
  MPIVariables  *mpi     = (MPIVariables*) context->mpi;
  int           i, j, size, ndof, ierr;
  double        *f, *f0, *u, *u0, *rhs, *drhs;
  FILE          *fout;
  char          filename[_MAX_STRING_SIZE_];
  Vec           U, Udot, F;

  if (mpi->nproc > 1) return(0);

  int ndims  = solver->ndims;
  int nvars  = solver->nvars;
  int ghosts = solver->ghosts;
  int *dim   = solver->dim_local;
  int index[ndims];

  double epsilon = 1e-6;
  double tolerance = 1e-15;

  /* allocate arrays */
  size = solver->npoints_local_wghosts * nvars;
  u    = (double*) calloc (size,sizeof(double));
  u0   = (double*) calloc (size,sizeof(double));
  rhs  = (double*) calloc (size,sizeof(double));
  drhs = (double*) calloc (size,sizeof(double));
  ndof = solver->npoints_local * nvars;
  f    = (double*) calloc (ndof,sizeof(double));
  f0   = (double*) calloc (ndof,sizeof(double));

  /* create PETSc vectors */
  ierr = VecCreate(mpi->world,&U);          CHKERRQ(ierr);
  ierr = VecSetSizes(U,ndof,PETSC_DECIDE);  CHKERRQ(ierr);
  ierr = VecSetUp(U);                       CHKERRQ(ierr);
  ierr = VecDuplicate(U,&Udot);             CHKERRQ(ierr);
  ierr = VecDuplicate(U,&F);                CHKERRQ(ierr);
  ierr = VecSet(U   ,0.0);                  CHKERRQ(ierr);
  ierr = VecSet(Udot,0.0);                  CHKERRQ(ierr);
  ierr = VecSet(F   ,0.0);                  CHKERRQ(ierr);

  /* copy the current solution to u0 */
  _ArrayCopy1D_(solver->u,u0,size); 
  /* apply boundary conditions to the solution u0 */
  ierr = solver->ApplyBoundaryConditions(solver,mpi,u0,NULL,0,t);CHECKERR(ierr);
  ierr = MPIExchangeBoundariesnD(ndims,nvars,dim,ghosts,mpi,u0); CHECKERR(ierr);

  /* compute linearized matrix for the RHSFunctionExpl */
  strcpy(filename,"Mat_RHSFunctionExpl_");
  strcat(filename,solver->filename_index);
  strcat(filename,".dat");
  printf("PetscComputeRHSOperators(): Computing linearized matrix operator for RHSFunctionExpl. ndof=%d.\n",ndof);
  printf("PetscComputeRHSOperators(): Writing to sparse matrix file %s.\n",filename);
  fout = fopen(filename,"w");
  fprintf(fout,"%d\n",ndof);
  /* compute the RHSFunction of u0 */
  ierr = TransferVecToPETSc(u0,U,context);        CHECKERR(ierr);
  ierr = PetscRHSFunctionExpl(ts,t,U,F,context);  CHKERRQ(ierr);
  ierr = TransferVecFromPETSc(rhs,F,context);     CHECKERR(ierr);
  /* transfer to an array f0 which as no ghost points */
  ierr = ArrayCopynD(ndims,rhs,f0,dim,ghosts,0,index,nvars); CHECKERR(ierr);
  for (i=0; i<ndof; i++) {
    /* copy the solution u0 to u */
    _ArrayCopy1D_(u0,u,size);
    /* find the 1D index p in an array with ghosts points (u),
     * corresponding to index i which assumes no ghost points */
    int ii, p, v;
    ii = i / nvars;
    v  = i - ii*nvars;
    _ArrayIndexnD_(ndims,ii,dim,index,0); 
    _ArrayIndex1D_(ndims,dim,index,ghosts,p);
    /* add a perturbation */
    u[nvars*p+v] += epsilon;
    /* apply boundary conditions to the perturbed solution u */
    ierr = solver->ApplyBoundaryConditions(solver,mpi,u,NULL,0,t);CHECKERR(ierr);
    ierr = MPIExchangeBoundariesnD(ndims,nvars,dim,ghosts,mpi,u); CHECKERR(ierr);
    /* compute the RHSFunctionExpl of u */
    ierr = TransferVecToPETSc(u,U,context);         CHECKERR(ierr);
    ierr = PetscRHSFunctionExpl(ts,t,U,F,context);  CHKERRQ(ierr);
    ierr = TransferVecFromPETSc(rhs,F,context);     CHECKERR(ierr);
    /* transfer to an array f which as no ghost points */
    _ArraySetValue_(f,ndof,0.0);
    ierr = ArrayCopynD(ndims,rhs,f,dim,ghosts,0,index,nvars); CHECKERR(ierr);
    /* subtract f0 from f */
    _ArrayAXPY_(f0,-1.0,f,ndof);
    /* f is now espilon*column of the matrix */
    for (j=0; j<ndof; j++) {
      double mat_elem = f[j] / epsilon;
      /* write to file if element is non-zero */
      if (absolute(mat_elem) > tolerance) fprintf(fout,"%5d %5d %+1.16e\n",j+1,i+1,mat_elem);
    }
  }
  fclose(fout);
  

  /* compute linearized matrix for the RHSFunctionIMEX */
  strcpy(filename,"Mat_RHSFunctionIMEX_");
  strcat(filename,solver->filename_index);
  strcat(filename,".dat");
  printf("PetscComputeRHSOperators(): Computing linearized matrix operator for RHSFunctionIMEX. ndof=%d.\n",ndof);
  printf("PetscComputeRHSOperators(): Writing to sparse matrix file %s.\n",filename);
  fout = fopen(filename,"w");
  fprintf(fout,"%d\n",ndof);
  /* compute the RHSFunction of u0 */
  ierr = TransferVecToPETSc(u0,U,context);        CHECKERR(ierr);
  ierr = PetscRHSFunctionIMEX(ts,t,U,F,context);  CHKERRQ(ierr);
  ierr = TransferVecFromPETSc(rhs,F,context);     CHECKERR(ierr);
  /* transfer to an array f0 which as no ghost points */
  ierr = ArrayCopynD(ndims,rhs,f0,dim,ghosts,0,index,nvars); CHECKERR(ierr);
  for (i=0; i<ndof; i++) {
    /* copy the solution u0 to u */
    _ArrayCopy1D_(u0,u,size);
    /* find the 1D index p in an array with ghosts points (u),
     * corresponding to index i which assumes no ghost points */
    int ii, p, v;
    ii = i / nvars;
    v  = i - ii*nvars;
    _ArrayIndexnD_(ndims,ii,dim,index,0); 
    _ArrayIndex1D_(ndims,dim,index,ghosts,p);
    /* add a perturbation */
    u[nvars*p+v] += epsilon;
    /* apply boundary conditions to the perturbed solution u */
    ierr = solver->ApplyBoundaryConditions(solver,mpi,u,NULL,0,t);CHECKERR(ierr);
    ierr = MPIExchangeBoundariesnD(ndims,nvars,dim,ghosts,mpi,u); CHECKERR(ierr);
    /* compute the RHSFunctionIMEX of u */
    ierr = TransferVecToPETSc(u,U,context);           CHECKERR(ierr);
    ierr = PetscRHSFunctionIMEX(ts,t,U,F,context);    CHKERRQ(ierr);
    ierr = TransferVecFromPETSc(rhs,F,context);       CHECKERR(ierr);
    /* transfer to an array f which as no ghost points */
    ierr = ArrayCopynD(ndims,rhs,f,dim,ghosts,0,index,nvars); CHECKERR(ierr);
    /* subtract f0 from f */
    _ArrayAXPY_(f0,-1.0,f,ndof);
    /* f is now espilon*column of the matrix */
    for (j=0; j<ndof; j++) {
      double mat_elem = f[j] / epsilon;
      /* write to file if element is non-zero */
      if (absolute(mat_elem) > tolerance) fprintf(fout,"%5d %5d %+1.16e\n",j+1,i+1,mat_elem);
    }
  }
  fclose(fout);

  /* compute linearized matrix for the IFunctionIMEX */
  strcpy(filename,"Mat_IFunctionIMEX_");
  strcat(filename,solver->filename_index);
  strcat(filename,".dat");
  printf("PetscComputeRHSOperators(): Computing linearized matrix operator for IFunctionIMEX. ndof=%d.\n",ndof);
  printf("PetscComputeRHSOperators(): Writing to sparse matrix file %s.\n",filename);
  fout = fopen(filename,"w");
  fprintf(fout,"%d\n",ndof);
  /* compute the IFunction of u0 */
  ierr = TransferVecToPETSc(u0,U,context);          CHECKERR(ierr);
  ierr = PetscIFunctionIMEX(ts,t,U,Udot,F,context); CHKERRQ(ierr);
  ierr = TransferVecFromPETSc(rhs,F,context);       CHECKERR(ierr);
  /* transfer to an array f0 which as no ghost points */
  ierr = ArrayCopynD(ndims,rhs,f0,dim,ghosts,0,index,nvars); CHECKERR(ierr);
  _ArrayScale1D_(f0,-1.0,ndof);
  for (i=0; i<ndof; i++) {
    /* copy the solution u0 to u */
    _ArrayCopy1D_(u0,u,size);
    /* find the 1D index p in an array with ghosts points (u),
     * corresponding to index i which assumes no ghost points */
    int ii, p, v;
    ii = i / nvars;
    v  = i - ii*nvars;
    _ArrayIndexnD_(ndims,ii,dim,index,0); 
    _ArrayIndex1D_(ndims,dim,index,ghosts,p);
    /* add a perturbation */
    u[nvars*p+v] += epsilon;
    /* apply boundary conditions to the perturbed solution u */
    ierr = solver->ApplyBoundaryConditions(solver,mpi,u,NULL,0,t);CHECKERR(ierr);
    ierr = MPIExchangeBoundariesnD(ndims,nvars,dim,ghosts,mpi,u); CHECKERR(ierr);
    /* compute the IFunctionIMEX of u */
    ierr = TransferVecToPETSc(u,U,context);             CHECKERR(ierr);
    ierr = PetscIFunctionIMEX(ts,t,U,Udot,F,context);   CHKERRQ(ierr);
    ierr = TransferVecFromPETSc(rhs,F,context);         CHECKERR(ierr);
    /* transfer to an array f which as no ghost points */
    _ArraySetValue_(f,ndof,0.0);
    ierr = ArrayCopynD(ndims,rhs,f,dim,ghosts,0,index,nvars); CHECKERR(ierr);
    _ArrayScale1D_(f,-1.0,ndof);
    /* subtract f0 from f */
    _ArrayAXPY_(f0,-1.0,f,ndof);
    /* f is now espilon*column of the matrix */
    for (j=0; j<ndof; j++) {
      double mat_elem = f[j] / epsilon;
      /* write to file if element is non-zero */
      if (absolute(mat_elem) > tolerance) fprintf(fout,"%5d %5d %+1.16e\n",j+1,i+1,mat_elem);
    }
  }
  fclose(fout);

  /* clean up */
  ierr = VecDestroy(&U);    CHKERRQ(ierr);
  ierr = VecDestroy(&Udot); CHKERRQ(ierr);
  ierr = VecDestroy(&F);    CHKERRQ(ierr);
  free(u);
  free(u0);
  free(rhs);
  free(drhs);
  free(f);
  free(f0);
  return(0);
}

#endif