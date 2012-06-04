/** @file primordial.c Documented primordial module.
 *
 * Julien Lesgourgues, 24.08.2010    
 *
 * This module computes the primordial spectra. Can be used in different modes:
 * simple parametric form, evolving inflaton perturbations, etc. So far only
 * the mode corresponding to a simple analytic form in terms of amplitudes, tilts 
 * and runnings has been developped.
 *
 * The following functions can be called from other modules:
 *
 * -# primordial_init() at the beginning (anytime after perturb_init() and before spectra_init())
 * -# primordial_spectrum_at_k() at any time for computing P(k) at any k
 * -# primordial_free() at the end
 */

#include "primordial.h"

/** 
 * Primordial spectra for arbitrary argument and for all initial conditions. 
 *
 * This routine evaluates the primordial spectrum at a given value of k by
 * interpolating in the pre-computed table. 
 * 
 * When k is not in the pre-computed range but the spectrum can be found
 * analytically, finds it. Otherwise returns an error.
 * 
 * Can be called in two modes: linear or logarithmic.
 * 
 * - linear: takes k, returns P(k)
 * 
 * - logarithmic: takes ln(k), return ln(P(k))
 *
 * One little subtlety: in case of several correlated initial conditions,
 * the cross-correlation spectrum can be negative. Then, in logarithmic mode, 
 * the non-diagonal elements contain the cross-correlation angle P_12/sqrt(P_11 P_22) 
 * (from -1 to 1) instead of ln(P_12)
 *
 * This function can be
 * called from whatever module at whatever time, provided that
 * primordial_init() has been called before, and primordial_free() has not
 * been called yet.
 *
 * @param ppm        Input: pointer to primordial structure containing tabulated primordial spectrum 
 * @param index_mode Input: index of mode (scalar, tensor, ...) 
 * @param mode       Input: linear or logarithmic
 * @param k          Input: wavenumber in 1/Mpc (linear mode) or its logarithm (logarithmic mode)
 * @param pk         Ouput: for each pair of initial conditions, primordial spectra P(k) in Mpc**3 (linear mode), or their logarithms and cross-correlation angles (logarithmic mode)
 * @return the error status
 */

int primordial_spectrum_at_k(
			     struct primordial * ppm,
			     int index_mode,
			     enum linear_or_logarithmic mode, 
			     double input,  
			     double * output /* array with argument output[index_ic1_ic2] (must be already allocated) */
			     ) {

  /** Summary: */

  /** - define local variables */

  int index_ic1,index_ic2,index_ic1_ic2;
  double lnk;
  int last_index;

  /** - infer ln(k) from input. In linear mode, reject negative value of input k value. */

  if (mode == linear) {
    class_test(input<=0.,
	       ppm->error_message,
	       "k = %e",input);
    lnk=log(input);
  }
  else {
    lnk = input;
  }

  /** - if ln(k) is not in the interpolation range, return an error, unless 
      we are in the case of a analytic spectrum, for which a direct computation is possible */
  
  if ((lnk > ppm->lnk[ppm->lnk_size-1]) || (lnk < ppm->lnk[0])) {

    class_test(ppm->primordial_spec_type != analytic_Pk,
	       ppm->error_message,
	       "k=%e out of range [%e : %e]",exp(lnk),exp(ppm->lnk[0]),exp(ppm->lnk[ppm->lnk_size-1]));

    /* direct computation */

    for (index_ic1 = 0; index_ic1 < ppm->ic_size[index_mode]; index_ic1++) {
      for (index_ic2 = index_ic1; index_ic2 < ppm->ic_size[index_mode]; index_ic2++) {
	
	index_ic1_ic2 = index_symmetric_matrix(index_ic1,index_ic2,ppm->ic_size[index_mode]);

	if (ppm->is_non_zero[index_mode][index_ic1_ic2] == _TRUE_) {

	  class_call(primordial_analytic_spectrum(ppm,
						  index_mode,
						  index_ic1_ic2,
						  exp(lnk),
						  &(output[index_ic1_ic2])),
		     ppm->error_message,
		     ppm->error_message);
	}
	else {
	  output[index_ic1_ic2] = 0.;
	}
      }
    }
    
    /* if mode==linear, output is already in the correct format. Otherwise, apply necessary transformation. */

    if (mode == logarithmic) {

      for (index_ic1 = 0; index_ic1 < ppm->ic_size[index_mode]; index_ic1++) {
	index_ic1_ic2 = index_symmetric_matrix(index_ic1,index_ic1,ppm->ic_size[index_mode]);
	output[index_ic1_ic2] = log(output[index_ic1_ic2]);
      }
      for (index_ic1 = 0; index_ic1 < ppm->ic_size[index_mode]; index_ic1++) {
	for (index_ic2 = index_ic1+1; index_ic2 < ppm->ic_size[index_mode]; index_ic2++) {
	  index_ic1_ic2 = index_symmetric_matrix(index_ic1,index_ic2,ppm->ic_size[index_mode]);
	  if (ppm->is_non_zero[index_mode][index_ic1_ic2] == _TRUE_) {
	    output[index_ic1_ic2] /= sqrt(output[index_symmetric_matrix(index_ic1,index_ic1,ppm->ic_size[index_mode])]*
					  output[index_symmetric_matrix(index_ic2,index_ic2,ppm->ic_size[index_mode])]);
	  }
	}
      }
    }
  } 

  /** - otherwise, interpolate in the pre-computed table: */

  else {

    class_call(array_interpolate_spline(
					ppm->lnk,
					ppm->lnk_size,
					ppm->lnpk[index_mode],
					ppm->ddlnpk[index_mode],
					ppm->ic_ic_size[index_mode],
					lnk,
					&last_index,
					output,
					ppm->ic_ic_size[index_mode],
					ppm->error_message),
	       ppm->error_message,
	       ppm->error_message);
  
    /* if mode==logarithmic, output is already in the correct format. Otherwise, apply necessary transformation. */

    if (mode == linear) {

      for (index_ic1 = 0; index_ic1 < ppm->ic_size[index_mode]; index_ic1++) {
	index_ic1_ic2 = index_symmetric_matrix(index_ic1,index_ic1,ppm->ic_size[index_mode]);
	output[index_ic1_ic2]=exp(output[index_ic1_ic2]);
      }
      for (index_ic1 = 0; index_ic1 < ppm->ic_size[index_mode]; index_ic1++) {
	for (index_ic2 = index_ic1+1; index_ic2 < ppm->ic_size[index_mode]; index_ic2++) {
	  index_ic1_ic2 = index_symmetric_matrix(index_ic1,index_ic2,ppm->ic_size[index_mode]);
	  if (ppm->is_non_zero[index_mode][index_ic1_ic2] == _TRUE_) {
	    output[index_ic1_ic2] *= sqrt(output[index_symmetric_matrix(index_ic1,index_ic1,ppm->ic_size[index_mode])]*
					  output[index_symmetric_matrix(index_ic2,index_ic2,ppm->ic_size[index_mode])]);
	  }
	  else {
	    output[index_ic1_ic2] = 0.;
	  }
	}
      }
    }
  }
  
  return _SUCCESS_;
  
}

/**
 * This routine initializes the primordial structure (in particular, compute table of primordial spectrum values)
 * 
 * @param ppr Input : pointer to precision structure (defines method and precision for all computations)
 * @param ppt Input : pointer to perturbation structure (useful for knowing k_min, k_max, etc.)
 * @param ppm Output: pointer to initialized primordial structure 
 * @return the error status
 */

