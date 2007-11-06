/* smvspatial.c -- utility for manipulating 2-D detector images

	Copyright (c) 2007, Illinois Institute of Technology
	Specialized image-manipulation software.
	author: Andrew J Howard, BCPS Department, IIT
 See the file "LICENSE" for information on usage and redistribution
	of this file, and for a DISCLAIMER OF ALL WARRANTIES.

Change record:
	30 Oct 2007: substantial upgrade to handle masking correctly
	 and (I hope!) manage binned data correctly.
	22 Sep 2007: cleanup and corrections.
	20 Sep 2007: eliminate image rotation: we'll rotate
		the VSPLINE instead
	15 Sep 2007: make this a single file containing the whole calling
		sequence for this process.
	 6 Sep 2007: increase the standalone-ness of this code;
		eliminate use of image tables
	29 Jun 2007: recreated out of imutils.c
   Purpose: utility for performing a spatial correction on data
	derived from an SMV image using an X-GEN USPLINE file for input
   Cross references: internal to X-GEN
*************************************************************************/
#include	<stdio.h>
#include	<errno.h>

/* If we are being compiled by Microsoft Visual C++, we need a few
 * extra definitions.  (18 Sep 2007: William Lavender)
 */
#if defined(_MSC_VER)
  /* Suppress warnings about "deprecated" functions like fopen(). */
# pragma warning( disable:4996 )
#endif /* _MSC_VER */

	/* return values from routines */
#define		EOK		0	/* normal exit status */
#define		EALLOC		12	/* memory allocation error (ENOMEM) */
#define		EREADERR	5	/* error in reading (EIO) */
#define		EOPENERR	2	/* error in opening a file (ENOENT) */
#define		EBADVALUE	22	/* bad arg to a function (EINVAL) */
	/* floating and double parameters */
#define		FUZZ		(0.119e-6)	/* hardware FUZZ value */
#define		DFUZZ		(0.220e-15)	/* hardware double FUZZ value */
	/* flag bits for image conversions */
#define		PARTIAL_VERBOSE	0x1
#define		VERY_VERBOSE	0x2
#define		ANY_VERBOSE	(PARTIAL_VERBOSE | VERY_VERBOSE)
#define		ACTIVE_ONLY	0x8
	/* system-level declarations, appropriately latched */
#ifndef	__stdlib_h
extern void	*calloc(size_t count, size_t size);
extern void	*malloc(size_t size);
extern void	free(void *ptr);
#endif /* __stdlib_h */

#ifndef _UNISTD_H_
extern int	access(const char *path, int mode);
#endif /* _UNISTD_H_ */

/* structure type definitions */

 typedef struct { /* (X,Y) calibration point structure; totals   32 bytes:*/
		float	c_ptocx, c_ptocy, c_ctopx, c_ctopy;	/*    16 */
		float		c_qec, c_frfu[2];		/*    12 */
		unsigned char	c_ift, c_set, c_cset, c_ucrfu;	/*     4 */
		} CALPOINT;

 typedef struct { /* complete calibration structure;	totals 512 bytes:*/
		short		v_detw, v_deth, v_ncol, v_nrow;	 /*    8 */
		short		v_ntrue, v_midc, v_midr, v_nreg; /*    8 */
		float		v_axc, v_ayc, v_rxc, v_ryc;	 /*   16 */
		float		v_xrcm, v_yrcm, v_xrpx, v_yrpx;	 /*   16 */
		float		v_x0cm, v_y0cm, v_x0px, v_y0px;	 /*   16 */
		float		v_rmsshift, v_maxshift;		 /*    8 */
		float		v_maxx, v_maxy;			 /*    8 */
		double		v_lax[21], v_lay[21];		 /*  336 */
		double		v_ldx[6], v_ldy[6];		 /*   96 */
		/* These doubles and pointers are not read and written */
		double		v_dex[6], v_dey[6];		 /*   96 */
		CALPOINT	*v_cpt;				 /*    4 */
		int		*v_rffu;			 /*    4 */
		} VSPLINE;	/* total (not counting last stuff):  512 */

 typedef struct {
		int		imf_flags, imf_ht, imf_imsize, imf_wid;
		double		imf_x00, imf_xsl, imf_y00, imf_ysl;
		float		*imf_locim, *imf_locwt;
		unsigned char	*imf_lomask;
		unsigned short	*imf_data16, *imf_row;
		char		*imf_splname, *imf_masknam;
		VSPLINE		*imf_spl;
		} IMWORK;

extern int smvspatial(void *ima, int imw, int imh, int cfl, char *sn, char *mn);

static short sswab(short sh)
{ /*	This byte-swaps a SIGNED short.  This is comparatively tricky. */
	unsigned short	ush;

	ush = (unsigned short)sh;
	if (ush & 0x80)	return -1 - ((short)
		(0xffff - (((ush & 0xff) << 8) | (ush >> 8))));
	else		return (((ush & 0x7f) << 8) | (ush >> 8));
}

