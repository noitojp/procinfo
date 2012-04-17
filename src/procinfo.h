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
} procinfo_t;

#ifdef __cplusplus
extern "C" {
#endif

extern int load_procinfo(procinfo_t **dest,const char *path);
extern int parse_procinfo_line(procinfo_t *dest,const char *str);
extern int get_procinfo_index(const procinfo_t *arr,const procinfo_t *tgt,int num);
extern int get_procinfo_str(char *dest,const procinfo_t *src);
extern int get_procinfo_index_by_bin(const procinfo_t *arr,const char *tgt,int num);
extern int get_procinfo_index_by_name(const procinfo_t *arr,const char *tgt,int num);
extern const procinfo_t* get_procinfo_ptr_by_bin(const procinfo_t *arr,const char *tgt,int num);
extern const procinfo_t* get_procinfo_ptr_by_name(const procinfo_t *arr,const char *tgt,int num);
extern void destroy_procinfo(procinfo_t *procinfo);

#ifdef __cplusplus
}
#endif

#endif
