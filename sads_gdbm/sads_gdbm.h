#ifndef __SADS_GDBM__
#define __SADS_GDBM__

#include <stdio.h>
#include <stdlib.h>
//#include <string.h>
#include <gdbm.h>

#include "typedef.h"


#include <glib.h>


#define GDBM_LABEL_FILENAME "label.gdbm"
#define GDBM_LEAF_ANSWER_FILENAME "leaf_answer.gdbm"


GDBM_FILE gdbm_label_file;
GDBM_FILE gdbm_leaf_answer_file;



/*****************************************************************************
*
*	interface to MySQL database
*
*****************************************************************************/
void sgdbm_init();
void sgdbm_close();

/*****************************************************************************
*
*	SELECT Queries
*
*****************************************************************************/
UINT sgdbm_get_leaf_val(ULONG nodeid);
char *sgdbm_get_node_label(ULONG nodeid);
void sgdbm_get_leaf_nodeids_in_range(ULONG start_nodeid, ULONG end_nodeid, GHashTable *nodeid_table);
//void sgdbm_get_node_info(ULONG nodeid, UINT *value, char *label_buffer);


/*****************************************************************************
*
*	INSERT query
*
*****************************************************************************/
void sgdbm_insert_value(ULONG nodeid, UINT visit_count);
void sgdbm_insert_label(ULONG nodeid, char *label_buffer, UINT label_buffer_len);

//void sgdbm_add_node(ULONG nodeid, char *label_buffer, UINT label_buffer_len, BOOL isLeaf);


/*****************************************************************************
*
*	UPDATE query
*
*****************************************************************************/
//void sgdbm_update_node(ULONG nodeid, UINT new_visit_count, char *label_buffer, UINT label_buffer_len, BOOL isLeaf);





#endif
