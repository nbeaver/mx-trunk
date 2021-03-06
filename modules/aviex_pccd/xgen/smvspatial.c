/* smvspatial.c -- utility for manipulating 2-D detector images

	Copyright (c) 2007, Illinois Institute of Technology
	Specialized image-manipulation software.
	author: Andrew J Howard, BCPS Department, IIT
 See the file "LICENSE" for information on usage and redistribution
	of this file, and for a DISCLAIMER OF ALL WARRANTIES.

Change record:
	13 Dec 2007: switch to getting active-pixel mask from memory
	 rather than from disk.
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
#define		EXTRAP_REGION	0x10
#define		SCALED_MASK	0x20
	/* system-level declarations, appropriately latched */
#ifndef	__stdlib_h
extern void	*calloc(size_t count, size_t size);
extern void	*malloc(size_t size);
extern void	free(void *ptr);
extern int	atoi(const char *nptr);
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
		unsigned short	*imf_lomask, *imf_data16;
		char		*imf_splname;
		VSPLINE		*imf_spl;
		} IMWORK;

extern unsigned short *locactive(char *ma, unsigned short *inm,
 int inw, int inh);

static int mystrncmp(char *s1, char *s2, size_t len)
{ /* local version of strncmp */
	int		i;
	unsigned char	*us1, *us2, *uc1, *uc2;

	if (NULL == s1) return ((NULL == s2) ? 0 : -1); /* check args */
	if (NULL == s2) return 1;
	uc1 = us1 = (unsigned char *)s1; uc2 = us2 = (unsigned char *)s2;
	/* walk through the two strings, looking for either termination or
	 differences */
	for (i = 0; i < len; i++, uc1++, uc2++)
	   {
		if ('\0' == *uc1) return ('\0' == *uc2) ? 0 : -1;
		if ('\0' == *uc2) return 1;
		if (*uc1 < *uc2) return -1;
		else if (*uc1 > *uc2) return 1;
	   }
	return 0;
}

static int headtodim(FILE *fp, int *hwid, int *hht)
{ /* this returns values for the mask-file image width and height.
  It looks backward for keywords: only the last one counts! */
	int		i;
	char		hbuf[512], *cp, *lnn;
	static char	d1[] = "SIZE1=";
	static char	d2[] = "SIZE2=";

	*hwid = *hht = -1;
	if (1 != fread((void *)hbuf, 512, 1, fp)) return -1;
	for (cp = hbuf + 511, i = 511, lnn = hbuf; i >= 0; cp--, i--)
		if ('\0' != *cp)
		  {
			lnn = cp;	break;
		  }
	if (lnn < hbuf + 4) return -2;
	for (cp = lnn; cp > hbuf; cp--)
		if (!mystrncmp(cp, d2, 5))
		  {
			*hht = atoi(cp + 6); break;
		  }
	for (cp = lnn; cp > hbuf; cp--)
		if (!mystrncmp(cp, d1, 5))
		  {
			*hwid = atoi(cp + 6); break;
		  }
	if ((*hwid < 0) || (*hht < 0)) return -3;
	return 0;
}

