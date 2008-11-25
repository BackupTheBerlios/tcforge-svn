/*
 *  filter_transform.c
 *
 *  Copyright (C) Georg Martius - June 2007
 *   georg dot martius at web dot de  
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
 * Typical call:
 * transcode -J transform="crop=0" -i inp.m2v -y xdiv,pcm inp_stab.avi
*/

#define MOD_NAME    "filter_transform.so"
#define MOD_VERSION "v0.4.2 (2008-09-18)"
#define MOD_CAP     "transforms each frame according to transformations\n\
 given in an input file (e.g. translation, rotate) see also filter stabilize"
#define MOD_AUTHOR  "Georg Martius"

#define MOD_FEATURES \
    TC_MODULE_FEATURE_FILTER|TC_MODULE_FEATURE_VIDEO
#define MOD_FLAGS \
    TC_MODULE_FLAG_RECONFIGURABLE
  
#include <math.h>

#include "src/transcode.h"
#include "src/filter.h"
#include "libtc/libtc.h"
#include "libtc/tccodecs.h"
#include "libtcutil/optstr.h"
#include "libtcmodule/tcmodule-plugin.h"
#include "transform.h"

#define DEFAULT_TRANS_FILE_NAME     "transforms.dat"

#define PIXEL(img, x, y, w, h, def) ((x) < 0 || (y) < 0) ? def       \
    : (((x) >=w || (y) >= h) ? def : img[(x) + (y) * w]) 

typedef struct {
    size_t framesize_src;  // size of frame buffer in bytes (src)
    size_t framesize_dest; // size of frame buffer in bytes (dest)
    unsigned char* src;  // copy of the current frame buffer
    unsigned char* dest; // pointer to the current frame buffer (to overwrite)

    vob_t* vob;          // pointer to information structure
    int width_src, height_src;
    int width_dest, height_dest;
    Transform* trans;    // array of transformations
    int current_trans;   // index to current transformation
    int trans_len;       // length of trans array
 
    /* Options */
    int maxshift;        // maximum number of pixels we will shift
    double maxangle;     // maximum angle in rad

    /* whether to consider transforms as relative (to previous frame) 
     * or absolute transforms  
     */
    int relative;  
    /* number of frames (forward and backward) 
     * to use for smoothing transforms */
    int smoothing;  
    int crop;       // 1: black bg, 0: keep border from last frame(s)
    int invert;     // 1: invert transforms, 0: nothing
    /* constants */
    /* threshhold below which no rotation is performed */
    double rotation_threshhold; 
    
    char input[TC_BUF_LINE];
    FILE* f;

    char conf_str[TC_BUF_MIN];
} TransformData;

static const char transform_help[] = ""
    "Overview\n"
    "    Reads a file with transform information for each frame\n"
    "     and applies them. See also stabilize.\n" 
    "Options\n"
    "    'input'     path to the file used to read the transforms\n\
                (def: inputfile.stab)\n"
    "    'maxshift'  maximal number of pixels to translate image\n\
                (def: -1 no limit)\n"
    "    'maxangle'  maximal angle in rad to rotate image (def: -1 no limit)\n"
    "    'crop'      0: keep border (def), 1: black backgr\n"
    "    'invert'    1: invert transforms(def: 0)\n"
    "    'relative'  consider transforms as 0: absolute, 1: relative (def)\n"
    "    'smoothing' number of frames*2 + 1 used for lowpass filtering \n"
    "                used for stabilizing (def: 10)\n"
    "    'help'      print this help message\n";

/* forward deklarations, please look below for documentation*/
void interpolate(unsigned char *rv, float x, float y, 
        		 unsigned char* img, int width, int height, unsigned char def);
int transformRGB(TransformData* td);
int transformYUV(TransformData* td);
int read_input_file(TransformData* td);
int preprocess_transforms(TransformData* td);


