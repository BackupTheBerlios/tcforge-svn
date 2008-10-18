/*
 *  filter_stabilize.c
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
 */

/* Typical call:
 *  transcode -V -J stabilize="maxshift=48:fieldsize=48" 
 *         -i inp.m2v -y null,null -o dummy
 *  parameters are optional
*/

#define MOD_NAME    "filter_stabilize.so"
#define MOD_VERSION "v0.4.2 (2008-09-16)"
#define MOD_CAP     "extracts relative transformations of \n\
\tsubsequent frames (used for stabilization together with the\n\
\ttransform filter in a second pass)"
#define MOD_AUTHOR  "Georg Martius"

#define MOD_FEATURES \
    TC_MODULE_FEATURE_FILTER|TC_MODULE_FEATURE_VIDEO
#define MOD_FLAGS  \
    TC_MODULE_FLAG_RECONFIGURABLE | TC_MODULE_FLAG_DELAY
  
#include <math.h>

#include "src/transcode.h"
#include "src/filter.h"
#include "libtc/libtc.h"
#include "libtc/tccodecs.h"
#include "libtcutil/optstr.h"
#include "libtcutil/tclist.h"
#include "libtcmodule/tcmodule-plugin.h"

#include "transform.h"

#define DEFAULT_TRANS_FILE_NAME     "transforms.dat"

/* if defined we are very verbose and generate files to analyse
 * this is really just for debugging and development */
// #define STABVERBOSE

typedef struct _field {
    int x;     // middle position x
    int y;     // middle position y
    int size;  // size of field
} Field;

/* private date structure of this filter*/
typedef struct _stab_data {
    size_t framesize;  // size of frame buffer in bytes (prev)
    unsigned char* curr; // current frame buffer (only pointer)
    unsigned char* prev; // frame buffer for last frame (copied)
    short hasSeenOneFrame; // true if we have a valid previous frame

    vob_t* vob;  // pointer to information structure
    int width, height;

    /* list of transforms*/
    TCList* transs;

    Field* fields;

    /* Options */
    /* maximum number of pixels we expect a shift of subsequent frames */
    int maxshift; 
    int stepsize; // stepsize of field transformation detection
    int allowmax; // 1 if maximal shift is allowed
    int algo;     // algorithm to use
    int field_num;   // number of meaurement fields
    int field_size; // size    = MIN(sd->width, sd->height)/10;
  
    int t;
    char* result;
    FILE* f;

    char conf_str[TC_BUF_MIN];
} StabData;

/* type for a function that calculates the transformation of a certain field 
 */
typedef Transform (*calcFieldTransFunc)(StabData*, const Field*, int);

static const char stabilize_help[] = ""
    "Overview:\n"
    "    Generates a file with relative transform information\n"
    "     (translation, rotation) about subsequent frames."
    " See also transform.\n" 
    "Options\n"
    "    'result'     path to the file used to write the transforms\n"
    "                 (def:inputfile.stab)\n"
    "    'maxshift'   maximal number of pixels to search for a transformation\
\n                 (def:48, preferably a multiple of stepsize)\n"
    "    'stepsize'   stepsize of search process, \n"
    "                 region around minimum is scanned with 1 pixel\n"
    "                 resolution (def:2)\n"
    "    'allowmax'   0: maximal shift is set to 0 (considered to be an error)\
\n                 1: maximal shift is allowed (def:1)\n"
    "    'algo'       0: brute force (translation only);\n"
    "                 1: small measurement fields(def)\n"
    "    'fieldsetup' number of measurement fields in each dim: \n\
                 1: 1; 3: 9; 5: 25 (def: 3 meaning 9 fields)\n"
    "    'fieldsize'  size of measurement field (default height/10)\n"
    "    'help'       print this help message\n";

int initFields(StabData* sd, int field_setup);
double compareImg(unsigned char* I1, unsigned char* I2, 
		  int width, int height,  int bytesPerPixel, int d_x, int d_y);
double compareSubImg(unsigned char* I1, unsigned char* I2, 
		     const Field* field, 
		     int width, int height, int bytesPerPixel,int d_x,int d_y);
Transform calcShiftRGBSimple(StabData* sd);
Transform calcShiftYUVSimple(StabData* sd);
double calcAngle(StabData* sd, Field* field, Transform* t);
Transform calcFieldTransYUV(StabData* sd, const Field* field, 
                            int fieldnum);