unsigned short *locactive(char *masknam, unsigned short *inmask,
 int inwid, int inht)
{ /* This reads in the information associated with the active-area mask.
	It returns NULL if it's unable to set up the active-area mask,
	a pointer to the mask array otherwise.
  	We assume that the mask from which the active-area array is taken
	was created at the same level of binning as the image
	to which it'll be applied.
	We assume that we do _not_ need to read in the mask if the input
	mask is nonzero and its dimensions are equal to the dimensions
	of the one we would otherwise be reading in */
	int		mht, mwid, ngt1, nr, pos, jy, npos;
	size_t		busiz, busiz2, imsiz, memneed;
	unsigned short	*row, *usp0, *usp, *ousp;
	static FILE	*fpmas;

	/* make sure we've named the mask */
	if ((NULL == masknam) || ('\0' == *masknam))
	  {
		fprintf(stderr, "No mask name specified\n");
		return inmask;
	  }
	/* first we read enough of the input mask file to know its dimensions.
	*/
	if (NULL == (fpmas = fopen(masknam, "r")))
	  {
		fprintf(stderr, "Unable to open mask file name %s\n", masknam);
		return inmask;
	  }
	mwid = inwid;	mht = inht;
	if (0 != headtodim(fpmas, &mwid, &mht))
	  {
		fprintf(stderr, "Can't determine mask-file dimensions in %s\n",
			masknam);
		return inmask;
	  }
	/* if the dimensions haven't changed and the input mask is good,
	just return in the input mask.
	We may eventually need a search for at least one good pixel here...*/
	if ((mwid == inwid) && (mht == inht) && (NULL != inmask))
	  {
		(void)fclose(fpmas);	return inmask;
	  }
	/* otherwise, read in the local-area active pixel mask from a file.
	Note that the file pointer is already positioned at the beginning
	of the 16-bit data in `headtodim'. */
	busiz = mht;	imsiz = mwid * busiz;
	memneed = 2 * (imsiz + busiz);
	if (NULL == (usp0 = (unsigned short *)malloc(memneed)))
	  {
		fprintf(stderr,
 "Unable to allocate %8d bytes for the active-pixel mask\n", (int)memneed);
		return inmask;
	  }
	row = (unsigned short *)(usp0 + imsiz);
	busiz2 = busiz * 2;
	for (ousp = usp0, pos = 0; pos < mwid; pos++)
	   { /* read through mask columns */
		if (1 != (nr = fread((void *)row, busiz2, 1, fpmas)))
		  {
			fprintf(stderr,
 "Error reading column %6d of the active-pixel mask\n", pos);
			fprintf(stderr, "fread returned %d; errno = %d\n",
			 nr, (int)errno);
			free((void *)usp0);	return inmask;
		  }
		for (usp = row, jy = 0; jy < busiz; jy++, ousp++, usp++)
		   {
			if (*usp > 253)
			  {
				fprintf(stderr, "Tilt! byteorder wrong!\n");
				free((void *)usp);	return inmask;
			  }
			*ousp = *usp;	/* put result in output buffer */
		   }
	   }
	(void)fclose(fpmas);	fpmas = NULL;
	usp = usp0;
	for (ngt1 = npos = pos = 0; pos < mwid; pos++)
	   for (jy = 0; jy < busiz; jy++, usp++)
	      {
		if (*usp)
		  {
			npos++; if (*usp > 1) ngt1++;
		  }
	      }
	fprintf(stderr, " %d pixels out of a possible %d are active\n",
		npos, (int)(mwid * busiz));
	if (ngt1 > 0) fprintf(stderr, " Weighted mask is being used\n");
	return usp0;
}