/** 
 * interpolate: quatratic interpolation function. 
 *
 * Parameters:
 *             rv: destination pixel (call by reference)
 *            x,y: the source coordinates in the image img. Note this 
 *                 are real-value coordinates, that's why we interpolate
 *            img: source image
 *   width,height: dimension of image
 *            def: default value if coordinates are out of range
 * Return value:  None
 */
void interpolate(unsigned char *rv, float x, float y, 
		 unsigned char* img, int width, int height, unsigned char def)
{
    if (x < - 1 || x > width || y < -1 || y > height) {
        *rv = def;    
    } else {
        int x_c = (int)ceilf(x);
        int x_f = (int)floorf(x);
        int y_c = (int)ceilf(y);
        int y_f = (int)floorf(y);
        short v1 = PIXEL(img, x_c, y_c, width, height, def);
        short v2 = PIXEL(img, x_c, y_f, width, height, def);
        short v3 = PIXEL(img, x_f, y_c, width, height, def);
        short v4 = PIXEL(img, x_f, y_f, width, height, def);
        float f1 = 1 - sqrt(fabs(x_c - x) * fabs(y_c - y));
        float f2 = 1 - sqrt(fabs(x_c - x) * fabs(y_f - y));
        float f3 = 1 - sqrt(fabs(x_f - x) * fabs(y_c - y));
        float f4 = 1 - sqrt(fabs(x_f - x) * fabs(y_f - y));
        float s  = (v1*f1 + v2*f2 + v3*f3+ v4*f4)/(f1 + f2 + f3 + f4);
        *rv = (unsigned char)s;
    }
}

/** 
 * transformRGB: applies current transformation to frame
 *   Todo: implement
 * Parameters:
 *         td: private data structure of this filter
 * Return value: 
 *         0 for failture, 1 for success
 * Preconditions:
 *  The frame must be in RGB format
 */
int transformRGB(TransformData* td)
{
    tc_log_warn(MOD_NAME, "Not Supported yet!"); 
    return 0;
}

/** 
 * transformYUV: applies current transformation to frame
 *
 * Parameters:
 *         td: private data structure of this filter
 * Return value: 
 *         0 for failture, 1 for success
 * Preconditions:
 *  The frame must be in YUV format
 */