Transform calcFieldTransRGB(StabData* sd, const Field* field, 
                            int fieldnum);
Transform calcTransFields(StabData* sd, calcFieldTransFunc fieldfunc);
void addTrans(StabData* sd, Transform sl);

void addTrans(StabData* sd, Transform sl)
{
    if (!sd->transs) {
        sd->transs = tc_list_new(0);
    }
    tc_list_append_dup(sd->transs, &sl, sizeof(sl));
}


/** initialise measurement fields on the frame
    @param field_setup 1: only one field, 3 some fields (9), 5: many (25)
*/
int initFields(StabData* sd, int field_setup)
{
    if (field_setup < 1) 
        field_setup = 1;
  
    sd->field_num = field_setup*field_setup;  
    if (!(sd->fields = NEW(Field, sd->field_num))) {
        tc_log_error(MOD_NAME, "malloc failed!\n");
        return 0;
    } else {
        int i, j;
        int s;
        int f=0;
        int center_x= sd->width/2;
        int center_y= sd->height/2;
        int size    = sd->field_size;
        int step_x  = (field_setup == 1) ? 0 : 
            (sd->width  - size - 2*sd->maxshift - 2)/(field_setup - 1);
        int step_y  = (field_setup == 1) ? 0 : 
            (sd->height - size - 2*sd->maxshift - 2)/(field_setup - 1);
        s=field_setup/2;
        for (i = -s; i <= s; i++) {
            for (j = -s; j <= s; j++) {
                sd->fields[f].x = center_x + i*step_x;
                sd->fields[f].y = center_y + j*step_y;
                sd->fields[f].size = size;
#ifdef STABVERBOSE
                tc_log_msg(MOD_NAME, "field %2i: %i, %i\n", 
                           f, sd->fields[f].x, sd->fields[f].y);
#endif
                f++;
            }
        }
    }
    return 1;
}


/**
   compares the two given images and returns the average absolute difference
   \param d_x shift in x direction
   \param d_y shift in y direction
*/
double compareImg(unsigned char* I1, unsigned char* I2, 
                  int width, int height,  int bytesPerPixel, int d_x, int d_y)
{
    int i, j;
    unsigned char* p1 = NULL;
    unsigned char* p2 = NULL;
    long int sum = 0;  
    int effectWidth = width - abs(d_x);
    int effectHeight = height - abs(d_y);

/*   DEBUGGING code to export single frames */
/*   char buffer[100]; */
/*   sprintf(buffer, "pic_%02ix%02i_1.ppm", d_x, d_y); */
/*   FILE *pic1 = fopen(buffer, "w"); */
/*   sprintf(buffer, "pic_%02ix%02i_2.ppm", d_x, d_y); */
/*   FILE *pic2 = fopen(buffer, "w"); */
/*   fprintf(pic1, "P6\n%i %i\n255\n", effectWidth, effectHeight); */
/*   fprintf(pic2, "P6\n%i %i\n255\n", effectWidth, effectHeight); */

    for (i = 0; i < effectHeight; i++) {
        p1 = I1;
        p2 = I2;
        if (d_y > 0 ){ 
            p1 += (i + d_y) * width * bytesPerPixel;
            p2 += i * width * bytesPerPixel;
        } else {
            p1 += i * width * bytesPerPixel;
            p2 += (i - d_y) * width * bytesPerPixel;
        }
        if (d_x > 0) { 
            p1 += d_x * bytesPerPixel;
        } else {
            p2 -= d_x * bytesPerPixel; 
        }
        // TODO: use some mmx or sse stuff here
        for(j = 0; j < effectWidth * bytesPerPixel; j++) {
            /* debugging code continued */
            /* fwrite(p1,1,1,pic1);fwrite(p1,1,1,pic1);fwrite(p1,1,1,pic1);
               fwrite(p2,1,1,pic2);fwrite(p2,1,1,pic2);fwrite(p2,1,1,pic2); 
             */
            sum += abs((int)*p1 - (int)*p2);
            p1++;
            p2++;      
        }
    }
    /*  fclose(pic1);
        fclose(pic2); 
     */
    return sum/((double) effectWidth * effectHeight * bytesPerPixel);
}

