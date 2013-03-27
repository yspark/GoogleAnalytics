#include <my_global.h>
#include <mysql.h>

#include "typedef.h"
#include "sads_common.h"

#define SADS_TABLE sads_tree


/** variables **/
MYSQL *smysql_conn;


/** functions **/
void smysql_init();
void smysql_close();
void smysql_table_init();

MYSQL_RES* smysql_query(char *query);
MYSQL_RES* smysql_real_query(char *query, int query_len);

UINT smysql_get_leaf_val(ULONG nodeid);
void smysql_add_leaf(ULONG nodeid);
void smysql_update_leaf(ULONG nodeid, UINT new_visit_count);

void smysql_add_inter_node(ULONG nodeid, char *label_buffer, UINT label_buffer_len);
void smysql_update_inter_node(ULONG nodeid, char *label_buffer, UINT label_buffer_len);
char *smysql_get_node_label(ULONG nodeid);

#if 0
void smysql_add_element(UINT ip_addr);
#endif