int transformYUV(TransformData* td)
{
    Transform t;
    int x = 0, y = 0;
    unsigned char *Y_1, *Y_2, *Cb_1, *Cb_2, *Cr_1, *Cr_2;
    t = td->trans[td->current_trans];
  
    Y_1  = td->src;  
    Y_2  = td->dest;  
    Cb_1 = td->src + td->width_src * td->height_src;
    Cb_2 = td->dest + td->width_dest * td->height_dest;
    Cr_1 = td->src + 5*td->width_src * td->height_src/4;
    Cr_2 = td->dest + 5*td->width_dest * td->height_dest/4;
    float c_s_x = td->width_src/2.0;
    float c_s_y = td->height_src/2.0;
    float c_d_x = td->width_dest/2.0;
    float c_d_y = td->height_dest/2.0;    

    /* for each pixel in the destination image we calc the source
     * coordinate and make an interpolation: 
     *      p_d = c_d + M(p_s - c_s) + t 
     * where p are the points, c the center coordinate, 
     *  _s source and _d destination, 
     *  t the translation, and M the rotation matrix
     *      p_s = M^{-1}(p_d - c_d - t) + c_s
     */
    /* Luminance channel */
    if (fabs(t.alpha) > td->rotation_threshhold) {
        for (x = 0; x < td->width_dest; x++) {
            for (y = 0; y < td->height_dest; y++) {
                float x_d1 = (x - c_d_x);
                float y_d1 = (y - c_d_y);
                float x_s  =  cos(-t.alpha) * x_d1 
                    + sin(-t.alpha) * y_d1 + c_s_x -t.x;
                float y_s  = -sin(-t.alpha) * x_d1 
                    + cos(-t.alpha) * y_d1 + c_s_y -t.y;
                unsigned char* dest = &Y_2[x + y * td->width_dest];
                interpolate(dest, x_s, y_s, Y_1, 
                            td->width_src, td->height_src, 
                            td->crop ? 16 : *dest);
            }
        }
     }else { 
        /* no rotation, just translation 
         *(also no interpolation, since no size change (so far) 
         */
        int round_tx = myround(t.x);
        int round_ty = myround(t.y);
        for (x = 0; x < td->width_dest; x++) {
            for (y = 0; y < td->height_dest; y++) {
                short p = PIXEL(Y_1, x - round_tx, y - round_ty, 
                                td->width_src, td->height_src, -1);
                if (p == -1) {
                    if (td->crop == 1)
                        Y_2[x + y * td->width_dest] = 16;
                } else {
                    Y_2[x + y * td->width_dest] = (unsigned char)p;
                }
            }
        }
    }

    /* Color channels */
    int ws2 = td->width_src/2;
    int wd2 = td->width_dest/2;
    int hs2 = td->height_src/2;
    int hd2 = td->height_dest/2;
    if (fabs(t.alpha) > td->rotation_threshhold) {
        for (x = 0; x < wd2; x++) {
            for (y = 0; y < hd2; y++) {
                float x_d1 = x - (c_d_x)/2;
                float y_d1 = y - (c_d_y)/2;
                float x_s  =  cos(-t.alpha) * x_d1 
                    + sin(-t.alpha) * y_d1 + (c_s_x -t.x)/2;
                float y_s  = -sin(-t.alpha) * x_d1 
                    + cos(-t.alpha) * y_d1 + (c_s_y -t.y)/2;
                unsigned char* dest = &Cr_2[x + y * wd2];
                interpolate(dest, x_s, y_s, Cr_1, ws2, hs2, 
                            td->crop ? 128 : *dest);
                dest = &Cb_2[x + y * wd2];
                interpolate(dest, x_s, y_s, Cb_1, ws2, hs2, 
                            td->crop ? 128 : *dest);      	
            }
        }
    } else { // no rotation, no interpolation, just translation 
        int round_tx2 = myround(t.x/2.0);
        int round_ty2 = myround(t.y/2.0);        
        for (x = 0; x < wd2; x++) {
            for (y = 0; y < hd2; y++) {
                short cr = PIXEL(Cr_1, x - round_tx2, y - round_ty2, 
                                 wd2, hd2, -1);
                short cb = PIXEL(Cb_1, x - round_tx2, y - round_ty2, 
                                 wd2, hd2, -1);
                if (cr == -1) {
                    if (td->crop==1) { 
                        Cr_2[x + y * wd2] = 128;
                        Cb_2[x + y * wd2] = 128;
                    }
                } else {
                    Cr_2[x + y * wd2] = (unsigned char)cr;
                    Cb_2[x + y * wd2] = (unsigned char)cb;
                }
            }
        }
    }
    return 1;
}


/** 
 * read_input_file: read transforms file
 *  The format is as follows:
 *   Lines with # at the beginning are comments and will be ignored
 *   Data lines have 5 columns seperated by space or tab containing
 *   time, x-translation, y-translation, alpha-rotation, extra
 *   where time and extra are integers 
 *   and the latter is unused at the moment
 *
 * Parameters:
 *         td: private data structure of this filter
 * Return value: 
 *         number of transforms read
 * Preconditions: td->f is opened
 */
int read_input_file(TransformData* td)
{
    char l[TC_BUF_MAX];
    int s = 0;
    int i = 0;
    int ti; // time (ignored)
    Transform t;

    while (fgets(l, sizeof(l), td->f)) {
        if (l[0] == '#')
            continue;    //  ignore comments
        if (strlen(l) == 0)
            continue; //  ignore empty lines
    
        if (sscanf(l, "%i %lf %lf %lf %i", &ti, &t.x, &t.y, &t.alpha, 
                  &t.extra) != 5) {
            tc_log_error(MOD_NAME, "Cannot parse line: %s", l);
            return 0;
        }
    
        if (i>=s) { // resize transform array
            if (s == 0)
                s = 256;
            else
                s*=2;
            /* tc_log_info(MOD_NAME, "resize: %i\n", s); */
            td->trans = tc_realloc(td->trans, sizeof(Transform)* s);
            if (!td->trans) {
                tc_log_error(MOD_NAME, "Cannot allocate memory"
                                       " for transformations: %i\n", s);
                return 0;
            }
        }
        td->trans[i] = t;
        i++;
    }
    td->trans_len = i;

    return i;
}

