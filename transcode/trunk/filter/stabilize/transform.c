/*
 *  transform.c
 *
 *  Copyright (C) Georg Martius - June 2007
 *
 *  This file is part of transcode, a video stream processing tool
 *      
 *  transcode is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *   
 *  transcode is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *   
 *  You should have received a copy of the GNU General Public License
 *  along with GNU Make; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA. 
 *
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#include "transcode.h"
#include "libtc/libtc.h"
#include <stdlib.h>
#include <math.h>
#include "transform.h"

/* normal round function */
int myround(double x)
{
    double x_;
    x_ = floor(x);
    if (x - x_ >= 0.5)
        return ((int)x_) + 1;
    else
        return (int)x_;
}

/***********************************************************************
 * helper functions to create and operate with transforms.
 * all functions are non-destructive
 */

/* create an initialized transform*/
Transform new_transform(double x, double y, double alpha, int extra)
{ 
    Transform t;
    t.x     = x;
    t.y     = y;
    t.alpha = alpha;
    t.extra = extra;
    return t;
}

/* create a zero initialized transform*/
Transform null_transform(void)
{ 
    return new_transform(0, 0, 0, 0);
}

/* adds two transforms */
Transform add_transforms(const Transform* t1, const Transform* t2)
{
    Transform t;
    t.x     = t1->x + t2->x;
    t.y     = t1->y + t2->y;
    t.alpha = t1->alpha + t2->alpha;
    t.extra = 0;
    return t;
}

/* like add_transform but with non-pointer signature */
Transform add_transforms_(const Transform t1, const Transform t2)
{
    return add_transforms(&t1, &t2);
}

/* subtracts two transforms */
Transform sub_transforms(const Transform* t1, const Transform* t2)
{
    Transform t;
    t.x     = t1->x - t2->x;
    t.y     = t1->y - t2->y;
    t.alpha = t1->alpha - t2->alpha;
    t.extra = 0;
    return t;
}

/* multiplies a transforms with a scalar */
Transform mult_transform(const Transform* t1, double f)
{
    Transform t;
    t.x     = t1->x * f;
    t.y     = t1->y * f;
    t.alpha = t1->alpha * f;
    t.extra = 0;
    return t;
}

/* like mult_transform but with non-pointer signature */
Transform mult_transform_(const Transform t1, double f)
{
    return mult_transform(&t1,f);
}

/* compares a transform with respect to x (for sort function) */
int cmp_trans_x(const void *t1, const void* t2)
{
    double a = ((Transform*)t1)->x;
    double b = ((Transform*)t2)->x;
    return a < b ? -1 : ( a > b ? 1 : 0 );
}

/* compares a transform with respect to y (for sort function) */
int cmp_trans_y(const void *t1, const void* t2)
{
    double a = ((Transform*)t1)->y;
    double b = ((Transform*)t2)->y;
    return a < b ? -1 : ( a > b ? 1: 0 );
}

/* static int cmp_trans_alpha(const void *t1, const void* t2){ */
/*   double a = ((Transform*)t1)->alpha; */
/*   double b = ((Transform*)t2)->alpha; */
/*   return a < b ? -1 : ( a > b ? 1 : 0 ); */
/* } */


/* compares two double values (for sort function)*/
int cmp_double(const void *t1, const void* t2)
{
    double a = *((double*)t1);
    double b = *((double*)t2);
    return a < b ? -1 : ( a > b ? 1 : 0 );
}

/**
 * median_xy_transform: calulcates the median of an array 
 * of transforms, considering only x and y
 *
 * Parameters:
 *    transforms: array of transforms.
 *           len: length  of array
 * Return value:
 *     A new transform with x and y beeing the median of 
 *     all transforms. alpha and other fields are 0.
 * Preconditions:
 *     len>0
 * Side effects:
 *     None
 */
Transform median_xy_transform(const Transform* transforms, int len)
{
    Transform* ts = NEW(Transform,len);
    Transform t;
    memcpy(ts,transforms, sizeof(Transform)*len ); 
    int half = len/2;
    qsort(ts, len, sizeof(Transform), cmp_trans_x);
    t.x = len % 2 == 0 ? ts[half].x : (ts[half].x + ts[half+1].x)/2;
    qsort(ts, len, sizeof(Transform), cmp_trans_y);
    t.y = len % 2 == 0 ? ts[half].y : (ts[half].y + ts[half+1].y)/2;
    t.alpha = 0;
    t.extra = 0;
    tc_free(ts);
    return t;
}

/**
 * cleanmean_xy_transform: calulcates the cleaned mean of an array 
 * of transforms, considering only x and y
 *
 * Parameters:
 *    transforms: array of transforms.
 *           len: length  of array
 * Return value:
 *     A new transform with x and y beeing the cleaned mean 
 *     (meaning upper and lower pentile are removed) of 
 *     all transforms. alpha and other fields are 0.
 * Preconditions:
 *     len>0
 * Side effects:
 *     None
 */
Transform cleanmean_xy_transform(const Transform* transforms, int len)
{
    Transform* ts = NEW(Transform,len);
    Transform t = null_transform();
    int i, cut = len / 5;
    memcpy(ts, transforms, sizeof(Transform) * len); 
    qsort(ts,len, sizeof(Transform), cmp_trans_x);
    for(i = cut; i < len - cut; i++){ // all but cutted
        t.x += ts[i].x;
    }
    qsort(ts, len, sizeof(Transform), cmp_trans_y);
    for(i = cut; i < len - cut; i++){ // all but cutted
        t.y += ts[i].y;
    }
    tc_free(ts);
    return mult_transform(&t, 1.0 / (len - (2.0 * cut)));
}


/**
 * media: median of a double array
 *
 * Parameters:
 *            ds: array of values
 *           len: length  of array
 * Return value:
 *     the median value of the array
 * Preconditions: len>0
 * Side effects:  ds will be sorted!
 */
double median(double* ds, int len)
{
    int half=len/2;
    qsort(ds,len, sizeof(double), cmp_double);
    return len % 2 == 0 ? ds[half] : (ds[half] + ds[half+1])/2;
}

/**
 * mean: mean of a double array 
 *
 * Parameters:
 *            ds: array of values
 *           len: length  of array
 * Return value: the mean value of the array
 * Preconditions: len>0
 * Side effects:  None
 */
double mean(const double* ds, int len)
{
    double sum=0;
    int i = 0;
    for(i = 0; i < len; i++)
        sum += ds[i];
    return sum / len;
}

/**
 * cleanmean: mean with cutted upper and lower pentile 
 *
 * Parameters:
 *            ds: array of values
 *           len: length  of array
 * Return value:
 *     the mean value of the array without the upper 
 *     and lower pentile (20% each)
 * Preconditions: len>0
 * Side effects:  ds will be sorted!
 */
double cleanmean(double* ds, int len)
{
    int cut    = len / 5;
    double sum = 0;
    int i      = 0;
    qsort(ds, len, sizeof(double), cmp_double);
    for(i = cut; i < len - cut; i++){ // all but first and last
        sum += ds[i];
    }
    return sum / (len - (2.0 * cut));
}


/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
