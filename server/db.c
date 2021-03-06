#include <mysql.h>
#include "event.h"
#include "comfunc.h"
#include "cwind.h"

extern cw_config_t server;

int save_site(site_t * site)
{
	char            sql[MAX_LINE] = { 0 };
	string_t        sql_cmd = { 0 };

	sprintf(sql, "update site_info set site_md5='%s' where id = %d", site->md5, site->id);

	sql_cmd.str = sql;
	sql_cmd.len = strlen(sql);

	if (mydb_exec(server.cwind_db, &sql_cmd)) {
		log_error("%s exec sql command error.", __func__);
		return MRT_ERR;
	}

	return MRT_OK;
}

/*
   int load_class_id(T_class *class)
   {
   MYSQL_RES *res;
   MYSQL_ROW row;
   char sqlcmd[MAX_LINE] = {0};
   MYSQL *srv = &server.mysql_info.srv;

   sprintf(sqlcmd, "select first_id, second_id from class_name where class_name='%s'", class->caption);
   if(mysql_real_query(srv, sqlcmd, strlen(sqlcmd)))
   {
   log_error("%s mysql_real_query error, sql:%s, error:%s.", __func__, sqlcmd, mysql_error(srv));
   return MRT_ERR;
   }

   res =  mysql_store_result(srv);

   while((row = mysql_fetch_row(res)))
   {
   class->first_id = atoi(row[0]);
   class->second_id = atoi(row[1]);
   }

   mysql_free_result(res);

   return MRT_SUC;
   }
   */

int load_store_info(mydb_t * mysql_info, site_t * site)
{
	MYSQL_RES      *res;
	MYSQL_ROW       row;
	string_t        cmd = { 0 };

	string_printf(&cmd, "select host, port, username, password, dbname from store where id=%d", site->store_id);

	if (mydb_query(mysql_info, &cmd)) {
		log_error("mydb_query error, sql:%s, error:%s.", cmd.str, mysql_error(&mysql_info->srv));
		string_free(&cmd);
		return MRT_ERR;
	}

	res = mysql_store_result(&mysql_info->srv);

	if (!res || mysql_num_rows(res) == 0) {
		log_error("No found record, sql:%s.", cmd.str);
		string_free(&cmd);
		return MRT_ERR;
	}
	string_free(&cmd);

	if ((row = mysql_fetch_row(res))) {
		site->mysql_info = M_alloc(sizeof(*site->mysql_info));
		strcpy(site->mysql_info->ip, row[0]);
		site->mysql_info->port = atoi(row[1]);
		strcpy(site->mysql_info->user, row[2]);
		strcpy(site->mysql_info->pass, row[3]);
		strcpy(site->mysql_info->name, row[4]);
	}

	mysql_free_result(res);

	return MRT_SUC;
}

int load_content_tag(mydb_t * mdb, site_t * site)
{
	MYSQL_RES      *res;
	MYSQL_ROW       row;
	char            sqlcmd[MAX_LINE] = { 0 };
	string_t        cmd = { 0 };

	sprintf(sqlcmd, "select "
		"body_begin, body_end, date_begin, date_end, desc_begin, desc_end, title_begin, title_end, content_begin, content_end "
		"from content where id=%d", site->content_id);

	cmd.str = sqlcmd;
	cmd.len = strlen(sqlcmd);

	if (mydb_query(mdb, &cmd)) {
		log_error("mysql_real_query error:%s, sql:%s", get_error(), sqlcmd);
		return MRT_ERR;
	}

	res = mysql_store_result(&mdb->srv);
	if (!res || mysql_num_rows(res) == 0) {
		log_error("No found record, sql:%s.", sqlcmd);
		return MRT_ERR;
	}

	if ((row = mysql_fetch_row(res))) {
		strcpy(site->content_tag.body_begin, row[0]);
		strcpy(site->content_tag.body_end, row[1]);
		strcpy(site->content_tag.date_begin, row[2]);
		strcpy(site->content_tag.date_end, row[3]);
		strcpy(site->content_tag.desc_begin, row[4]);
		strcpy(site->content_tag.desc_end, row[5]);
		strcpy(site->content_tag.title_begin, row[6]);
		strcpy(site->content_tag.title_end, row[7]);
		strcpy(site->content_tag.content_begin, row[8]);
		strcpy(site->content_tag.content_end, row[9]);
	}

	mysql_free_result(res);

	return MRT_SUC;
}