int primordial_init(
		    struct precision  * ppr,
		    struct perturbs   * ppt,
		    struct primordial * ppm
		    ) {

  /** Summary: */

  /** - define local variables */

  double k,k_min,k_max;
  int index_mode,index_ic1,index_ic2,index_ic1_ic2,index_k;
  double pk,pk1,pk2;
  double dlnk,lnpk_pivot,lnpk_minus,lnpk_plus;

  /** - check that we really need to compute the primordial spectra */

  if (ppt->has_perturbations == _FALSE_) {
    ppm->lnk_size=0;
    if (ppm->primordial_verbose > 0)
      printf("No perturbations requested. Primordial module skipped.\n");
    return _SUCCESS_;
  }
  else {
    if (ppm->primordial_verbose > 0)
      printf("Computing primordial spectra");
  }

  /** - get kmin and kmax from perturbation structure. Test that they make sense. */

  k_min=_HUGE_; /* huge initial value before scanning all modes */
  k_max=0.;     /* zero initial value before scanning all modes */

  for (index_mode = 0; index_mode < ppt->md_size; index_mode++) {
    k_min = min(k_min,ppt->k[index_mode][0]); /* first value, inferred from perturbations structure */
    k_max = max(k_max,ppt->k[index_mode][ppt->k_size[index_mode]-1]); /* last value, inferred from perturbations structure */
  }
  
  class_test(k_min <= 0.,
	     ppm->error_message,
	     "k_min negative or null: stop to avoid segmentation fault");

  class_test(k_max <= 0.,
	     ppm->error_message,
	     "k_max negative or null: stop to avoid segmentation fault");

  class_test(ppm->k_pivot <= 0.,
	     ppm->error_message,
	     "k_pivot negative or null: stop to avoid segmentation fault");

  class_test(ppr->k_per_decade_primordial <= 0.,
	     ppm->error_message,
	     "k_per_decade_primordial negative or null: stop to avoid segmentation fault");

  class_test(ppr->k_per_decade_primordial <= _K_PER_DECADE_PRIMORDIAL_MIN_,
	     ppm->error_message,
	     "k_per_decade_primordial = %e: you ask for such a sparse sampling of the primordial spectrum that this is probably a mistake",
	     ppr->k_per_decade_primordial);

  /** - allocate and fill values of lnk's */

  class_call(primordial_get_lnk_list(ppm,
				     k_min,
				     k_max,
				     ppr->k_per_decade_primordial
				     ),
	     ppm->error_message,
	     ppm->error_message);

  /** - define indices and allocate tables in primordial structure */

  class_call(primordial_indices(ppt,
				ppm),
	     ppm->error_message,
	     ppm->error_message);
		
  /** - deal with case of analytic primordial spectra (with amplitudes, tilts, runnings etc.) */

  if (ppm->primordial_spec_type == analytic_Pk) {

    if (ppm->primordial_verbose > 0)
      printf(" (analytic spectrum)\n");

    class_call(primordial_analytic_spectrum_init(ppt,
						 ppm),
	       ppm->error_message,
	       ppm->error_message);
    
    for (index_k = 0; index_k < ppm->lnk_size; index_k++) {

      k=exp(ppm->lnk[index_k]);

      for (index_mode = 0; index_mode < ppt->md_size; index_mode++) {
	for (index_ic1 = 0; index_ic1 < ppm->ic_size[index_mode]; index_ic1++) {
	  for (index_ic2 = index_ic1; index_ic2 < ppm->ic_size[index_mode]; index_ic2++) {

	    index_ic1_ic2 = index_symmetric_matrix(index_ic1,index_ic2,ppm->ic_size[index_mode]);

	    if (ppm->is_non_zero[index_mode][index_ic1_ic2] == _TRUE_) {

	      class_call(primordial_analytic_spectrum(ppm,
						      index_mode,
						      index_ic1_ic2,
						      k,
						      &pk),
			 ppm->error_message,
			 ppm->error_message);
	      
	      if (index_ic1 == index_ic2) {

		/* diagonal coefficients: ln[P(k)] */

		ppm->lnpk[index_mode][index_k*ppm->ic_ic_size[index_mode]+index_ic1_ic2] = log(pk);
	      }
	      else {

		/* non-diagonal coefficients: cosDelta(k) = P(k)_12/sqrt[P(k)_1 P(k)_2] */

		class_call(primordial_analytic_spectrum(ppm,
							index_mode,
							index_symmetric_matrix(index_ic1,index_ic1,ppm->ic_size[index_mode]),
							k,
							&pk1),
			   ppm->error_message,
			   ppm->error_message);			     

		class_call(primordial_analytic_spectrum(ppm,
							index_mode,
							index_symmetric_matrix(index_ic2,index_ic2,ppm->ic_size[index_mode]),
							k,
							&pk2),
			   ppm->error_message,
			   ppm->error_message);	

		ppm->lnpk[index_mode][index_k*ppm->ic_ic_size[index_mode]+index_ic1_ic2] = pk/sqrt(pk1*pk2);
	      }
	    }
	    else {
	      
	      /* non-diagonal coefficients when ic's are uncorrelated */

	      ppm->lnpk[index_mode][index_k*ppm->ic_ic_size[index_mode]+index_ic1_ic2] = 0.;
	    }
	  }
	}
      }
    }
  }

  /** - deal with case of inflation with given V(phi) */

  else if (ppm->primordial_spec_type == inflation_V) {

    class_test(ppt->has_scalars == _FALSE_,
	       ppm->error_message,
	       "inflationary module cannot work if you do not ask for scalar modes");

    class_test(ppt->has_vectors == _TRUE_,
	       ppm->error_message,
	       "inflationary module cannot work if you ask for vector modes");

    class_test(ppt->has_tensors == _FALSE_,
	       ppm->error_message,
	       "inflationary module cannot work if you do not ask for tensor modes");

    class_test(ppt->has_bi == _TRUE_ || ppt->has_cdi == _TRUE_ || ppt->has_nid == _TRUE_ || ppt->has_niv == _TRUE_,
	       ppm->error_message,
	       "inflationary module cannot work if you ask for isocurvature modes");

    class_call(primordial_inflation_indices(ppm),
	       ppm->error_message,
	       ppm->error_message);

    if (ppm->primordial_verbose > 0)
      printf(" (simulating inflation)\n");

    class_call(primordial_inflation_solve_inflation(ppt,ppm,ppr),
	       ppm->error_message,
	       ppm->error_message);

  }

  else {

    class_test(0==0,
	       ppm->error_message,
	       "only analytic and inflation_V primordial spectrum coded yet");

  }     

  /** - compute second derivative of each lnpk versus lnk  with spline, in view of interpolation */

  for (index_mode = 0; index_mode < ppm->md_size; index_mode++) {

    class_call(array_spline_table_lines(ppm->lnk,
					ppm->lnk_size,
					ppm->lnpk[index_mode],
					ppm->ic_ic_size[index_mode],
					ppm->ddlnpk[index_mode],
					_SPLINE_EST_DERIV_,
					ppm->error_message),
	       ppm->error_message,
	       ppm->error_message);
    
  }
  
  /** derive spectral parameters from numerically computed spectra
      (not used by the rest of the code, but useful to keep in memory for several types of investigations) */
  
  if (ppm->primordial_spec_type != analytic_Pk) {

    dlnk = log(10.)/ppr->k_per_decade_primordial;

    if (ppt->has_scalars == _TRUE_) {

      class_call(primordial_spectrum_at_k(ppm,
					  ppt->index_md_scalars,
					  logarithmic,
					  log(ppm->k_pivot),
					  &lnpk_pivot),
		 ppm->error_message,
		 ppm->error_message);

      class_call(primordial_spectrum_at_k(ppm,
					  ppt->index_md_scalars,
					  logarithmic,
					  log(ppm->k_pivot)+dlnk,

					  &lnpk_plus),
		 ppm->error_message,
		 ppm->error_message);

      class_call(primordial_spectrum_at_k(ppm,
					  ppt->index_md_scalars,
					  logarithmic,
					  log(ppm->k_pivot)-dlnk,
					  &lnpk_minus),
		 ppm->error_message,
		 ppm->error_message);

      ppm->A_s = exp(lnpk_pivot);
      ppm->n_s = (lnpk_plus-lnpk_minus)/(2.*dlnk)+1.;
      ppm->alpha_s = (lnpk_plus-2.*lnpk_pivot+lnpk_minus)/pow(dlnk,2);

      if (ppm->primordial_verbose > 0)
	printf(" -> A_s=%g  n_s=%g  alpha_s=%g\n",ppm->A_s,ppm->n_s,ppm->alpha_s);

    }

    if (ppt->has_tensors == _TRUE_) {

      class_call(primordial_spectrum_at_k(ppm,
					  ppt->index_md_tensors,
					  logarithmic,
					  log(ppm->k_pivot),
					  &lnpk_pivot),
		 ppm->error_message,
		 ppm->error_message);

      class_call(primordial_spectrum_at_k(ppm,
					  ppt->index_md_tensors,
					  logarithmic,
					  log(ppm->k_pivot)+dlnk,
					  &lnpk_plus),
		 ppm->error_message,
		 ppm->error_message);

      class_call(primordial_spectrum_at_k(ppm,
					  ppt->index_md_tensors,
					  logarithmic,
					  log(ppm->k_pivot)-dlnk,
					  &lnpk_minus),
		 ppm->error_message,
		 ppm->error_message);

      ppm->r = exp(lnpk_pivot)/ppm->A_s;
      ppm->n_t = (lnpk_plus-lnpk_minus)/(2.*dlnk);
      ppm->alpha_t = (lnpk_plus-2.*lnpk_pivot+lnpk_minus)/pow(dlnk,2);

      if (ppm->primordial_verbose > 0)
	printf(" -> r=%g  n_r=%g  alpha_r=%g\n",ppm->r,ppm->n_t,ppm->alpha_t);

    }

  }

  return _SUCCESS_;
  
}