/**
   compares a small part of two given images 
   and returns the average absolute difference.
   Field center, size and shift have to be choosen, 
   so that no clipping is required
     
   \param field Field specifies position(center) and size of subimage 
   \param d_x shift in x direction
   \param d_y shift in y direction   
*/
double compareSubImg(unsigned char* I1, unsigned char* I2, const Field* field, 
                     int width, int height, int bytesPerPixel, int d_x, int d_y)
{
    int k, j;
    unsigned char* p1 = NULL;
    unsigned char* p2 = NULL;
    int s2 = field->size / 2;
    double sum = 0;

    p1=I1 + ((field->x - s2) + (field->y - s2)*width)*bytesPerPixel;
    p2=I2 + ((field->x - s2 + d_x) + (field->y - s2 + d_y)*width)*bytesPerPixel;
    // TODO: use some mmx or sse stuff here
    for (j = 0; j < field->size; j++){
        for (k = 0; k < field->size * bytesPerPixel; k++) {
            sum += abs((int)*p1 - (int)*p2);
            p1++;
            p2++;     
        }
        p1 += (width - field->size) * bytesPerPixel;
        p2 += (width - field->size) * bytesPerPixel;
    }
    return sum/((double) field->size *field->size* bytesPerPixel);
}



/** tries to register current frame onto previous frame.
    This is the most simple algorithm:
    shift images to all possible positions and calc summed error
    Shift with minimal error is selected.
*/
Transform calcShiftRGBSimple(StabData* sd)
{
    int x = 0, y = 0;
    int i, j;
    double minerror = 1e20;  
    for (i = -sd->maxshift; i <= sd->maxshift; i++) {
        for (j = -sd->maxshift; j <= sd->maxshift; j++) {
            double error = compareImg(sd->curr, sd->prev, 
                                      sd->width, sd->height, 3, i, j);
            if (error < minerror) {
                minerror = error;
                x = i;
                y = j;
            }	
        }
    } 
    return new_transform(x, y, 0, 0);
}


/** tries to register current frame onto previous frame. 
    (only the luminance is used)
    This is the most simple algorithm:
    shift images to all possible positions and calc summed error
    Shift with minimal error is selected.
*/
Transform calcShiftYUVSimple(StabData* sd)
{
    int x = 0, y = 0;
    int i, j;
    unsigned char *Y_c, *Y_p;// , *Cb, *Cr;
#ifdef STABVERBOSE
    FILE *f = NULL;
    char buffer[32];
    tc_snprintf(buffer, sizeof(buffer), "f%04i.dat", sd->t);
    f = fopen(buffer, "w");
    fprintf(f, "# splot \"%s\"\n", buffer);
#endif


    // we only use the luminance part of the image
    Y_c  = sd->curr;  
    //  Cb_c = sd->curr + sd->width*sd->height;
    //Cr_c = sd->curr + 5*sd->width*sd->height/4;
    Y_p  = sd->prev;  
    //Cb_p = sd->prev + sd->width*sd->height;
    //Cr_p = sd->prev + 5*sd->width*sd->height/4;

    double minerror = 1e20;  
    for (i = -sd->maxshift; i <= sd->maxshift; i++) {
        for (j = -sd->maxshift; j <= sd->maxshift; j++) {
            double error = compareImg(Y_c, Y_p, 
                                      sd->width, sd->height, 1, i, j);
#ifdef STABVERBOSE
            fprintf(f, "%i %i %f\n", i, j, error);
#endif
            if (error < minerror) {
                minerror = error;
                x = i;
                y = j;
            }	
        }
    }  
#ifdef STABVERBOSE
    fclose(f);
    tc_log_msg(MOD_NAME, "Minerror: %f\n", minerror);
#endif
    return new_transform(x, y, 0, 0);
}



/* calulcates rotation angle for the given transform and 
 * field with respect to center
 */
double calcAngle(StabData* sd, Field* field, Transform* t)
{
    int center_x = sd->width/2;
    int center_y = sd->height/2;
    if (field->x == center_x && field->y == center_y) {
        return 0;
    } else {
        // double r = sqrt(field->x*field->x + field->y*field->y);   
        double a1 = atan2(field->y - center_y, field->x - center_x);
        double a2 = atan2(field->y - center_y + t->y, 
                          field->x - center_x + t->x);
        double diff = a2 - a1;
        return (diff>M_PI) ? diff - 2*M_PI 
            : ( (diff<-M_PI) ? diff + 2*M_PI : diff);    
    }
}


