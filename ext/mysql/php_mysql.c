/*
   +----------------------------------------------------------------------+
   | PHP version 4.0                                                      |
   +----------------------------------------------------------------------+
   | Copyright (c) 1997, 1998, 1999 The PHP Group                         |
   +----------------------------------------------------------------------+
   | This source file is subject to version 2.0 of the PHP license,       |
   | that is bundled with this package in the file LICENSE, and is        |
   | available at through the world-wide-web at                           |
   | http://www.php.net/license/2_0.txt.                                  |
   | If you did not receive a copy of the PHP license and are unable to   |
   | obtain it through the world-wide-web, please send a note to          |
   | license@php.net so we can mail you a copy immediately.               |
   +----------------------------------------------------------------------+
   | Authors: Zeev Suraski <zeev@zend.com>                                |
   +----------------------------------------------------------------------+
*/
 
/* $Id$ */


/* TODO:
 *
 * ? Safe mode implementation
 */

#include "php.h"
#include "php_globals.h"
#include "ext/standard/php_standard.h"
#include "php_mysql.h"
#include "php_globals.h"


#if WIN32|WINNT
#include <winsock.h>
#else
#include "build-defs.h"

#if HAVE_SYS_TYPES_H
#include <sys/types.h>
#endif
#include <netdb.h>
#include <netinet/in.h>
#endif


/* True globals, no need for thread safety */
static int le_result,le_link,le_plink;

#include "php_ini.h"

#if HAVE_MYSQL
# if HAVE_MYSQL_MYSQL_H
#  include <mysql/mysql.h>
# else
#  include <mysql.h>
# endif
#ifdef HAVE_MYSQL_REAL_CONNECT
#ifdef HAVE_ERRMSG_H
#include <errmsg.h>
#endif
#endif

#define SAFE_STRING(s) ((s)?(s):"")

#if MYSQL_VERSION_ID > 32199
#define mysql_row_length_type unsigned long
#define HAVE_MYSQL_ERRNO
#else
#define mysql_row_length_type unsigned int
#	ifdef mysql_errno
#	define HAVE_MYSQL_ERRNO
#	endif
#endif

#define MYSQL_ASSOC		1<<0
#define MYSQL_NUM		1<<1
#define MYSQL_BOTH		(MYSQL_ASSOC|MYSQL_NUM)

#if MYSQL_VERSION_ID < 32224
#define PHP_MYSQL_VALID_RESULT(mysql)		\
	(mysql_num_fields(mysql)>0)
#else
#define PHP_MYSQL_VALID_RESULT(mysql)		\
	(mysql_field_count(mysql)>0)
#endif

function_entry mysql_functions[] = {
	PHP_FE(mysql_connect,								NULL)
	PHP_FE(mysql_pconnect,								NULL)
	PHP_FE(mysql_close,									NULL)
	PHP_FE(mysql_select_db,								NULL)
	PHP_FE(mysql_create_db,								NULL)
	PHP_FE(mysql_drop_db,								NULL)
	PHP_FE(mysql_query,									NULL)
	PHP_FE(mysql_db_query,								NULL)
	PHP_FE(mysql_list_dbs,								NULL)
	PHP_FE(mysql_list_tables,							NULL)
	PHP_FE(mysql_list_fields,							NULL)
	PHP_FE(mysql_error,									NULL)
#ifdef HAVE_MYSQL_ERRNO
	PHP_FE(mysql_errno,									NULL)
#endif
	PHP_FE(mysql_affected_rows,							NULL)
	PHP_FE(mysql_insert_id,								NULL)
	PHP_FE(mysql_result,								NULL)
	PHP_FE(mysql_num_rows,								NULL)
	PHP_FE(mysql_num_fields,							NULL)
	PHP_FE(mysql_fetch_row,								NULL)
	PHP_FE(mysql_fetch_array,							NULL)
	PHP_FE(mysql_fetch_object,							NULL)
	PHP_FE(mysql_data_seek,								NULL)
	PHP_FE(mysql_fetch_lengths,							NULL)
	PHP_FE(mysql_fetch_field,							NULL)
	PHP_FE(mysql_field_seek,							NULL)
	PHP_FE(mysql_free_result,							NULL)
	PHP_FE(mysql_field_name,							NULL)
	PHP_FE(mysql_field_table,							NULL)
	PHP_FE(mysql_field_len,								NULL)
	PHP_FE(mysql_field_type,							NULL)
	PHP_FE(mysql_field_flags,							NULL)
	 
	/* for downwards compatability */
	PHP_FALIAS(mysql,				mysql_db_query,		NULL)
	PHP_FALIAS(mysql_fieldname,		mysql_field_name,	NULL)
	PHP_FALIAS(mysql_fieldtable,	mysql_field_table,	NULL)
	PHP_FALIAS(mysql_fieldlen,		mysql_field_len,	NULL)
	PHP_FALIAS(mysql_fieldtype,		mysql_field_type,	NULL)
	PHP_FALIAS(mysql_fieldflags,	mysql_field_flags,	NULL)
	PHP_FALIAS(mysql_selectdb,		mysql_select_db,	NULL)
	PHP_FALIAS(mysql_createdb,		mysql_create_db,	NULL)
	PHP_FALIAS(mysql_dropdb,		mysql_drop_db,		NULL)
	PHP_FALIAS(mysql_freeresult,	mysql_free_result,	NULL)
	PHP_FALIAS(mysql_numfields,		mysql_num_fields,	NULL)
	PHP_FALIAS(mysql_numrows,		mysql_num_rows,		NULL)
	PHP_FALIAS(mysql_listdbs,		mysql_list_dbs,		NULL)
	PHP_FALIAS(mysql_listtables,	mysql_list_tables,	NULL)
	PHP_FALIAS(mysql_listfields,	mysql_list_fields,	NULL)
	PHP_FALIAS(mysql_db_name,		mysql_result,		NULL)
	PHP_FALIAS(mysql_dbname,		mysql_result,		NULL)
	PHP_FALIAS(mysql_tablename,		mysql_result,		NULL)
	{NULL, NULL, NULL}
};

zend_module_entry mysql_module_entry = {
	"MySQL", mysql_functions, PHP_MINIT(mysql), PHP_MSHUTDOWN(mysql), PHP_RINIT(mysql), NULL, 
			 PHP_MINFO(mysql), STANDARD_MODULE_PROPERTIES
};

#ifdef ZTS
int mysql_globals_id;
#else
PHP_MYSQL_API php_mysql_globals mysql_globals;
#endif

#ifdef COMPILE_DL_MYSQL
# include "dl/phpdl.h"
DLEXPORT zend_module_entry *get_module(void) { return &mysql_module_entry; }
#endif

