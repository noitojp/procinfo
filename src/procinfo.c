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

static const procinfo_t* _find_procinfo(const procinfo_t *src,const procinfo_t *tgt);
static int _parse_procinfo_line(procinfo_t *dest,const char *str);
static int _cmp_procinfo(const procinfo_t *p1,const procinfo_t *p2);
static int _is_active_line(const char *str);
static int _get_filep_line_num(FILE *fp);
static int _get_column_index(char **dest,char *src,int maxcol);
static int _is_not_null_or_empty(const char *str);
static int _validate_port(const char *str);
static void _destroy_procinfo(procinfo_t *procinfo);

int load_procinfo(procinfo_t **dest,const char *path)
{
	FILE *fp = NULL;
	char buf[4096];
	int num = 0;
	int ix;

	if( NULL == dest || NULL == path ){
		goto error;
	}

	*dest = NULL;
	fp = fopen(path,"r");
	if( NULL == fp ){
		fprintf(stderr,"ERROR: can't open %s: %s\n",path,strerror(errno));
		goto error;
	}

	num = _get_filep_line_num(fp);
	if( num <= 0 ){
		fprintf(stderr,"ERROR: %s has no active record\n",path);
		goto error;
	}

	*dest = (procinfo_t*)malloc(sizeof(procinfo_t) * (num + 1));
	if( NULL == *dest ){
		fprintf(stderr,"ERROR: malloc: %s \n",strerror(errno));
		goto error;
	}

	memset(*dest,'\0',sizeof(procinfo_t) * (num + 1));
	for( ix = 0; ix < num && fgets(buf,sizeof(buf),fp); ){
		if( nax_rstrip(buf,"\r\n") <= 0 ){ continue; }
		if( _parse_procinfo_line(&((*dest)[ix]),buf) <= 0 ){ continue; }
		((*dest)[ix]).index = ix;
		ix++;
	}

	((*dest)[ix]).valid = 0;
	((*dest)[ix]).line =NULL;
	if( ferror(fp) ){
		fprintf(stderr,"ERROR: can't read %s: %s\n",path,strerror(errno));
		goto error;
	}

	if( fp != NULL ){
		fclose(fp); fp = NULL;
	}

	return(ix);

 error:
	if( fp != NULL ){
		fclose(fp); fp = NULL;
	}

	destroy_procinfo(*dest); *dest = NULL;
	return(0);
}

const procinfo_t* get_procinfo(const procinfo_t *src,const char *str)
{
	const procinfo_t *rc = NULL;
	procinfo_t line;

	if( NULL == src || NULL == str ){
		return(NULL);
	}

	memset(&line,'\0',sizeof(line));
	if( _parse_procinfo_line(&line,str) <= 0 ){
		return(NULL);
	}

	rc = _find_procinfo(src,&line);
	_destroy_procinfo(&line);
	return(rc);
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

const procinfo_t* get_procinfo_by_bin(const procinfo_t *src,const char *tgt)
{
	int ix;

	if( NULL == src || NULL == tgt ){
		return(NULL);
	}

	for( ix = 0; src[ix].line != NULL; ix++ ){
		if( strcmp(src[ix].bin_name,tgt) == 0 ){
			return(&src[ix]);
		}
	}

	return(NULL);
}

const procinfo_t* get_procinfo_by_name(const procinfo_t *src,const char *tgt)
{
	int ix;

	if( NULL == src || NULL == tgt ){
		return(NULL);
	}

	for( ix = 0; src[ix].line != NULL; ix++ ){
		if( strcmp(src[ix].name,tgt) == 0 ){
			return(&src[ix]);
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

	for(ix = 0; procinfo[ix].line != NULL; ix++ ){
		_destroy_procinfo(&procinfo[ix]);
	}

	free(procinfo);
	return;
}

static const procinfo_t* _find_procinfo(const procinfo_t *src,const procinfo_t *tgt)
{
	int ix;

	for( ix = 0; src[ix].line != NULL; ix++ ){
		if( _cmp_procinfo(&src[ix],tgt) == 0 ){
			return(&src[ix]);
		}
	}

	return(NULL);
}

static int _parse_procinfo_line(procinfo_t *dest,const char *src)
{
	int ix;
	int ret;
	char *str = NULL;
	char **dest_ptr;
	valid_func_t valid_func_ptr = NULL;
	char *index[_PROCINFO_COLUME_NUM];

	if( !_is_active_line(src) ){
		goto error;
	}

	memset(dest,'\0',sizeof(dest));
	dest->valid = 0;
	str = strdup(src);
	ret = _get_column_index(index,str,_PROCINFO_COLUME_NUM);
	if( ret != _PROCINFO_COLUME_NUM ){
		goto error;
	}

	for(ix = 0; ix < _PROCINFO_COLUME_NUM; ix++ ){
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

		*dest_ptr = index[ix];
		if( valid_func_ptr != NULL && !valid_func_ptr(*dest_ptr) ){
			goto error;
		}
	}

	dest->ioport = strtol(dest->ioport_str,NULL,10);
	dest->cmdport = strtol(dest->cmdport_str,NULL,10);
	dest->line = str;
	dest->valid = 1;
	return(1);

 error:
	free(str); str = NULL;
	return(0);
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

static int _get_column_index(char **dest,char *src,int maxcol)
{
	int ix;
	int jx;

	for( ix = 0, jx = 0; src[ix] != '\0'; ){
		while(  ' ' == src[ix] || '\t' == src[ix] ){
			src[ix++] = '\0';
		}

		if( jx < maxcol ){
			dest[jx++] = &src[ix];
		}

		while( src[ix] != '\0' && src[ix] != ' ' && src[ix] != '\t' ){
			ix++;
		}
	}

	return(jx);
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

static void _destroy_procinfo(procinfo_t *procinfo)
{
	if( NULL == procinfo ){
		return;
	}

	free(procinfo->line); procinfo->line = NULL;
	procinfo->valid = 0;
	return;
}