static void lflcvt(unsigned char *z)
{ /*	Converts floating-point numbers between big-endian and little-endian */
	unsigned char	t;

	t = *(z + 3);	*(z + 3) = *z;		*z = t;
	t = *(z + 1);	*(z + 1) = *(z + 2);	*(z + 2) = t;
}

static int checkorder(void)
{ /* returns 1 if the system is little-endian; 0 if not.
	We're hoping we don't end up with mixed word order */
	union {
		unsigned short	word_value;
		unsigned char	byte_value[2];
	} u;

	u.word_value = 0x1234;
	return (u.byte_value[0] == 0x34);
}

static void killimwork(IMWORK *imp, int which)
{ /* simple tool to clear allocated memory within the IMWORK structure */
	VSPLINE		*lspl;

	if ((which & 01) && (NULL != (lspl = imp->imf_spl)) &&
	 (NULL != lspl->v_cpt))
	  {
		if (NULL != lspl->v_cpt) free((void *)(lspl->v_cpt));
		lspl->v_cpt = NULL;
	  }
	if ((which & 02) && (NULL != (lspl = imp->imf_spl)))
	  {
		if (NULL != lspl->v_cpt) free((void *)(lspl->v_cpt));
		lspl->v_cpt = NULL;
		free((void *)(imp->imf_spl));	imp->imf_spl = NULL;
	  }
	if ((which & 04) && (NULL != (imp->imf_locim)))
	  {
		free((void *)(imp->imf_locim)); imp->imf_locim = NULL;
	  }
	if ((which & 010) && (NULL != (imp->imf_row)))
	  {
		free((void *)(imp->imf_lomask));
		imp->imf_lomask = NULL;	imp->imf_row = NULL;
	  }
}

static int smvspline(IMWORK *imp)
{ /* This populates a VSPLINE structure by reading it from the file
	imp->imf_splname.
	It returns EOK if successful, EREADERR or EALLOC otherwise.
	In this version, if the spline is already correctly populated,
	we leave it alone. */

	int		i, islittle, ncpt, nread;
	short		*sp;
	float		*flp;
	CALPOINT	*lca;
	static VSPLINE	*lspl;
	static FILE	*fspl;
	static size_t	ntor = 512;

	if (NULL == (lspl = imp->imf_spl))
	  {	
		if (NULL == (lspl = (VSPLINE *)calloc(1, sizeof (VSPLINE))))
		  {
			fprintf(stderr,
			 "Failed to allocate correction structure memory\n");
			return EALLOC;
		  }
	  }
	else {
		/* see if VSPLINE structure is populated appropriately */
		if ((1 < lspl->v_detw) && (lspl->v_detw < 32000) &&
		 (1 < lspl->v_deth) && (lspl->v_deth < 32000) &&
		 (NULL != lspl->v_cpt)) return EOK;
	    }
	killimwork(imp, 01);	/* destroy old v_cpt structures */
	if (NULL == (fspl = fopen(imp->imf_splname, "r"))) return EOPENERR;
	if (1 != fread((void *)lspl, ntor, 1, fspl))
	  {
		fprintf(stderr,
		 "Error reading first block of correction file %s\n",
			imp->imf_splname);
		return EREADERR;
	  }
	if ((islittle = checkorder())) /* is this a little-endian machine? */
	  {	/* byte swap shorts and flip bytes & words in floats */
		for (sp = &(lspl->v_detw); sp <= &(lspl->v_midr); sp++)
			*sp = sswab(*sp);
		for (flp = &(lspl->v_axc); flp <= &(lspl->v_y0px); flp++)
			lflcvt((void *)flp);
	  }
	ncpt = lspl->v_nrow * lspl->v_ncol;
	if (NULL == (lspl->v_cpt = (CALPOINT *)calloc(ncpt, sizeof (CALPOINT))))
	  {
		fprintf(stderr,
		 "Error allocating memory to handle correction operation\n");
		killimwork(imp, 03);
		(void)fclose(fspl);	fspl = NULL;	return EALLOC;
	  }
	if (imp->imf_flags & ANY_VERBOSE) printf(
	 " Correction file %s: Dimensions [%4d,%4d], %3d columns, %3d rows\n",
		imp->imf_splname, lspl->v_detw, lspl->v_deth,
		lspl->v_nrow, lspl->v_ncol);
	nread = fread((void *)(lspl->v_cpt), sizeof (CALPOINT), ncpt, fspl);
	(void)fclose(fspl);	fspl = NULL;
	if (nread != ncpt)
	  {
		fprintf(stderr, "Failed to read correction elements from %s\n",
			imp->imf_splname);
		killimwork(imp, 03);	return EREADERR;
	  }
	if (islittle) /* byte and word swaps again */
	  {
		for (lca = lspl->v_cpt, i = 0; i < ncpt; i++, lca++)
		   {
			for (flp = &(lca->c_ptocx); flp <= &(lca->c_qec);
			 flp++) lflcvt((void *)flp);
		   }
	  }
	imp->imf_spl = lspl;	/* set external pointer appropriately */
	return EOK;
}