/**
 * This routine frees all the memory space allocated by primordial_init().
 *
 * To be called at the end of each run.
 *
 * @param ppm Input: pointer to primordial structure (which fields must be freed)
 * @return the error status
 */

int primordial_free(
		    struct primordial * ppm
		    ) {

  int index_mode;

  if (ppm->lnk_size > 0) {

    if (ppm->primordial_spec_type == analytic_Pk) {
      for (index_mode = 0; index_mode < ppm->md_size; index_mode++) {
	free(ppm->amplitude[index_mode]);
	free(ppm->tilt[index_mode]);
	free(ppm->running[index_mode]);
      }
      free(ppm->amplitude);
      free(ppm->tilt);
      free(ppm->running);
    }

    for (index_mode = 0; index_mode < ppm->md_size; index_mode++) {
      free(ppm->lnpk[index_mode]);
      free(ppm->ddlnpk[index_mode]);
      free(ppm->is_non_zero[index_mode]);
    }

    free(ppm->lnpk);
    free(ppm->ddlnpk);
    free(ppm->is_non_zero);
    free(ppm->ic_size);
    free(ppm->ic_ic_size);

    free(ppm->lnk);
    
  }

  return _SUCCESS_; 
}

/**
 * This routine defines indices and allocates tables in the primordial structure 
 *
 * @param ppt  Input : pointer to perturbation structure
 * @param ppm  Input/output: pointer to primordial structure 
 * @return the error status
 */

int primordial_indices(
		       struct perturbs   * ppt,
		       struct primordial * ppm
		       ) {

  int index_mode;

  ppm->md_size = ppt->md_size;

  class_alloc(ppm->lnpk,ppt->md_size*sizeof(double*),ppm->error_message);

  class_alloc(ppm->ddlnpk,ppt->md_size*sizeof(double*),ppm->error_message);

  class_alloc(ppm->ic_size,ppt->md_size*sizeof(int*),ppm->error_message);

  class_alloc(ppm->ic_ic_size,ppt->md_size*sizeof(int*),ppm->error_message);

  class_alloc(ppm->is_non_zero,ppm->md_size*sizeof(short *),ppm->error_message);

  for (index_mode = 0; index_mode < ppt->md_size; index_mode++) {		     

    ppm->ic_size[index_mode] = ppt->ic_size[index_mode];

    ppm->ic_ic_size[index_mode] = (ppm->ic_size[index_mode]*(ppm->ic_size[index_mode]+1))/2;

    class_alloc(ppm->lnpk[index_mode],
		ppm->lnk_size*ppm->ic_ic_size[index_mode]*sizeof(double),
		ppm->error_message);

    class_alloc(ppm->ddlnpk[index_mode],
		ppm->lnk_size*ppm->ic_ic_size[index_mode]*sizeof(double),
		ppm->error_message);

    class_alloc(ppm->is_non_zero[index_mode],
		ppm->ic_ic_size[index_mode]*sizeof(short),
		ppm->error_message);
    

  }

  return _SUCCESS_;

}

/**
 * This routine allocates and fills the list of wavenumbers k
 *
 *
 * @param ppm  Input/output: pointer to primordial structure 
 * @param kmin Input : first value
 * @param kmax Input : last value that we should encompass
 * @param k_per_decade Input : number of k per decade
 * @return the error status
 */

int primordial_get_lnk_list(
			    struct primordial * ppm,
			    double kmin,
			    double kmax,
			    double k_per_decade
			    ) {

  int i;

  class_test((kmin <= 0.) || (kmax <= kmin),
	     ppm->error_message,
	     "inconsistent values of kmin=%e, kmax=%e",kmin,kmax);
 
  ppm->lnk_size = (int)(log(kmax/kmin)/log(10.)*k_per_decade) + 2;

  class_alloc(ppm->lnk,ppm->lnk_size*sizeof(double),ppm->error_message);

  for (i=0; i<ppm->lnk_size; i++)
    ppm->lnk[i]=log(kmin)+i*log(10.)/k_per_decade;
        
  return _SUCCESS_;
  
}

/**
 * This routine interprets and stores in a condensed form the input parameters 
 * in the case of a simple analytic spectra with amplitudes, tilts, runnings, 
 * in such way that later on, the spectrum can be obtained by a quick call to
 * the routine primordial_analytic_spectrum(()
 *
 * @param ppt  Input : pointer to perturbation structure
 * @param ppm  Input/output: pointer to primordial structure 
 * @return the error status
 */