int load_list_tag(mydb_t * mdb, site_t * site)
{
	MYSQL_RES      *res;
	MYSQL_ROW       row;
	char            sqlcmd[MAX_LINE] = { 0 };
	string_t        cmd = { 0 };

	sprintf(sqlcmd,
		"select body_begin, body_end, item_begin, item_end, "
		"url_begin, url_end, title_begin, title_end, page_begin, page_end, next_label from list where id=%d", site->list_id);

	cmd.str = sqlcmd;
	cmd.len = strlen(sqlcmd);

	if (mydb_query(mdb, &cmd)) {
		log_error("mysql_real_query error:%s, sql:%s.", get_error(), sqlcmd);
		return MRT_ERR;
	}

	res = mysql_store_result(&mdb->srv);
	if (!res || mysql_num_rows(res) == 0) {
		log_error("No found record, sql:%s.", sqlcmd);
		return MRT_ERR;
	}

	if ((row = mysql_fetch_row(res))) {
		strcpy(site->list_tag.body_begin, row[0]);
		strcpy(site->list_tag.body_end, row[1]);
		strcpy(site->list_tag.item_begin, row[2]);
		strcpy(site->list_tag.item_end, row[3]);
		strcpy(site->list_tag.url_begin, row[4]);
		strcpy(site->list_tag.url_end, row[5]);
		strcpy(site->list_tag.title_begin, row[6]);
		strcpy(site->list_tag.title_end, row[7]);
		strcpy(site->list_tag.page_begin, row[8]);
		strcpy(site->list_tag.page_end, row[9]);
		strcpy(site->list_tag.next_label, row[10]);
	} else {
		log_error("no found record error, sql:%s.", sqlcmd);
		return MRT_ERR;
	}

	mysql_free_result(res);

	return MRT_SUC;
}

int load_filter(mydb_t * mdb, site_t * site)
{
	MYSQL_RES      *res;
	MYSQL_ROW       row;
	string_t        cmd = { 0 };
	int             r1 = 0, r2 = 0;
	mrt_mark_t     *pmk = NULL;

	LIST_INIT(&site->filter.remove_list, head);
	LIST_INIT(&site->filter.replace_list, head);

	string_printf(&cmd, "select type, key1, key2 from filter where id in (select filter_id from relation where site_id=%d)", site->id);

	if (mydb_query(mdb, &cmd)) {
		log_error("mysql_real_query error:%s, sql:%s", get_error(), cmd.str);
		string_free(&cmd);
		return MRT_ERR;
	}

	res = mysql_store_result(&mdb->srv);
	if (!res || mysql_num_rows(res) == 0) {
		log_error("No found record, sql:%s.", cmd.str);
		string_free(&cmd);
		//没找到就算了
		return MRT_OK;
	}
	string_free(&cmd);

	while ((row = mysql_fetch_row(res))) {
		if (atoi(row[0]) == 0) {
			pmk = M_alloc(sizeof(mrt_mark_t));
			strcpy(pmk->begin, row[1]);
			strcpy(pmk->end, row[2]);
			pmk->new_len = 0;
			LIST_INSERT_HEAD(&site->filter.remove_list, head, pmk, node);
			r1++;
		} else {
			pmk = M_alloc(sizeof(mrt_mark_t));
			strcpy(pmk->begin, row[1]);
			strcpy(pmk->end, row[2]);
			pmk->new_len = (strlen(row[1]) > strlen(row[2])) ? 0 : strlen(row[2]);
			LIST_INSERT_HEAD(&site->filter.replace_list, head, pmk, node);
			r2++;
		}
	}

	mysql_free_result(res);

	log_info("load filter success, type remove:%d, type replace:%d", r1, r2);

	return MRT_SUC;
}

