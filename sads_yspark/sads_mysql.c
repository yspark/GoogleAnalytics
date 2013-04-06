#include <stdlib.h>
#include <string.h>

#include "sads_mysql.h"


void smysql_init() {
  smysql_conn = mysql_init(NULL);

  if (smysql_conn == NULL) {
      printf("Error %u: %s\n", mysql_errno(smysql_conn), mysql_error(smysql_conn));
      exit(1);
  }

  if (mysql_real_connect(smysql_conn, "localhost", "piwik", 
          "piwik", "piwik", 0, NULL, 0) == NULL) {
      printf("Error %u: %s\n", mysql_errno(smysql_conn), mysql_error(smysql_conn));
      exit(1);
  }	
  
  smysql_table_init();
  
}

void smysql_close() {
  mysql_close(smysql_conn);
}


MYSQL_RES* smysql_query(char *query) {
	if (mysql_query(smysql_conn, query)) {
		printf("Error %u: %s\n", mysql_errno(smysql_conn), mysql_error(smysql_conn));
		exit(1);
 	}
 	
 	return mysql_store_result(smysql_conn);
}

MYSQL_RES* smysql_real_query(char *query, int query_len) {
	if (mysql_real_query(smysql_conn, query, query_len)) {
		printf("Error %u: %s\n", mysql_errno(smysql_conn), mysql_error(smysql_conn));
		exit(1);
 	}
 	
 	return mysql_store_result(smysql_conn);	
}



void smysql_table_init() {
	//smysql_query("CREATE TABLE IF NOT EXISTS sads_tree (nodeid BIGINT UNSIGNED, PRIMARY KEY(nodeid), visit_count INT UNSIGNED, digest LONGBLOB, digest_len INT UNSIGNED, label LONGBLOB, label_len INT UNSIGNED)");
	smysql_query("CREATE TABLE IF NOT EXISTS sads_tree (nodeid BIGINT UNSIGNED, PRIMARY KEY(nodeid), visit_count INT UNSIGNED, label LONGBLOB)");

}


void smysql_add_node(ULONG nodeid, char *label_buffer, UINT label_buffer_len, BOOL isLeaf) {
	char *command = NULL;
	char *query = NULL;
	UINT query_len = 0, est_query_len = 0;

	char *label_chunk = malloc(label_buffer_len * 2 + 1);
	UINT label_chunk_len = 0;

	DEBUG_TRACE(("smysql_add__node(%llu)\n", nodeid));

	// Handle termination characters
	label_chunk_len = mysql_real_escape_string(smysql_conn, label_chunk, label_buffer, label_buffer_len);
	//printf("label_chunk:%u, label_buffer:%u\n", (UINT)strlen(label_chunk), label_buffer_len);

	if(isLeaf)
		command = "INSERT INTO sads_tree(nodeid, visit_count, label) VALUES(%llu, 1, '%s')";
	else
		command = "INSERT INTO sads_tree(nodeid, label) VALUES(%llu, '%s')";

	est_query_len = strlen(command) + sizeof(ULONG) + strlen(label_chunk);
	//printf("command %d, label %u, est_query_len %u\n", (int)strlen(command), label_chunk_len, est_query_len);

	query = malloc(est_query_len);
	query_len = snprintf(query, est_query_len, command, nodeid, label_chunk, label_chunk_len);

	smysql_real_query(query, query_len);

}


void smysql_update_node(ULONG nodeid, UINT new_visit_count, char *label_buffer, UINT label_buffer_len, BOOL isLeaf) {
	char *command = NULL;
	char *query = NULL;
	UINT query_len = 0, est_query_len = 0;

	char *label_chunk = malloc(label_buffer_len * 2 + 1);
	UINT label_chunk_len = 0;

	DEBUG_TRACE(("smysql_update_node(%llu)\n", nodeid));

	// Handle termination characters
	label_chunk_len = mysql_real_escape_string(smysql_conn, label_chunk, label_buffer, label_buffer_len);
	//printf("label_chunk:%u, label_buffer:%u\n", (UINT)strlen(label_chunk), label_buffer_len);

	if(isLeaf) {
		command = "UPDATE sads_tree SET visit_count='%u', label='%s' WHERE nodeid=%llu";
		est_query_len = strlen(command) + sizeof(UINT) + label_chunk_len + sizeof(ULONG);

		query = malloc(est_query_len);
		query_len = snprintf(query, est_query_len, command, new_visit_count, label_chunk, nodeid, label_chunk_len);
	}
	else {
		command = "UPDATE sads_tree SET label='%s' WHERE nodeid=%llu";
		est_query_len = strlen(command) + label_chunk_len + sizeof(ULONG);

		query = malloc(est_query_len);
		query_len = snprintf(query, est_query_len, command, label_chunk, nodeid, label_chunk_len);
	}

	//printf("command %d, label %u, est_query_len %u\n", (int)strlen(command), label_chunk_len, est_query_len);

	smysql_real_query(query, query_len);

}


