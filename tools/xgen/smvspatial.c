/* smvspatial.c -- utility for manipulating 2-D detector images

	Copyright (c) 2002, Illinois Institute of Technology
	X-GEN: Crystallographic Data Processing Software
	author: Andrew J Howard, BCPS Department, IIT
 See the file "LICENSE" for information on usage and redistribution
	of this file, and for a DISCLAIMER OF ALL WARRANTIES.

Change record:
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
#include	<math.h>
#include	<sys/types.h>
#include	<sys/stat.h>

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
	/* system-level declarations, appropriately latched */
#ifndef	__stdlib_h
extern void	*calloc(size_t count, size_t size);
extern void	free(void *ptr);
#endif /* __stdlib_h */
#include	<string.h>

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
		unsigned short	imf_zmax, imf_zmaxx, imf_zmaxy;
		float		*imf_locim, *imf_locwt;
		unsigned short	*imf_data16;
		char		*imf_splname;
		VSPLINE		*imf_spl;
		} IMWORK;

extern int smvspatial(void *ia, int iw, int ih, int cfl, char *sn);

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

static int splread(VSPLINE *lspl, FILE *fpi)
{ /* This reads a VSPLINE structure in from the stream *fpi.
	It returns the number of bytes read in */
	static size_t	ntor = 512;
	int		i, islittle, ncpt, nread;
	short		*sp;
	float		*flp;
	CALPOINT	*lca;

	if ((NULL == lspl) || (NULL == fpi)) return 0;
	islittle = checkorder();	/*is this is a little-endian machine?*/
	if (1 != fread((void *)lspl, ntor, 1, fpi)) return 0;
	if (islittle) /* byte swap shorts and flip bytes & words in floats */
	  {
		for (sp = &(lspl->v_detw); sp <= &(lspl->v_midr); sp++)
			*sp = sswab(*sp);
		for (flp = &(lspl->v_axc); flp <= &(lspl->v_y0px); flp++)
			lflcvt((void *)flp);
	  }
	ncpt = lspl->v_nrow * lspl->v_ncol;
	if (NULL != (lspl->v_cpt)) free((void *)(lspl->v_cpt));
	if (NULL == (lspl->v_cpt = (CALPOINT *)calloc(ncpt,
	 sizeof (CALPOINT))))
	  {
		(void)fclose(fpi);	return ntor;
	  }
	nread = fread((void *)(lspl->v_cpt), sizeof (CALPOINT), ncpt, fpi);
	(void)fclose(fpi);
	if (islittle) /* byte and word swaps again */
	  {
		for (lca = lspl->v_cpt, i = 0; i < ncpt; i++, lca++)
		   {
			for (flp = &(lca->c_ptocx); flp <= &(lca->c_qec);
			 flp++) lflcvt((void *)flp);
		   }
	  }
	return ntor + nread * sizeof (CALPOINT);
}

static VSPLINE *smvspline(int opno, int *errtp, char *splname)
{ /* This manipulates a VSPLINE structure according to the value of opno.
	If opno == 0, we free the memory for lspl and return NULL.
	If opno == 1, we simply create a structure and return a pointer to it.
	If opno == 3, we return a pointer to a VSPLINE structure,
		creating it only if necessary.
	If opno == 5, we start over, i.e. we populate a VSPLINE structure
	from a diskfile, and then return it.
	Note that there is no provision for writing the VSPLINE here.
	The filename argument `splname' is only relevant for opno =5
	*/

	int		nread, ntoread;
	static VSPLINE	*lspl;
	static FILE	*fspl;

	*errtp = EOK;
	if (opno == 0)
	  {
		if (NULL != lspl)
		  {
			(void)free((void *)(lspl->v_cpt));
			(void)free((void *)lspl);
		  }
		return NULL;
	  }
	if (NULL == lspl)
	  {	
		if (NULL == (lspl = (VSPLINE *)calloc(1, sizeof (VSPLINE))))
			return NULL;
	  }
	if (opno == 3) return lspl;	/* in this case, we just receive */
	if (opno == 5)
	  {
		(void)free((void *)(lspl->v_cpt));
		if (NULL == (fspl = fopen(splname, "r")))
		  {
			*errtp = EOPENERR;	return NULL;
		  }
		nread = splread((void *)lspl, fspl);
		ntoread = 512 + lspl->v_nrow * lspl->v_ncol * sizeof (CALPOINT);
		if (nread < ntoread)
		  {
			fprintf(stderr,
	 "Warning: expected to read %d bytes from %s; actually read %d\n",
				ntoread, splname, nread);
			*errtp = EREADERR;	return NULL;
		  }
		(void)fclose(fspl);	fspl = NULL;
		lspl->v_maxx = lspl->v_detw + 10;
		lspl->v_maxy = lspl->v_deth + 10;
		return lspl;
	  }
	return (*errtp != 0) ? NULL : lspl;
}

