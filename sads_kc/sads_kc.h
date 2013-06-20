#ifndef __SADS_GDBM__
#define __SADS_GDBM__

//#include <stdlib.h>
//#include <string.h>

#include "typedef.h"


#define KC_LABEL_FILENAME "label.kch"
#define KC_LEAF_ANSWER_FILENAME "leaf_answer.kch"


/*****************************************************************************
*
*	interface to MySQL database
*
*****************************************************************************/
void kc_init();
void kc_close();

/*****************************************************************************
*
*	SELECT Queries
*
*****************************************************************************/
ULONG kc_get_leaf_val(ULONG nodeid);
char *kc_get_node_label(ULONG nodeid, UINT buffer_len);

//void skc_get_leaf_nodeids_in_range(ULONG start_nodeid, ULONG end_nodeid, GHashTable *nodeid_table);
//void sgdbm_get_node_info(ULONG nodeid, UINT *value, char *label_buffer);


/*****************************************************************************
*
*	INSERT query
*
*****************************************************************************/
void kc_insert_value(ULONG nodeid, ULONG visit_count);
void kc_insert_label(ULONG nodeid, char *label_buffer, UINT label_buffer_len);

//void sgdbm_add_node(ULONG nodeid, char *label_buffer, UINT label_buffer_len, BOOL isLeaf);


/*****************************************************************************
*
*	UPDATE query
*
*****************************************************************************/
//void sgdbm_update_node(ULONG nodeid, UINT new_visit_count, char *label_buffer, UINT label_buffer_len, BOOL isLeaf);



#endif