/**
 * preprocess_transforms: does smoothing, relative to absolute conversion,
 *  and cropping of too large transforms.
 *  This is actually the core algorithm for canceling the jiggle in the 
 *  movie. We perform a low-pass filter in terms of transformation size.
 *  This enables still camera movement, but in a smooth fasion.
 *
 * Parameters:
 *            td: tranform private data structure
 * Return value:
 *     1 for success and 0 for failture
 * Preconditions:
 *     None
 * Side effects:
 *     td->trans will be modified
 */
int preprocess_transforms(TransformData* td)
{
    Transform* ts = td->trans;
    int i;

    if (td->trans_len < 1)
        return 0;
    if (verbose & TC_DEBUG) {
        tc_log_msg(MOD_NAME, "Preprocess transforms:");
    }
    if (td->smoothing>0) {
        /* smoothing */
        Transform* ts2 = NEW(Transform, td->trans_len);
        memcpy(ts2, ts, sizeof(Transform) * td->trans_len);

        /*  we will do a sliding average with minimal update
         *   \hat x_{n/2} = x_1+x_2 + .. + x_n
         *   \hat x_{n/2+1} = x_2+x_3 + .. + x_{n+1} = x_{n/2} - x_1 + x_{n+1}
         *   avg = \hat x / n
         */
        int s = td->smoothing * 2 + 1;
        Transform null = null_transform();
        /* avg is the average over [-smoothing, smoothing] transforms 
           around the current point */
        Transform avg;
        /* avg2 is a sliding average over the filtered signal! (only to past) 
         *  with smoothing * 10 horizont to kill offsets */
        Transform avg2 = null_transform();
        double tau = 1.0/(3 * s);
        /* initialise sliding sum with hypothetic sum centered around
         * -1st element. We have two choices:
         * a) assume the camera is not moving at the beginning 
         * b) assume that the camera moves and we use the first transforms
         */
        Transform s_sum = null; 
        for(i = 0; i < td->smoothing; i++){
            s_sum = add_transforms(&s_sum, i < td->trans_len ? &ts2[i]:&null);
        }
        mult_transform(&s_sum, 2); // choice b (comment out for choice a)

        for (i = 0; i < td->trans_len; i++) {
            Transform* old = ((i - td->smoothing - 1) < 0) 
                ? &null : &ts2[(i - td->smoothing - 1)];
            Transform* new = ((i + td->smoothing) >= td->trans_len) 
                ? &null : &ts2[(i + td->smoothing)];
            s_sum = sub_transforms(&s_sum, old);
            s_sum = add_transforms(&s_sum, new);

            avg = mult_transform(&s_sum, 1.0/s);

            /* lowpass filter: 
             * meaning high frequency must be transformed away
             */
            ts[i] = sub_transforms(&ts2[i], &avg);
            /* kill accumulating offset in the filtered signal*/
            avg2 = add_transforms_(mult_transform(&avg2, 1 - tau),
                                   mult_transform(&ts[i], tau));
            ts[i] = sub_transforms(&ts[i], &avg2);

            if (verbose & TC_DEBUG) {
                tc_log_msg(MOD_NAME, 
                           "s_sum: %5lf %5lf %5lf, ts: %5lf, %5lf, %5lf\n", 
                           s_sum.x, s_sum.y, s_sum.alpha, 
                           ts[i].x, ts[i].y, ts[i].alpha);
                tc_log_msg(MOD_NAME, 
                           "\tavg: %5lf, %5lf, %5lf avg2: %5lf, %5lf, %5lf", 
                           avg.x, avg.y, avg.alpha, 
                           avg2.x, avg2.y, avg2.alpha);      
            }
        }
        tc_free(ts2);
    }
  
  
    /*  invert? */
    if (td->invert) {
        for (i = 0; i < td->trans_len; i++) {
            ts[i] = mult_transform(&ts[i], -1);      
        }
    }
  
    /* relative to absolute */
    if (td->relative) {
        Transform t = ts[0];
        for (i = 1; i < td->trans_len; i++) {
            if (verbose  & TC_DEBUG) {
                tc_log_msg(MOD_NAME, "shift: %5lf\t %5lf\t %lf \n", 
                           t.x, t.y, t.alpha *180/M_PI);
            }
            ts[i] = add_transforms(&ts[i], &t); 
            t = ts[i];
        }
    }
    /* crop at maximal shift */
    if (td->maxshift != -1)
        for (i = 0; i < td->trans_len; i++) {
            ts[i].x     = TC_CLAMP(ts[i].x, -td->maxshift, td->maxshift);
            ts[i].y     = TC_CLAMP(ts[i].y, -td->maxshift, td->maxshift);
        }
    if (td->maxangle != - 1.0)
        for (i = 0; i < td->trans_len; i++)
            ts[i].alpha = TC_CLAMP(ts[i].alpha, -td->maxangle, td->maxangle);
  
    return 1;
}

