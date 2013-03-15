#include <stdlib.h>
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

void smysql_table_init() {
	smysql_query("CREATE TABLE IF NOT EXISTS sads_tree (nodeid BIGINT UNSIGNED, PRIMARY KEY(nodeid), visit_count INT UNSIGNED, digest LONGBLOB, label LONGBLOB)");

	/*
	int i = 0;
	char query[128];

	for(i = 0; i < UNIVERSE_SIZE_IN_BIT; i++) {
		sprintf(query, "CREATE TABLE IF NOT EXISTS sads_tree_%d (nodeid INT UNSIGNED, visit_count INT UNSIGNED, digest LONGBLOB, label LONGBLOB)", i);	
	  send_mysql_query(query);
	}
	*/
}

BOOL smysql_get_node_info(UINT ip_addr, UINT *curr_count, gsl_vector *digest, gsl_vector *label) {
	char query[128];
	MYSQL_RES *result;
	MYSQL_ROW result_row;
	BOOL retVal;

	sprintf(query, "SELECT * FROM sads_tree WHERE nodeid=%d LIMIT 1", ip_addr);
	result = smysql_query(query);
	
	result_row = mysql_fetch_row(result);
	retVal = (result_row == NULL) ? false : true;
	
	mysql_free_result(result);
	
	DEBUG_TRACE(("get_node_info(%d):%d\n", ip_addr, retVal));
	
	return retVal;


}

void smysql_add_leaf(UINT ip_addr, char *digest_buffer, UINT digest_buffer_len, char *label_buffer, UINT label_buffer_len) {
	char query[128];

	sprintf(query, "INSERT INTO sads_tree(nodeid, visit_count) VALUES(%d, 1)", ip_addr);
	//smysql_query(query);
	
	DEBUG_TRACE(("add a new node(%d)\n", ip_addr));
}

void smysql_update_leaf(UINT ip_addr) {
	/*
	char query[128];
	int curr_visit_count = atoi(result_row[0]);
	
	sprintf(query, "UPDATE sads_tree SET visit_count=%d WHERE nodeid=%d", ++curr_visit_count, ip_addr);
	smysql_query(query);		
	
	DEBUG_TRACE(("Update: node(%d) visit_count(%d)\n", ip_addr, curr_visit_count));	
	*/
}

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