static int pix2cm(float xp, float yp, float *xcm, float *ycm)
{ /*	This returns the x,y coordinates in cm of a position
	specified in pixel coordinates.
	Return values:
		 1	: cm position found; (x,y) inside active area
		 2	: cm position found; (x,y) outside active area
				but still close to detector
		-1	: cm position found; (x,y) outside active area
				and a long way from the detector
		 0	: cm position couldn't be determined. */
	int		retv, usex, usey;
	VSPLINE		*lspl;
	CALPOINT	*cz00, *cz01, *cz10, *cz11;
	static int	ix, iy;
	static double	p, q, rx, ry;
	float		dx, dy;

	if (NULL == (lspl = smvspline(3, &retv, NULL))) return 0;
	dx = xp - lspl->v_x0px;	dy = yp - lspl->v_y0px;
	rx = dx / lspl->v_xrpx;	ry = dy / lspl->v_yrpx;
	ix = (rx < 0. ? 0 : (rx < lspl->v_ncol - 1 ?
		rx : lspl->v_ncol - 2));
	iy = (ry < 0. ? 0 : (ry < lspl->v_nrow - 1 ?
		ry : lspl->v_nrow - 2));
	if (lspl->v_nreg == 4)
	  {
		if (ix == lspl->v_ncol / 2 - 1) ix--;
		else if (ix == lspl->v_ncol / 2) ix++;
		if (iy == lspl->v_nrow / 2 - 1) iy--;
		else if (iy == lspl->v_nrow / 2) iy++;
	  }
	cz00 = lspl->v_cpt + iy * lspl->v_ncol + ix;
	cz01 = cz00 + 1; cz10 = cz00 + lspl->v_ncol; cz11 = cz10 + 1;
	p = rx - (double)ix;	q = ry - (double)iy;
	*xcm = (1.-p) * (1.-q) * cz00->c_ctopx + (1.-p) * q * cz10->c_ctopx +
		   p  * (1.-q) * cz01->c_ctopx +     p  * q * cz11->c_ctopx;
	*ycm = (1.-p) * (1.-q) * cz00->c_ctopy + (1.-p) * q * cz10->c_ctopy +
		   p  * (1.-q) * cz01->c_ctopy +     p  * q * cz11->c_ctopy;
	usex = xp + 0.49999;	usey = yp + 0.499999;
	if ((usex < -10) || (usex > lspl->v_maxx) || (usey < -10) ||
		(usey > lspl->v_maxy)) return -1;
	if ((usex < 0) || (usex >= lspl->v_detw) ||
	 (usey < 0) || (usey > lspl->v_deth)) return 2;
	return 1;
}

static int find_limits(IMWORK *imp)
{ /*	This determines the limits of x and y in centimeters and populates
	the x and y slope and intercept values accordingly */
	int	x, y;
	float	xc, yc;
	double	xma, xmi, yma, ymi;

	yma = xma = -1.e9;	ymi = xmi = -xma;
	for (y = 0; y < (imp->imf_spl)->v_deth; y++)
		for (x = 0; x < (imp->imf_spl)->v_detw; x++)
		   {
			if (-1 != pix2cm((float)x, (float)y, &xc, &yc))
			  {
				if (xc < xmi) xmi = xc;
				if (xc > xma) xma = xc;
				if (yc < ymi) ymi = yc;
				if (yc > yma) yma = yc;
			  }
		   }
	if ((yma < -0.9e9) || (xma < -0.9e9) ||
	 (xmi > 0.9e9) || (ymi > 0.9e9)) return EBADVALUE;
	if (xma < xmi + DFUZZ) return EBADVALUE;
	if (yma < ymi + DFUZZ) return EBADVALUE;
	imp->imf_xsl = ((double)(imp->imf_wid - 1)) / (xma - xmi);
	imp->imf_x00 = -xmi * imp->imf_xsl;
	imp->imf_ysl = ((double)(imp->imf_ht - 1)) / (ymi - yma);
	imp->imf_y00 = -yma * imp->imf_ysl;
	return EOK;
}

