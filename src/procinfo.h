#ifndef __PROCINFO_H_
#define __PROCINFO_H_

#include <stdint.h>

typedef struct{
	int32_t index;
	int32_t valid;
	char *name;
	char *svr;
	char *ioport_str;
	char *cmdport_str;
	char *bin_name;
	char *attr;
	int ioport;
	int cmdport;
} procline_t;

typedef struct{
	int num;
	procline_t *line;
} procinfo_t;

#ifdef __cplusplus
extern "C" {
#endif

extern int load_procinfo(procinfo_t *dest,const char *path);
extern int parse_procinfo_line(procline_t *dest,const char *str);
extern const procline_t* get_procline(const procinfo_t *src,const procline_t *tgt);
extern int get_procline_str(char *dest,const procline_t *src);
extern const procline_t* get_procline_by_bin(const procinfo_t *src,const char *tgt);
extern const procline_t* get_procline_by_name(const procinfo_t *src,const char *tgt);
extern void destroy_procinfo(procinfo_t *procinfo);

#ifdef __cplusplus
}
#endif

#endif
