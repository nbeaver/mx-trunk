/* smvspatial.c -- utility for manipulating 2-D detector images

	Copyright (c) 2002, Illinois Institute of Technology
	X-GEN: Crystallographic Data Processing Software
	author: Andrew J Howard, BCPS Department, IIT
 See the file "LICENSE" for information on usage and redistribution
	of this file, and for a DISCLAIMER OF ALL WARRANTIES.

Change record:
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
	/* return values from routines */
#define		EOK		0	/* normal exit status */
#define		EALLOC		12	/* memory allocation error (ENOMEM) */
#define		EREADERR	5	/* error in reading (EIO) */
#define		EOPENERR	2	/* error in opening a file (ENOENT) */
#define		EBADVALUE	22	/* bad arg to a function (EINVAL) */
	/* management of byte order */
#ifdef LITTLEND
#undef	LITTLEND	/* we'll re-define this for some systems below. */
#endif
#ifdef ultrix
#define	LITTLEND	1
#endif /* ultrix */
#ifdef __alpha
#define	LITTLEND	1
#endif /* __alpha */
#ifdef i386 
#define	LITTLEND	1
#endif /* i386 */
#ifdef linux
#include	<endian.h>
#if __BYTE_ORDER==__LITTLE_ENDIAN
#define	LITTLEND	1
#endif /* __BYTE_ORDER==__LITTLE_ENDIAN */
#endif /* linux */
	/* unambigous 32-bit integers */
#ifdef ANCIENT_16 /* xglong is always 32 bit */
#define xgulong	unsigned long
#define xglong	long
#else /* else of ANCIENT_16 */
#define	xgulong unsigned int
#define xglong	int
#endif /* ANCIENT_16 */
	/* floating and double parameters */
#define		FUZZ		(0.119e-6)	/* hardware FUZZ value */
#define		DFUZZ		(0.220e-15)	/* hardware double FUZZ value */
	/* flag bits for image conversions */
#define		INPUT_ROTCONV	0x7	/* bits for input rotation*/
#define		OUTPUT_ROTCONV	0x38	/* bits for input rotation*/
#define		OVERWRITE_ORIGINAL	0x40 /* overwrite original memory */
#define		RESCALE65K	0x80	/* rescale < 65K during interpolation*/
#define		PARTIAL_VERBOSE	0x100	/* moderate number of comments */
#define		VERY_VERBOSE	0x200	/* lots of comments */
#define		REDIMENSION	0x400	/* redimension detector image */
#define		OVERRIDE_COORDS	0x800	/* override coordinate conventions */
#define		IPR_XFASTEST	0x1	/* bit describing X faster than Y */
#define		IPR_STARTBOTTOM	0x2	/* bit describing starting @ bottom */
#define		IPR_STARTRIGHT	0x4	/* bit describing starting @ right */
#define		IPR_RAXISCOMPRESS	0x4000	/* R-Axis Compression type */
	/* system-level declarations, appropriately latched */
#ifndef _H_STANDARDS
extern int	access(const char *path, int mode);
#endif /* _H_STANDARDS */
#ifndef	__stdlib_h
extern void	*calloc(size_t count, size_t size);
extern void	free(void *ptr);
extern char	*getenv(const char *name);
#endif /* __stdlib_h */
extern int	creat(const char *path, mode_t mode);
#include	<string.h>
#define		MINRESERVED	800

/* structure type definitions */

 typedef struct { /* (X,Y) calibration point structure; totals   32 bytes:*/
		float	c_ptocx, c_ptocy, c_ctopx, c_ctopy;	/*    16 */
		float	c_qec, c_frfu[2];			/*    12 */
		u_char	c_ift, c_set, c_cset, c_ucrfu;		/*     4 */
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
		double		imf_totct;
		double		imf_x00, imf_xsl, imf_y00, imf_ysl;
		u_short		imf_zmax, imf_zmaxx, imf_zmaxy;
		float		*imf_locim, *imf_locwt;
		u_short		*imf_data16;
		char		*imf_splname;
		VSPLINE		*imf_spl;
		} IMWORK;