int primordial_analytic_spectrum_init(
				      struct perturbs   * ppt,
				      struct primordial * ppm
				      ) {

  int index_mode,index_ic1,index_ic2;
  int index_ic1_ic2,index_ic1_ic1,index_ic2_ic2;
  double one_amplitude=0.;
  double one_tilt=0.;
  double one_running=0.;
  double one_correlation=0.;

  class_alloc(ppm->amplitude,
	      ppm->md_size*sizeof(double *),
	      ppm->error_message);

  class_alloc(ppm->tilt,
	      ppm->md_size*sizeof(double *),
	      ppm->error_message);

  class_alloc(ppm->running,
	      ppm->md_size*sizeof(double *),
	      ppm->error_message);

  for (index_mode = 0; index_mode < ppm->md_size; index_mode++) {

    class_alloc(ppm->amplitude[index_mode],
		ppm->ic_ic_size[index_mode]*sizeof(double),
		ppm->error_message);

    class_alloc(ppm->tilt[index_mode],
		ppm->ic_ic_size[index_mode]*sizeof(double),
		ppm->error_message);

    class_alloc(ppm->running[index_mode],
		ppm->ic_ic_size[index_mode]*sizeof(double),
		ppm->error_message);

  }
      
  for (index_mode = 0; index_mode < ppm->md_size; index_mode++) {

    /* digonal coefficients */
    
    for (index_ic1 = 0; index_ic1 < ppm->ic_size[index_mode]; index_ic1++) {

      if ((ppt->has_scalars == _TRUE_) && (index_mode == ppt->index_md_scalars)) {

	if ((ppt->has_ad == _TRUE_) && (index_ic1 == ppt->index_ic_ad)) {
	  one_amplitude = ppm->A_s;
	  one_tilt = ppm->n_s;
	  one_running = ppm->alpha_s;
	}

	if ((ppt->has_bi == _TRUE_) && (index_ic1 == ppt->index_ic_bi)) {
	  one_amplitude = ppm->A_s*ppm->f_bi*ppm->f_bi;
	  one_tilt = ppm->n_bi;
	  one_running = ppm->alpha_bi;
	}

	if ((ppt->has_cdi == _TRUE_) && (index_ic1 == ppt->index_ic_cdi)) {
	  one_amplitude = ppm->A_s*ppm->f_cdi*ppm->f_cdi;
	  one_tilt = ppm->n_cdi;
	  one_running = ppm->alpha_cdi;
	}

	if ((ppt->has_nid == _TRUE_) && (index_ic1 == ppt->index_ic_nid)) {
	  one_amplitude = ppm->A_s*ppm->f_nid*ppm->f_nid;
	  one_tilt = ppm->n_nid;
	  one_running = ppm->alpha_nid;
	}
    
	if ((ppt->has_niv == _TRUE_) && (index_ic1 == ppt->index_ic_niv)) {
	  one_amplitude = ppm->A_s*ppm->f_niv*ppm->f_niv;
	  one_tilt = ppm->n_niv;
	  one_running = ppm->alpha_niv;
	}
      }

      if ((ppt->has_tensors == _TRUE_) && (index_mode == ppt->index_md_tensors)) {

	if (index_ic1 == ppt->index_ic_ten) {
	  one_amplitude = ppm->A_s*ppm->r;
	  one_tilt = ppm->n_t+1.; /* +1 to match usual definition of n_t (equivalent to n_s-1) */
	  one_running = ppm->alpha_t;
	}
      }

      class_test(one_amplitude <= 0.,
		 ppm->error_message,
		 "inconsistent input for primordial amplitude: %g for index_mode=%d, index_ic=%d\n",
		 one_amplitude,index_mode,index_ic1);

      index_ic1_ic2 = index_symmetric_matrix(index_ic1,index_ic1,ppm->ic_size[index_mode]);
      
      ppm->is_non_zero[index_mode][index_ic1_ic2] = _TRUE_;
      ppm->amplitude[index_mode][index_ic1_ic2] = one_amplitude;
      ppm->tilt[index_mode][index_ic1_ic2] = one_tilt;
      ppm->running[index_mode][index_ic1_ic2] = one_running;
    }

    /* non-diagonal coefficients */

    for (index_ic1 = 0; index_ic1 < ppm->ic_size[index_mode]; index_ic1++) {
      for (index_ic2 = index_ic1+1; index_ic2 < ppm->ic_size[index_mode]; index_ic2++) {
     
	if ((ppt->has_scalars == _TRUE_) && (index_mode == ppt->index_md_scalars)) {
 
	  if ((ppt->has_ad == _TRUE_) && (ppt->has_bi == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_ad) && (index_ic2 == ppt->index_ic_bi)) ||
	       ((index_ic1 == ppt->index_ic_ad) && (index_ic1 == ppt->index_ic_bi)))) {
	    one_correlation = ppm->c_ad_bi;
	    one_tilt = ppm->n_ad_bi;
	    one_running = ppm->alpha_ad_bi;
	  }

	  if ((ppt->has_ad == _TRUE_) && (ppt->has_cdi == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_ad) && (index_ic2 == ppt->index_ic_cdi)) ||
	       ((index_ic2 == ppt->index_ic_ad) && (index_ic1 == ppt->index_ic_cdi)))) {
	    one_correlation = ppm->c_ad_cdi;
	    one_tilt = ppm->n_ad_cdi;
	    one_running = ppm->alpha_ad_cdi;
	  }

	  if ((ppt->has_ad == _TRUE_) && (ppt->has_nid == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_ad) && (index_ic2 == ppt->index_ic_nid)) ||
	       ((index_ic2 == ppt->index_ic_ad) && (index_ic1 == ppt->index_ic_nid)))) {
	    one_correlation = ppm->c_ad_nid;
	    one_tilt = ppm->n_ad_nid;
	    one_running = ppm->alpha_ad_nid;
	  }

	  if ((ppt->has_ad == _TRUE_) && (ppt->has_niv == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_ad) && (index_ic2 == ppt->index_ic_niv)) ||
	       ((index_ic2 == ppt->index_ic_ad) && (index_ic1 == ppt->index_ic_niv)))) {
	    one_correlation = ppm->c_ad_niv;
	    one_tilt = ppm->n_ad_niv;
	    one_running = ppm->alpha_ad_niv;
	  }

	  if ((ppt->has_bi == _TRUE_) && (ppt->has_cdi == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_bi) && (index_ic2 == ppt->index_ic_cdi)) ||
	       ((index_ic2 == ppt->index_ic_bi) && (index_ic1 == ppt->index_ic_cdi)))) {
	    one_correlation = ppm->c_bi_cdi;
	    one_tilt = ppm->n_bi_cdi;
	    one_running = ppm->alpha_bi_cdi;
	  }

	  if ((ppt->has_bi == _TRUE_) && (ppt->has_nid == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_bi) && (index_ic2 == ppt->index_ic_nid)) ||
	       ((index_ic2 == ppt->index_ic_bi) && (index_ic1 == ppt->index_ic_nid)))) {
	    one_correlation = ppm->c_bi_nid;
	    one_tilt = ppm->n_bi_nid;
	    one_running = ppm->alpha_bi_nid;
	  }

	  if ((ppt->has_bi == _TRUE_) && (ppt->has_niv == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_bi) && (index_ic2 == ppt->index_ic_niv)) ||
	       ((index_ic2 == ppt->index_ic_bi) && (index_ic1 == ppt->index_ic_niv)))) {
	    one_correlation = ppm->c_bi_niv;
	    one_tilt = ppm->n_bi_niv;
	    one_running = ppm->alpha_bi_niv;
	  }

	  if ((ppt->has_cdi == _TRUE_) && (ppt->has_nid == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_cdi) && (index_ic2 == ppt->index_ic_nid)) ||
	       ((index_ic2 == ppt->index_ic_cdi) && (index_ic1 == ppt->index_ic_nid)))) {
	    one_correlation = ppm->c_cdi_nid;
	    one_tilt = ppm->n_cdi_nid;
	    one_running = ppm->alpha_cdi_nid;
	  }

	  if ((ppt->has_cdi == _TRUE_) && (ppt->has_niv == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_cdi) && (index_ic2 == ppt->index_ic_niv)) ||
	       ((index_ic2 == ppt->index_ic_cdi) && (index_ic1 == ppt->index_ic_niv)))) {
	    one_correlation = ppm->c_cdi_niv;
	    one_tilt = ppm->n_cdi_niv;
	    one_running = ppm->alpha_cdi_niv;
	  }

	  if ((ppt->has_nid == _TRUE_) && (ppt->has_niv == _TRUE_) && 
	      (((index_ic1 == ppt->index_ic_nid) && (index_ic2 == ppt->index_ic_niv)) ||
	       ((index_ic2 == ppt->index_ic_nid) && (index_ic1 == ppt->index_ic_niv)))) {
	    one_correlation = ppm->c_nid_niv;
	    one_tilt = ppm->n_nid_niv;
	    one_running = ppm->alpha_nid_niv;
	  }

	}

	class_test((one_correlation < -1) || (one_correlation > 1),
		   ppm->error_message,
		   "inconsistent input for isocurvature cross-correlation\n");

	index_ic1_ic2 = index_symmetric_matrix(index_ic1,index_ic2,ppm->ic_size[index_mode]);
	index_ic1_ic1 = index_symmetric_matrix(index_ic1,index_ic1,ppm->ic_size[index_mode]);
	index_ic2_ic2 = index_symmetric_matrix(index_ic2,index_ic2,ppm->ic_size[index_mode]);

	if (one_correlation == 0.) {
	  ppm->is_non_zero[index_mode][index_ic1_ic2] = _FALSE_;
	  ppm->amplitude[index_mode][index_ic1_ic2] = 0.;
	  ppm->tilt[index_mode][index_ic1_ic2] = 0.;
	  ppm->running[index_mode][index_ic1_ic2] = 0.;
	}
	else {
	  ppm->is_non_zero[index_mode][index_ic1_ic2] = _TRUE_;
	  ppm->amplitude[index_mode][index_ic1_ic2] = 
	    sqrt(ppm->amplitude[index_mode][index_ic1_ic1]*
		 ppm->amplitude[index_mode][index_ic2_ic2])*
	    one_correlation;
	  ppm->tilt[index_mode][index_ic1_ic2] = 
	    0.5*(ppm->tilt[index_mode][index_ic1_ic1]
		 +ppm->tilt[index_mode][index_ic2_ic2])
	    + one_tilt;
	  ppm->running[index_mode][index_ic1_ic2] = 
	    0.5*(ppm->running[index_mode][index_ic1_ic1]
		 +ppm->running[index_mode][index_ic2_ic2])
	    + one_running;
	}
      }
    }
  }
  
  return _SUCCESS_;

}