/* calculates the optimal transformation for one field in YUV frames
 * (only luminance)
 */
Transform calcFieldTransYUV(StabData* sd, const Field* field, int fieldnum)
{
    Transform t = null_transform();
    uint8_t *Y_c = sd->curr, *Y_p = sd->prev;
    // we only use the luminance part of the image
    int i, j;

#ifdef STABVERBOSE
    FILE *f = NULL;
    char buffer[32];
    tc_snprintf(buffer, sizeof(buffer), "f%04i_%02i.dat", sd->t, fieldnum);
    f = fopen(buffer, "w");
    fprintf(f, "# splot \"%s\"\n", buffer);
#endif
  
    double minerror = 1e20;  
    for (i = -sd->maxshift; i <= sd->maxshift; i += sd->stepsize) {
        for(j = -sd->maxshift; j <= sd->maxshift; j += sd->stepsize) {
            double error = compareSubImg(Y_c, Y_p, field, 
                                         sd->width, sd->height, 1, i, j);
#ifdef STABVERBOSE
            fprintf(f, "%i %i %f\n", i, j, error);
#endif
            if (error < minerror) {
                minerror = error;
                t.x = i;
                t.y = j;
            }	
        }
    }
    if (sd->stepsize > 1) {
        int r = sd->stepsize - 1;
        for (i = t.x - r; i <= t.x + r; i += 1) {
            for (j = -t.y - r; j <= t.y + r; j += 1) {
                if (i == t.x && j == t.y) 
                    continue; //no need to check this since already done
                double error = compareSubImg(Y_c, Y_p, field, 
                                             sd->width, sd->height, 1, i, j);
#ifdef STABVERBOSE
                fprintf(f, "%i %i %f\n", i, j, error);
#endif 	
                if (error < minerror){
                    minerror = error;
                    t.x = i;
                    t.y = j;
                }	
            }
        }
    }
#ifdef STABVERBOSE 
    fclose(f); 
    tc_log_msg(MOD_NAME, "Minerror: %f\n", minerror);
#endif

    if (!sd->allowmax && fabs(t.x) == sd->maxshift) {
#ifdef STABVERBOSE 
        tc_log_msg(MOD_NAME, "maximal x shift ");
#endif
        t.x = 0;
    }
    if (!sd->allowmax && fabs(t.y) == sd->maxshift) {
#ifdef STABVERBOSE 
        tc_log_msg(MOD_NAME, "maximal y shift ");
#endif
        t.y = 0;
    }
    return t;
}

/* calculates the optimal transformation for one field in RGB 
 *   slower than the YUV version because it uses all three color channels
 */
Transform calcFieldTransRGB(StabData* sd, const Field* field, int fieldnum)
{
    Transform t = null_transform();
    uint8_t *I_c = sd->curr, *I_p = sd->prev;
    int i, j;
  
    double minerror = 1e20;  
    for (i = -sd->maxshift; i <= sd->maxshift; i += 2) {
        for(j=-sd->maxshift; j <= sd->maxshift; j += 2) {      
            double error = compareSubImg(I_c, I_p, field, 
                                         sd->width, sd->height, 3, i, j);
            if (error < minerror) {
                minerror = error;
                t.x = i;
                t.y = j;
            }	
        }
    }
    for (i = t.x - 1; i <= t.x + 1; i += 2) {
        for (j = -t.y - 1; j <= t.y + 1; j += 2) {
            double error = compareSubImg(I_c, I_p, field, 
                                         sd->width, sd->height, 3, i, j);
            if(error < minerror) {
                minerror = error;
                t.x = i;
                t.y = j;
            }	
        }
    }
    if (!sd->allowmax && fabs(t.x) == sd->maxshift) {
        t.x = 0;
    }
    if (!sd->allowmax && fabs(t.y) == sd->maxshift) {
        t.y = 0;
    }
    return t;
}