int load_site_list(mydb_t * mdb, site_list_t ** list)
{
	MYSQL_RES      *res;
	MYSQL_ROW       row;
	string_t        cmd;
	site_t         *site_info;
	site_list_t    *site_list;

	s_zero(cmd);
	M_cvril((site_list = M_alloc(sizeof(site_list_t))), "malloc site list error");

	LIST_INIT(site_list, head);

	string_printf(&cmd, "select id, name, url, md5, level, "
		      "area_begin, area_end, list_id, content_id, store_id from site where status = 1 and mod(id, %d) = %d", server.max_proc, server.cur_proc);

	if (mydb_query(mdb, &cmd)) {
		log_error("mydb_query error:%s, sql:%s.", get_error(), cmd.str);
		mysql_info_close(server.cwind_db);
		string_free(&cmd);
		return MRT_ERR;
	}

	if (!(res = mysql_store_result(&mdb->srv))) {
		log_error("No found record, sql:%s.", cmd.str);
		mysql_info_close(server.cwind_db);
		string_free(&cmd);
		return MRT_ERR;
	}

	string_free(&cmd);

	while ((row = mysql_fetch_row(res))) {
		M_cvril((site_info = M_alloc(sizeof(site_t))), "malloc site info error.");

		site_info->id = atoi(row[0]);
		strcpy(site_info->name, row[1]);
		strcpy(site_info->url, row[2]);
		strcpy(site_info->md5, row[3]);
		site_info->level = atoi(row[4]);
		site_info->area_begin = str_newcpy(row[5], strlen(row[5]));
		site_info->area_end = str_newcpy(row[6], strlen(row[6]));
		site_info->list_id = atoi(row[7]);
		site_info->content_id = atoi(row[8]);
		site_info->store_id = atoi(row[9]);

		site_info->all_sum = 0;
		site_info->recv_num = 0;

		log_info("site_id:%s, site_name:%s, site_url:%s, site_md5:%s, site_level:%s", row[0], row[1], row[2], row[3], row[4]);

		LIST_INSERT_HEAD(site_list, head, site_info, node);
		site_list->all_sum++;
	}

	mysql_free_result(res);

	*list = site_list;

	return MRT_OK;
}

int site_log_init(site_t * site)
{
	char            buf[MAX_LINE] = { 0 };
	string_t        cmd = { 0 };
	MYSQL          *srv = &(server.cwind_db->srv);

	snprintf(buf, sizeof(buf), "CREATE TABLE `site_%d_log` ( "
		 " `id` bigint(20) unsigned NOT NULL AUTO_INCREMENT, "
		 " `url_crc32` bigint(33) NOT NULL, "
		 " `url` varchar(128) NOT NULL, "
		 " `type` int(10) unsigned NOT NULL COMMENT '类型，1：列表页，2：内容页', "
		 " `fetch_date` int(11) unsigned NOT NULL DEFAULT '0' COMMENT '最后更新日期', "
		 " `status` int(11) unsigned NOT NULL COMMENT '1:采集成功 2:采集失败了', "
		 " PRIMARY KEY (`id`), "
		 " UNIQUE KEY `uniq_url` (`url_crc32`,`url`) USING BTREE, "
		 " KEY `idx_type` (`type`) USING BTREE " "     ) ENGINE=MyISAM AUTO_INCREMENT=0 DEFAULT CHARSET=utf8 ", site->id);

	cmd.str = buf;
	cmd.len = strlen(buf);

	if (mydb_query(server.cwind_db, &cmd) == MRT_ERR) {
		if (mysql_errno(srv) == 1050)
			return MRT_OK;

		log_error("query error[%d]:%s, sql:%s.", mysql_errno(srv), mysql_error(srv), buf);
		return MRT_ERR;
	}

	return MRT_OK;
}

int             dba = 0;

//type:1为列表页, 2:内容页
//返回：
//      1: 不需要采集
//      2: 需要采集
int find_url_status(site_t * site, char *url, int type)
{
	char            buf[MAX_LINE] = { 0 };
	string_t        cmd = { 0 };
	MYSQL_RES      *res = NULL;
	MYSQL          *srv = &(server.cwind_db->srv);
	MYSQL_ROW       row;

	snprintf(buf, sizeof(buf), "select status from `site_%d_log` where type=%d and url='%s'", site->id, type, url);

	cmd.str = buf;
	cmd.len = strlen(buf);

	if (mydb_query(server.cwind_db, &cmd) == MRT_ERR) {
		log_error("query error, sql:%s.", buf);
		return MRT_ERR;
	}

	res = mysql_store_result(srv);
	if (!res) {
		log_info("No found page url:%s.", url);
		return 2;
	}

	while ((row = mysql_fetch_row(res))) {
		if (!strcmp((char *)row[0], "1")) {
			log_info("page has been collected over, url:%s", url);
			mysql_free_result(res);
			return 1;
		}
		log_info("page statue is %s, url:%s", row[0], url);
	}

	mysql_free_result(res);

	return 2;
}