static int rescalespl(IMWORK *imp)
{ /* This rescales the spline file so it's correctly sized relative to
  the images to which it will be applied. */
	int		ix, iy;
	double		xsca, ysca;
	VSPLINE		*lspl;
	CALPOINT	*calp;

	if (NULL == (lspl = imp->imf_spl)) return EBADVALUE;
	if ((lspl->v_detw < 1) || (lspl->v_deth < 1)) return EBADVALUE;
	xsca = ((double)(imp->imf_wid)) / ((double)(lspl->v_detw));
	ysca = ((double)(imp->imf_ht)) / ((double)(lspl->v_deth));
	if ((xsca < FUZZ) || (ysca < FUZZ)) return EBADVALUE;
	/* if the image and the spline are the same dimensions, we can quit */
	if ((xsca < 1. - FUZZ) || (xsca > 1. + FUZZ) ||
		(ysca < 1. - FUZZ) || (ysca > 1. + FUZZ))
	  {
		if (imp->imf_flags & ANY_VERBOSE) printf(
		 "Rescaling spline by a factor of %7.4f in X, %7.4f in Y\n",
			xsca, ysca);
		lspl->v_detw *= xsca;		lspl->v_deth *= ysca;
		lspl->v_axc *= xsca;		lspl->v_ayc *= ysca;
		lspl->v_rxc *= xsca;		lspl->v_ryc *= ysca;
		lspl->v_xrpx *= xsca;		lspl->v_yrpx *= ysca;
		for (calp = lspl->v_cpt, iy = 0; iy < lspl->v_nrow; iy++)
		   for (ix = 0; ix < lspl->v_ncol; ix++, calp++)
		      {
			calp->c_ptocx *= xsca; calp->c_ptocy *= ysca;
		      }
	  }
	else if (imp->imf_flags & ANY_VERBOSE) printf(
 "No rescaling of spline required: image and spline have same dimensions\n");
	return EOK;
}

static int locactive(IMWORK *imp)
{ /* This reads in the information associated with the active-area mask.
	It returns -2 if it's unable to set up the active-area mask,
	EOK otherwise.
  	We assume that the mask from which the active-area array is taken
	was created at the same level of binning as the image
	to which it'll be applied. */
	int		nr, pos, jy, npos;
	size_t		busiz, busiz2, imsiz, memneed;
	unsigned short	*usp;
	unsigned char	*ucp;
	char		*cp;
	static FILE	*fpmas;

	if ((NULL == (cp = imp->imf_masknam)) || ('\0' == *cp))
	  {
		killimwork(imp, 010);	return 0; /* no mask in this case */
	  }
	/* don't do anything if there's already a good mask.
	 We may eventually need a search for at least one good pixel here...*/
	if (NULL != imp->imf_lomask) return 0;
	/* read in the local-area active pixel mask from a file */
	busiz = imp->imf_ht;	imsiz = imp->imf_wid * busiz;
	memneed = imsiz + 2 * busiz;
	if (NULL == (ucp = (unsigned char *)malloc(memneed)))
	  {
		fprintf(stderr,
 "Unable to allocate %8d bytes for the active-pixel mask\n", (int)imsiz);
		return -2;
	  }
	imp->imf_lomask = ucp;
	imp->imf_row = (unsigned short *)(cp + imsiz);
	if (NULL == (fpmas = fopen(imp->imf_masknam, "r")))
	  {
		fprintf(stderr,
"Unable to open %s as input active-pixel mask\n", imp->imf_masknam);
		killimwork(imp, 010);	return -2;
	  }
	if (-1 == fseek(fpmas, 512L, 0))
	  {
		fprintf(stderr,
 "Unable to skip past image header during mask reading\n");
		killimwork(imp, 010);	return -2;
	  }
	busiz2 = busiz * 2;
	for (ucp = imp->imf_lomask, pos = 0; pos < imp->imf_wid; pos++)
	   { /* read through mask columns */
		if (1 != (nr = fread((void *)(imp->imf_row),busiz2, 1,fpmas)))
		  {
			fprintf(stderr,
 "Error reading column %6d of the active-pixel mask\n", pos);
			fprintf(stderr, "fread returned %d; errno = %d\n",
			 nr, (int)errno);
			killimwork(imp, 010);	return -2;
		  }
		for (usp = imp->imf_row, jy = 0; jy < busiz;
		 jy++, ucp++, usp++)
			*ucp = (*usp > 0); /* convert 16-bit->8-bit */
	   }
	(void)fclose(fpmas);	fpmas = NULL;
	if (imp->imf_flags & ANY_VERBOSE)
	  {
		ucp = imp->imf_lomask;
		for (npos = pos = 0; pos < imp->imf_wid; pos++)
		   for (jy = 0; jy < busiz; jy++, ucp++)
			if (*ucp == 1) npos++;
		fprintf(stderr,
		 " %d pixels out of a possible %d are active\n",
			npos, (int)(imp->imf_wid * busiz));
	  }
	return EOK;
}

