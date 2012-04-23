#include <sys/types.h>
#include <stdint.h>
#include <errno.h>
#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <str_util.h>
#include "procinfo.h"

enum{
	_PROCINFO_NAME,
	_PROCINFO_SVR,
	_PROCINFO_IOPORT,
	_PROCINFO_CMDPORT,
	_PROCINFO_BIN,
	_PROCINFO_ATTR,
	_PROCINFO_COLUME_NUM,
};

typedef int (*valid_func_t)(const char*);

static const procline_t* _find_procline(const procinfo_t *src,const procline_t *tgt);
static int _parse_procinfo_line(procline_t *dest,const char *str);
static int _cmp_procline(const procline_t *p1,const procline_t *p2);
static int _is_active_line(const char *str);
static int _get_filep_line_num(FILE *fp);
static int _get_column(const char *str,int *start,int *len);
static int _is_not_null_or_empty(const char *str);
static int _validate_port(const char *str);
static void _destroy_procline(procline_t *procline);

int load_procinfo(procinfo_t *dest,const char *path)
{
	procline_t *line = NULL;
	FILE *fp = NULL;
	char buf[1024];
	int ix;

	if( NULL == dest || NULL == path ){
		goto error;
	}

	fp = fopen(path,"r");
	if( NULL == fp ){
		goto error;
	}

	dest->num = _get_filep_line_num(fp);
	if( dest->num <= 0 ){
		goto error;
	}

	line = (procline_t*)malloc(sizeof(procline_t) * dest->num);
	if( NULL == line ){
		goto error;
	}

	memset(line,'\0',sizeof(procline_t) * dest->num);
	for( ix = 0; ix < dest->num && fgets(buf,sizeof(buf),fp); ){
		if( _parse_procinfo_line(&line[ix],buf) > 0 ){
			line[ix].index = ix;
			ix++;
		}
	}

	if( ferror(fp) ){
		dest->num = 0;
		goto error;
	}

	dest->num = ix;
	if( fp != NULL ){
		fclose(fp); fp = NULL;
	}

	return(dest->num);

 error:
	if( fp != NULL ){
		fclose(fp); fp = NULL;
	}

	free(line); line = NULL;
	dest->num = 0;
	return(0);
}

const procline_t* get_procline(const procinfo_t *src,const char *str)
{
	const procline_t *rc = NULL;
	procline_t line;

	if( NULL == src || NULL == str ){
		return(NULL);
	}

	memset(&line,'\0',sizeof(line));
	if( _parse_procinfo_line(&line,str) <= 0 ){
		return(NULL);
	}

	rc = _find_procline(src,&line);
	_destroy_procline(&line);
	return(rc);
}

int get_procline_str(char *dest,const procline_t *src)
{
	int rc;

	if( NULL == dest || NULL == src ){
		return(-1);
	}

	rc = sprintf(dest,"%s %s %s %s %s %s",src->name
										,src->svr
										,src->ioport_str
										,src->cmdport_str
										,src->bin_name
										,src->attr);
	return(rc);
}

const procline_t* get_procline_by_bin(const procinfo_t *src,const char *tgt)
{
	int ix;

	if( NULL == src || NULL == tgt ){
		return(NULL);
	}

	for( ix = 0; ix < src->num; ix++ ){
		if( strcmp(src->line[ix].bin_name,tgt) == 0 ){
			return(&src->line[ix]);
		}
	}

	return(NULL);
}

const procline_t* get_procline_by_name(const procinfo_t *src,const char *tgt)
{
	int ix;

	if( NULL == src || NULL == tgt ){
		return(NULL);
	}

	for( ix = 0; ix < src->num; ix++ ){
		if( strcmp(src->line[ix].name,tgt) == 0 ){
			return(&src->line[ix]);
		}
	}

	return(NULL);
}

void destroy_procinfo(procinfo_t *procinfo)
{
	int ix;

	if( NULL == procinfo ){
		return;
	}

	for(ix = 0; ix < procinfo->num; ix++ ){
		_destroy_procline(&procinfo->line[ix]);
	}

	return;
}

static const procline_t* _find_procline(const procinfo_t *src,const procline_t *tgt)
{
	int ix;

	if(NULL == src || NULL == tgt ){
		return(NULL);
	}

	for( ix = 0; ix < src->num; ix++ ){
		if( _cmp_procline(&src->line[ix],tgt) == 0 ){
			return(&src->line[ix]);
		}
	}

	return(NULL);
}