/* tries to register current frame onto previous frame. 
 *   Algorithm:
 *   check all fields for vertical and horizontal transformation 
 *   use minimal difference of all possible positions
 *   calculate shift as cleaned mean of all fields
 *   calculate rotation angle of each field in respect to center 
 *   after shift removal
 *   calculate rotation angle as cleaned mean of all angle
*/
Transform calcTransFields(StabData* sd, calcFieldTransFunc fieldfunc)
{
    Transform* ts = NEW(Transform, sd->field_num);
    double *angles = NEW(double, sd->field_num);
    int i;
    Transform t;
#ifdef STABVERBOSE
    FILE *f = NULL;
    char buffer[32];
    tc_snprintf(buffer, sizeof(buffer), "k%04i.dat", sd->t);
    f = fopen(buffer, "w");
    fprintf(f, "# plot \"%s\" w l, \"\" every 2:1:0\n", buffer);
#endif

    for (i = 0; i < sd->field_num; i++) {
        ts[i] = fieldfunc(sd, &sd->fields[i], i);
        //ts[i] = calcFieldTransYUV(sd, &sd->fields[i], i);
#ifdef STABVERBOSE
        fprintf(f, "%i %i\n%f %f\n \n\n", sd->fields[i].x, sd->fields[i].y, 
                sd->fields[i].x + ts[i].x, sd->fields[i].y + ts[i].y);
#endif
    }

/*   // average over all transforms */
/*   { */
/*     Transform sum = null_transform(); */
/*     for(i=0; i < sd->field_num; i++){ */
/*       sum = add_transforms(&sum, &ts[i]); */
/*     } */
/*     t = mult_transform(&sum, 1.0/ sd->field_num); */
/*   } */
    /* median over all transforms
     * t= median_xy_transform(ts, sd->field_num);
     * cleaned mean
     */
    t= cleanmean_xy_transform(ts, sd->field_num);

    // substract avg
    for (i = 0; i < sd->field_num; i++) {
        ts[i] = sub_transforms(&ts[i], &t);
    }
    // figure out angle
    if (sd->field_num == 1) {
        t.alpha = 0; // one is always the center
    } else{      
        for (i = 0; i < sd->field_num; i++) {
            angles[i] = calcAngle(sd, &sd->fields[i], &ts[i]);
        }
        // t.alpha = - mean(angles, sd->field_num);
        // t.alpha = - median(angles, sd->field_num);
        t.alpha = -cleanmean(angles, sd->field_num);
    }
#ifdef STABVERBOSE
    fclose(f);
#endif
    return t;
}

struct iterdata {
    FILE *f;
    int  counter;
};

static int stabilize_dump_trans(TCListItem *item, void *userdata)
{
    struct iterdata *ID = userdata;

    if (item->data) {
        Transform* t = item->data;
        fprintf(ID->f, "%i %5.4lf %5.4lf %8.5lf %i\n",
                ID->counter, t->x, t->y, t->alpha, t->extra);
        ID->counter++;
    }
    return 0; /* never give up */
}

/*************************************************************************/

/* Module interface routines and data. */

/*************************************************************************/

/**
 * stabilize_init:  Initialize this instance of the module.  See
 * tcmodule-data.h for function details.
 */

static int stabilize_init(TCModuleInstance *self, uint32_t features)
{

    StabData* sd = NULL;
    TC_MODULE_SELF_CHECK(self, "init");
    TC_MODULE_INIT_CHECK(self, MOD_FEATURES, features);

    sd = tc_zalloc(sizeof(StabData));
    if (!sd) {
        tc_log_error(MOD_NAME, "init: out of memory!");
        return TC_ERROR;
    }

    sd->vob = tc_get_vob();
    if (!sd->vob)
        return TC_ERROR;

    /**** Initialise private data structure */

    sd->t = 0;
    sd->hasSeenOneFrame = 0;
    sd->transs = 0;
    sd->prev = 0;

    self->userdata = sd;
    if (verbose) {
        tc_log_info(MOD_NAME, "%s %s", MOD_VERSION, MOD_CAP);
    }
    return TC_OK;
}


/*
 * stabilize_fini:  Clean up after this instance of the module.  See
 * tcmodule-data.h for function details.
 */
static int stabilize_fini(TCModuleInstance *self)
{
    StabData *sd = NULL;
    TC_MODULE_SELF_CHECK(self, "fini");
    sd = self->userdata;

    tc_free(sd);
    self->userdata = NULL;
    return TC_OK;
}

/*
 * stabilize_configure:  Configure this instance of the module.  See
 * tcmodule-data.h for function details.
 */