#if APACHE
void timeout(int sig);
#endif

#define CHECK_LINK(link) { if (link==-1) { php_error(E_WARNING,"MySQL:  A link to the server could not be established"); RETURN_FALSE; } }

/* NOTE  Don't ask me why, but soon as I made this the list
 * destructor, I stoped getting access violations in windows
 * with mysql 3.22.7a
 */
static void _free_mysql_result(MYSQL_RES *mysql_result){
	mysql_free_result(mysql_result);
}


static void php_mysql_set_default_link(int id)
{
	MySLS_FETCH();

	if (MySG(default_link)!=-1) {
		zend_list_delete(MySG(default_link));
	}
	MySG(default_link) = id;
	zend_list_addref(id);
}


static void _close_mysql_link(MYSQL *link)
{
#if APACHE
	void (*handler) (int);
#endif
	MySLS_FETCH();

#if APACHE
	handler = signal(SIGPIPE, SIG_IGN);
#endif

	mysql_close(link);

#if APACHE
	signal(SIGPIPE,handler);
#endif

	efree(link);
	MySG(num_links)--;
}

static void _close_mysql_plink(MYSQL *link)
{
#if APACHE
	void (*handler) (int);
#endif
	MySLS_FETCH();

#if APACHE
	handler = signal(SIGPIPE, SIG_IGN);
#endif

	mysql_close(link);

#if APACHE
	signal(SIGPIPE,handler);
#endif

	free(link);
	MySG(num_persistent)--;
	MySG(num_links)--;
}


static PHP_INI_MH(OnMySQLPort)
{
	MySLS_FETCH();
	
	if (new_value==NULL) { /* default port */
#if !(WIN32|WINNT)
		struct servent *serv_ptr;
		char *env;
		
		MySG(default_port) = MYSQL_PORT;
		if ((serv_ptr = getservbyname("mysql", "tcp"))) {
			MySG(default_port) = (uint) ntohs((ushort) serv_ptr->s_port);
		}
		if ((env = getenv("MYSQL_TCP_PORT"))) {
			MySG(default_port) = (uint) atoi(env);
		}
#else
		MySG(default_port) = MYSQL_PORT;
#endif
	} else {
		MySG(default_port) = atoi(new_value);
	}
	return SUCCESS;
}


static PHP_INI_DISP(display_link_numbers)
{
	char *value;

	if (type==PHP_INI_DISPLAY_ORIG && ini_entry->modified) {
		value = ini_entry->orig_value;
	} else if (ini_entry->value) {
		value = ini_entry->value;
	} else {
		value = NULL;
	}

	if (value) {
		if (atoi(value)==-1) {
			PUTS("Unlimited");
		} else {
			php_printf("%s", value);
		}
	}
}


PHP_INI_BEGIN()
	STD_PHP_INI_BOOLEAN("mysql.allow_persistent",	"1",	PHP_INI_SYSTEM,		OnUpdateInt,		allow_persistent,	php_mysql_globals,		mysql_globals)
	STD_PHP_INI_ENTRY_EX("mysql.max_persistent",	"-1",	PHP_INI_SYSTEM,		OnUpdateInt,		max_persistent,		php_mysql_globals,		mysql_globals,	display_link_numbers)
	STD_PHP_INI_ENTRY_EX("mysql.max_links",		"-1",	PHP_INI_SYSTEM,			OnUpdateInt,		max_links,			php_mysql_globals,		mysql_globals,	display_link_numbers)
	STD_PHP_INI_ENTRY("mysql.default_host",		NULL,	PHP_INI_ALL,			OnUpdateString,		default_host,		php_mysql_globals,		mysql_globals)
	STD_PHP_INI_ENTRY("mysql.default_user",		NULL,	PHP_INI_ALL,			OnUpdateString,		default_user,		php_mysql_globals,		mysql_globals)
	STD_PHP_INI_ENTRY("mysql.default_password",	NULL,	PHP_INI_ALL,			OnUpdateString,		default_password,	php_mysql_globals,		mysql_globals)
	PHP_INI_ENTRY("mysql.default_port",		NULL,	PHP_INI_ALL,			OnMySQLPort)
PHP_INI_END()


#ifdef ZTS
static void php_mysql_init_globals(php_mysql_globals *mysql_globals)
{
	MySG(num_persistent) = 0;
}
#endif