/**
 * This routine returns the primordial spectrum in the simple analytic case with
 * amplitudes, tilts, runnings, for each mode (scalar/tensor...),
 * pair of initial conditions, and wavenumber.
 *
 * @param ppm            Input/output: pointer to primordial structure 
 * @param index_mode     Input: index of mode (scalar, tensor, ...) 
 * @param index_ic1_ic2  Input: pair of initial conditions (ic1, ic2)
 * @param k              Input: wavenumber in same units as pivot scale, i.e. in 1/Mpc 
 * @param pk             Output: primordial power spectrum A (k/k_pivot)^(n+...)
 * @return the error status
 */

int primordial_analytic_spectrum(
				 struct primordial * ppm,
				 int index_mode,
				 int index_ic1_ic2,
				 double k,
				 double * pk
				 ) {  
  
  if (ppm->is_non_zero[index_mode][index_ic1_ic2] == _TRUE_) {
    *pk = ppm->amplitude[index_mode][index_ic1_ic2]
      *exp((ppm->tilt[index_mode][index_ic1_ic2]-1.)*log(k/ppm->k_pivot)
	   + 0.5 * ppm->running[index_mode][index_ic1_ic2] * pow(log(k/ppm->k_pivot), 2.));
  }
  else {
    *pk = 0.;
  }

  return _SUCCESS_;
  
}

int primordial_inflation_potential(
				   struct primordial * ppm,
				   double phi,
				   double * V,
				   double * dV,
				   double * ddV
				   ) {

  if (ppm->potential == polynomial) {

    *V   = ppm->V0+(phi-ppm->phi_pivot)*ppm->V1+pow((phi-ppm->phi_pivot),2)/2.*ppm->V2+pow((phi-ppm->phi_pivot),3)/6.*ppm->V3+pow((phi-ppm->phi_pivot),4)/24.*ppm->V4;
    *dV  = ppm->V1+(phi-ppm->phi_pivot)*ppm->V2+pow((phi-ppm->phi_pivot),2)/2.*ppm->V3+pow((phi-ppm->phi_pivot),3)/6.*ppm->V4;
    *ddV = ppm->V2+(phi-ppm->phi_pivot)*ppm->V3+pow((phi-ppm->phi_pivot),2)/2.*ppm->V4;
  
  }

  return _SUCCESS_;
}

int primordial_inflation_indices(
				 struct primordial * ppm
				 ) {

  int index_in;

  index_in = 0;

  ppm->index_in_a = index_in;
  index_in ++;
  ppm->index_in_phi = index_in;
  index_in ++;
  ppm->index_in_dphi = index_in;
  index_in ++;

  ppm->in_bg_size = index_in;

  ppm->index_in_ksi_re = index_in;
  index_in ++;
  ppm->index_in_ksi_im = index_in;
  index_in ++;
  ppm->index_in_dksi_re = index_in;
  index_in ++;
  ppm->index_in_dksi_im = index_in;
  index_in ++;
  ppm->index_in_ah_re = index_in;
  index_in ++;
  ppm->index_in_ah_im = index_in;
  index_in ++;
  ppm->index_in_dah_re = index_in;
  index_in ++;
  ppm->index_in_dah_im = index_in;
  index_in ++;

  ppm->in_size = index_in;

  return _SUCCESS_;
}

