/*
 *  Copyright (C) 2005 Francesco Romani <fromani@gmail.com>
 * 
 *  Permission is hereby granted, free of charge, to any person obtaining a copy
 *  of this software and associated documentation files (the "Software"), to deal
 *  in the Software without restriction, including without limitation the rights
 *  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell 
 *  copies of the Software, and to permit persons to whom the Software is 
 *  furnished to do so, subject to the following conditions:
 *  
 *  The above copyright notice and this permission notice shall be included in 
 *  all copies or substantial portions of the Software.
 *  
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS 
 *  OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 *  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 *  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 *  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 *  THE SOFTWARE.
 *  
 * malloc-failure.c -- preload library to help to debug/test memory-related issues
 * 
 * Compilation example:
 * gcc -Wall -shared -rdyanmic -nostartfile -fPIC malloc-failure.c -o malloc-failure.so -ldl
 *
 * Usage example:
 * LD_PRELOAD=./malloc-failure.so your_program
 */

#define _GNU_SOURCE
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <time.h>

#define DEFAULT_FAILURE_RATIO			(0)

static void* (*orig_malloc)(size_t size) = NULL;
static unsigned int rand_state = 0;
static int failure_ratio = DEFAULT_FAILURE_RATIO;
static unsigned int mallocs = 0;
static unsigned int faileds = 0;

/* Library initialization function */
//void mf_init(void) __attribute__((constructor));

void _fini(void)
{
    fprintf(stderr, "(%s) SUMMARY: mallocs: %i, faileds: %i (%02i%%)\n",
		    __FILE__, mallocs, faileds, 
		    (int)((100.0*faileds)/(double)mallocs));
}

void _init(void)
{
    const char *envp = NULL;
    rand_state = time(NULL) ^ geteuid();
    
    faileds = 0;
    mallocs = 0;
    
    orig_malloc = dlsym(RTLD_NEXT, "malloc");
    if(orig_malloc == NULL)
    {
        char *error = dlerror();
        if(error == NULL)
        {
            error = "malloc is NULL";
        }
        fprintf(stderr, "(%s) %s\n", __FILE__, error);
        exit(EXIT_FAILURE);
    }
    fprintf(stderr, "(%s) hooked LibC malloc at %p\n", __FILE__, orig_malloc);
    
    envp = getenv("MALLOC_FAILURE_RATIO");
    if(envp != NULL) {
        failure_ratio = atoi(envp);
	if(failure_ratio < 0 || failure_ratio > 100) {
	    fprintf(stderr, "(%s) insane failure ratio %02i%%, "
                            "reverting to default (%02i%%)\n",
	                    __FILE__, failure_ratio, DEFAULT_FAILURE_RATIO);
	}
    }
    fprintf(stderr, "(%s) failure ratio will be %02i%%\n", __FILE__, failure_ratio);
}

void *malloc(size_t size)
{
    int j = (int) (100.0*rand_r(&rand_state)/(RAND_MAX+1.0));
    
    mallocs++;

#ifdef VERBOSE    
    fprintf(stderr, "(%s) random = %02i; failure_ratio = %02i"
		    " -> %s [requested %lu bytes]\n", 
		    __FILE__, j, failure_ratio, 
		    (j >= failure_ratio) ?"OK" :"KO",
		    (unsigned long)size);
#endif    
    if(j < failure_ratio) {
	faileds++;
        return NULL;
    }
    /* Yep, this also can fail. It's a feature, not a bug! ;) */
    return orig_malloc(size);
}