/**
 * transform_init:  Initialize this instance of the module.  See
 * tcmodule-data.h for function details.
 */
static int transform_init(TCModuleInstance *self, uint32_t features)
{

    TransformData* td = NULL;
    TC_MODULE_SELF_CHECK(self, "init");
    TC_MODULE_INIT_CHECK(self, MOD_FEATURES, features);
    
    td = tc_malloc(sizeof(TransformData));
    if (td == NULL) {
        tc_log_error(MOD_NAME, "init: out of memory!");
        return TC_ERROR;
    }
    self->userdata = td;
    if (verbose) {
        tc_log_info(MOD_NAME, "%s %s", MOD_VERSION, MOD_CAP);
    }

    return TC_OK;
}


/**
 * transform_configure:  Configure this instance of the module.  See
 * tcmodule-data.h for function details.
 */
static int transform_configure(TCModuleInstance *self, 
			       const char *options, vob_t *vob)
{
    TransformData *td = NULL;
    TC_MODULE_SELF_CHECK(self, "configure");

    td = self->userdata;

    td->vob = vob;
    if (!td->vob)
        return TC_ERROR; /* cannot happen */

    /**** Initialise private data structure */

    /* td->framesize = td->vob->im_v_width *
     *  MAX_PLANES * sizeof(char) * 2 * td->vob->im_v_height * 2;    
     */
    td->framesize_src = td->vob->im_v_size;    
    td->src = tc_zalloc(td->framesize_src); /* FIXME */
    if (td->src == NULL) {
        tc_log_error(MOD_NAME, "tc_malloc failed\n");
        return TC_ERROR;
    }
  
    td->width_src  = td->vob->ex_v_width;
    td->height_src = td->vob->ex_v_height;
  
    /* Todo: in case we can scale the images, calc new size later */
    td->width_dest  = td->vob->ex_v_width;
    td->height_dest = td->vob->ex_v_height;
    td->framesize_dest = td->vob->im_v_size;
    td->dest = 0;
  
    td->trans = 0;
    td->trans_len = 0;
    td->current_trans = 0;
  
    /* Options */
    td->maxshift = -1;
    td->maxangle = -1;
    if (strlen(td->vob->video_in_file) < 250) {
        tc_snprintf(td->input, TC_BUF_LINE, "%s.trf", td->vob->video_in_file);
    } else {
        tc_log_warn(MOD_NAME, "input name too long, using default `%s'",
                    DEFAULT_TRANS_FILE_NAME);
        tc_snprintf(td->input, TC_BUF_LINE, "transforms.dat");
    }
    td->crop = 0;
    td->relative = 1;
    td->invert = 0;
    td->smoothing = 10;
  
    td->rotation_threshhold = 0.25/(180/M_PI);
  
    if (options != NULL) {
        optstr_get(options, "input", "%[^:]", (char*)&td->input);
    }
    td->f = fopen(td->input, "r");
    if(td->f == NULL) {
        tc_log_error(MOD_NAME, "cannot open input file %s!\n", td->input);
        /* return (-1); when called using tcmodinfo this will fail */ 
    } else if(!read_input_file(td)) { /* read input file */
        tc_log_info(MOD_NAME, "error parsing input file %s!\n", td->input);
        // return (-1);      
    }

    /* process remaining options */
    if (options != NULL) {    
        optstr_get(options, "maxshift",  "%d", &td->maxshift);
        optstr_get(options, "maxangle",  "%lf", &td->maxangle);
        optstr_get(options, "smoothing", "%d", &td->smoothing);
        optstr_get(options, "crop"     , "%d", &td->crop);
        optstr_get(options, "invert"   , "%d", &td->invert);
        optstr_get(options, "relative" , "%d", &td->relative);
    }
    if (verbose) {
        tc_log_info(MOD_NAME, "Image Transformation/Stabilization Settings:");
        tc_log_info(MOD_NAME, "    maxshift  = %d", td->maxshift);
        tc_log_info(MOD_NAME, "    maxangle  = %f", td->maxangle);
        tc_log_info(MOD_NAME, "    smoothing = %d", td->smoothing);
        tc_log_info(MOD_NAME, "    crop      = %s", 
                        td->crop ? "Black" : "Keep");
        tc_log_info(MOD_NAME, "    relative  = %s", 
                    td->relative ? "True": "False");
        tc_log_info(MOD_NAME, "    invert    = %s", 
                    td->invert ? "True" : "False");
        tc_log_info(MOD_NAME, "    input     = %s", td->input);
    }
  
    if (td->maxshift > td->width_dest/2
        ) td->maxshift = td->width_dest/2;
    if (td->maxshift > td->height_dest/2)
        td->maxshift = td->height_dest/2;
  
    if (!preprocess_transforms(td)) {
        tc_log_error(MOD_NAME, "error while preprocessing transforms!");
        return TC_ERROR;            
    }
  
    return TC_OK;
}


