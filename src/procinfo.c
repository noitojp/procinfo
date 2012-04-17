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

typedef int (valid_func_t)(const char*);

static int _cmp_procinfo(const procinfo_t *p1,const procinfo_t *p2);
static int _is_active_line(const char *str);
static int _get_filep_line_num(FILE *fp);
static int _get_column(const char *str,int *start,int *len);
static int _is_not_null_or_empty(const char *str);
static int _validate_port(const char *str);

int load_procinfo(procinfo_t **dest,const char *path)
{
	int rc = 1;
	FILE *fp = NULL;
	char buf[1024];
	int line_num;
	int ix;

	if( NULL == dest || NULL == path ){
		return(0);
	}

	*dest = NULL;
	fp = fopen(path,"r");
	if( NULL == fp ){
		rc = 0;
		goto finally;
	}

	line_num = _get_filep_line_num(fp);
	if( line_num <= 0 ){
		rc = 0;
		goto finally;
	}

	*dest = (procinfo_t*)malloc(sizeof(procinfo_t) * line_num);
	if( NULL == *dest ){
		rc = 0;
		goto finally;
	}

	memset(*dest,'\0',sizeof(procinfo_t) * line_num);
	for( ix = 0; ix < line_num && fgets(buf,sizeof(buf),fp); ){
		if( parse_procinfo_line(&((*dest)[ix]),buf) > 0 ){
			(*dest)[ix].index = ix;
			ix++;
		}
	}

	if( ferror(fp) ){
		rc = 0;
		goto finally;
	}

	rc = ix;

 finally:
	if( fp != NULL ){
		fclose(fp); fp = NULL;
	}

	return(rc);
}

int parse_procinfo_line(procinfo_t *dest,const char *str)
{
	int start;
	int len;
	int ix;
	int jx;
	int ret;
	char **dest_ptr;
	valid_func_t *valid_func_ptr = NULL;

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
			goto finally;
		}
		memcpy(*dest_ptr,&str[jx + start],len);
		(*dest_ptr)[len] = '\0';
		if( valid_func_ptr != NULL && !valid_func_ptr(*dest_ptr) ){
			goto finally;
		}
	}

	dest->ioport = strtol(dest->ioport_str,NULL,10);
	dest->cmdport = strtol(dest->cmdport_str,NULL,10);
	dest->valid = 1;
 finally:
	return(1);
}

int get_procinfo_index(const procinfo_t *arr,const procinfo_t *tgt,int num)
{
	int ix;

	for( ix = 0; ix < num; ix++ ){
		if( _cmp_procinfo(&arr[ix],tgt) == 0 ){
			return(ix);
		}
	}

	return(-1);
}

int get_procinfo_str(char *dest,const procinfo_t *src)
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

int get_procinfo_index_by_bin(const procinfo_t *arr,const char *tgt,int num)
{
	int ix;

	for( ix = 0; ix < num; ix++ ){
		if( strcmp(arr[ix].bin_name,tgt) == 0 ){
			return(ix);
		}
	}

	return(-1);
}

int get_procinfo_index_by_name(const procinfo_t *arr,const char *tgt,int num)
{
	int ix;

	for( ix = 0; ix < num; ix++ ){
		if( strcmp(arr[ix].name,tgt) == 0 ){
			return(ix);
		}
	}

	return(-1);
}

const procinfo_t* get_procinfo_ptr_by_bin(const procinfo_t *arr,const char *tgt,int num)
{
	int index;

	index = get_procinfo_index_by_bin(arr,tgt,num);
	if( index < 0 ){
		return(NULL);
	}

	return(&arr[index]);
}

const procinfo_t* get_procinfo_ptr_by_name(const procinfo_t *arr,const char *tgt,int num)
{
	int index;

	index = get_procinfo_index_by_name(arr,tgt,num);
	if( index < 0 ){
		return(NULL);
	}

	return(&arr[index]);
}

void destroy_procinfo(procinfo_t *procinfo)
{
	if( NULL == procinfo ){
		return;
	}

	free(procinfo->name); procinfo->name = NULL;
	free(procinfo->svr); procinfo->svr = NULL;
	free(procinfo->ioport_str); procinfo->ioport_str = NULL;
	free(procinfo->cmdport_str); procinfo->cmdport_str = NULL;
	free(procinfo->bin_name); procinfo->bin_name = NULL;
	free(procinfo->attr); procinfo->attr = NULL;
	procinfo->index = -1;
	procinfo->valid = 0;
	procinfo->ioport = -1;
	procinfo->cmdport = -1;
	return;
}

static int _cmp_procinfo(const procinfo_t *p1,const procinfo_t *p2)
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