static int find_limits(IMWORK *imp)
{ /*	This determines the limits of x and y in centimeters and populates
	the x and y slope and intercept values accordingly */
	int		jx, jy, pxxl, pxyl, pyxl, pyyl, pxxu, pxyu, pyxu, pyyu;
	int		x, xdel, xmax, xmin, y, ydel, ymin, ymax;
	double		dx, dy, omp, omq, p, q, rx, ry;
	double		xc, xma, xmi, yc, yma, ymi;
	unsigned char	*looff, *lom;
	CALPOINT	*cz00, *cz01, *cz10, *cz11, *czy00;
	VSPLINE		*lspl;

	lspl = imp->imf_spl;
	/* now walk through the values */
	if (imp->imf_flags & ANY_VERBOSE)
	  {
		fprintf(stdout,
 "     Minima(X,Y)     Maxima(X,Y)  Intervals(X,Y)    Offsets(X,Y)");
		fprintf(stdout, "***[X,Y] values where the extrema are***\n");
	  }
	ymi = xmi = 1.e9;		yma = xma = -ymi;	ymin = xmin = 0;
	xdel = xmax = imp->imf_wid;	ymax = ydel = imp->imf_ht;
	pxxl = pxyl = pyxl = pyyl = 10000; pxxu = pxyu = pyxu = pyyu = -pxxl;
	if (imp->imf_flags & ACTIVE_ONLY)
	  { /* just look at data within the active region */
		if (-2 == locactive(imp))
		  {
			fprintf(stderr, "Failure in locactive\n");
			return EREADERR;
		  }
		for (y = ymin; y < ymax; y++)
		   {
			dy = ((double)y) - lspl->v_y0px;
			ry = dy / lspl->v_yrpx;
			jy = (ry < 0.) ? 0 : ((ry < lspl->v_nrow - 1) ?
				ry : lspl->v_nrow - 2);
			if (lspl->v_nreg == 4)	 /* 4-element det: special */
			  {
				if (jy == lspl->v_nrow / 2 - 1) jy--;
				else if (jy == lspl->v_nrow / 2) jy++;
			  }
			/* remember the v_cpt offset for this row */
			czy00 = lspl->v_cpt + jy * lspl->v_ncol;
			/* calculate fractional offsets from this row */
			q = ry - (double)jy; /* fractional offset from row */
			looff = imp->imf_lomask + y;
			omq = 1. - q;
			for (x = xmin, lom = looff + xmin * imp->imf_ht;
			 x < xmax; x++, lom += imp->imf_ht)
			 if (*lom)
			   {
				dx = ((double)x) - lspl->v_x0px;
				rx = dx / lspl->v_xrpx;
				jx = (rx < 0.) ? 0 : ((rx < lspl->v_ncol - 1) ?
					rx : lspl->v_ncol - 2);
				if (lspl->v_nreg >= 2) /* special: 2&4 elt. */
				  {
					if (jx == lspl->v_ncol / 2 - 1) jx--;
					else if (jx == lspl->v_ncol / 2) jx++;
				  }
				/* find the v_cpt structures associated with
				 the 4 nearest points to this current pixel */
				cz00 = czy00 + jx;
				cz01 = cz00 + 1;
				cz10 = cz00 + lspl->v_ncol; cz11 = cz10 + 1;
				p = rx - (double)jx; /* fractional offset */
				omp = 1. - p;
				xc =	omp * omq * cz00->c_ctopx +
					omp *   q * cz10->c_ctopx +
					  p * omq * cz01->c_ctopx +
					  p *   q * cz11->c_ctopx;
				yc =	omp * omq * cz00->c_ctopy +
					omp *   q * cz10->c_ctopy +
					  p * omq * cz01->c_ctopy +
					  p *   q * cz11->c_ctopy;
				if (xc < xmi)
				  {
					xmi = xc; pxxl = x; pxyl = y;
				  }
				if (xc > xma)
				  {
					xma = xc; pxxu = x; pxyu = y;
				  }
				if (yc < ymi)
				  {
					ymi = yc; pyxl = x;	pyyl = y;
				  }
				if (yc > yma)
				  {
					yma = yc; pyxu = x;	pyyu = y;
				  }
			   } /* close of loop through x */
		   } /* close of loop through y */
	  } /* close of "if (imp->imf_flags & ACTIVE_ONLY)" clause */
	else { /* look at all data in establishing limits */
		for (y = ymin; y < ymax; y++)
		   {
			dy = ((double)y) - lspl->v_y0px;
			ry = dy / lspl->v_yrpx;
			jy = (ry < 0.) ? 0 : ((ry < lspl->v_nrow - 1) ?
				ry : lspl->v_nrow - 2);
			if (lspl->v_nreg == 4)	 /* 4-element det: special */
			  {
				if (jy == lspl->v_nrow / 2 - 1) jy--;
				else if (jy == lspl->v_nrow / 2) jy++;
			  }
			/* remember the v_cpt offset for this row */
			czy00 = lspl->v_cpt + jy * lspl->v_ncol;
			/* calculate fractional offsets from this row */
			q = ry - (double)jy; /* fractional offset from row */
			omq = 1. - q;
			for (x = xmin; x < xmax; x++)
			   {
				dx = ((double)x) - lspl->v_x0px;
				rx = dx / lspl->v_xrpx;
				jx = (rx < 0.) ? 0 : ((rx < lspl->v_ncol - 1) ?
					rx : lspl->v_ncol - 2);
				if (lspl->v_nreg >= 2) /* special: 2&4 elt. */
				  {
					if (jx == lspl->v_ncol / 2 - 1) jx--;
					else if (jx == lspl->v_ncol / 2) jx++;
				  }
				/* find the v_cpt structures associated with
				 the 4 nearest points to this current pixel */
				cz00 = czy00 + jx;
				cz01 = cz00 + 1;
				cz10 = cz00 + lspl->v_ncol; cz11 = cz10 + 1;
				p = rx - (double)jx; /* fractional offset */
				omp = 1. - p;
				xc =	omp * omq * cz00->c_ctopx +
					omp *   q * cz10->c_ctopx +
					  p * omq * cz01->c_ctopx +
					  p *   q * cz11->c_ctopx;
				yc =	omp * omq * cz00->c_ctopy +
					omp *   q * cz10->c_ctopy +
					  p * omq * cz01->c_ctopy +
					  p *   q * cz11->c_ctopy;
				if (xc < xmi)
				  {
					xmi = xc; pxxl = x; pxyl = y;
				  }
				if (xc > xma)
				  {
					xma = xc; pxxu = x; pxyu = y;
				  }
				if (yc < ymi)
				  {
					ymi = yc; pyxl = x;	pyyl = y;
				  }
				if (yc > yma)
				  {
					yma = yc; pyxu = x;	pyyu = y;
				  }
			   } /* close of loop through x */
		   } /* close of loop through y */
	     } /* close of else of "if (imp->imf_flags & ACTIVE_ONLY)" clause*/
	imp->imf_xsl = ((double)(xdel-1)) / (xma - xmi);
	imp->imf_x00 = -xmi * imp->imf_xsl;
	imp->imf_ysl = ((double)(ydel-1)) / (ymi - yma);
	imp->imf_y00 = -yma * imp->imf_ysl;
	if (imp->imf_flags & ANY_VERBOSE)
	  {
		fprintf(stdout,
		 " %7.3f %7.3f %7.3f %7.3f %7.2f %7.2f %7.2f %7.2f ",
		 xmi, ymi, xma, yma,
		 imp->imf_xsl, imp->imf_ysl, imp->imf_x00, imp->imf_y00);
		fprintf(stdout, " %4d %4d %4d %4d %4d %4d %4d %4d\n",
		 pxxl, pxyl, pyxl, pyyl, pxxu, pxyu, pyxu, pyyu);
	  }
	return EOK;
}