static int stabilize_configure(TCModuleInstance *self,
            			       const char *options, vob_t *vob)
{
    int field_setup;

    StabData *sd = NULL;
    TC_MODULE_SELF_CHECK(self, "configure");

    sd = self->userdata;

    //    sd->framesize = sd->vob->im_v_width * MAX_PLANES * sizeof(char) * 2 * sd->vob->im_v_height * 2;    
    sd->framesize = sd->vob->im_v_size;    
    sd->prev = tc_zalloc(sd->framesize); /* FIXME */
    if (!sd->prev) {
        tc_log_error(MOD_NAME, "malloc failed");
        return TC_ERROR;
    }

    sd->width  = sd->vob->ex_v_width;
    sd->height = sd->vob->ex_v_height;

    sd->hasSeenOneFrame = 0;
    sd->transs = 0;
    
    // Options
    sd->maxshift = 48;
    sd->stepsize = 2;
    sd->allowmax = 1;
    sd->result = tc_malloc(TC_BUF_LINE);
    if (strlen(sd->vob->video_in_file) < TC_BUF_LINE - 1) {
        tc_snprintf(sd->result, TC_BUF_LINE, "%s.trf", sd->vob->video_in_file);
    } else {
        tc_log_warn(MOD_NAME, "input name too long, using default `%s'",
                    DEFAULT_TRANS_FILE_NAME);
        tc_snprintf(sd->result, TC_BUF_LINE, "transforms.dat");
    }
    sd->algo = 1;
    field_setup = 3;
    sd->field_size = TC_MIN(sd->width, sd->height)/10;

    if (options != NULL) {            
        optstr_get(options, "result",    "%[^:]", sd->result);
        optstr_get(options, "maxshift",  "%d", &sd->maxshift);
        optstr_get(options, "stepsize",  "%d", &sd->stepsize);
        optstr_get(options, "allowmax",  "%d", &sd->allowmax);
        optstr_get(options, "algo",      "%d", &sd->algo);
        optstr_get(options, "fieldsetup","%d", &field_setup);
        optstr_get(options, "fieldsize", "%d", &sd->field_size);
    }
    if (verbose) {
        tc_log_info(MOD_NAME, "Image Stabilization Settings:");
        tc_log_info(MOD_NAME, "      maxshift = %d", sd->maxshift);
        tc_log_info(MOD_NAME, "      stepsize = %d", sd->stepsize);
        tc_log_info(MOD_NAME, "      allowmax = %d", sd->allowmax);
        tc_log_info(MOD_NAME, "          algo = %d", sd->algo);
        tc_log_info(MOD_NAME, "    fieldsetup = %d", field_setup);
        tc_log_info(MOD_NAME, "     fieldsize = %d", sd->field_size);
        tc_log_info(MOD_NAME, "        result = %s", sd->result);
    }
    
    if (sd->maxshift > sd->width / 2)
        sd->maxshift = sd->width / 2;
    if (sd->maxshift > sd->height / 2)
        sd->maxshift = sd->height / 2;

    if (sd->algo==1) {
        if (!initFields(sd, field_setup)) {
            return TC_ERROR;
        }
    }
    sd->f = fopen(sd->result, "w");
    if (sd->f == NULL) {
        tc_log_error(MOD_NAME, "cannot open result file %s!\n", sd->result);
        return TC_ERROR;
    }    
    return TC_OK;
}


/**
 * stabilize_filter_video: performs the analysis of subsequent frames
 * See tcmodule-data.h for function details.
 */

static int stabilize_filter_video(TCModuleInstance *self, 
                                  vframe_list_t *frame)
{
    StabData *sd = NULL;
  
    TC_MODULE_SELF_CHECK(self, "filter_video");
    TC_MODULE_SELF_CHECK(frame, "filter_video");
  
    sd = self->userdata;
  
    if (sd->hasSeenOneFrame) {
        sd->curr = frame->video_buf;
        if (sd->vob->im_v_codec == CODEC_RGB) {
            if(sd->algo == 0)
                addTrans(sd, calcShiftRGBSimple(sd));
            else if (sd->algo == 1)
                addTrans(sd, calcTransFields(sd, calcFieldTransRGB));
        } else if(sd->vob->im_v_codec == CODEC_YUV) {
            if (sd->algo == 0)
                addTrans(sd, calcShiftYUVSimple(sd));
            else if (sd->algo == 1)
                addTrans(sd, calcTransFields(sd, calcFieldTransYUV));
        } else {
            tc_log_warn(MOD_NAME, "unsupported Codec: %i\n",
                        sd->vob->im_v_codec);
            return TC_ERROR;
        }
    } else {
        sd->hasSeenOneFrame = 1;
        addTrans(sd, null_transform());
    }
    memcpy(sd->prev, frame->video_buf, sd->framesize);
    sd->t++;
    return TC_OK;
}