extern int smvspatial(void *ima, int imw, int imh, int cfl, char *sn,
 unsigned short *mn);

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
	/* we don't mess with the lomask memory */
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
	if (imp->imf_flags & ANY_VERBOSE) fprintf(stderr,
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
		if (imp->imf_flags & ANY_VERBOSE) fprintf(stderr,
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
	else if (imp->imf_flags & ANY_VERBOSE) fprintf(stderr,
 "No rescaling of spline required: image and spline have same dimensions\n");
	return EOK;
}

static void countactive(IMWORK *imp)
{ /* This validates the active-pixel mask by counting the number of
	nonzero pixels in it.
  	We assume that the mask from which the active-area array is taken
	was created at the same level of binning as the image
	to which it'll be applied. */
	int		jy, ngt1, npos, pos;
	unsigned short	*usp;

	usp = imp->imf_lomask;
	for (ngt1 = npos = pos = 0; pos < imp->imf_wid; pos++)
	   for (jy = 0; jy < imp->imf_ht; jy++, usp++)
	      {
		if (*usp)
		  {
			npos++;	if (*usp > 1) ngt1++;
		  }
	      }
	if (ngt1 > 0) imp->imf_flags |= SCALED_MASK;
	if (imp->imf_flags & ANY_VERBOSE)
	  {
		fprintf(stderr,
		 " %d pixels out of a possible %d are active\n",
			npos, (int)(imp->imf_wid * imp->imf_ht));
		if (ngt1 > 0) fprintf(stderr," Weighted mask is being used\n");
	  }
}

static int find_limits(IMWORK *imp)
{ /*	This determines the limits of x and y in centimeters and populates
	the x and y slope and intercept values accordingly */
	int		jx, jy, pxxl, pxyl, pyxl, pyyl, pxxu, pxyu, pyxu, pyyu;
	int		x, xdel, xmax, xmin, y, ydel, ymin, ymax;
	double		dx, dy, omp, omq, p, q, rx, ry;
	double		xc, xma, xmi, yc, yma, ymi;
	unsigned short	*looff, *lom;
	CALPOINT	*cz00, *cz01, *cz10, *cz11, *czy00;
	VSPLINE		*lspl;

	lspl = imp->imf_spl;
	/* now walk through the values */
	if (imp->imf_flags & ANY_VERBOSE)
	  {
		fprintf(stderr,
 "     Minima(X,Y)     Maxima(X,Y)  Intervals(X,Y)    Offsets(X,Y)");
		fprintf(stderr, "***[X,Y] values where the extrema are***\n");
	  }
	ymi = xmi = 1.e9;		yma = xma = -ymi;	ymin = xmin = 0;
	xdel = xmax = imp->imf_wid;	ymax = ydel = imp->imf_ht;
	pxxl = pxyl = pyxl = pyyl = 10000; pxxu = pxyu = pyxu = pyyu = -pxxl;
	if (imp->imf_flags & ACTIVE_ONLY)
	  { /* just look at data within the active region */
		countactive(imp);
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
			looff = imp->imf_lomask + y * imp->imf_wid + xmin;
			omq = 1. - q;
			for (x = xmin, lom = looff; x < xmax; x++, lom++)
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
		fprintf(stderr,
		 " %7.3f %7.3f %7.3f %7.3f %7.2f %7.2f %7.2f %7.2f ",
		 xmi, ymi, xma, yma,
		 imp->imf_xsl, imp->imf_ysl, imp->imf_x00, imp->imf_y00);
		fprintf(stderr, " %4d %4d %4d %4d %4d %4d %4d %4d\n",
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
	size_t		imsiz, imsiz2;
	unsigned short	*looff, *lom;
	CALPOINT	*cz00, *cz01, *cz10, *cz11, *czx00;
	VSPLINE		*lspl;
	static int	saveimsiz;

	/* size of data16 buffer */
	imp->imf_imsize = imsiz = imp->imf_wid * imp->imf_ht;
	imsiz2 = imsiz * 2;
	/* reserve memory for floating-point image and weight values if needed*/
	if ((NULL == (imp->imf_locim)) || (saveimsiz != imsiz))
	  {
		if (NULL != (imp->imf_locim)) free((void *)(imp->imf_locim));
		if (NULL == (loim = (float *)calloc(imsiz2, sizeof (float))))
		  {
			fprintf(stderr,
			 "Error allocating memory for floating-point image\n");
			return EALLOC;
		  }
		imp->imf_locim = loim;
	  }
	else {
		loim = imp->imf_locim;
		for (ix = 0, lowt = loim; ix < imsiz2; ix++, lowt++)
			*lowt = 0;
	     }
	saveimsiz = imsiz;	imp->imf_locwt = lowt = loim + imsiz;
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
			looff = imp->imf_lomask + x;
		else	looff = (unsigned short *)(imp->imf_data16);
		for (y = 0, lom = looff; y < imp->imf_ht;
		 y++, lom += imp->imf_wid)
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
			q = ry - (double)jy;	omq = 1. - q;
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
				if ((imp->imf_flags & SCALED_MASK) &&
				 (*lom != 40))
					rimv *= 0.025 * ((double)(*lom));
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
			else obounds++;
		   } /* end of check for active pixel */
		} /* end of loop through Y */
	   } /* end of loop through X */
	if ((obounds > 0) && (imp->imf_flags & ANY_VERBOSE)) fprintf(stderr,
	 " %6d pixels mapped to coordinates outside the adjusted limits\n",
		obounds);
	return EOK;
}