int primordial_inflation_solve_inflation(
					 struct perturbs * ppt,
					 struct primordial * ppm,
					 struct precision *ppr
					 ) {

  double * y;
  double * y_ini;
  double * dy;
  double a_pivot,a_try;
  double H_pivot,H_try;
  double phi_try;
  double dphidt_pivot,dphidt_try;
  double aH_ini;
  double k_min;
  double k_max;
  int counter;
  double V,dV,ddV;

  //  fprintf(stdout,"Expected slow-roll A_s: %g\n",128.*_PI_/3.*pow(ppm->V0,3)/pow(ppm->V1,2));
  //  fprintf(stdout,"Expected slow-roll T/S: %g\n",pow(ppm->V1/ppm->V0,2)/_PI_);
  //  fprintf(stdout,"Expected slow-roll A_T: %g\n",pow(ppm->V1/ppm->V0,2)/_PI_*128.*_PI_/3.*pow(ppm->V0,3)/pow(ppm->V1,2));
  //  fprintf(stdout,"Expected slow-roll n_s: %g\n",1.-6./16./_PI_*pow(ppm->V1/ppm->V0,2)+2./8./_PI_*(ppm->V2/ppm->V0));
  //  fprintf(stdout,"Expected slow-roll n_t: %g\n",-2./16./_PI_*pow(ppm->V1/ppm->V0,2));

  class_alloc(y,ppm->in_size*sizeof(double),ppm->error_message);
  class_alloc(y_ini,ppm->in_size*sizeof(double),ppm->error_message);
  class_alloc(dy,ppm->in_size*sizeof(double),ppm->error_message);

  class_call(primordial_inflation_check_potential(ppm,ppm->phi_pivot),
	     ppm->error_message,
	     ppm->error_message);

  class_call(primordial_inflation_find_attractor(ppm,
						 ppr,
						 ppm->phi_pivot,
						 ppr->primordial_inflation_attractor_precision_pivot,
						 y,
						 dy,
						 &H_pivot,
						 &dphidt_pivot),
	     ppm->error_message,
	     ppm->error_message);

  a_pivot = ppm->k_pivot/H_pivot;
  
  k_max = exp(ppm->lnk[ppm->lnk_size-1]);
  y[ppm->index_in_a] = a_pivot;
  y[ppm->index_in_phi] = ppm->phi_pivot;
  y[ppm->index_in_dphi] = a_pivot*dphidt_pivot;
  class_call(primordial_inflation_reach_aH(ppm,ppr,y,dy,k_max/ppr->primordial_inflation_ratio_max),
	     ppm->error_message,
	     ppm->error_message);

  aH_ini = exp(ppm->lnk[0])/ppr->primordial_inflation_ratio_min;
	       
  a_try = a_pivot;
  H_try = H_pivot;
  phi_try = ppm->phi_pivot;
  counter = 0;

  while ((a_try*H_try) >= aH_ini) {

    counter ++;
    class_test(counter >= ppr->primordial_inflation_phi_ini_maxit,
	       ppm->error_message,
	       "when searching for an initial value of phi just before observable inflation takes place, could not converge after %d iterations. The potential does not allow eough inflationary e-folds before reaching the pivot scale",
	       counter);

    class_call(primordial_inflation_potential(ppm,phi_try,&V,&dV,&ddV),
	       ppm->error_message,
	       ppm->error_message);

    phi_try += ppr->primordial_inflation_jump_initial*log(a_try*H_try/aH_ini)*dV/V/8./_PI_;
    
    class_call(primordial_inflation_find_attractor(ppm,
						   ppr,
						   phi_try,
						   ppr->primordial_inflation_attractor_precision_initial,
						   y,
						   dy,
						   &H_try,
						   &dphidt_try),
	       ppm->error_message,
	       ppm->error_message);

    y[ppm->index_in_a] = 1.;
    y[ppm->index_in_phi] = phi_try;
    y[ppm->index_in_dphi] = y[ppm->index_in_a]*dphidt_try;

    class_call(primordial_inflation_evolve_background(ppm,ppr,y,dy,ppm->phi_pivot),
	       ppm->error_message,
	       ppm->error_message);

    a_try = a_pivot/y[ppm->index_in_a];

  }

  y_ini[ppm->index_in_a] = a_try;
  y_ini[ppm->index_in_phi] = phi_try;
  y_ini[ppm->index_in_dphi] = a_try*dphidt_try;

  class_call(primordial_inflation_spectra(ppt,ppm,ppr,y_ini,y,dy),
	     ppm->error_message,
	     ppm->error_message);

  free(y);
  free(y_ini);
  free(dy);

  return _SUCCESS_;
};

int primordial_inflation_spectra(
				 struct perturbs * ppt,
				 struct primordial * ppm,
				 struct precision * ppr,
				 double * y_ini,
				 double * y,
				 double * dy
				 ) {
  double aH,k;
  int index_k;
  double V,dV,ddV;
  double curvature,tensors;

  class_call(primordial_inflation_check_potential(ppm,y_ini[ppm->index_in_phi]),
	     ppm->error_message,
	     ppm->error_message);

  class_call(primordial_inflation_potential(ppm,y_ini[ppm->index_in_phi],&V,&dV,&ddV),
	     ppm->error_message,
	     ppm->error_message);
  
  aH = sqrt((8*_PI_/3.)*(0.5*y_ini[ppm->index_in_dphi]*y_ini[ppm->index_in_dphi]+y_ini[ppm->index_in_a]*y_ini[ppm->index_in_a]*V));

  class_test(aH >= exp(ppm->lnk[0])/ppr->primordial_inflation_ratio_min,
	     ppm->error_message,
	     "at initial time, a_k_min > a*H*ratio_min");
  
  for (index_k=0; index_k < ppm->lnk_size; index_k++) {

    k = exp(ppm->lnk[index_k]);

    y[ppm->index_in_a] = y_ini[ppm->index_in_a];
    y[ppm->index_in_phi] = y_ini[ppm->index_in_phi];
    y[ppm->index_in_dphi] = y_ini[ppm->index_in_dphi];

    class_call(primordial_inflation_reach_aH(ppm,ppr,y,dy,k/ppr->primordial_inflation_ratio_min),
	       ppm->error_message,
	       ppm->error_message);

    class_call(primordial_inflation_one_k(ppm,ppr,k,y,dy,&curvature,&tensors),
	       ppm->error_message,
	       ppm->error_message);	     

    class_test(curvature<=0.,
	       ppm->error_message,
	       "negative curvature spectrum");

    class_test(tensors<=0.,
	       ppm->error_message,
	       "negative tensor spectrum");

    ppm->lnpk[ppt->index_md_scalars][index_k] = log(curvature);
    ppm->lnpk[ppt->index_md_tensors][index_k] = log(tensors);
    ppm->is_non_zero[ppt->index_md_scalars][0] = _TRUE_;
    ppm->is_non_zero[ppt->index_md_tensors][0] = _TRUE_;


    /* fprintf(stderr,"%e %e %e\n", */
    /* 	    ppm->lnk[index_k], */
    /* 	    ppm->lnpk[ppt->index_md_scalars][index_k], */
    /* 	    ppm->lnpk[ppt->index_md_tensors][index_k]); */
	    
  }

  return _SUCCESS_;

}

int primordial_inflation_one_k(
			       struct primordial * ppm,
			       struct precision * ppr,
			       double k,
			       double * y,
			       double * dy,
			       double * curvature,
			       double * tensor
			       ) {
  
  double tau_start,tau_end,dtau;
  double z,ksi2,ah2;
  double aH;
  double curvature_old;
  double curvature_new;
  double dlnPdN;

  struct primordial_inflation_parameters_and_workspace pipaw;
  struct generic_integrator_workspace gi;

  pipaw.ppm = ppm;
  pipaw.N = ppm->in_size;
  pipaw.k = k;

  class_call(initialize_generic_integrator(pipaw.N,&gi),
	     gi.error_message,
	     ppm->error_message);

  y[ppm->index_in_ksi_re]=1./sqrt(2.*k);
  y[ppm->index_in_ksi_im]=0.;
  y[ppm->index_in_dksi_re]=0.;
  y[ppm->index_in_dksi_im]=-k*y[ppm->index_in_ksi_re];

  y[ppm->index_in_ah_re]=1./sqrt(2.*k);
  y[ppm->index_in_ah_im]=0.;
  y[ppm->index_in_dah_re]=0.;
  y[ppm->index_in_dah_im]=-k*y[ppm->index_in_ah_re];

  curvature_new=1.e10;
  tau_end = 0;

  class_call(primordial_inflation_derivs(tau_end,
					 y,
					 dy,
					 &pipaw,
					 ppm->error_message),
	     ppm->error_message,
	     ppm->error_message);
    
  dtau = ppr->primordial_inflation_pt_stepsize*2.*_PI_/max(sqrt(fabs(dy[ppm->index_in_dksi_re]/y[ppm->index_in_ksi_re])),k);

  do {
    
    tau_start = tau_end;
    
    tau_end = tau_start + dtau;
      
    class_call(generic_integrator(primordial_inflation_derivs,
				  tau_start,
				  tau_end,
				  y,
				  &pipaw,
				  ppr->primordial_inflation_tol_integration,
				  ppr->smallest_allowed_variation,
				  &gi),
	       gi.error_message,
	       ppm->error_message);

    class_call(primordial_inflation_derivs(tau_end,
					   y,
					   dy,
					   &pipaw,
					   ppm->error_message),
	       ppm->error_message,
	       ppm->error_message);
    
    dtau = ppr->primordial_inflation_pt_stepsize*2.*_PI_/max(sqrt(fabs(dy[ppm->index_in_dksi_re]/y[ppm->index_in_ksi_re])),k);

    aH = dy[ppm->index_in_a]/y[ppm->index_in_a];

    curvature_old =  curvature_new;

    z = y[ppm->index_in_a]*y[ppm->index_in_dphi]/aH;
    ksi2 = y[ppm->index_in_ksi_re]*y[ppm->index_in_ksi_re]+y[ppm->index_in_ksi_im]*y[ppm->index_in_ksi_im];
    curvature_new = k*k*k/2./_PI_/_PI_*ksi2/z/z;

    dlnPdN = (curvature_new-curvature_old)/dtau*y[ppm->index_in_a]/dy[ppm->index_in_a]/curvature_new;

  } while ((k/aH >= ppr->primordial_inflation_ratio_max) || (fabs(dlnPdN) > ppr->primordial_inflation_tol_curvature));


  class_call(cleanup_generic_integrator(&gi),
	     gi.error_message,
	     ppm->error_message);

  *curvature = curvature_new;
  ah2 = y[ppm->index_in_ah_re]*y[ppm->index_in_ah_re]+y[ppm->index_in_ah_im]*y[ppm->index_in_ah_im];
  *tensor = 32.*k*k*k/_PI_*ah2/y[ppm->index_in_a]/y[ppm->index_in_a];

  //fprintf(stdout,"%g %g %g %g %g\n",k,*curvature,*tensor,*tensor/(*curvature),dlnPdN);

  return _SUCCESS_;
}