/**
 * stabilize_stop:  Reset this instance of the module.  See tcmodule-data.h
 * for function details.
 */

static int stabilize_stop(TCModuleInstance *self)
{
    StabData *sd = NULL;
    TC_MODULE_SELF_CHECK(self, "stop");
    sd = self->userdata;

    // print transs
    if (sd->f) {
        struct iterdata ID;
        ID.counter = 0;
        ID.f       = sd->f;

        fprintf(sd->f, "# Transforms\n#C FrameNr x y alpha extra\n");
   
        tc_list_foreach(sd->transs, stabilize_dump_trans, &ID);
    
        fclose(sd->f);
        sd->f = NULL;
    }
    tc_list_del(sd->transs, 1);
    if (sd->prev) {
        tc_free(sd->prev);
        sd->prev = NULL;
    }
    if (sd->result) {
        tc_free(sd->result);
        sd->result = NULL;
    }
    return TC_OK;
}

/* checks for parameter in function _inspect */
#define CHECKPARAM(paramname, formatstring, variable)       \
    if (optstr_lookup(param, paramname)) {                \
        tc_snprintf(sd->conf_str, sizeof(sd->conf_str),   \
                    "maxshift=%i", sd->maxshift);         \
        *value = sd->conf_str;                            \
    }

/**
 * stabilize_inspect:  Return the value of an option in this instance of
 * the module.  See tcmodule-data.h for function details.
 */

static int stabilize_inspect(TCModuleInstance *self,
			     const char *param, const char **value)
{
    StabData *sd = NULL;

    TC_MODULE_SELF_CHECK(self, "inspect");
    TC_MODULE_SELF_CHECK(param, "inspect");
    TC_MODULE_SELF_CHECK(value, "inspect");
    sd = self->userdata;

    if (optstr_lookup(param, "help")) {
        *value = stabilize_help;
    }
    CHECKPARAM("maxshift", "maxshift=%d",  sd->maxshift);
    CHECKPARAM("stepsize", "stepsize=%d",  sd->stepsize);
    CHECKPARAM("allowmax", "allowmax=%d",  sd->allowmax);
    CHECKPARAM("algo",     "algo=%d",      sd->algo);
/*    CHECKPARAM("fieldsetup","fieldsetup=%d",sd->field_setup); */
    CHECKPARAM("fieldsize","fieldsize=%d", sd->field_size);
    CHECKPARAM("result",   "result=%s",    sd->result);
    return TC_OK;
}

static const TCCodecID stabilize_codecs_in[] = { 
    TC_CODEC_YUV420P, TC_CODEC_YUV422P, TC_CODEC_RGB, TC_CODEC_ERROR 
};
static const TCCodecID stabilize_codecs_out[] = { 
    TC_CODEC_YUV420P, TC_CODEC_YUV422P, TC_CODEC_RGB, TC_CODEC_ERROR 
};
TC_MODULE_FILTER_FORMATS(stabilize); 

TC_MODULE_INFO(stabilize);

static const TCModuleClass stabilize_class = {
    TC_MODULE_CLASS_HEAD(stabilize),

    .init         = stabilize_init,
    .fini         = stabilize_fini,
    .configure    = stabilize_configure,
    .stop         = stabilize_stop,
    .inspect      = stabilize_inspect,

    .filter_video = stabilize_filter_video,
};

TC_MODULE_ENTRY_POINT(stabilize)

/*************************************************************************/

static int stabilize_get_config(TCModuleInstance *self, char *options)
{
    TC_MODULE_SELF_CHECK(self, "get_config");

    optstr_filter_desc(options, MOD_NAME, MOD_CAP, MOD_VERSION,
                       MOD_AUTHOR, "VRY4", "1");

    return TC_OK;
}

static int stabilize_process(TCModuleInstance *self, frame_list_t *frame)
{
    TC_MODULE_SELF_CHECK(self, "process");

    if (frame->tag & TC_PREVIEW && frame->tag & TC_VIDEO) {
        return stabilize_filter_video(self, (vframe_list_t *)frame);
    }
    return TC_OK;
}

/*************************************************************************/

TC_FILTER_OLDINTERFACE(stabilize)

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