/**
 * transform_filter_video: performs the transformation of frames
 * See tcmodule-data.h for function details.
 */
static int transform_filter_video(TCModuleInstance *self, 
                                  vframe_list_t *frame) 
{
    TransformData *td = NULL;
  
    TC_MODULE_SELF_CHECK(self, "filter_video");
    TC_MODULE_SELF_CHECK(frame, "filter_video");
  
    td = self->userdata;

    td->dest = frame->video_buf;
    memcpy(td->src, frame->video_buf, td->framesize_src);
    if (td->current_trans >= td->trans_len) {
        tc_log_error(MOD_NAME, "not enough transforms found!\n");
        return TC_ERROR;
    }
  
    if (td->vob->im_v_codec == TC_CODEC_RGB24) {
        transformRGB(td);
    } else if(td->vob->im_v_codec == TC_CODEC_YUV420P) {
        transformYUV(td);
    } else {
        tc_log_error(MOD_NAME, "unsupported Codec: %i\n", td->vob->im_v_codec);
        return TC_ERROR;
    }
    td->current_trans++;
    return TC_OK;
}


/**
 * transform_fini:  Clean up after this instance of the module.  See
 * tcmodule-data.h for function details.
 */
static int transform_fini(TCModuleInstance *self)
{
    TransformData *td = NULL;
    TC_MODULE_SELF_CHECK(self, "fini");
    td = self->userdata;
    tc_free(td);
    self->userdata = NULL;
    return TC_OK;
}