int primordial_inflation_find_attractor(
					struct primordial * ppm,
					struct precision * ppr,
					double phi_0,
					double precision,
					double * y,
					double * dy,
					double * H_0,
					double * dphidt_0
					) {

  double V_0,dV_0,ddV_0;
  double V,dV,ddV;
  double a;
  double dphidt,dphidt_0new,dphidt_0old,phi;
  int counter;

  class_call(primordial_inflation_potential(ppm,phi,&V_0,&dV_0,&ddV_0),
	     ppm->error_message,
	     ppm->error_message);

  dphidt_0new = -dV_0/3./sqrt((8.*_PI_/3.)*V_0);
  phi = phi_0;
  counter = 0;

  dphidt_0old = dphidt_0new/(precision+2.);

  while (fabs(dphidt_0new/dphidt_0old-1.) >= precision) {

    //fprintf(stderr,"-> %d %e %e %e %e\n",
    //    counter,dphidt_0new,dphidt_0old,fabs(dphidt_0new/dphidt_0old-1.),precision);
	    

    counter ++;
    class_test(counter >= ppr->primordial_inflation_attractor_maxit,
	       ppm->error_message,
	       "could not converge after %d iterations: there exists no attractor solution near phi=%g. Potential probably too steep in this region, or precision parameter primordial_inflation_attractor_precision=%g too small",
	       counter,
	       phi_0,
	       precision);

    dphidt_0old = dphidt_0new;

    phi=phi+dV_0/V_0/16./_PI_;

    class_call(primordial_inflation_check_potential(ppm,phi),
	       ppm->error_message,
	       ppm->error_message);

    class_call(primordial_inflation_potential(ppm,phi,&V,&dV,&ddV),
	       ppm->error_message,
	       ppm->error_message);

    a = 1.;
    dphidt = -dV/3./sqrt((8.*_PI_/3.)*V);
    y[ppm->index_in_a]=a;
    y[ppm->index_in_phi]=phi;
    y[ppm->index_in_dphi]=a*dphidt;
    
    //fprintf(stderr,"-> trying to evolve from %g to %g\n",
    //	    phi,phi_0);

    class_call(primordial_inflation_evolve_background(ppm,ppr,y,dy,phi_0),
	       ppm->error_message,
	       ppm->error_message);

    //    fprintf(stderr,"-> done\n");

    dphidt_0new = y[ppm->index_in_dphi]/y[ppm->index_in_a];

    // fprintf(stderr,"-> now %g\n",fabs(dphidt_0new/dphidt_0old-1.));

  }

  *dphidt_0 = dphidt_0new;
  *H_0 = sqrt((8.*_PI_/3.)*(0.5*dphidt_0new*dphidt_0new+V_0));

  // fprintf(stderr,"attractor found with %g %g\n",*dphidt_0,*H_0);

  return _SUCCESS_;
}

int primordial_inflation_evolve_background(
					   struct primordial * ppm,
					   struct precision * ppr,
					   double * y,
					   double * dy,
					   double phi_stop) {

  double tau_start,tau_end,dtau;
  double aH;
  double epsilon,epsilon_old;
  struct primordial_inflation_parameters_and_workspace pipaw;
  struct generic_integrator_workspace gi;

  pipaw.ppm = ppm;
  pipaw.N = ppm->in_bg_size;

  class_call(initialize_generic_integrator(pipaw.N,&gi),
	     gi.error_message,
	     ppm->error_message);

  class_call(primordial_inflation_get_epsilon(ppm,
					      y[ppm->index_in_dphi],
					      &epsilon),
	     ppm->error_message,
	     ppm->error_message);

  tau_end = 0;

  class_call(primordial_inflation_derivs(tau_end,
					 y,
					 dy,
					 &pipaw,
					 ppm->error_message),
	     ppm->error_message,
	     ppm->error_message);
      
  aH = dy[ppm->index_in_a]/y[ppm->index_in_a];
  dtau = ppr->primordial_inflation_bg_stepsize*min(1./aH,fabs(y[ppm->index_in_dphi]/dy[ppm->index_in_dphi]));

  while (y[ppm->index_in_phi] <= (phi_stop-y[ppm->index_in_dphi]*dtau)) {
    
    class_call(primordial_inflation_check_potential(ppm,
						    y[ppm->index_in_phi]),
	       ppm->error_message,
	       ppm->error_message);

    tau_start = tau_end;

    class_call(primordial_inflation_derivs(tau_start,
					   y,
					   dy,
					   &pipaw,
					   ppm->error_message),
	       ppm->error_message,
	       ppm->error_message);
      
    aH = dy[ppm->index_in_a]/y[ppm->index_in_a];
    dtau = ppr->primordial_inflation_bg_stepsize*min(1./aH,fabs(y[ppm->index_in_dphi]/dy[ppm->index_in_dphi]));

    tau_end = tau_start + dtau;

    class_call(generic_integrator(primordial_inflation_derivs,
				  tau_start,
				  tau_end,
				  y,
				  &pipaw,
				  ppr->primordial_inflation_tol_integration,
				  ppr->smallest_allowed_variation,
				  &gi),
	       gi.error_message,
	       ppm->error_message);

    epsilon_old = epsilon;

    class_call(primordial_inflation_get_epsilon(ppm,
						y[ppm->index_in_dphi],
						&epsilon),
	       ppm->error_message,
	       ppm->error_message);

    class_test((epsilon > 1) && (epsilon_old <= 1),
	       ppm->error_message,
	       "Inflaton evolution crosses the border from epsilon<1 to epsilon>1 at phi=%g. Inflation disrupted during the observable e-folds",
	       y[ppm->index_in_dphi]);



  }

  class_call(cleanup_generic_integrator(&gi),
	     gi.error_message,
	     ppm->error_message);

  class_call(primordial_inflation_derivs(tau_end,
					 y,
					 dy,
					 &pipaw,
					 ppm->error_message),
	     ppm->error_message,
	     ppm->error_message);

  dtau = (phi_stop-y[ppm->index_in_phi])/dy[ppm->index_in_dphi];
  y[ppm->index_in_a] += dy[ppm->index_in_a]*dtau;
  y[ppm->index_in_phi] += dy[ppm->index_in_phi]*dtau;
  y[ppm->index_in_dphi] += dy[ppm->index_in_dphi]*dtau;

  return _SUCCESS_;
}