static int normalize(IMWORK *imp)
{ /*	this renormalizes by dividing the weighted sums by the weights.
	The routine returns the number of non-found pixels */
	int	nearedge, nonz, noreps, onedge, x, y;
	float	*fpv, *fpw, *loim, *lowt;

	noreps = nearedge = onedge = 0;
	loim = imp->imf_locim;	lowt = imp->imf_locwt;
	for (fpv = loim, fpw = lowt, nonz = x = 0; x < imp->imf_wid; x++)
	   for (y = 0; y < imp->imf_ht; y++, fpv++, fpw++)
	      {
		if (*fpw > FUZZ)
		  {
			*fpv /= *fpw;	nonz++;
		  }
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
	if (imp->imf_flags & ANY_VERBOSE) /* code for this printout */
	  {
		fprintf(stderr,
		 " %8d pixels out of %8d gave nonzero intensities\n",
		 nonz, imp->imf_imsize);
		fprintf(stderr,
		 " %8d pixels out of %8d were missing interpolatable values\n",
			noreps, imp->imf_imsize);
		fprintf(stderr,
		 " of these, %8d were within three pixels of the edge\n",
			nearedge);
		fprintf(stderr,
		 " of these, %8d were within   one pixel  of the edge\n",
			onedge);
	  }
	return noreps;
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
		if (imp->imf_flags & VERY_VERBOSE) fprintf(stderr,
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
	if (imp->imf_flags & ANY_VERBOSE) fprintf(stderr,
		"Image has %12.0f counts; zmax = %7d at [%4d,%4d]\n",
			ecount, (int)zma, zmxx, (int)(imp->imf_ht - 1 - zmxy));
	return EOK;
}

static void sumsmvspat(FILE *fp, IMWORK *imp)
{ /*	This summarizes the run of 'smvspatial' */

	fprintf(fp, "An image is being spatially corrected.\n");
	fprintf(fp, "Spatial correction derived from file %s\n",
		imp->imf_splname);
	if (imp->imf_flags & ACTIVE_ONLY) fprintf(fp,
	 "Active-pixel mask derived from pre-read array\n");
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
#if 1
	if ( noreps ) {
		/* Suppress GCC 4.6 unused-but-set-variable warning. */
	}
#endif
	/* phew. Now we can make an image out of this array. */
	imerr = creat_undistor(imp);
	return imerr;
}

int smvspatial(void *imarr, int imwid, int imht, int cflags,
 char *splname, unsigned short *maskval)
{ /* Mainline for performing a spatial correction on an SMV image
	that is already in memory. Arguments:
	imarr	1-D array of data values (probably always unsigned shorts)
		The output will be written back to this pointer as well.
	cflags	flag value. Unlike earlier versions,
	 the only active bits in this flag now are associated with verbosity:
		bit 1	moderate amounts of verbosity (0x1)
		bit 2	higher verbosity (0x2)
		bit 3	even higher verbosity (not currently used) (0x4)
		bit 4	only use active area in correction (0x8)
		bit 5	use extrapolations beyond the correction region (0x10)
	splname	Name of the spatial-correction (*.uca) file.
	maskval pointer to the 16-bit active-pixel mask
	 (whether it's 0's and 1's or something more complex)
  */
	int		resu;
	static IMWORK	imfs;

	/* special call to clear all volatile memory in imfs */
	if ((imwid == -1) && (imht == -1))
	  {
		killimwork(&imfs, 07);	return EOK;
	  }
	imfs.imf_data16 = (unsigned short *)imarr;
	imfs.imf_splname = splname;
	/* temporary */
	cflags |= PARTIAL_VERBOSE; 
	if (NULL != maskval)
	  {
		imfs.imf_flags = cflags | ACTIVE_ONLY;
		fprintf(stderr, "Active-pixel mask input from memory\n");
	  }
	else imfs.imf_flags = cflags & ~ACTIVE_ONLY;
	imfs.imf_wid = imwid;	imfs.imf_ht = imht;
	imfs.imf_lomask = maskval;
	if (cflags & ANY_VERBOSE) sumsmvspat(stderr, &imfs);
	resu = img_conversion(&imfs);	/* perform spatial correction */
	return resu;
}