PHP_MINIT_FUNCTION(mysql)
{
	ELS_FETCH();

#ifdef ZTS
	mysql_globals_id = ts_allocate_id(sizeof(php_mysql_globals), php_mysql_init_globals, NULL);
#else
	MySG(num_persistent)=0;
#endif

	REGISTER_INI_ENTRIES();
	le_result = register_list_destructors(_free_mysql_result,NULL);
	le_link = register_list_destructors(_close_mysql_link,NULL);
	le_plink = register_list_destructors(NULL,_close_mysql_plink);
	mysql_module_entry.type = type;
	
	REGISTER_LONG_CONSTANT("MYSQL_ASSOC", MYSQL_ASSOC, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQL_NUM", MYSQL_NUM, CONST_CS | CONST_PERSISTENT);
	REGISTER_LONG_CONSTANT("MYSQL_BOTH", MYSQL_BOTH, CONST_CS | CONST_PERSISTENT);

	return SUCCESS;
}


PHP_MSHUTDOWN_FUNCTION(mysql)
{
	UNREGISTER_INI_ENTRIES();
	return SUCCESS;
}


PHP_RINIT_FUNCTION(mysql)
{
	MySLS_FETCH();
	
	MySG(default_link)=-1;
	MySG(num_links) = MySG(num_persistent);
	return SUCCESS;
}


PHP_MINFO_FUNCTION(mysql)
{
	char buf[32];
	MySLS_FETCH();

	DISPLAY_INI_ENTRIES();

	php_printf("<table border=5 width=\"600\">");
	php_info_print_table_header(2, "Key", "Value");
	sprintf(buf, "%ld", MySG(num_persistent));
	php_info_print_table_row(2, "Active Persistent Links", buf);
	sprintf(buf, "%ld", MySG(num_links));
	php_info_print_table_row(2, "Active Links", buf);
	php_info_print_table_row(2, "Client API version", mysql_get_client_info());
#if !(WIN32|WINNT)
	php_info_print_table_row(2, "MYSQL_INCLUDE", PHP_MYSQL_INCLUDE);
	php_info_print_table_row(2, "MYSQL_LFLAGS", PHP_MYSQL_LFLAGS);
	php_info_print_table_row(2, "MYSQL_LIBS", PHP_MYSQL_LIBS);
#endif
	php_printf("</table>\n");
}


static void php_mysql_do_connect(INTERNAL_FUNCTION_PARAMETERS,int persistent)
{
#if APACHE
	void (*handler) (int);
#endif
	char *user,*passwd,*host,*socket=NULL,*tmp;
	char *hashed_details;
	int hashed_details_length,port = MYSQL_PORT;
	MYSQL *mysql;
	MySLS_FETCH();
	PLS_FETCH();

	if (PG(sql_safe_mode)) {
		if (ARG_COUNT(ht)>0) {
			php_error(E_NOTICE,"SQL safe mode in effect - ignoring host/user/password information");
		}
		host=passwd=NULL;
		user=php_get_current_user();
		hashed_details_length = strlen(user)+5+3;
		hashed_details = (char *) emalloc(hashed_details_length+1);
		sprintf(hashed_details,"mysql__%s_",user);
	} else {
		host = MySG(default_host);
		user = MySG(default_user);
		passwd = MySG(default_password);
		
		switch(ARG_COUNT(ht)) {
			case 0: /* defaults */
				break;
			case 1: {
					pval **yyhost;
					
					if (getParametersEx(1, &yyhost)==FAILURE) {
						RETURN_FALSE;
					}
					convert_to_string_ex(yyhost);
					host = (*yyhost)->value.str.val;
				}
				break;
			case 2: {
					pval **yyhost, **yyuser;
					
					if (getParametersEx(2, &yyhost, &yyuser)==FAILURE) {
						RETURN_FALSE;
					}
					convert_to_string_ex(yyhost);
					convert_to_string_ex(yyuser);
					host = (*yyhost)->value.str.val;
					user = (*yyuser)->value.str.val;
				}
				break;
			case 3: {
					pval **yyhost,**yyuser,**yypasswd;
				
					if (getParametersEx(3, &yyhost, &yyuser, &yypasswd) == FAILURE) {
						RETURN_FALSE;
					}
					convert_to_string_ex(yyhost);
					convert_to_string_ex(yyuser);
					convert_to_string_ex(yypasswd);
					host = (*yyhost)->value.str.val;
					user = (*yyuser)->value.str.val;
					passwd = (*yypasswd)->value.str.val;
				}
				break;
			default:
				WRONG_PARAM_COUNT;
				break;
		}
		hashed_details_length = sizeof("mysql___")-1 + strlen(SAFE_STRING(host))+strlen(SAFE_STRING(user))+strlen(SAFE_STRING(passwd));
		hashed_details = (char *) emalloc(hashed_details_length+1);
		sprintf(hashed_details,"mysql_%s_%s_%s",SAFE_STRING(host), SAFE_STRING(user), SAFE_STRING(passwd));
	}

	/* We cannot use mysql_port anymore in windows, need to use
	 * mysql_real_connect() to set the port.
	 */
	if (host && (tmp=strchr(host,':'))) {
		*tmp=0;
		tmp++;
		if (tmp[0] != '/') {
			port = atoi(tmp);
		} else {
			socket = tmp;
		}
	} else {
		port = MySG(default_port);
	}

#if MYSQL_VERSION_ID < 32200
	mysql_port = port;
#endif

	if (!MySG(allow_persistent)) {
		persistent=0;
	}
	if (persistent) {
		list_entry *le;
		
		/* try to find if we already have this link in our persistent list */
		if (zend_hash_find(plist, hashed_details, hashed_details_length+1, (void **) &le)==FAILURE) {  /* we don't */
			list_entry new_le;

			if (MySG(max_links)!=-1 && MySG(num_links)>=MySG(max_links)) {
				php_error(E_WARNING,"MySQL:  Too many open links (%d)",MySG(num_links));
				efree(hashed_details);
				RETURN_FALSE;
			}
			if (MySG(max_persistent)!=-1 && MySG(num_persistent)>=MySG(max_persistent)) {
				php_error(E_WARNING,"MySQL:  Too many open persistent links (%d)",MySG(num_persistent));
				efree(hashed_details);
				RETURN_FALSE;
			}
			/* create the link */
		mysql = (MYSQL *) malloc(sizeof(MYSQL));
#if MYSQL_VERSION_ID > 32199 /* this lets us set the port number */
		mysql_init(mysql);
		if (mysql_real_connect(mysql,host,user,passwd,NULL,port,socket,0)==NULL) {
#else
		if (mysql_connect(mysql,host,user,passwd)==NULL) {
#endif
				php_error(E_WARNING,mysql_error(mysql));
				free(mysql);
				efree(hashed_details);
				RETURN_FALSE;
			}
			
			/* hash it up */
			new_le.type = le_plink;
			new_le.ptr = mysql;
			if (zend_hash_update(plist, hashed_details, hashed_details_length+1, (void *) &new_le, sizeof(list_entry), NULL)==FAILURE) {
				free(mysql);
				efree(hashed_details);
				RETURN_FALSE;
			}
			MySG(num_persistent)++;
			MySG(num_links)++;
		} else {  /* we do */
			if (le->type != le_plink) {
				RETURN_FALSE;
			}
			/* ensure that the link did not die */
#if APACHE
			handler=signal(SIGPIPE,SIG_IGN);
#endif
#if defined(HAVE_MYSQL_ERRNO) && defined(CR_SERVER_GONE_ERROR)
			mysql_stat(le->ptr);
			if (mysql_errno((MYSQL *)le->ptr) == CR_SERVER_GONE_ERROR) {
#else
			if (!strcasecmp(mysql_stat(le->ptr),"mysql server has gone away")) { /* the link died */
#endif
#if APACHE
				signal(SIGPIPE,handler);
#endif
#if MYSQL_VERSION_ID > 32199 /* this lets us set the port number */
				if (mysql_real_connect(le->ptr,host,user,passwd,NULL,port,socket,0)==NULL) {
#else
				if (mysql_connect(le->ptr,host,user,passwd)==NULL) {
#endif
					php_error(E_WARNING,"MySQL:  Link to server lost, unable to reconnect");
					zend_hash_del(plist, hashed_details, hashed_details_length+1);
					efree(hashed_details);
					RETURN_FALSE;
				}
			}
#if APACHE
			signal(SIGPIPE,handler);
#endif
			mysql = (MYSQL *) le->ptr;
		}
		ZEND_REGISTER_RESOURCE(return_value, mysql, le_plink);
	} else { /* non persistent */
		list_entry *index_ptr,new_index_ptr;
		
		/* first we check the hash for the hashed_details key.  if it exists,
		 * it should point us to the right offset where the actual mysql link sits.
		 * if it doesn't, open a new mysql link, add it to the resource list,
		 * and add a pointer to it with hashed_details as the key.
		 */
		if (zend_hash_find(list,hashed_details,hashed_details_length+1,(void **) &index_ptr)==SUCCESS) {
			int type,link;
			void *ptr;

			if (index_ptr->type != le_index_ptr) {
				RETURN_FALSE;
			}
			link = (int) index_ptr->ptr;
			ptr = zend_list_find(link,&type);   /* check if the link is still there */
			if (ptr && (type==le_link || type==le_plink)) {
				zend_list_addref(link);
				return_value->value.lval = link;
				php_mysql_set_default_link(link);
				return_value->type = IS_RESOURCE;
				efree(hashed_details);
				return;
			} else {
				zend_hash_del(list,hashed_details,hashed_details_length+1);
			}
		}
		if (MySG(max_links)!=-1 && MySG(num_links)>=MySG(max_links)) {
			php_error(E_WARNING,"MySQL:  Too many open links (%d)",MySG(num_links));
			efree(hashed_details);
			RETURN_FALSE;
		}

		mysql = (MYSQL *) emalloc(sizeof(MYSQL));
#if MYSQL_VERSION_ID > 32199 /* this lets us set the port number */
		mysql_init(mysql);
		if (mysql_real_connect(mysql,host,user,passwd,NULL,port,NULL,0)==NULL) {
#else
		if (mysql_connect(mysql,host,user,passwd)==NULL) {
#endif
			php_error(E_WARNING,"MySQL Connection Failed: %s\n",mysql_error(mysql));
			efree(hashed_details);
			efree(mysql);
			RETURN_FALSE;
		}

		/* add it to the list */
		ZEND_REGISTER_RESOURCE(return_value, mysql, le_link);

		/* add it to the hash */
		new_index_ptr.ptr = (void *) return_value->value.lval;
		new_index_ptr.type = le_index_ptr;
		if (zend_hash_update(list,hashed_details,hashed_details_length+1,(void *) &new_index_ptr, sizeof(list_entry), NULL)==FAILURE) {
			efree(hashed_details);
			RETURN_FALSE;
		}
		MySG(num_links)++;
	}

	efree(hashed_details);
	php_mysql_set_default_link(return_value->value.lval);
}


static int php_mysql_get_default_link(INTERNAL_FUNCTION_PARAMETERS MySLS_DC)
{
	if (MySG(default_link)==-1) { /* no link opened yet, implicitly open one */
		ht = 0;
		php_mysql_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
	}
	return MySG(default_link);
}


/* {{{ proto int mysql_connect([string hostname[:port][:/path/to/socket]] [, string username] [, string password])
   Open a connection to a MySQL Server */
PHP_FUNCTION(mysql_connect)
{
	php_mysql_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */


/* {{{ proto int mysql_pconnect([string hostname[:port][:/path/to/socket]] [, string username] [, string password])
   Open a persistent connection to a MySQL Server */
PHP_FUNCTION(mysql_pconnect)
{
	php_mysql_do_connect(INTERNAL_FUNCTION_PARAM_PASSTHRU, 1);
}
/* }}} */


/* {{{ proto int mysql_close([int link_identifier])
   Close a MySQL connection */
PHP_FUNCTION(mysql_close)
{
	pval **mysql_link=NULL;
	int id;
	MYSQL *mysql;
	MySLS_FETCH();

	switch (ARG_COUNT(ht)) {
		case 0:
			id = MySG(default_link);
			break;
		case 1:
			if (getParametersEx(1, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	zend_list_delete(id);
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto int mysql_select_db(string database_name [, int link_identifier])
   Select a MySQL database */
PHP_FUNCTION(mysql_select_db)
{
	pval **db, **mysql_link;
	int id;
	MYSQL *mysql;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 1:
			if (getParametersEx(1, &db)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mysql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU MySLS_CC);
			CHECK_LINK(id);
			break;
		case 2:
			if (getParametersEx(2, &db, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	convert_to_string_ex(db);
	
	if (mysql_select_db(mysql, (*db)->value.str.val)!=0) {
		RETURN_FALSE;
	} else {
		RETURN_TRUE;
	}
}
/* }}} */


/* {{{ proto int mysql_create_db(string database_name [, int link_identifier])
   Create a MySQL database */
PHP_FUNCTION(mysql_create_db)
{
	pval **db,**mysql_link;
	int id;
	MYSQL *mysql;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 1:
			if (getParametersEx(1, &db)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mysql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU MySLS_CC);
			CHECK_LINK(id);
			break;
		case 2:
			if (getParametersEx(2, &db, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	convert_to_string_ex(db);
	if (mysql_create_db(mysql, (*db)->value.str.val)==0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto int mysql_drop_db(string database_name [, int link_identifier])
   Drop (delete) a MySQL database */
PHP_FUNCTION(mysql_drop_db)
{
	pval **db, **mysql_link;
	int id;
	MYSQL *mysql;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 1:
			if (getParametersEx(1, &db)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mysql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU MySLS_CC);
			CHECK_LINK(id);
			break;
		case 2:
			if (getParametersEx(2, &db, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	convert_to_string_ex(db);
	if (mysql_drop_db(mysql, (*db)->value.str.val)==0) {
		RETURN_TRUE;
	} else {
		RETURN_FALSE;
	}
}
/* }}} */


/* {{{ proto int mysql_query(string query [, int link_identifier])
   Send an SQL query to MySQL */
PHP_FUNCTION(mysql_query)
{
	pval **query, **mysql_link;
	int id;
	MYSQL *mysql;
	MYSQL_RES *mysql_result;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 1:
			if (getParametersEx(1, &query)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mysql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU MySLS_CC);
			CHECK_LINK(id);
			break;
		case 2:
			if (getParametersEx(2, &query, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	convert_to_string_ex(query);
	/* mysql_query binary unsafe, use mysql_real_query */
#if MYSQL_VERSION_ID > 32199 
	if (mysql_real_query(mysql, (*query)->value.str.val, (*query)->value.str.len)!=0) {
		RETURN_FALSE;
	}
#else
	if (mysql_query(mysql, (*query)->value.str.val)!=0) {
		RETURN_FALSE;
	}
#endif
	if ((mysql_result=mysql_store_result(mysql))==NULL) {
		if (PHP_MYSQL_VALID_RESULT(mysql)) { /* query should have returned rows */
			php_error(E_WARNING, "MySQL:  Unable to save result set");
			RETURN_FALSE;
		} else {
			RETURN_TRUE;
		}
	}
	ZEND_REGISTER_RESOURCE(return_value, mysql_result, le_result);
}
/* }}} */


/* {{{ proto int mysql_db_query(string database_name, string query [, int link_identifier])
   Send an SQL query to MySQL */
PHP_FUNCTION(mysql_db_query)
{
	pval **db, **query, **mysql_link;
	int id;
	MYSQL *mysql;
	MYSQL_RES *mysql_result;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 2:
			if (getParametersEx(2, &db, &query)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mysql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU MySLS_CC);
			CHECK_LINK(id);
			break;
		case 3:
			if (getParametersEx(3, &db, &query, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	convert_to_string_ex(db);
	if (mysql_select_db(mysql, (*db)->value.str.val)!=0) {
		RETURN_FALSE;
	}
	
	convert_to_string_ex(query);
	/* mysql_query is binary unsafe, use mysql_real_query */
#if MYSQL_VERSION_ID > 32199 
	if (mysql_real_query(mysql, (*query)->value.str.val, (*query)->value.str.len)!=0) {
		RETURN_FALSE;
	}
#else
	if (mysql_query(mysql, (*query)->value.str.val)!=0) {
		RETURN_FALSE;
	}
#endif
	if ((mysql_result=mysql_store_result(mysql))==NULL) {
		if (PHP_MYSQL_VALID_RESULT(mysql)) { /* query should have returned rows */
			php_error(E_WARNING, "MySQL:  Unable to save result set");
			RETURN_FALSE;
		} else {
			RETURN_TRUE;
		}
	}
	ZEND_REGISTER_RESOURCE(return_value, mysql_result, le_result);
}
/* }}} */


/* {{{ proto int mysql_list_dbs([int link_identifier])
   List databases available on a MySQL server */
PHP_FUNCTION(mysql_list_dbs)
{
	pval **mysql_link;
	int id;
	MYSQL *mysql;
	MYSQL_RES *mysql_result;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 0:
			id = php_mysql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU MySLS_CC);
			CHECK_LINK(id);
			break;
		case 1:
			if (getParametersEx(1, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);

	if ((mysql_result=mysql_list_dbs(mysql, NULL))==NULL) {
		php_error(E_WARNING,"Unable to save MySQL query result");
		RETURN_FALSE;
	}
	ZEND_REGISTER_RESOURCE(return_value, mysql_result, le_result);
}
/* }}} */


/* {{{ proto int mysql_list_tables(string database_name [, int link_identifier])
   List tables in a MySQL database */
PHP_FUNCTION(mysql_list_tables)
{
	pval **db, **mysql_link;
	int id;
	MYSQL *mysql;
	MYSQL_RES *mysql_result;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 1:
			if (getParametersEx(1, &db)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mysql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU MySLS_CC);
			CHECK_LINK(id);
			break;
		case 2:
			if (getParametersEx(2, &db, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
		
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	convert_to_string_ex(db);
	if (mysql_select_db(mysql, (*db)->value.str.val)!=0) {
		RETURN_FALSE;
	}
	if ((mysql_result=mysql_list_tables(mysql, NULL))==NULL) {
		php_error(E_WARNING,"Unable to save MySQL query result");
		RETURN_FALSE;
	}
	ZEND_REGISTER_RESOURCE(return_value, mysql_result, le_result);
}
/* }}} */


/* {{{ proto int mysql_list_fields(string database_name, string table_name [, int link_identifier])
   List MySQL result fields */
PHP_FUNCTION(mysql_list_fields)
{
	pval **db, **table, **mysql_link;
	int id;
	MYSQL *mysql;
	MYSQL_RES *mysql_result;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 2:
			if (getParametersEx(2, &db, &table)==FAILURE) {
				RETURN_FALSE;
			}
			id = php_mysql_get_default_link(INTERNAL_FUNCTION_PARAM_PASSTHRU MySLS_CC);
			CHECK_LINK(id);
			break;
		case 3:
			if (getParametersEx(3, &db, &table, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
		
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	convert_to_string_ex(db);
	if (mysql_select_db(mysql, (*db)->value.str.val)!=0) {
		RETURN_FALSE;
	}
	convert_to_string_ex(table);
	if ((mysql_result=mysql_list_fields(mysql, (*table)->value.str.val,NULL))==NULL) {
		php_error(E_WARNING,"Unable to save MySQL query result");
		RETURN_FALSE;
	}
	ZEND_REGISTER_RESOURCE(return_value, mysql_result, le_result);
}
/* }}} */


/* {{{ proto string mysql_error([int link_identifier])
   Returns the text of the error message from previous MySQL operation */
PHP_FUNCTION(mysql_error)
{
	pval **mysql_link;
	int id;
	MYSQL *mysql;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 0:
			id = MySG(default_link);
			if (id==-1) {
				RETURN_FALSE;
			}
			break;
		case 1:
			if (getParametersEx(1, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	RETURN_STRING(mysql_error(mysql),1);
}
/* }}} */


/* {{{ proto int mysql_errno([int link_identifier])
   Returns the number of the error message from previous MySQL operation */
#ifdef HAVE_MYSQL_ERRNO
PHP_FUNCTION(mysql_errno)
{
	pval **mysql_link;
	int id;
	MYSQL *mysql;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 0:
			id = MySG(default_link);
			if (id==-1) {
				RETURN_FALSE;
			}
			break;
		case 1:
			if (getParametersEx(1, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	RETURN_LONG(mysql_errno(mysql));
}
#endif
/* }}} */


/* {{{ proto int mysql_affected_rows([int link_identifier])
   Get number of affected rows in previous MySQL operation */
PHP_FUNCTION(mysql_affected_rows)
{
	pval **mysql_link;
	int id;
	MYSQL *mysql;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 0:
			id = MySG(default_link);
			CHECK_LINK(id);
			break;
		case 1:
			if (getParametersEx(1, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	/* conversion from int64 to long happing here */
	return_value->value.lval = (long) mysql_affected_rows(mysql);
	return_value->type = IS_LONG;
}
/* }}} */


/* {{{ proto int mysql_insert_id([int link_identifier])
   Get the id generated from the previous INSERT operation */
PHP_FUNCTION(mysql_insert_id)
{
	pval **mysql_link;
	int id;
	MYSQL *mysql;
	MySLS_FETCH();
	
	switch(ARG_COUNT(ht)) {
		case 0:
			id = MySG(default_link);
			CHECK_LINK(id);
			break;
		case 1:
			if (getParametersEx(1, &mysql_link)==FAILURE) {
				RETURN_FALSE;
			}
			id = -1;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE2(mysql, MYSQL *, mysql_link, id, "MySQL-Link", le_link, le_plink);
	
	/* conversion from int64 to long happing here */
	return_value->value.lval = (long) mysql_insert_id(mysql);
	return_value->type = IS_LONG;
}
/* }}} */


/* {{{ proto int mysql_result(int result, int row [, mixed field])
   Get result data */
PHP_FUNCTION(mysql_result)
{
	pval **result, **row, **field=NULL;
	MYSQL_RES *mysql_result;
	MYSQL_ROW sql_row;
	mysql_row_length_type *sql_row_lengths;
	int field_offset=0;
	PLS_FETCH();

	switch (ARG_COUNT(ht)) {
		case 2:
			if (getParametersEx(2, &result, &row)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 3:
			if (getParametersEx(3, &result, &row, &field)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);
		
	convert_to_long_ex(row);
	if ((*row)->value.lval<0 || (*row)->value.lval>=(int)mysql_num_rows(mysql_result)) {
		php_error(E_WARNING,"Unable to jump to row %d on MySQL result index %d", (*row)->value.lval, (*result)->value.lval);
		RETURN_FALSE;
	}
	mysql_data_seek(mysql_result, (*row)->value.lval);
	if ((sql_row=mysql_fetch_row(mysql_result))==NULL 
		|| (sql_row_lengths=mysql_fetch_lengths(mysql_result))==NULL) { /* shouldn't happen? */
		RETURN_FALSE;
	}
	
	if (field) {
		switch((*field)->type) {
			case IS_STRING: {
					int i=0;
					MYSQL_FIELD *tmp_field;
					char *table_name, *field_name, *tmp;

					if ((tmp=strchr((*field)->value.str.val,'.'))) {
						*tmp = 0;
						table_name = estrdup((*field)->value.str.val);
						field_name = estrdup(tmp+1);
					} else {
						table_name = NULL;
						field_name = estrndup((*field)->value.str.val,(*field)->value.str.len);
					}
					mysql_field_seek(mysql_result,0);
					while ((tmp_field=mysql_fetch_field(mysql_result))) {
						if ((!table_name || !strcasecmp(tmp_field->table,table_name)) && !strcasecmp(tmp_field->name,field_name)) {
							field_offset = i;
							break;
						}
						i++;
					}
					if (!tmp_field) { /* no match found */
						php_error(E_WARNING,"%s%s%s not found in MySQL result index %d",
									(table_name?table_name:""), (table_name?".":""), field_name, (*result)->value.lval);
						efree(field_name);
						if (table_name) {
							efree(table_name);
						}
						RETURN_FALSE;
					}
					efree(field_name);
					if (table_name) {
						efree(table_name);
					}
				}
				break;
			default:
				convert_to_long_ex(field);
				field_offset = (*field)->value.lval;
				if (field_offset<0 || field_offset>=(int)mysql_num_fields(mysql_result)) {
					php_error(E_WARNING,"Bad column offset specified");
					RETURN_FALSE;
				}
				break;
		}
	}

	if (sql_row[field_offset]) {
		if (PG(magic_quotes_runtime)) {
			return_value->value.str.val = php_addslashes(sql_row[field_offset],sql_row_lengths[field_offset],&return_value->value.str.len,0);
		} else {	
			return_value->value.str.len = sql_row_lengths[field_offset];
			return_value->value.str.val = (char *) safe_estrndup(sql_row[field_offset],return_value->value.str.len);
		}
	} else {
		return_value->value.str.val = undefined_variable_string;
		return_value->value.str.len=0;
		return_value->type = IS_STRING;
	}
	
	return_value->type = IS_STRING;
}
/* }}} */


/* {{{ proto int mysql_num_rows(int result)
   Get number of rows in a result */
PHP_FUNCTION(mysql_num_rows)
{
	pval **result;
	MYSQL_RES *mysql_result;
	
	if (ARG_COUNT(ht)!=1 || getParametersEx(1, &result)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);
	
	/* conversion from int64 to long happing here */
	return_value->value.lval = (long)mysql_num_rows(mysql_result);
	return_value->type = IS_LONG;
}
/* }}} */

/* {{{ proto int mysql_num_fields(int result)
   Get number of fields in a result */
PHP_FUNCTION(mysql_num_fields)
{
	pval **result;
	MYSQL_RES *mysql_result;
	
	if (ARG_COUNT(ht)!=1 || getParametersEx(1, &result)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);
	
	return_value->value.lval = mysql_num_fields(mysql_result);
	return_value->type = IS_LONG;
}
/* }}} */


static void php_mysql_fetch_hash(INTERNAL_FUNCTION_PARAMETERS, int result_type)
{
	pval **result, **arg2;
	MYSQL_RES *mysql_result;
	MYSQL_ROW mysql_row;
	MYSQL_FIELD *mysql_field;
	mysql_row_length_type *mysql_row_lengths;
	int num_fields;
	int i;
	PLS_FETCH();

	switch (ARG_COUNT(ht)) {
		case 1:
			if (getParametersEx(1, &result)==FAILURE) {
				RETURN_FALSE;
			}
			if (!result_type) {
				result_type = MYSQL_BOTH;
			}
			break;
		case 2:
			if (getParametersEx(2, &result, &arg2)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(arg2);
			result_type = (*arg2)->value.lval;
			break;
		default:
			WRONG_PARAM_COUNT;
			break;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);

	if ((mysql_row=mysql_fetch_row(mysql_result))==NULL 
		|| (mysql_row_lengths=mysql_fetch_lengths(mysql_result))==NULL) {
		RETURN_FALSE;
	}

	num_fields = mysql_num_fields(mysql_result);
	
	if (array_init(return_value)==FAILURE) {
		RETURN_FALSE;
	}
	
	mysql_field_seek(mysql_result,0);
	for (mysql_field=mysql_fetch_field(mysql_result),i=0; mysql_field; mysql_field=mysql_fetch_field(mysql_result),i++) {
		if (mysql_row[i]) {
			char *data;
			int data_len;
			int should_copy;

			if (PG(magic_quotes_runtime)) {
				data = php_addslashes(mysql_row[i],mysql_row_lengths[i],&data_len,0);
				should_copy = 0;
			} else {
				data = mysql_row[i];
				data_len = mysql_row_lengths[i];
				should_copy = 1;
			}
			
			if (result_type & MYSQL_NUM) {
				add_index_stringl(return_value, i, data, data_len, should_copy);
				should_copy = 1;
			}
			
			if (result_type & MYSQL_ASSOC) {
				add_assoc_stringl(return_value, mysql_field->name, data, data_len, should_copy);
			}
		} else {
			/* NULL field, don't set it */
			/* add_get_index_stringl(return_value, i, empty_string, 0, (void **) &pval_ptr); */
		}
	}
}


/* {{{ proto array mysql_fetch_row(int result)
   Get a result row as an enumerated array */
PHP_FUNCTION(mysql_fetch_row)
{
	php_mysql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, MYSQL_NUM);
}
/* }}} */


/* {{{ proto object mysql_fetch_object(int result [, int result_type])
   Fetch a result row as an object */
PHP_FUNCTION(mysql_fetch_object)
{
	php_mysql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
	if (return_value->type==IS_ARRAY) {
		return_value->type=IS_OBJECT;
		return_value->value.obj.properties = return_value->value.ht;
		return_value->value.obj.ce = &zend_standard_class_def;
	}
}
/* }}} */


/* {{{ proto array mysql_fetch_array(int result [, int result_type])
   Fetch a result row as an associative array */
PHP_FUNCTION(mysql_fetch_array)
{
	php_mysql_fetch_hash(INTERNAL_FUNCTION_PARAM_PASSTHRU, 0);
}
/* }}} */


/* {{{ proto int mysql_data_seek(int result, int row_number)
   Move internal result pointer */
PHP_FUNCTION(mysql_data_seek)
{
	pval **result, **offset;
	MYSQL_RES *mysql_result;
	
	if (ARG_COUNT(ht)!=2 || getParametersEx(2, &result, &offset)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);

	convert_to_long_ex(offset);
	if ((*offset)->value.lval<0 || (*offset)->value.lval>=(int)mysql_num_rows(mysql_result)) {
		php_error(E_WARNING,"Offset %d is invalid for MySQL result index %d", (*offset)->value.lval, (*result)->value.lval);
		RETURN_FALSE;
	}
	mysql_data_seek(mysql_result, (*offset)->value.lval);
	RETURN_TRUE;
}
/* }}} */


/* {{{ proto array mysql_fetch_lengths(int result)
   Get max data size of each column in a result */
PHP_FUNCTION(mysql_fetch_lengths)
{
	pval **result;
	MYSQL_RES *mysql_result;
	mysql_row_length_type *lengths;
	int num_fields;
	int i;

	
	if (ARG_COUNT(ht)!=1 || getParametersEx(1, &result)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);

	if ((lengths=mysql_fetch_lengths(mysql_result))==NULL) {
		RETURN_FALSE;
	}
	if (array_init(return_value)==FAILURE) {
		RETURN_FALSE;
	}
	num_fields = mysql_num_fields(mysql_result);
	
	for (i=0; i<num_fields; i++) {
		add_index_long(return_value, i, lengths[i]);
	}
}
/* }}} */


static char *php_mysql_get_field_name(int field_type)
{
	switch(field_type) {
		case FIELD_TYPE_STRING:
		case FIELD_TYPE_VAR_STRING:
			return "string";
			break;
#ifdef FIELD_TYPE_TINY
		case FIELD_TYPE_TINY:
#endif
		case FIELD_TYPE_SHORT:
		case FIELD_TYPE_LONG:
		case FIELD_TYPE_LONGLONG:
		case FIELD_TYPE_INT24:
			return "int";
			break;
		case FIELD_TYPE_FLOAT:
		case FIELD_TYPE_DOUBLE:
		case FIELD_TYPE_DECIMAL:
			return "real";
			break;
		case FIELD_TYPE_TIMESTAMP:
			return "timestamp";
			break;
		case FIELD_TYPE_DATE:
			return "date";
			break;
		case FIELD_TYPE_TIME:
			return "time";
			break;
		case FIELD_TYPE_DATETIME:
			return "datetime";
			break;
		case FIELD_TYPE_TINY_BLOB:
		case FIELD_TYPE_MEDIUM_BLOB:
		case FIELD_TYPE_LONG_BLOB:
		case FIELD_TYPE_BLOB:
			return "blob";
			break;
		case FIELD_TYPE_NULL:
			return "null";
			break;
		default:
			return "unknown";
			break;
	}
}


/* {{{ proto object mysql_fetch_field(int result [, int field_offset])
   Get column information from a result and return as an object */
PHP_FUNCTION(mysql_fetch_field)
{
	pval **result, **field=NULL;
	MYSQL_RES *mysql_result;
	MYSQL_FIELD *mysql_field;
	
	switch (ARG_COUNT(ht)) {
		case 1:
			if (getParametersEx(1, &result)==FAILURE) {
				RETURN_FALSE;
			}
			break;
		case 2:
			if (getParametersEx(2, &result, &field)==FAILURE) {
				RETURN_FALSE;
			}
			convert_to_long_ex(field);
			break;
		default:
			WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);

	if (field) {
		if ((*field)->value.lval<0 || (*field)->value.lval>=(int)mysql_num_fields(mysql_result)) {
			php_error(E_WARNING,"MySQL:  Bad field offset");
			RETURN_FALSE;
		}
		mysql_field_seek(mysql_result, (*field)->value.lval);
	}
	if ((mysql_field=mysql_fetch_field(mysql_result))==NULL) {
		RETURN_FALSE;
	}
	if (object_init(return_value)==FAILURE) {
		RETURN_FALSE;
	}

	add_property_string(return_value, "name",(mysql_field->name?mysql_field->name:empty_string), 1);
	add_property_string(return_value, "table",(mysql_field->table?mysql_field->table:empty_string), 1);
	add_property_string(return_value, "def",(mysql_field->def?mysql_field->def:empty_string), 1);
	add_property_long(return_value, "max_length",mysql_field->max_length);
	add_property_long(return_value, "not_null",IS_NOT_NULL(mysql_field->flags)?1:0);
	add_property_long(return_value, "primary_key",IS_PRI_KEY(mysql_field->flags)?1:0);
	add_property_long(return_value, "multiple_key",(mysql_field->flags&MULTIPLE_KEY_FLAG?1:0));
	add_property_long(return_value, "unique_key",(mysql_field->flags&UNIQUE_KEY_FLAG?1:0));
	add_property_long(return_value, "numeric",IS_NUM(mysql_field->type)?1:0);
	add_property_long(return_value, "blob",IS_BLOB(mysql_field->flags)?1:0);
	add_property_string(return_value, "type",php_mysql_get_field_name(mysql_field->type), 1);
	add_property_long(return_value, "unsigned",(mysql_field->flags&UNSIGNED_FLAG?1:0));
	add_property_long(return_value, "zerofill",(mysql_field->flags&ZEROFILL_FLAG?1:0));
}
/* }}} */


/* {{{ proto int mysql_field_seek(int result, int field_offset)
   Set result pointer to a specific field offset */
PHP_FUNCTION(mysql_field_seek)
{
	pval **result, **offset;
	MYSQL_RES *mysql_result;
	
	if (ARG_COUNT(ht)!=2 || getParametersEx(2, &result, &offset)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);

	convert_to_long_ex(offset);
	if ((*offset)->value.lval<0 || (*offset)->value.lval>=(int)mysql_num_fields(mysql_result)) {
		php_error(E_WARNING,"Field %d is invalid for MySQL result index %d", (*offset)->value.lval, (*result)->value.lval);
		RETURN_FALSE;
	}
	mysql_field_seek(mysql_result, (*offset)->value.lval);
	RETURN_TRUE;
}
/* }}} */


#define PHP3_MYSQL_FIELD_NAME 1
#define PHP3_MYSQL_FIELD_TABLE 2
#define PHP3_MYSQL_FIELD_LEN 3
#define PHP3_MYSQL_FIELD_TYPE 4
#define PHP3_MYSQL_FIELD_FLAGS 5
 

static void php_mysql_field_info(INTERNAL_FUNCTION_PARAMETERS, int entry_type)
{
	pval **result, **field;
	MYSQL_RES *mysql_result;
	MYSQL_FIELD *mysql_field;
	char buf[512];
	int  len;

	if (ARG_COUNT(ht)!=2 || getParametersEx(2, &result, &field)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);
	
	convert_to_long_ex(field);
	if ((*field)->value.lval<0 || (*field)->value.lval>=(int)mysql_num_fields(mysql_result)) {
		php_error(E_WARNING,"Field %d is invalid for MySQL result index %d", (*field)->value.lval, (*result)->value.lval);
		RETURN_FALSE;
	}
	mysql_field_seek(mysql_result, (*field)->value.lval);
	if ((mysql_field=mysql_fetch_field(mysql_result))==NULL) {
		RETURN_FALSE;
	}
	
	switch (entry_type) {
		case PHP3_MYSQL_FIELD_NAME:
			return_value->value.str.len = strlen(mysql_field->name);
			return_value->value.str.val = estrndup(mysql_field->name,return_value->value.str.len);
			return_value->type = IS_STRING;
			break;
		case PHP3_MYSQL_FIELD_TABLE:
			return_value->value.str.len = strlen(mysql_field->table);
			return_value->value.str.val = estrndup(mysql_field->table,return_value->value.str.len);
			return_value->type = IS_STRING;
			break;
		case PHP3_MYSQL_FIELD_LEN:
			return_value->value.lval = mysql_field->length;
			return_value->type = IS_LONG;
			break;
		case PHP3_MYSQL_FIELD_TYPE:
			return_value->value.str.val = php_mysql_get_field_name(mysql_field->type);
			return_value->value.str.len = strlen(return_value->value.str.val);
			return_value->value.str.val = estrndup(return_value->value.str.val, return_value->value.str.len);
			return_value->type = IS_STRING;
			break;
		case PHP3_MYSQL_FIELD_FLAGS:
			strcpy(buf, "");
#ifdef IS_NOT_NULL
			if (IS_NOT_NULL(mysql_field->flags)) {
				strcat(buf, "not_null ");
			}
#endif
#ifdef IS_PRI_KEY
			if (IS_PRI_KEY(mysql_field->flags)) {
				strcat(buf, "primary_key ");
			}
#endif
#ifdef UNIQUE_KEY_FLAG
			if (mysql_field->flags&UNIQUE_KEY_FLAG) {
				strcat(buf, "unique_key ");
			}
#endif
#ifdef MULTIPLE_KEY_FLAG
			if (mysql_field->flags&MULTIPLE_KEY_FLAG) {
				strcat(buf, "multiple_key ");
			}
#endif
#ifdef IS_BLOB
			if (IS_BLOB(mysql_field->flags)) {
				strcat(buf, "blob ");
			}
#endif
#ifdef UNSIGNED_FLAG
			if (mysql_field->flags&UNSIGNED_FLAG) {
				strcat(buf, "unsigned ");
			}
#endif
#ifdef ZEROFILL_FLAG
			if (mysql_field->flags&ZEROFILL_FLAG) {
				strcat(buf, "zerofill ");
			}
#endif
#ifdef BINARY_FLAG
			if (mysql_field->flags&BINARY_FLAG) {
				strcat(buf, "binary ");
			}
#endif
#ifdef ENUM_FLAG
			if (mysql_field->flags&ENUM_FLAG) {
				strcat(buf, "enum ");
			}
#endif
#ifdef AUTO_INCREMENT_FLAG
			if (mysql_field->flags&AUTO_INCREMENT_FLAG) {
				strcat(buf, "auto_increment ");
			}
#endif
#ifdef TIMESTAMP_FLAG
			if (mysql_field->flags&TIMESTAMP_FLAG) {
				strcat(buf, "timestamp ");
			}
#endif
			len = strlen(buf);
			/* remove trailing space, if present */
			if (len && buf[len-1] == ' ') {
				buf[len-1] = 0;
				len--;
			}
			
	   		return_value->value.str.len = len;
   			return_value->value.str.val = estrndup(buf, len);
   			return_value->type = IS_STRING;
			break;

		default:
			RETURN_FALSE;
	}
}


/* {{{ proto string mysql_field_name(int result, int field_index)
   Get the name of the specified field in a result */
PHP_FUNCTION(mysql_field_name)
{
	php_mysql_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU,PHP3_MYSQL_FIELD_NAME);
}
/* }}} */


/* {{{ proto string mysql_field_table(int result, int field_offset)
   Get name of the table the specified field is in */
PHP_FUNCTION(mysql_field_table)
{
	php_mysql_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU,PHP3_MYSQL_FIELD_TABLE);
}
/* }}} */


/* {{{ proto int mysql_field_len(int result, int field_offet)
   Returns the length of the specified field */
PHP_FUNCTION(mysql_field_len)
{
	php_mysql_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU,PHP3_MYSQL_FIELD_LEN);
}
/* }}} */


/* {{{ proto string mysql_field_type(int result, int field_offset)
   Get the type of the specified field in a result */
PHP_FUNCTION(mysql_field_type)
{
	php_mysql_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU,PHP3_MYSQL_FIELD_TYPE);
}
/* }}} */


/* {{{ proto string mysql_field_flags(int result, int field_offset)
   Get the flags associated with the specified field in a result */
PHP_FUNCTION(mysql_field_flags)
{
	php_mysql_field_info(INTERNAL_FUNCTION_PARAM_PASSTHRU,PHP3_MYSQL_FIELD_FLAGS);
}
/* }}} */


/* {{{ proto int mysql_free_result(int result)
   Free result memory */
PHP_FUNCTION(mysql_free_result)
{
	pval **result;
	MYSQL_RES *mysql_result;

	if (ARG_COUNT(ht)!=1 || getParametersEx(1, &result)==FAILURE) {
		WRONG_PARAM_COUNT;
	}
	
	if ((*result)->type==IS_RESOURCE && (*result)->value.lval==0) {
		RETURN_FALSE;
	}
	
	ZEND_FETCH_RESOURCE(mysql_result, MYSQL_RES *, result, -1, "MySQL result", le_result);

	zend_list_delete((*result)->value.lval);
	RETURN_TRUE;
}
/* }}} */
#endif


/*
 * Local variables:
 * tab-width: 4
 * c-basic-offset: 4
 * End:
 */