static int interp_image(IMWORK *imp)
{ /*	Here is where the actual interpolation takes place. */
	int		imv, ix, iy, jx, jy, obounds;
	int		off0, off1, off2, off3, x, y;
	double		dx, dy, omp, omq, p, px, py, q, rimv, rx, ry;
	double		wt0, wt1, wt2, wt3;
	float		xc, yc, *loim, *lowt;
	size_t		imsiz;
	unsigned char	*looff, *lom;
	CALPOINT	*cz00, *cz01, *cz10, *cz11, *czx00;
	VSPLINE		*lspl;

	/* size of data16 buffer */
	imp->imf_imsize = imsiz = imp->imf_wid * imp->imf_ht;
	/* reserve memory for floating-point image and weight values */
	if (NULL == (loim = (float *)calloc(2 * imsiz, sizeof (float))))
	  {
		fprintf(stderr,
		 "Error allocating memory for floating-point image\n");
		return EALLOC;
	  }
	imp->imf_locim = loim;	imp->imf_locwt = lowt = loim + imsiz;
	lspl = imp->imf_spl;
	for (obounds = x = 0; x < imp->imf_wid; x++) /* convert pix to cm */
	   {
		dx = ((double)x) - lspl->v_x0px; rx = dx / lspl->v_xrpx;
		jx = (rx < 0.) ? 0 : ((rx < lspl->v_ncol - 1) ?
			rx : lspl->v_ncol - 2);
		if (lspl->v_nreg >= 2) /* special handling,2&4 elt. detectors*/
		  {
			if (jx == lspl->v_ncol / 2 - 1) jx--;
			else if (jx == lspl->v_ncol / 2) jx++;
		  }
		/* find v_cpt structure offsets for this column */
		czx00 = lspl->v_cpt + jx;
		/* calculate the fractional offsets from those points */
		p = rx - (double)jx;	omp = 1. - p;
		/* looff only matters when imf_flags & ACTIVE_ONLY is on,
		 but we don't want to run into memory overruns */
		if (imp->imf_flags & ACTIVE_ONLY)
			looff = imp->imf_lomask + x * imp->imf_ht;
		else	looff = (unsigned char *)(imp->imf_data16);
		for (y = 0, lom = looff; y < imp->imf_ht; y++, lom++)
		{
		 if ((!(imp->imf_flags & ACTIVE_ONLY)) || (*lom))
		   {
			dy = ((double)y) - lspl->v_y0px; ry = dy / lspl->v_yrpx;
			jy = (ry < 0.) ? 0 : ((ry < lspl->v_nrow - 1) ?
				ry : lspl->v_nrow - 2);
			if (lspl->v_nreg == 4)	 /* 4-element detectors*/
			  {
				if (jy == lspl->v_nrow / 2 - 1) jy--;
				else if (jy == lspl->v_nrow / 2) jy++;
			  }
			/* find v_cpt structures associated with
			 the 4 nearest points to this current pixel */
			cz00 = czx00 + jy * lspl->v_ncol;
			cz01 = cz00 + 1;
			cz10 = cz00 + lspl->v_ncol; cz11 = cz10 + 1;
			/* calculate fractional offset from this row */
			q = ry - (double)y;	omq = 1. - q;
			xc =	omp * omq * cz00->c_ctopx +
				omp *   q * cz10->c_ctopx +
				  p * omq * cz01->c_ctopx +
				  p *   q * cz11->c_ctopx;
			yc =	omp * omq * cz00->c_ctopy +
				omp *   q * cz10->c_ctopy +
				  p * omq * cz01->c_ctopy +
				  p *   q * cz11->c_ctopy;
	     		/* rescale coordinates into the proper dimensions */
			xc = imp->imf_xsl * xc + imp->imf_x00;
			yc = imp->imf_ysl * yc + imp->imf_y00;
			/* add a fraction of input value into sum for each
			 of the four corners of the box in which pixel rests.
			 Be careful at the edges */
			if ((-1 <= xc) && (xc < imp->imf_wid) &&
			 (-1 <= yc) && (yc < imp->imf_ht))
			  {
				ix = xc;	iy = yc;
				px = xc - (double)ix;	if (xc < 0) ix--;
				py = yc - (double)iy;	if (yc < 0) iy--;
				off0 = ix * imp->imf_ht + iy;	off1 = off0 + 1;
				off2 = off0 + imp->imf_ht;	off3 = off2 + 1;
				wt0 = (1. - px) * (1. - py);
				wt1 = py * (1. - px);
				wt2 = px * (1. - py);
				wt3 = px * py;
				/* find the input intensity value */
				imv = imp->imf_data16[x * imp->imf_ht + y];
				rimv = ((double)imv);
				if ((ix >= 0) && (iy >= 0))
				  {
					*(loim + off0) += wt0 * rimv;
					*(lowt + off0) += wt0;
				  }
				if ((ix >= 0) && (iy < imp->imf_ht - 1))
				  {
					*(loim + off1) += wt1 * rimv;
					*(lowt + off1) += wt1;
				  }
				if ((ix < imp->imf_wid - 1) && (iy >= 0))
				  {
					*(loim + off2) += wt2 * rimv;
					*(lowt + off2) += wt2;
				  }
				if ((ix < imp->imf_wid - 1) &&
				 (iy < imp->imf_ht - 1))
				  {
					*(loim + off3) += wt3 * rimv;
					*(lowt + off3) += wt3;
				  }
			  } /* end of "if ((-1 <= xc" clause */
		   } /* end of check for active pixel */
		  else obounds++;
		} /* end of loop through Y */
	   } /* end of loop through X */
	if ((obounds > 0) && (imp->imf_flags & ANY_VERBOSE)) printf(
	 " %6d pixels mapped to coordinates outside the adjusted limits\n",
		obounds);
	return EOK;
}

