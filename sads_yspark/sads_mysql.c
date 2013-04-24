#include <stdlib.h>
#include <string.h>

#include "sads_mysql.h"

/*****************************************************************************
*
*	interface to MySQL database
*
*****************************************************************************/
void smysql_init() {
  smysql_conn = mysql_init(NULL);

  if (smysql_conn == NULL) {
      printf("Error %u: %s\n", mysql_errno(smysql_conn), mysql_error(smysql_conn));
      exit(1);
  }

  if (mysql_real_connect(smysql_conn, "localhost", "piwik", \
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
	smysql_query("CREATE TABLE IF NOT EXISTS sads_tree (nodeid BIGINT UNSIGNED, PRIMARY KEY(nodeid), visit_count INT UNSIGNED, label LONGBLOB)");
}



/*****************************************************************************
*
*	INSERT query
*
*****************************************************************************/
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


/*****************************************************************************
*
*	UPDATE query
*
*****************************************************************************/
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

/*****************************************************************************
*
*	SELECT Queries
*
*****************************************************************************/
/**
 * SELECT visit_count
 */
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


/**
 * SELECT label
 */
char *smysql_get_node_label(ULONG nodeid) {
	char query[128];
	MYSQL_RES *result;
	MYSQL_ROW result_row;
	char *label_buffer = NULL;
	unsigned long *lengths;

	DEBUG_TRACE(("smysql_get_node_label(%llu)\n", nodeid));

	sprintf(query, "SELECT label FROM sads_tree WHERE nodeid=%llu LIMIT 1", nodeid);
	result = smysql_query(query);

	result_row = mysql_fetch_row(result);

	if(result_row != NULL) {
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


/**
 * SELECT nodeid WHERE range
 */
void smysql_get_leaf_nodeids_in_range(ULONG start_nodeid, ULONG end_nodeid, GHashTable *nodeid_table) {
	char query[256];
	MYSQL_RES *result;
	MYSQL_ROW result_row;
	ULONG nodeid = 0;

	sprintf(query, "SELECT nodeid FROM sads_tree WHERE nodeid>=%llu AND nodeid <=%llu", start_nodeid, end_nodeid);
	result = smysql_query(query);

	DEBUG_TRACE(("smysql_get_leaf_nodeids_in_range(%llu, %llu): %u\n", start_nodeid, end_nodeid, num_rows));

	while((result_row = mysql_fetch_row(result))) {
		nodeid = strtoull(result_row[0], NULL, 10);
		g_hash_table_insert (nodeid_table, g_memdup(&nodeid, sizeof(nodeid)), g_memdup(&nodeid, sizeof(nodeid)));
	}

	return;
}

/**
 * SELECT visit_count, label
 */
void smysql_get_node_info(ULONG nodeid, UINT *value, char *label_buffer) {
	char query[128];
	MYSQL_RES *result;
	MYSQL_ROW result_row;

	unsigned long *lengths;

	DEBUG_TRACE(("smysql_get_node_info(%llu)\n", nodeid));

	sprintf(query, "SELECT label, visit_count FROM sads_tree WHERE nodeid=%llu LIMIT 1", nodeid);
	result = smysql_query(query);

	result_row = mysql_fetch_row(result);

	if(result_row != NULL) {
		lengths = mysql_fetch_lengths(result);

		DEBUG_TRACE(("MYSQL_RES rows(%d), num_field(%u), length(%u, %u)\n", (UINT)mysql_num_rows(result), (UINT)num_fields, (UINT)lengths[0], (UINT)lengths[1]));

		/** Node label **/
		if(lengths[0]) {
			if(lengths[0] != LABEL_BUFFER_LEN) {
				printf("wrong label_buffer_length(%llu)", nodeid);
				exit(1);
			}
			memcpy(label_buffer, result_row[0], lengths[0]);
		}

		/** Visit count **/
		if(lengths[1]) {
			*value = (UINT)strtoull(result_row[1], NULL, 10);
		}
	}

	mysql_free_result(result);

	return;

}