static void interp_image(IMWORK *imp)
{ /*	Here is where the actual interpolation takes place. */
	int	imv, ix, iy, off0, off1, off2, off3, x, y;
	double	px, py, rimv, wt0, wt1, wt2, wt3;
	float	xc, yc, *loim, *lowt;

	loim = imp->imf_locim;	lowt = imp->imf_locwt;
	for (x = 0; x < (imp->imf_spl)->v_detw; x++) /* convert pix to cm */
	  for (y = 0; y < (imp->imf_spl)->v_deth; y++)
	   if (-1 != pix2cm((float)x, (float)y, &xc, &yc))
	     {	/* rescale coordinates into the proper dimensions */
		xc = imp->imf_xsl * xc + imp->imf_x00;
		yc = imp->imf_ysl * yc + imp->imf_y00;
		/* add a fraction of the input value into the sum for each
		 of the four corners of the box in which the pixel rests.
		 Be careful at the edges */
		if ((-1 <= xc) && (xc < imp->imf_wid) &&
		 (-1 <= yc) && (yc < imp->imf_ht))
		  {
			ix = xc;	iy = yc;
			px = xc - (double)ix;	if (xc < 0) ix--;
			py = yc - (double)iy;	if (yc < 0) iy--;
			off0 = ix * imp->imf_ht + iy;
			off1 = off0 + 1;
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
	     } /* end of "if (-1 != pix2cm" clause and "for (ix" loop */
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
{ /*	This puts the floating-point image back into integers */
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
		fprintf(stdout,
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
	imp->imf_zmax = zma;	imp->imf_zmaxx = zmxx;	imp->imf_zmaxy = zmxy;
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
	If it is called with a NULL argument, we use it to free up memory.
	It returns EOK if successful, various errors if not */
	int		imerr, noreps;
	size_t		imsiz;
	static float	*locim;

	if (NULL == imp) /* use this as a way of freeing local memory */
	  {
		(void)smvspline(0, &imerr, NULL); /* elim spline memory */
		if (NULL != locim) (void)free((void *)locim);
		return EOK;
	  }
	/* size of data16 buffer */
	imp->imf_imsize = imsiz = imp->imf_wid * imp->imf_ht;
	/* reserve memory for floating-point image and weight values */
	if (NULL == (locim = (float *)calloc((size_t)
	 (2 * imp->imf_imsize), sizeof (float)))) return EALLOC;
	imp->imf_locim = locim;	imp->imf_locwt = locim + imp->imf_imsize;
	/* read in the spatial-correction information */
	if (NULL == (imp->imf_spl = smvspline(3, &imerr, imp->imf_splname)))
		return imerr;
	/* find the limits of x and y in cm */
	if (EOK != (imerr = find_limits(imp))) return imerr;
	interp_image(imp);	/* perform spatial-correction interpolation */
	/* re-generate the output values by dividing the weighted
	 count values by the weights */
	noreps = normalize(imp);
	/* extrapolate to take care of missing pixels */
	noreps = extrap_pix(imp, noreps);
	/* phew. Now we can make an image out of this array. */
	return creat_undistor(imp);
}

int smvspatial(void *imarr, int imwid, int imhit, int cflags, char *splname)
{ /* Mainline for performing a spatial correction on an SMV image
	that is already in memory. Arguments:
	imarr	1-D array of data values (probably always unsigned shorts)
		as derived from a Lavender image_data structure.
		The output will be written back to this pointer as well.
	imwid	fast-varying detector dimension ("width")
	imhit	slow-varying detector dimension ("height")
	cflags	flag value. Unlike earlier versions,
	 the only active bits in this flag now are associated with verbosity:
		bit 1	moderate amounts of verbosity
		bit 2	higher verbosity
		bit 3	even higher verbosity
  */
	int		resu;
	static IMWORK	imfs;

	imfs.imf_flags = cflags;
	imfs.imf_wid = imwid;	imfs.imf_ht = imhit;
	imfs.imf_data16 = (unsigned short *)imarr;
	imfs.imf_splname = splname;
	if (cflags & ANY_VERBOSE) sumsmvspat(stdout, &imfs);
	resu = img_conversion(&imfs);	/* perform spatial correction */
	(void)img_conversion(NULL);
	return resu;
}

/* end of smvspatial.c */
