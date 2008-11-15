/*
 *  Copyright (C) 2005 Francesco Romani <fromani@gmail.com>
 * 
 *  This Software is heavily based on tcvp's (http://tcvp.sf.net) 
 *  mpeg muxer/demuxer, which is
 *
 *  Copyright (C) 2001-2002 Michael Ahlberg, M<C3><A5>ns Rullg<C3><A5>rd
 *  Copyright (C) 2003-2004 Michael Ahlberg, M<C3><A5>ns Rullg<C3><A5>rd
 *  Copyright (C) 2005 Michael Ahlberg, M<C3><A5>ns Rullg<C3><A5>rd
 *
 *  Permission is hereby granted, free of charge, to any person
 *  obtaining a copy of this software and associated documentation
 *  files (the "Software"), to deal in the Software without
 *  restriction, including without limitation the rights to use, copy,
 *  modify, merge, publish, distribute, sublicense, and/or sell copies
 *  of the Software, and to permit persons to whom the Software is
 *  furnished to do so, subject to the following conditions:
 *
 *  The above copyright notice and this permission notice shall be
 *  included in all copies or substantial portions of the Software.
 *
 *  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 *  EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 *  MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 *  NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
 *  HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
 *  WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 *  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 *  DEALINGS IN THE SOFTWARE.
 */

#include <string.h>
#include <stdarg.h>
#include <assert.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "mpeglib.h"
#include "mpeg-private.h"

/** mempry handling hooks */
static mpeg_macquire_fn_t mem_acquire = malloc;
static mpeg_mrelease_fn_t mem_release = free;

void 
mpeg_set_mem_handling(mpeg_macquire_fn_t acquire,
                      mpeg_mrelease_fn_t release) {
    assert(acquire != NULL);
    assert(release != NULL);

    mem_acquire = acquire;
    mem_release = release;
}
        

/** logging hooks */
static void *logdest = NULL;
static mpeg_log_fn_t logger = mpeg_log_file;

void
mpeg_set_logging(mpeg_log_fn_t nlogger, void *nlogdest) {
    assert(nlogger != NULL);
    assert(nlogdest != NULL);

    logger = nlogger;
    logdest = nlogdest;
}

mpeg_res_t
mpeg_log(int level, const char *fmt, ...) {
    int ret = MPEG_OK;
    va_list ap;

    assert(logger != NULL);
    if(logdest == NULL) {
        logdest = stderr;
    }

    va_start(ap, fmt);
    ret = logger(logdest, level, fmt, ap);
    va_end(ap);
    return ret;
}


static size_t 
mpeg_file_read(mpeg_file_t *MFILE, void *ptr, 
            size_t size, size_t num) {
    assert(NULL != MFILE);
    assert(NULL != ptr);
    return fread(ptr, size, num, MFILE->priv);
}

static size_t 
mpeg_file_write(mpeg_file_t *MFILE, const void *ptr, 
            size_t size, size_t num) {
    assert(NULL != MFILE);
    assert(NULL != ptr);
    return fwrite(ptr, size, num, MFILE->priv);
}

static int
mpeg_file_seek(mpeg_file_t *MFILE, uint64_t offset, int whence) {
    assert(NULL != MFILE);
    return fseek(MFILE->priv, offset, whence);
}

static int64_t
mpeg_file_tell(mpeg_file_t *MFILE) {
    assert(NULL != MFILE);
    return ftell(MFILE->priv);
}

static int64_t
mpeg_file_get_size(mpeg_file_t *MFILE) {
    struct stat buf;
    int res;
    
    assert(NULL != MFILE);

    res = fstat(fileno(MFILE->priv), &buf);
    if(res != 0) {
        return -1;
    }
    return (int64_t)buf.st_size;
}

static int
mpeg_file_eof_reached(mpeg_file_t *MFILE) {
    assert(NULL != MFILE);

    return (feof(MFILE->priv)) ?TRUE :FALSE;
}

mpeg_file_t*
mpeg_file_open(const char *filename, const char *mode)
{
    mpeg_file_t *MFILE = mpeg_mallocz(sizeof(mpeg_file_t));
    FILE *f = NULL;
    if(NULL != MFILE) {
        f = fopen(filename, mode);
        if(NULL != f) {
            MFILE->streamed = FALSE;
            MFILE->priv = f;
            
            MFILE->read = mpeg_file_read;
            MFILE->write = mpeg_file_write;
            MFILE->seek = mpeg_file_seek;
            MFILE->tell = mpeg_file_tell;

            MFILE->get_size = mpeg_file_get_size;
            MFILE->eof_reached = mpeg_file_eof_reached;
        }
    }
    return MFILE;
}

mpeg_file_t*
mpeg_file_open_link(FILE *f)
{
    mpeg_file_t *MFILE = mpeg_mallocz(sizeof(mpeg_file_t));
    MFILE->streamed = TRUE;
    MFILE->priv = f;
        
    MFILE->read = mpeg_file_read;
    MFILE->write = mpeg_file_write;
    MFILE->seek = mpeg_file_seek;
    MFILE->tell = mpeg_file_tell;
            
    MFILE->get_size = mpeg_file_get_size;
    return MFILE;
}

int
mpeg_file_close(mpeg_file_t *MFILE) {
    int err = 0;
    
    assert(NULL != MFILE);
    assert(NULL != MFILE->priv);
    
    if(!MFILE->streamed) {
        err = fclose(MFILE->priv);
    }
    if(!err) {
        mpeg_free(MFILE);
    }
    return err;
}
    
static const char*
mpeg_log_level_string(int level) {
    const char *str = NULL;
    switch(level) {
    case MPEG_LOG_ERR:
        str = "[ERR] ";
        break;
    case MPEG_LOG_WARN:
        str = "[WRN] ";
        break;
    case MPEG_LOG_INFO:
        str = "[INF] ";
        break;
    default:
        str = "[???] ";
        break;
    }
    return str;
}

mpeg_res_t 
mpeg_log_file(void *dest, int level, const char *fmt, va_list ap) {
    fputs(mpeg_log_level_string(level), dest);
    vfprintf(dest, fmt, ap);
    return MPEG_OK;
}


mpeg_res_t
mpeg_log_null(void *dest, int level, const char *fmt, va_list ap) {
    return MPEG_OK;
}

void*
_mpeg_real_malloc(const char *where, int at, size_t size) {
    void *ptr = NULL;
    if(size > 0) {
        ptr = mem_acquire(size);
    }
    if(NULL == ptr) {
        mpeg_log(MPEG_LOG_ERR, "(%s) at line %i: "
                "unable to alloc %lu bytes\n",
                where, at, (unsigned long)size);
    }
    return ptr;
}
    
void*
_mpeg_real_mallocv(const char *where, int at, size_t size, uint8_t val) {
    void *ptr = _mpeg_real_malloc(where, at, size);
    if(NULL != ptr) {
        memset(ptr, val, size);
    }
    return ptr;
}

void
mpeg_free(void *ptr) {
    mem_release(ptr);   
}

void 
mpeg_fraction_reduce(mpeg_fraction_t *f)
{
    int n = f->num;
    int d = f->den;

    while(n > 0) {
        int t = n;
        n = d % n;
        d = t;
    }

    f->num /= d;
    f->den /= d;
}