/**
 * transform_stop:  Reset this instance of the module.  See tcmodule-data.h
 * for function details.
 */
static int transform_stop(TCModuleInstance *self)
{
    TransformData *td = NULL;
    TC_MODULE_SELF_CHECK(self, "stop");
    td = self->userdata;
    if (td->src) {
        tc_free(td->src);
        td->src = NULL;
    }
    if (td->trans) {
        tc_free(td->trans);
        td->trans = NULL;
    }
    if (td->f) {
        fclose(td->f);
        td->f = NULL;
    }
    return TC_OK;
}

/* checks for parameter in function _inspect */
#define CHECKPARAM(paramname, formatstring, variable)       \
    if (optstr_lookup(param, paramname)) {                \
        tc_snprintf(td->conf_str, sizeof(td->conf_str),   \
                    "maxshift=%i", td->maxshift);         \
        *value = td->conf_str;                            \
    }

/**
 * stabilize_inspect:  Return the value of an option in this instance of
 * the module.  See tcmodule-data.h for function details.
 */
static int transform_inspect(TCModuleInstance *self, 
            			     const char *param, const char **value)
{
    TransformData *td = NULL;
    
    TC_MODULE_SELF_CHECK(self,  "inspect");
    TC_MODULE_SELF_CHECK(param, "inspect");
    TC_MODULE_SELF_CHECK(value, "inspect");
  
    td = self->userdata;

    if (optstr_lookup(param, "help")) {
        *value = transform_help;
    }    
    CHECKPARAM("maxshift", "maxshift=%d",  td->maxshift);
    CHECKPARAM("maxangle", "maxangle=%f",  td->maxangle);
    CHECKPARAM("smoothing","smoothing=%d", td->smoothing);
    CHECKPARAM("crop",     "crop=%d",      td->crop);
    CHECKPARAM("relative", "relative=%d",  td->relative);
    CHECKPARAM("invert",   "invert=%i",    td->invert);
    CHECKPARAM("input",    "input=%s",     td->input);
        
    return TC_OK;
};


static const TCCodecID transform_codecs_in[] = { 
    TC_CODEC_YUV420P, TC_CODEC_YUV422P, TC_CODEC_RGB24, TC_CODEC_ERROR 
};
static const TCCodecID transform_codecs_out[] = { 
    TC_CODEC_YUV420P, TC_CODEC_YUV422P, TC_CODEC_RGB24, TC_CODEC_ERROR 
};
TC_MODULE_FILTER_FORMATS(transform);

TC_MODULE_INFO(transform);

static const TCModuleClass transform_class = {
    TC_MODULE_CLASS_HEAD(transform),
  
    .init         = transform_init,
    .fini         = transform_fini,
    .configure    = transform_configure,
    .stop         = transform_stop,
    .inspect      = transform_inspect,

    .filter_video = transform_filter_video,
};

TC_MODULE_ENTRY_POINT(transform)

/*************************************************************************/

static int transform_get_config(TCModuleInstance *self, char *options)
{
    TC_MODULE_SELF_CHECK(self, "get_config");

    optstr_filter_desc(options, MOD_NAME, MOD_CAP, MOD_VERSION,
                       MOD_AUTHOR, "VRY4", "1");

    return TC_OK;
}

static int transform_process(TCModuleInstance *self, frame_list_t *frame)
{
    TC_MODULE_SELF_CHECK(self, "process");

    if (frame->tag & TC_PREVIEW && frame->tag & TC_VIDEO) {
        return transform_filter_video(self, (vframe_list_t *)frame);
    }
    return TC_OK;
}

/*************************************************************************/

TC_FILTER_OLDINTERFACE(transform)

/*************************************************************************/

/*
 * Local variables:
 *   c-file-style: "stroustrup"
 *   c-file-offsets: ((case-label . *) (statement-case-intro . *))
 *   indent-tabs-mode: nil
 * End:
 *
 * vim: expandtab shiftwidth=4:
 */