static int normalize(IMWORK *imp)
{ /*	this renormalizes by dividing the weighted sums by the weights.
	The routine returns the number of non-found pixels */
	int	x, y, noreps, nearedge, onedge;
	float	*fpv, *fpw, *loim, *lowt;

	noreps = nearedge = onedge = 0;
	loim = imp->imf_locim;	lowt = imp->imf_locwt;
	for (fpv = loim, fpw = lowt, x = 0; x < imp->imf_wid; x++)
	   for (y = 0; y < imp->imf_ht; y++, fpv++, fpw++)
	      {
		if (*fpw > FUZZ) *fpv /= *fpw;
		else
		  {
			noreps++;
			if ((x < 4) || (y < 4) || (x >= imp->imf_wid - 4) ||
			 (y >= imp->imf_ht - 4))
			  {
				nearedge++;
				if ((x < 2) || (y < 2) ||
				 (x >= imp->imf_wid - 2) ||
				 (y >= imp->imf_ht  - 2)) onedge++;
			  }
		     }
	      }
	if (imp->imf_flags & VERY_VERBOSE) /* code for this printout */
	  {
		printf(
		 " %8d pixels out of %8d were missing interpolatable values\n",
			noreps, imp->imf_imsize);
		printf(" of these, %8d were within three pixels of the edge\n",
			nearedge);
		printf(" of these, %8d were within   one pixel  of the edge\n",
			onedge);
	  }
	return noreps;
}