UINT smysql_get_leaf_val(ULONG nodeid) {
	char query[128];
	MYSQL_RES *result;
	MYSQL_ROW result_row;
	UINT retVal;

	DEBUG_TRACE(("smysql_get_leaf_val(%llu)\n", nodeid));

	sprintf(query, "SELECT visit_count FROM sads_tree WHERE nodeid=%llu LIMIT 1", nodeid);
	result = smysql_query(query);
	
	result_row = mysql_fetch_row(result);
	retVal = (result_row == NULL) ? 0 : atoi(result_row[0]);
	
	mysql_free_result(result);

	return retVal;
}

char *smysql_get_node_label(ULONG nodeid) {
	char query[128];
	MYSQL_RES *result;
	MYSQL_ROW result_row;
	char *label_buffer = NULL;

	unsigned long *lengths;
	unsigned int num_fields;


	DEBUG_TRACE(("smysql_get_node_label(%llu)\n", nodeid));

	sprintf(query, "SELECT label FROM sads_tree WHERE nodeid=%llu LIMIT 1", nodeid);
	result = smysql_query(query);

	result_row = mysql_fetch_row(result);

	if(result_row != NULL) {
		num_fields = mysql_num_fields(result);
		lengths = mysql_fetch_lengths(result);

		DEBUG_TRACE(("MYSQL_RES rows(%d), num_field(%u), length(%u)\n", (UINT)mysql_num_rows(result), (UINT)num_fields, (UINT)lengths[0]));

		if(lengths[0]) {
			if(lengths[0] != LABEL_BUFFER_LEN) {
				printf("wrong label_buffer_length(%llu)", nodeid);
				exit(1);
			}
			label_buffer = malloc(lengths[0]);
			memcpy(label_buffer, result_row[0], lengths[0]);
		}
	}

	mysql_free_result(result);

	return label_buffer;
}


UINT smysql_get_range_result(ULONG start_nodeid, ULONG end_nodeid, ULONG **nodeid_list, UINT **value_list, char ***label_buffer_list) {
	char query[256];
	MYSQL_RES *result;
	MYSQL_ROW result_row;
	UINT num_rows = 0;
	//UINT num_fields = 0;
	UINT node_count = 0;

	DEBUG_TRACE(("smysql_get_range_result(%llu, %llu)\n", start_nodeid, end_nodeid));

	sprintf(query, "SELECT nodeid, visit_count, label FROM sads_tree WHERE nodeid>=%llu AND nodeid <=%llu", start_nodeid, end_nodeid);
	result = smysql_query(query);

	num_rows = (UINT)mysql_num_rows(result);
	//num_fields = (UINT)mysql_num_fields(result);

	*nodeid_list = malloc(sizeof(ULONG) * num_rows);
	*value_list = malloc(sizeof(UINT) * num_rows);
	*label_buffer_list = malloc(sizeof(char *) * num_rows);


	while((result_row = mysql_fetch_row(result))) {

		*nodeid_list[node_count] = strtoull(result_row[0], NULL, 10);
		*value_list[node_count] = (UINT)strtoul(result_row[1], NULL, 10);

		*label_buffer_list[node_count] = malloc(LABEL_BUFFER_LEN);
		memcpy(*label_buffer_list[node_count], result_row[2], LABEL_BUFFER_LEN);

		num_rows++;
	}

	mysql_free_result(result);

	return num_rows;
}