extern int smvspatial(void *ia, void **oa, int iw, int ih, int cfl, char *sn);

#ifdef LITTLEND /* We only need these routines on LITTLEND machines */
static short sswab(short sh)
{ /*	This converts a SIGNED short.  This is comparatively tricky. */
	u_short	ush;

	ush = (u_short)sh;
	if (ush & 0x80)	return -1 - ((short)
		(0xffff - (((ush & 0xff) << 8) | (ush >> 8))));
	else		return (((ush & 0x7f) << 8) | (ush >> 8));
}

static void lflcvt(u_char *z)
{ /*	Converts floating-point numbers between big-endian and little-endian */
	u_char	t;

	t = *(z + 3);	*(z + 3) = *z;		*z = t;
	t = *(z + 1);	*(z + 1) = *(z + 2);	*(z + 2) = t;
}
#endif /* LITTLEND */

static int splread(VSPLINE *lspl, FILE *fpi)
{ /* This reads a VSPLINE structure in from the stream *fpi.
	It returns the number of bytes read in */
	static size_t	ntor = 512;
	int		ncpt, nread;
#ifdef LITTLEND
	int		i;
	short		*sp;
	float		*flp;
	CALPOINT	*lca;
#endif

	if ((NULL == lspl) || (NULL == fpi)) return 0;
	if (1 != fread((void *)lspl, ntor, 1, fpi)) return 0;
#ifdef LITTLEND	/* floating-point conversions for little-endian machines */
	for (sp = &(lspl->v_detw); sp <= &(lspl->v_midr); sp++)
		*sp = sswab(*sp);
	for (flp = &(lspl->v_axc); flp <= &(lspl->v_y0px); flp++)
		lflcvt((void *)flp);
#endif
	ncpt = lspl->v_nrow * lspl->v_ncol;
	if (NULL != (lspl->v_cpt)) free((void *)(lspl->v_cpt));
	if (NULL == (lspl->v_cpt = (CALPOINT *)calloc(ncpt,
	 sizeof (CALPOINT))))
	  {
		(void)fclose(fpi);	return ntor;
	  }
	nread = fread((void *)(lspl->v_cpt), sizeof (CALPOINT), ncpt, fpi);
	(void)fclose(fpi);
#ifdef LITTLEND	/* floating-point conversions for little-endian machines */
	for (lca = lspl->v_cpt, i = 0; i < ncpt; i++, lca++)
	   {
		for (flp = &(lca->c_ptocx); flp <= &(lca->c_qec); flp++)
			lflcvt((void *)flp);
	   }
#endif
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
	xgulong		tct, zma;
	u_short		*ov, zmxx, zmxy;

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
				*ov = (u_short)(*fpv + 0.49999);
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

static int cspatial(IMWORK *imp)
{ /*	This converts an image to a form in which it's spatially corrected.
	It returns EOK if successful, various errors if not */
	int		noreps, sret;

	/* find the limits of x and y in cm */
	if (EOK != (sret = find_limits(imp))) return sret;
	interp_image(imp);
	/* re-generate the output values by dividing the weighted
	 count values by the weights */
	noreps = normalize(imp);
	/* extrapolate to take care of missing pixels */
	noreps = extrap_pix(imp, noreps);
	/* phew. Now we can make an image out of this array. */
	return creat_undistor(imp);
}

static void sumsmvspat(FILE *fp, IMWORK *imp)
{ /*	This summarizes the run of 'smvspatial' */

	fprintf(fp, "Images are being spatially corrected\n");
	fprintf(fp, "Spatial correction derived from file %s\n",
		imp->imf_splname);
	if (imp->imf_flags & OVERRIDE_COORDS) fprintf(fp,
 "Image coordinates will be output according to geometric convention %1d\n",
		(imp->imf_flags & 0x80) >> 5);
	if (imp->imf_flags & OVERWRITE_ORIGINAL) fprintf(fp,
	 "Image is being written back into the original memory\n");
}

static void rot16bitin(IMWORK *imp)
{ /* This rotates the image pointed to by imp->imf_data16 according to the
	conventions given in (imp->flags) & 07 into a standard ordering. */
	register int	i, j;
	int		deltax, deltay, hit, hitm1, inrot;
	int		startoff, wid, widm1;
	u_short		*iusp, *ousp, *iuspf, *ouspf;
	static u_short	*loim;

	inrot = imp->imf_flags & 0x7;
	if (inrot == 0) return;	/* no rotation required */
	if (NULL == (loim = calloc((size_t)imp->imf_imsize, sizeof (u_short))))
	  {
		fprintf(stderr, "Warning: first image rotation failed\n");
		return;
	  }
	wid = imp->imf_wid;	hit = imp->imf_ht;
	widm1 = wid - 1;	hitm1 = hit - 1;
	startoff = 0;	deltax = wid;	deltay = 1;
	if (inrot & IPR_XFASTEST)
	  {
		deltax = 1;	deltay = hit;
	  }
	if (inrot & IPR_STARTBOTTOM)
	  {
		deltay *= -1;	startoff += wid * hitm1;
	  }
	if (inrot & IPR_STARTRIGHT)
	  {
		deltax *= -1;	startoff += widm1;
	  }
	iusp = imp->imf_data16;	ousp = loim + startoff;
	for (i = 0; i < hit; i++, ousp += deltax, iusp += wid)
	   {
		for (j = 0, ouspf = ousp, iuspf = iusp;
		 j < wid; j++, iuspf++, ouspf += deltay) *ouspf = *iuspf;
	   }
	/* okay, that converts the image into the format that we'll use
	 internally, i.e. that we'll perform the VSPLINE operations on.
	 So we can now copy this back into the right place. */
	iusp = loim;	ousp = imp->imf_data16;
	for (i = 0; i < imp->imf_imsize; iusp++, ousp++) *ousp = *iusp;
	(void)free((void *)loim);	/* eliminate temporary memory */
}

static void rot16bitout(IMWORK *imp)
{ /* This rotates the image pointed to by imp->imf_data16
	from the internal standard position to one that works
	according to the conventions given in ((imp->flags) >> 3) & 07 */
	register int	i, j;
	int		deltax, deltay, hit, hitm1;
	int		outrot, startoff, wid, widm1;
	u_short		*iusp, *ousp, *iuspf, *ouspf;
	static u_short	*loim;

	outrot = ((imp->imf_flags) >> 3) & 0x7;
	if (outrot == 0) return;	/* no rotation required */
	if (NULL == (loim = calloc((size_t)imp->imf_imsize, sizeof (u_short))))
	  {
		fprintf(stderr, "Warning: second image rotation failed\n");
		return;
	  }
	wid = imp->imf_wid;	hit = imp->imf_ht;
	widm1 = wid - 1;	hitm1 = hit - 1;
	startoff = 0;	deltax = wid;	deltay = 1;
	if (outrot & IPR_XFASTEST)
	  {
		deltax = 1;	deltay = hit;
	  }
	if (outrot & IPR_STARTRIGHT)
	  {
		deltay *= -1;	startoff += wid * hitm1;
	  }
	if (outrot & IPR_STARTBOTTOM)
	  {
		deltax *= -1;	startoff += widm1;
	  }
	iusp = imp->imf_data16;	ousp = loim + startoff;
	for (i = 0; i < hit; i++, ousp += deltax, iusp += wid)
	   {
		for (j = 0, ouspf = ousp, iuspf = iusp;
		 j < wid; j++, iuspf++, ouspf += deltay) *ouspf = *iuspf;
	   }
	/* okay, that converts the image from the internal format to
	 the one that the caller wants. Copy it back. */
	iusp = loim;	ousp = imp->imf_data16;
	for (i = 0; i < imp->imf_imsize; iusp++, ousp++) *ousp = *iusp;
	(void)free((void *)loim);	/* eliminate temporary memory */
}

static int img_conversion(IMWORK *imp)
{ /*	This does the conversion on a single image.
	If it is called with a NULL argument, we use it to free up memory */
	int		imerr, retv;
	size_t		imsiz;
	static float	*locim;

	if (NULL == imp) /* use this as a way of freeing local memory */
	  {
		(void)smvspline(0, &imerr, NULL);
		if (NULL != locim) (void)free((void *)locim);
		return EOK;
	  }
	/* size of data16 buffer */
	imp->imf_imsize = imsiz = imp->imf_wid * imp->imf_ht;
	/* reserve memory for floating-point image and weight values */
	if (NULL == (locim = (float *)calloc((size_t)
	 (2 * imp->imf_imsize), sizeof (float)))) return EALLOC;
	imp->imf_locim = locim;	imp->imf_locwt = locim + imp->imf_imsize;
	rot16bitin(imp);	/* rotate image into standard ordering */
	if (EOK != (retv = cspatial(imp)))
		fprintf(stdout, "Failure %d in spatial correction\n", retv);
	rot16bitout(imp);	/* rotate back to initial ordering */
	return retv;
}

int smvspatial(void *imarr, void **outarr, int imwid, int imhit, int cflags,
 char *splname)
{ /* Mainline for performing a spatial correction on an SMV image
	that is already in memory. Arguments:
	imarr	1-D array of data values (probably always u_shorts)
		as derived from a Lavender image_data structure.
	imwid	fast-varying detector dimension ("width")
	imhit	slow-varying detector dimension ("height")
	cflags	flag value. Bits 1-3 provide the
		input coordinate rotation value:
	splname	name of the calibration spline file from X-GEN
 	dimension varying	read begins
 	fastest			from
 0	Y			top left	PCS (little-endian)
 1	X			top left	Mar, SDMS
 2	Y			bottom left
 3	X			bottom left	MacScience, SAXII
 4	Y			top right
 5	X			top right	PCS (big-endian)
 6	Y			bottom right
 7	X			bottom right	R-Axis, Fuji
	In each case the definition of left and right assumes that the
	view is OF the sample FROM the detector, not the other 'way around.
	In the alternative view (from the sample toward the detector)
	<val> = 0 corresponds to Y fastest, reading from top right,
	<val> = 5 corresponds to X fastest, reading from top left, etc.
	Higher order bits are used for other purposes:
	Bits 4-6: output coordinate convention, as above.
	Bit 7: if on, overlay the data on the original pointer rather than
		creating a new one.
	Bit 8: rescale intensity values to < 65000 during interpolation
	Bit 9: moderately verbose diagnostic output
	Bit 10: more verbose diagnostics
	Bit 11: signal that we need to redimension the image
	Bit 12: override the coordinate conventions,
		i.e. the input coordinate conventions (bits 1-3) aren't
		equal to the output coordinate conventions (bits 4-6)
	Bit 15: R-Axis compression type (not currently used with SMV images)
  */
	int		resu;
	static IMWORK	imfs;

	imfs.imf_flags = PARTIAL_VERBOSE | RESCALE65K;
	imfs.imf_wid = imwid;	imfs.imf_ht = imhit;
	imfs.imf_data16 = (u_short *)imarr;
	imfs.imf_wid = imfs.imf_ht = 0;
	imfs.imf_splname = splname;
	sumsmvspat(stdout, &imfs);
	/* in this version, we process this one particular image */
	resu = img_conversion(&imfs);
	(void)img_conversion(NULL);
	return resu;
}

/* end of smvspatial.c */