int primordial_inflation_reach_aH(
				  struct primordial * ppm,
				  struct precision * ppr,
				  double * y,
				  double * dy,
				  double aH_stop
				  ) {

  double tau_start,tau_end,dtau;
  double aH;
  struct primordial_inflation_parameters_and_workspace pipaw;
  struct generic_integrator_workspace gi;

  pipaw.ppm = ppm;
  pipaw.N = ppm->in_bg_size;

  class_call(initialize_generic_integrator(pipaw.N,&gi),
	     gi.error_message,
	     ppm->error_message);

  tau_end = 0;

  class_call(primordial_inflation_derivs(tau_end,
					 y,
					 dy,
					 &pipaw,
					 ppm->error_message),
	     ppm->error_message,
	     ppm->error_message);

  while (dy[ppm->index_in_a]/y[ppm->index_in_a] < aH_stop) {

    class_call(primordial_inflation_check_potential(ppm,
						    y[ppm->index_in_phi]),
	       ppm->error_message,
	       ppm->error_message);

    tau_start = tau_end;

    class_call(primordial_inflation_derivs(tau_start,
					   y,
					   dy,
					   &pipaw,
					   ppm->error_message),
	       ppm->error_message,
	       ppm->error_message);
      
    aH = dy[ppm->index_in_a]/y[ppm->index_in_a];
    dtau = ppr->primordial_inflation_bg_stepsize*min(1./aH,fabs(y[ppm->index_in_dphi]/dy[ppm->index_in_dphi]));

    tau_end = tau_start + dtau;

    class_call(generic_integrator(primordial_inflation_derivs,
				  tau_start,
				  tau_end,
				  y,
				  &pipaw,
				  ppr->primordial_inflation_tol_integration,
				  ppr->smallest_allowed_variation,
				  &gi),
	       gi.error_message,
	       ppm->error_message);

  }

  class_call(cleanup_generic_integrator(&gi),
	     gi.error_message,
	     ppm->error_message);

  return _SUCCESS_;
}

int primordial_inflation_check_potential(
					 struct primordial * ppm,
					 double phi
					 ) {

  double V,dV,ddV;

  class_call(primordial_inflation_potential(ppm,phi,&V,&dV,&ddV),
	     ppm->error_message,
	     ppm->error_message);

  class_test(V <= 0.,
	     ppm->error_message,
	     "This potential becomes negative at phi=%g, before the end of observable inflation. It  cannot be treated by this code",
	     phi);
  
  class_test(dV >= 0.,
	     ppm->error_message,
	     "All the code is written for the case dV/dphi<0. Here, in phi=%g, we have dV/dphi=%g. This potential cannot be treated by this code",
	     phi,dV);
  
  return _SUCCESS_;
}

int primordial_inflation_get_epsilon(
				     struct primordial * ppm,
				     double phi,
				     double * epsilon
				     ) {

  double V,dV,ddV;
  
  class_call(primordial_inflation_potential(ppm,phi,&V,&dV,&ddV),
	     ppm->error_message,
	     ppm->error_message);
  
  *epsilon = 1./16./_PI_*pow(dV/V,2);

  //*eta = 1./8./pi*(ddV/V)

  return _SUCCESS_;

}

int primordial_inflation_derivs(
				double tau,
				double * y,
				double * dy,
				void * parameters_and_workspace,
				ErrorMsg error_message
				) {

  struct primordial_inflation_parameters_and_workspace * ppipaw;
  struct primordial * ppm;
  
  ppipaw = parameters_and_workspace;
  ppm = ppipaw->ppm;
  
  class_call(primordial_inflation_potential(ppm,
					    y[ppm->index_in_phi],
					    &(ppipaw->V),
					    &(ppipaw->dV),
					    &(ppipaw->ddV)),
	     ppm->error_message,
	     ppm->error_message);

  // BACKGROUND

  // a**2 V
  ppipaw->a2V=y[ppm->index_in_a]*y[ppm->index_in_a]*ppipaw->V;
  // a**2 dV/dphi
  ppipaw->a2dV=y[ppm->index_in_a]*y[ppm->index_in_a]*ppipaw->dV;
  // a H = a'/a      
  ppipaw->aH = sqrt((8*_PI_/3.)*(0.5*y[ppm->index_in_dphi]*y[ppm->index_in_dphi]+ppipaw->a2V));

  // 1: a
  dy[ppm->index_in_a]=y[ppm->index_in_a]*ppipaw->aH;
  // 2: phi
  dy[ppm->index_in_phi]=y[ppm->index_in_dphi];
  // 3: dphi/dtau
  dy[ppm->index_in_dphi]=-2.*ppipaw->aH*y[ppm->index_in_dphi]-ppipaw->a2dV;

  if (ppipaw->N == ppm->in_bg_size)
    return _SUCCESS_;

  // PERTURBATIONS

  // a**2 d2V/dphi2
  ppipaw->a2ddV=y[ppm->index_in_a]*y[ppm->index_in_a]*ppipaw->ddV;
  // z''/z
  ppipaw->zpp_over_z=2*ppipaw->aH*ppipaw->aH - ppipaw->a2ddV - 4.*_PI_*(7.*y[ppm->index_in_dphi]*y[ppm->index_in_dphi]+4.*y[ppm->index_in_dphi]/ppipaw->aH*ppipaw->a2dV) 
    +32.*_PI_*_PI_*pow(y[ppm->index_in_dphi],4)/pow(ppipaw->aH,2);
  // a''/a
  ppipaw->app_over_a=2.*ppipaw->aH*ppipaw->aH - 4.*_PI_*y[ppm->index_in_dphi]*y[ppm->index_in_dphi];
  
  // SCALARS
  // 4: ksi_re
  dy[ppm->index_in_ksi_re]=y[ppm->index_in_dksi_re];
  // 5: ksi_im
  dy[ppm->index_in_ksi_im]=y[ppm->index_in_dksi_im];
  // 6: d ksi_re / dtau
  dy[ppm->index_in_dksi_re]=-(ppipaw->k*ppipaw->k-ppipaw->zpp_over_z)*y[ppm->index_in_ksi_re];
  // 7: d ksi_im / dtau
  dy[ppm->index_in_dksi_im]=-(ppipaw->k*ppipaw->k-ppipaw->zpp_over_z)*y[ppm->index_in_ksi_im];

  // TENSORS
  // 8: ah_re
  dy[ppm->index_in_ah_re]=y[ppm->index_in_dah_re];
  // 9: ah_im
  dy[ppm->index_in_ah_im]=y[ppm->index_in_dah_im];
  // 10: d ah_re / dtau
  dy[ppm->index_in_dah_re]=-(ppipaw->k*ppipaw->k-ppipaw->app_over_a)*y[ppm->index_in_ah_re];
  // 11: d ah_im / dtau
  dy[ppm->index_in_dah_im]=-(ppipaw->k*ppipaw->k-ppipaw->app_over_a)*y[ppm->index_in_ah_im];
    
  return _SUCCESS_;

}
