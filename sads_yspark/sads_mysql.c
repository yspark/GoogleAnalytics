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
	smysql_query("CREATE TABLE IF NOT EXISTS sads_tree (nodeid BIGINT UNSIGNED, PRIMARY KEY(nodeid), visit_count INT UNSIGNED, digest LONGBLOB, digest_len INT UNSIGNED, label LONGBLOB, label_len INT UNSIGNED)");

	/*
	int i = 0;
	char query[128];

	for(i = 0; i < UNIVERSE_SIZE_IN_BIT; i++) {
		sprintf(query, "CREATE TABLE IF NOT EXISTS sads_tree_%d (nodeid INT UNSIGNED, visit_count INT UNSIGNED, digest LONGBLOB, label LONGBLOB)", i);	
	  send_mysql_query(query);
	}
	*/
}



UINT smysql_get_leaf_val(ULONG nodeid) {
	char query[128];
	MYSQL_RES *result;
	MYSQL_ROW result_row;
	UINT retVal;

	sprintf(query, "SELECT visit_count FROM sads_tree WHERE nodeid=%llu LIMIT 1", nodeid);
	result = smysql_query(query);
	
	result_row = mysql_fetch_row(result);
	retVal = (result_row == NULL) ? 0 : atoi(result_row[0]);
	
	mysql_free_result(result);
	
	DEBUG_TRACE(("smysql_get_leaf_info(%llu):%u\n", nodeid, retVal));
	
	return retVal;


}

void smysql_add_leaf(ULONG nodeid, char *digest_buffer, UINT digest_buffer_len, char *label_buffer, UINT label_buffer_len) {
	char *digest_chunk = malloc(digest_buffer_len * 2);
	char *label_chunk = malloc(label_buffer_len * 2);
	char *command = NULL; 
	char *query = NULL;
	UINT query_len = 0, est_query_len = 0;

	//mysql_real_escape_string(smysql_conn, digest_chunk, digest_buffer, digest_buffer_len);
	//mysql_real_escape_string(smysql_conn, label_chunk, label_buffer, label_buffer_len);

	command = "INSERT INTO sads_tree(nodeid, visit_count, digest, digest_len, label, label_len) VALUES(%llu, 1, '%s', %d, '%s', %d)";
		
	est_query_len = strlen(command) + digest_buffer_len + sizeof(UINT) + label_buffer_len + sizeof(UINT);
	printf("command %d, digest %u, label %u, est_query_len %u\n", (int)strlen(command), digest_buffer_len, label_buffer_len, est_query_len);
	//est_query_len = strlen(command) + strlen(digest_chunk) + strlen(label_chunk);
	//printf("command %d, digest %d, label %d, est_query_len %d\n", (int)strlen(command), (int)strlen(digest_chunk), (int)strlen(label_chunk), est_query_len);
	
	query = malloc(est_query_len);
	
	query_len = snprintf(query, est_query_len, command, nodeid, digest_buffer, digest_buffer_len, label_buffer, label_buffer_len);
	//query_len = snprintf(query, est_query_len, command, nodeid, digest_chunk, strlen(digest_chunk), label_chunk, strlen(label_chunk));

	printf("%s\n", query);
	smysql_real_query(query, query_len);

	DEBUG_TRACE(("smysql_add_leaf(%llu)\n", nodeid));
}


void smysql_update_leaf(ULONG nodeid, UINT new_visit_count) {
	char query[128];
	
	sprintf(query, "UPDATE sads_tree SET visit_count=%u WHERE nodeid=%llu", new_visit_count, nodeid);
	smysql_query(query);		
	
	DEBUG_TRACE(("smysql_update_leaf: node(%llu) visit_count(%u)\n", nodeid, new_visit_count));
}


char *smysql_get_node_label(ULONG nodeid) {
	char query[128];
	MYSQL_RES *result;
	MYSQL_ROW result_row;
	char *label_buffer = NULL;

	DEBUG_TRACE(("smysql_get_node_label(%llu)\n", nodeid));

	sprintf(query, "SELECT label_len, label FROM sads_tree WHERE nodeid=%llu LIMIT 1", nodeid);
	result = smysql_query(query);

	result_row = mysql_fetch_row(result);
	if(result_row != NULL) {
		printf("%llu label\n", nodeid);
		printf("%s\n", result_row[0]);
		printf("%s\n", result_row[1]);
	}
	//retVal = (result_row == NULL) ? 0 : atoi(result_row[0]);

	mysql_free_result(result);

	return label_buffer;

}


void smysql_add_inter_node(ULONG nodeid, char *label_buffer, UINT label_buffer_len) {
	char *command = NULL;
	char *query = NULL;
	UINT query_len = 0, est_query_len = 0;

	command = "INSERT INTO sads_tree(nodeid, label, label_len) VALUES(%llu, '%s', %d)";

	est_query_len = strlen(command) + sizeof(ULONG) + label_buffer_len + sizeof(UINT) ;
	printf("command %d, label %u, est_query_len %u\n", (int)strlen(command), label_buffer_len, est_query_len);

	query = malloc(est_query_len);
	query_len = snprintf(query, est_query_len, command, nodeid, label_buffer, label_buffer_len);

	smysql_real_query(query, query_len);

	DEBUG_TRACE(("smysql_add_inter_node(%llu)\n", nodeid));

}


void smysql_update_inter_node(ULONG nodeid, char *label_buffer, UINT label_buffer_len) {
	char *command = NULL;
	char *query = NULL;
	UINT query_len = 0, est_query_len = 0;

	command = "UPDATE sads_tree SET label=%s WHERE nodeid=%llu";

	est_query_len = strlen(command) + label_buffer_len + sizeof(ULONG);
	printf("command %d, label %u, est_query_len %u\n", (int)strlen(command), label_buffer_len, est_query_len);

	query = malloc(est_query_len);
	query_len = snprintf(query, est_query_len, command, nodeid, label_buffer, label_buffer_len);

	smysql_real_query(query, query_len);

	DEBUG_TRACE(("smysql_update_inter_node(%llu)\n", nodeid));
}




#if 0
void smysql_add_element(UINT ip_addr) {
	char query[128];
	MYSQL_RES *result;
	MYSQL_ROW result_row;

	sprintf(query, "SELECT visit_count FROM sads_tree WHERE nodeid=%d LIMIT 1", ip_addr);
	result = smysql_query(query);
	
	result_row = mysql_fetch_row(result);
	
	// Add new node
	if( result_row == NULL ) {
		DEBUG_TRACE(("add a new node(%d)\n", ip_addr));
		sprintf(query, "INSERT INTO sads_tree(nodeid, visit_count) VALUES(%d, 1)", ip_addr);
		smysql_query(query);
	}
	// Update existing node
	else {
		int curr_visit_count = atoi(result_row[0]);
		
		sprintf(query, "UPDATE sads_tree SET visit_count=%d WHERE nodeid=%d", ++curr_visit_count, ip_addr);
		smysql_query(query);		
		
		DEBUG_TRACE(("Update: node(%d) visit_count(%d)\n", ip_addr, curr_visit_count));
	}
	
	mysql_free_result(result);
}
#endif