static void stredge(float *fpv, float *fpw, int offset)
{ /* simple extrapolation scheme */
	float	denom;	

	if ((denom = *(fpw + offset)) > FUZZ)
	  {
		*fpw += denom;	*fpv += denom * *(fpv + offset);
	  }
}

static int extrap_pix(IMWORK *imp, int norep)
{ /*	This extrapolates to account for missing pixels.
	It returns the number that are still missing when it's done */
	int	corrected, hit2, ix, iy, nearedge, off0, onedge, wid2, y;
	float	*fpv, *fpw, *loim, *lowt, *spmin, *spmax;

	loim = imp->imf_locim;	lowt = imp->imf_locwt;
	hit2 = imp->imf_ht / 2;	wid2 = imp->imf_wid / 2;
	spmin = imp->imf_locim;
	spmax = imp->imf_locim + imp->imf_wid * imp->imf_ht;
	y = imp->imf_ht;
	for (ix = hit2, corrected = 0; (ix >= 0) && (ix < imp->imf_wid); )
	   {
		for (iy = hit2; (iy >= 0) && (iy < imp->imf_ht); )
		   {
			off0 = ix * imp->imf_ht + iy;
			fpv = spmin + off0;
			fpw = imp->imf_locwt + off0;
			if (FUZZ > *fpw)
			  {
				*fpv = 0.;
				if (ix > 0)
				  {
					if (iy > 0) stredge(fpv, fpw, -y - 1);
					stredge(fpv, fpw, -y);
					if (iy < y-1) stredge(fpv, fpw, 1 - y);
				  }
				if (iy > 0) stredge(fpv, fpw, -1);
				if (iy < y - 1) stredge(fpv, fpw, 1);
				if (ix < imp->imf_wid - 1)
				  {
					if (iy > 0) stredge(fpv, fpw, y - 1);
					stredge(fpv, fpw, y);
					if (iy < y-1) stredge(fpv, fpw, 1 + y);
				  }
				if (*fpw > FUZZ)
				  { /* these shouldn't get high weights */
					*fpv /= *fpw;	corrected++;
					*fpw = 0.1;
					
				  }
				else {
					*fpw = *fpv = 0.;
				     }
			  }
			if (corrected >= norep) break;
			iy = (iy >= hit2) ?
			 imp->imf_ht - 1 - iy : imp->imf_ht - iy;
		   }
		if (corrected >= norep) break;
		ix = (ix >= wid2) ?
			imp->imf_wid - 1 - ix : imp->imf_wid - ix;
	   }
	if (corrected >= norep)
	  {
		if (imp->imf_flags & VERY_VERBOSE)
			printf("All points found on extrapolation pass\n");
		return 0;
	  }
	else {
		norep = nearedge = onedge = 0;
		for (fpv = imp->imf_locim, fpw = imp->imf_locwt, ix = 0;
		 ix < imp->imf_wid; ix++)
		   for (iy = 0; iy < imp->imf_ht; iy++, fpv++, fpw++)
		      {
			if (*fpw <= FUZZ)
			  {
				norep++;
				if ((ix < 4) || (iy < 4) ||
				 (ix >= imp->imf_wid - 4) ||
				 (iy >= imp->imf_ht - 4))
				  {
					nearedge++;
					if ((ix < 2) || (iy < 2) ||
					 (ix >= imp->imf_wid - 2) ||
					 (iy >= imp->imf_ht  - 2)) onedge++;
				  }
			     }
		      }
		if (imp->imf_flags & VERY_VERBOSE)
		  {
			printf("after extrapolating:\n");
			printf(
 " %8d pixels out of %8d were missing interpolatable values\n",
				norep, imp->imf_imsize);
			printf(
 " of these, %8d were within three pixels of the edge\n", nearedge);
			printf(
" of these, %8d were within   one pixel  of the edge\n", onedge);
		  }
		return norep;
	     }
}

