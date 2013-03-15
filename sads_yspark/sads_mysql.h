#include <my_global.h>
#include <mysql.h>

#include "typedef.h"
#include "sads_common.h"


void smysql_init();
void smysql_table_init();

BOOL smysql_get_node_info(UINT ip_addr, UINT *curr_count, gsl_vector *digest, gsl_vector *label);
void smysql_add_leaf(UINT ip_addr, char *digest_buffer, UINT digest_buffer_len, char *label_buffer, UINT label_buffer_len);
void smysql_update_leaf(UINT ip_addr);

MYSQL *smysql_conn;