int set_url_status(site_t * site, char *url, int type, int status)
{
	char            buf[MAX_LINE] = { 0 };
	string_t        cmd = { 0 };

	snprintf(buf, sizeof(buf),
		 "insert `site_%d_log` (url_crc32, url, type, fetch_date, status) values (crc32('%s'), '%s', %d, UNIX_TIMESTAMP(), %d)", site->id, url, url, type, status);

	cmd.str = buf;
	cmd.len = strlen(buf);

	if (mydb_exec(server.cwind_db, &cmd) != 1) {
		snprintf(buf, sizeof(buf), "update `site_%d_log` set fetch_date = UNIX_TIMESTAMP(), status = %d where url='%s'", site->id, status, url);
		if (mydb_exec(server.cwind_db, &cmd) != 1) {
			log_error("query error, sql:%s.", buf);
			return MRT_ERR;
		}
	}

	return MRT_SUC;
}

int find_class_id(site_t * site, char *desc, int *first, int *second)
{
	char            buf[MAX_LINE] = { 0 };
	string_t        cmd = { 0 };
	MYSQL_RES      *res = NULL;
	MYSQL          *srv = &(site->mysql_info->srv);
	MYSQL_ROW       row;

	snprintf(buf, sizeof(buf), "select parentId, id from `class` where `desc` like '%%%s%%'", desc);

	cmd.str = buf;
	cmd.len = strlen(buf);

	if (mydb_query(site->mysql_info, &cmd) == MRT_ERR) {
		log_error("query error, sql:%s.", buf);
		return MRT_ERR;
	}

	res = mysql_store_result(srv);
	if (!res) {
		log_error("No found record, sql:%s.", buf);
		return MRT_ERR;
	}

	if (!(row = mysql_fetch_row(res))) {
		log_error("Record is null, sql:%s.", buf);
		mysql_free_result(res);
		return MRT_ERR;
	}

	*first = atoi(row[0]);
	*second = atoi(row[1]);

	log_info("desc:`%s` in first:%d second:%d", desc, *first, *second);

	mysql_free_result(res);

	return MRT_SUC;
}

//测试用。。。。。。。。。。
int save_joke_info2(site_t * site)
{
	/*
	   int i=0;
	   char buf1[1000] = {0};
	   char buf2[1000] = {0};
	   cw_content_t joke;

	   sprintf(site->mysql_info->ip,  "127.0.0.1");
	   sprintf(site->mysql_info->pass, "mc123456");
	   sprintf(site->mysql_info->user, "root");
	   site->mysql_info->port = 3306;
	   sprintf(site->mysql_info->name, "kxbao");

	   for(;i< 2; i++)
	   {
	   sprintf(buf1, "%d%d", i, i * 9);
	   sprintf(buf2, "%s---%d", buf1, i * 9);
	   sprintf(joke.caption,"%s",  buf1);
	   joke.content = buf2;
	   save_joke_info(site, &joke);
	   log_info("i:%d", i);
	   }

	 */
	exit(0);
}

int content_info_save(site_t * site, content_t * ct)
{
	//int             nid = 0;
	string_t        cmd;

	M_cpvril(site);
	M_cpvril(ct);

	s_zero(cmd);

	format_mysql_string(ct->content.str);

	/* TODO 保存数据这里要修改
	   string_printf(&cmd, "call cwindPushArticle('%s', '%s', '%s')", ct->second_class.str, ct->caption.str, ct->content.str);

	   if (mydb_query_int(site->mysql_info, &cmd, &nid) == MRT_ERR) {
	   log_error("exec sql cmd error:%s, sql:%s.", get_error(), cmd.str);
	   string_free(&cmd);
	   return MRT_ERR;
	   }

	   log_debug("save content success, from:%s, news id:%d, sql:%s", ct->url_real.str, nid, cmd.str);
	 */

	string_free(&cmd);

	return MRT_SUC;
}