static int creat_undistor(IMWORK *imp)
{ /* This puts the floating-point image back into integers.
	In this version we require that the rescaled values
	be containable within 16-bit unsigned integers */
	double		ecount;
	float		maxfl, *fpv, *loim;
	int		x, y, nov;
	unsigned int	tct, zma;
	unsigned short	*ov, zmxx, zmxy;

	zma = ecount = 0;	zmxx = zmxy = 0;
	loim = imp->imf_locim;
	/* first, count the 16-bit overflows */
	maxfl = 0.;
	for (nov = x = 0, fpv = imp->imf_locim;
	 x < imp->imf_wid; x++)
	   for (y = 0; y < imp->imf_ht; y++, fpv++)
		if (*fpv > 65535.)
		  {
			nov++;	if (*fpv > maxfl) maxfl = *fpv;
		  }
	if (nov)
	  { /* rescale so brightest pixel is < 65K */
		if (imp->imf_flags & VERY_VERBOSE) fprintf(stdout,
		 " Brightest renormalized pixel has I = %12.5e\n",
			maxfl);
		maxfl /= 65000.;
		for (x = 0, fpv = loim; x < imp->imf_wid; x++)
		 for (y = 0; y < imp->imf_ht; y++, fpv++)
			*fpv /= maxfl;
		nov = 0;
	  }
	if (!nov) /* no 16bit 'flows: easy! */	
	 {
		for (x = 0, fpv = loim, ov = imp->imf_data16;
		 x < imp->imf_wid; x++)
		   {
			for (tct = y = 0; y < imp->imf_ht;
			 y++, fpv++, ov++)
			   {
				*ov = (unsigned short)(*fpv + 0.49999);
				tct += *ov;
				if (*ov > zma)
				  {
					zma = *ov;
					zmxx = x;	zmxy = y;
				  }
			   }
			ecount += (double)tct;
		   }
	 }
	else return EBADVALUE;	/* failure to set everything below 65k */
	if (imp->imf_flags & ANY_VERBOSE)
		printf("Image has %12.0f counts; zmax = %7d at [%4d,%4d]\n",
			ecount, (int)zma, zmxx, (int)(imp->imf_ht - 1 - zmxy));
	return EOK;
}

static void sumsmvspat(FILE *fp, IMWORK *imp)
{ /*	This summarizes the run of 'smvspatial' */

	fprintf(fp, "An image is being spatially corrected.\n");
	fprintf(fp, "Spatial correction derived from file %s\n",
		imp->imf_splname);
	fprintf(fp, "Image is being written back into the original memory\n");
}

static int img_conversion(IMWORK *imp)
{ /*	This does the conversion on a single image.
	It returns EOK if successful, various errors if not */
	int	imerr, noreps;

	/* read in the spatial-correction information */
	if (EOK != (imerr = smvspline(imp))) return imerr;
	if (EOK != (imerr = rescalespl(imp))) return imerr; /* rescale it */
	if (EOK != (imerr = find_limits(imp)))
	  { /* find the limits of x and y in cm */
		fprintf(stderr, "Error %d finding rescaling values in spline\n",
		 imerr);
		return imerr;
	  }
	if (EOK != (imerr = interp_image(imp)))
	  {
		fprintf(stderr, "Error %d interpolating the image\n", imerr);
		return imerr;
	  }
	/* re-generate the output values by dividing the weighted
	 count values by the weights */
	noreps = normalize(imp);
	/* extrapolate to take care of missing pixels */
	noreps = extrap_pix(imp, noreps);
	/* phew. Now we can make an image out of this array. */
	imerr = creat_undistor(imp);
	return imerr;
}

int smvspatial(void *imarr, int imwid, int imht, int cflags,
 char *splname, char *masknam)
{ /* Mainline for performing a spatial correction on an SMV image
	that is already in memory. Arguments:
	imarr	1-D array of data values (probably always unsigned shorts)
		The output will be written back to this pointer as well.
	cflags	flag value. Unlike earlier versions,
	 the only active bits in this flag now are associated with verbosity:
		bit 1	moderate amounts of verbosity
		bit 2	higher verbosity
		bit 3	even higher verbosity (not currently used)
		bit 4	only use active area in correction
	splname	Name of the spatial-correction (*.uca) file.
	masknam	Name of the active-pixel-mask image file.
	In this version, the detector dimensions don't need to be
	specified, because they're read in from splname.
  */
	int		resu;
	static IMWORK	imfs;

	/* special call to clear all volatile memory in imfs */
	if ((imwid == -1) && (imht == -1))
	  {
		killimwork(&imfs, 017);	return EOK;
	  }
	imfs.imf_data16 = (unsigned short *)imarr;
	imfs.imf_splname = splname;
	if ((NULL != masknam) && ('\0' != *masknam) &&
	 (0 == access(masknam, 04)))
	  {
		imfs.imf_flags = cflags | ACTIVE_ONLY;
		imfs.imf_masknam = masknam;
	  }
	else {
		imfs.imf_flags = cflags & ~ACTIVE_ONLY;
		imfs.imf_masknam = NULL;
	     }
	imfs.imf_wid = imwid;	imfs.imf_ht = imht;
	if (cflags & ANY_VERBOSE) sumsmvspat(stdout, &imfs);
	resu = img_conversion(&imfs);	/* perform spatial correction */
	return resu;
}
