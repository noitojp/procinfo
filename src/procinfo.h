#ifndef __PROCINFO_H_
#define __PROCINFO_H_

typedef struct{
	int index;
	int valid;
	char *line;
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
extern const procinfo_t* get_procinfo(const procinfo_t *src,const char *str);
extern int get_procinfo_str(char *dest,const procinfo_t *src);
extern const procinfo_t* get_procinfo_by_bin(const procinfo_t *src,const char *tgt);
extern const procinfo_t* get_procinfo_by_name(const procinfo_t *src,const char *tgt);
extern void destroy_procinfo(procinfo_t *procinfo);

#ifdef __cplusplus
}
#endif

#endif