static int _parse_procinfo_line(procline_t *dest,const char *str)
{
	int start;
	int len;
	int ix;
	int jx;
	int ret;
	char **dest_ptr;
	valid_func_t valid_func_ptr = NULL;

	if( NULL == dest || NULL == str ){
		return(-1);
	}

	if( !_is_active_line(str) ){
		return(0);
	}

	memset(dest,'\0',sizeof(*dest));
	dest->valid = 0;
	for(ix = 0, jx = 0; ix < _PROCINFO_COLUME_NUM; ix++, jx += ret ){
		ret = _get_column(&str[jx],&start,&len);
		switch( ix ){
		case _PROCINFO_NAME:
			dest_ptr = &dest->name;
			valid_func_ptr = _is_not_null_or_empty;
			break;
		case _PROCINFO_SVR:
			dest_ptr = &dest->svr;
			valid_func_ptr = _is_not_null_or_empty;
			break;
		case _PROCINFO_IOPORT:
			dest_ptr = &dest->ioport_str;
			valid_func_ptr = _validate_port;
			break;
		case _PROCINFO_CMDPORT:
			dest_ptr = &dest->cmdport_str;
			valid_func_ptr = _validate_port;
			break;
		case _PROCINFO_BIN:
			dest_ptr = &dest->bin_name;
			valid_func_ptr = _is_not_null_or_empty;
			break;
		case _PROCINFO_ATTR:
			dest_ptr = &dest->attr;
			valid_func_ptr = NULL;
			break;
		}

		*dest_ptr = (char*)malloc(len + 1);
		if( NULL == *dest_ptr ){
			goto error;
		}
		memcpy(*dest_ptr,&str[jx + start],len);
		(*dest_ptr)[len] = '\0';
		if( valid_func_ptr != NULL && !valid_func_ptr(*dest_ptr) ){
			goto error;
		}
	}

	dest->ioport = strtol(dest->ioport_str,NULL,10);
	dest->cmdport = strtol(dest->cmdport_str,NULL,10);
	dest->valid = 1;
	return(1);

 error:
	_destroy_procline(dest);
	return(0);
}

static int _cmp_procline(const procline_t *p1,const procline_t *p2)
{
	int ret;

	ret = strcmp(p1->name,p2->name);
	if( ret != 0 ){
		return(ret);
	}

	ret = strcmp(p1->svr,p2->svr);
	if( ret != 0 ){
		return(ret);
	}

	ret = strcmp(p1->ioport_str,p2->ioport_str);
	if( ret != 0 ){
		return(ret);
	}

	ret = strcmp(p1->cmdport_str,p2->cmdport_str);
	if( ret != 0 ){
		return(ret);
	}

	ret = strcmp(p1->bin_name,p2->bin_name);
	if( ret != 0 ){
		return(ret);
	}

	ret = strcmp(p1->attr,p2->attr);
	if( ret != 0 ){
		return(ret);
	}

	return(0);
}

static int _is_active_line(const char *str)
{
	if( '\0' == *str || '#' == *str || '\r' == *str || '\n' == *str ){
		return(0);
	}

	return(1);
}

static int _get_filep_line_num(FILE *fp)
{
	int rc = 0;
	char buf[1024];

	while( fgets(buf,sizeof(buf),fp) ){
		if( _is_active_line(buf) ){
			rc++;
		}
	}

	if( ferror(fp) ){
		rc = -1;
	}

	rewind(fp);
	return(rc);
}

static int _get_column(const char *str,int *start,int *len)
{
	int ix;

	if( '\0' == *str || '\r' == *str || '\n' == *str ){
		*start = 0;
		*len = 0;
		return(0);
	}

	for( ix = 0; isspace(str[ix]); ix++ ){ ; }
	for( *start = ix; !isspace(str[ix]); ix++ ){ ; }
	*len = ix - *start;
	return(ix);
}

static int _is_not_null_or_empty(const char *str)
{
	return(!nax_is_null_or_empty(str));
}

static int _validate_port(const char *str)
{
	int ix;

	if( NULL == str || '\0' == *str ){
		return(0);
	}

	for( ix = 0; str[ix] != '\0'; ix++ ){
		if( !isdigit(str[ix]) ){
			return(0);
		}
	}

	return(1);
}

static void _destroy_procline(procline_t *procline)
{
	if( NULL == procline ){
		return;
	}

	free(procline->name); procline->name = NULL;
	free(procline->svr); procline->svr = NULL;
	free(procline->ioport_str); procline->ioport_str = NULL;
	free(procline->cmdport_str); procline->cmdport_str = NULL;
	free(procline->bin_name); procline->bin_name = NULL;
	free(procline->attr); procline->attr = NULL;
	procline->index = -1;
	procline->valid = 0;
	procline->ioport = -1;
	procline->cmdport = -1;
	return;
}

