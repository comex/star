#pragma once
#define _COMMON_H
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
#include <sys/sysctl.h>
#include <signal.h>
#define ctassert(x, y) extern char XX_## y [(x) ? 1 : -1]
typedef unsigned long long ull;
typedef unsigned long ul;
#define TRY(name, x) _try(#name, (x))
#define TRY2(name, arg, x) _try2(#name, (arg), (x))
#define AST(name, x) _try(#name, !(x))
#define AST2(name, arg, x) _try2(#name, (arg), !(x))

#ifdef LOG_FP
extern FILE *log_fp;
#define E(fmt, args...) do { fprintf(log_fp, fmt "\n", ##args); fflush(log_fp); } while(0)
#define I(fmt, args...) do { fprintf(log_fp, fmt "\n", ##args); fflush(log_fp); } while(0)
#else
#include <syslog.h>
#define E(args...) syslog(LOG_EMERG, args)
#define I(args...) syslog(LOG_EMERG, args)
#endif
#if DEBUG
#define TIME(thing) do { ull _ta = getms(); thing; ull _tb = getms(); I("[%.4ld ms] %s", (long int) (_tb - _ta), #thing); } while(0)
#else
#define TIME(thing) thing
#endif

static inline void _try(const char name[], int err) {
    if(err) {
        E("%s failed: (%d,%d) %s\n", name, err, errno, strerror(errno));
        exit(1);
    }
}

static inline void _try2(const char name[], const char arg[], int err) {
    if(err) {
        E("%s[%s] failed: (%d,%d) %s\n", name, arg, err, errno, strerror(errno));
        exit(1);
    }
}

static inline ull getms() {
    struct timeval tp;
    gettimeofday(&tp, NULL);
    return ((ull) tp.tv_sec) * 1000000 + (ull) tp.tv_usec;
}

struct buf { char *data; size_t len; };
static struct buf r(char *filename, int string) {
    FILE *fp = fopen(filename, "rb");
    if(!fp) abort();
    fseek(fp, 0, SEEK_END);
    struct buf buf;
    buf.len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    buf.data = malloc(buf.len + 1);
    assert(1 == fread(buf.data, buf.len, 1, fp));
    if(string) {
        buf.data[buf.len] = 0;
        if(buf.len > 0 && buf.data[buf.len-1] == '\n') buf.data[buf.len-1] = 0;
    }
    fclose(fp);
    return buf;
}
static void w(struct buf buf, char *filename, int string) {
    FILE *fp = fopen(filename, "wb");
    if(!fp) abort();
    if(string) {
        if(buf.len > 0 && buf.data[buf.len-1] == '\0') buf.len--;
    }
    assert(1 == fwrite(buf.data, buf.len, 1, fp));
    fclose(fp);
}
static void hex_dump(void *data, int size)
{
    /* dumps size bytes of *data to stdout. Looks like:
     * [0000] 75 6E 6B 6E 6F 77 6E 20
     *                  30 FF 00 00 00 00 39 00 unknown 0.....9.
     * (in a single line of course)
     */

    unsigned char *p = data;
    unsigned char c;
    int n;
    char bytestr[4] = {0};
    char addrstr[10] = {0};
    char hexstr[ 16*3 + 5] = {0};
    char charstr[16*1 + 5] = {0};
    for(n=1;n<=size;n++) {
        if (n%16 == 1) {
            /* store address for this line */
            snprintf(addrstr, sizeof(addrstr), "%.4x",
               ((unsigned int)p-(unsigned int)data) );
        }
            
        c = *p;
        if (isalnum(c) == 0) {
            c = '.';
        }

        /* store hex str (for left side) */
        snprintf(bytestr, sizeof(bytestr), "%02X ", *p);
        strncat(hexstr, bytestr, sizeof(hexstr)-strlen(hexstr)-1);

        /* store char str (for right side) */
        snprintf(bytestr, sizeof(bytestr), "%c", c);
        strncat(charstr, bytestr, sizeof(charstr)-strlen(charstr)-1);

        if(n%16 == 0) { 
            /* line completed */
            printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
            hexstr[0] = 0;
            charstr[0] = 0;
        } else if(n%8 == 0) {
            /* half line: add whitespaces */
            strncat(hexstr, "  ", sizeof(hexstr)-strlen(hexstr)-1);
            strncat(charstr, " ", sizeof(charstr)-strlen(charstr)-1);
        }
        p++; /* next byte */
    }

    if (strlen(hexstr) > 0) {
        /* print rest of buffer if not empty */
        printf("[%4.4s]   %-50.50s  %s\n", addrstr, hexstr, charstr);
    }
}

static void killall(const char *name) {
    int mib[3];
    size_t size;
    mib[0] = CTL_KERN;
    mib[1] = KERN_PROC;
    mib[2] = KERN_PROC_ALL;
    assert(!sysctl(mib, 3, NULL, &size, NULL, 0));
    struct kinfo_proc *data = malloc(size);
    assert(!sysctl(mib, 3, data, &size, NULL, 0));
    int i;
    for(i = 0; i < (size / sizeof(*data)); i++) {
        if(!strncmp(data[i].kp_proc.p_comm, name, MAXCOMLEN)) {
            syslog(LOG_WARNING, "Killing pid %d", data[i].kp_proc.p_pid);
            kill(data[i].kp_proc.p_pid, SIGTERM);
        }
    }
    free(data);
}

#ifdef CFCOMMON
#include <CoreFoundation/CoreFoundation.h>

static CFDataRef cr(const char *fn) {
    FILE *fp = fopen(fn, "rb");
    AST2(r_open, fn, fp);
    fseek(fp, 0, SEEK_END);
    size_t len = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    void *buf = malloc(len);
    AST2(r_read, fn, len == fread(buf, 1, len, fp));
    return CFDataCreateWithBytesNoCopy(NULL, buf, len, NULL);
    // it will free it itself
}

static void cw(const char *fn, CFDataRef data) {
    char *temp;
    asprintf(&temp, "%s_temp", fn);
    FILE *fp = fopen(temp, "wb");
    AST2(w_open, fn, fp);
    size_t len = CFDataGetLength(data);
    AST2(r_write, fn, len == fwrite(CFDataGetBytePtr(data), 1, len, fp));
    fclose(fp);
    TRY2(r_rename, fn, rename(temp, fn));
    free(temp);
}
#endif 
