#include "sads_gdbm.h"


#define datum_set(um, buf, size) { um.dptr = buf; um.dsize = size; }

/*****************************************************************************
*
*	Variables
*
*****************************************************************************/
GDBM_FILE gdbm_label_file;
GDBM_FILE gdbm_leaf_answer_file;


/*****************************************************************************
*
*	interface to MySQL database
*
*****************************************************************************/
void sgdbm_init() {
	//gdbm_label_file = gdbm_open(GDBM_LABEL_FILENAME, 0, GDBM_WRCREAT, 0666, 0);
	gdbm_label_file = gdbm_open(GDBM_LABEL_FILENAME, 0, GDBM_NEWDB, 0666, 0);


	if(gdbm_label_file == NULL)
	{
		fprintf (stderr, "File %s either doesn't exist or is not a gdbm file.\n", GDBM_LABEL_FILENAME);
		exit (2);
	}

	//gdbm_leaf_answer_file = gdbm_open(GDBM_LEAF_ANSWER_FILENAME, 512, GDBM_WRCREAT, 0666, 0);
	gdbm_leaf_answer_file = gdbm_open(GDBM_LEAF_ANSWER_FILENAME, 512, GDBM_NEWDB, 0666, 0);

	if(gdbm_leaf_answer_file == NULL)
	{
		fprintf (stderr, "File %s either doesn't exist or is not a gdbm file.\n", GDBM_LEAF_ANSWER_FILENAME);
		exit (2);

	}

	DEBUG_TRACE(("sgdbm_init() done.\n"));

}

void sgdbm_close() {
	gdbm_close(gdbm_label_file);
	gdbm_close(gdbm_leaf_answer_file);
}

/*****************************************************************************
*
*	SELECT Queries
*
*****************************************************************************/
UINT sgdbm_get_leaf_val(ULONG nodeid) {
	datum key, data;
	UINT retVal;

	//DEBUG_TRACE(("sgdbm_get_leaf_val(%llu)\n", nodeid));

	/** build datum key */
	datum_set(key, (char*)&nodeid, sizeof(ULONG));

	/** retrieve datum data */
	data = gdbm_fetch(gdbm_leaf_answer_file, key);

	if (data.dptr == NULL)
		retVal = 0;
	else
		retVal = *(data.dptr);

	/** free memory */
	free(data.dptr);

	DEBUG_TRACE(("sgdbm_get_leaf_val(%llu, %u)\n", nodeid, retVal));

	return retVal;

}

char *sgdbm_get_node_label(ULONG nodeid) {
	datum key, data;

	/** build datum key */
	datum_set(key, (char*)&nodeid, sizeof(ULONG));

	/** retrieve datum data */
	data = gdbm_fetch(gdbm_label_file, key);


	/** return */
	DEBUG_TRACE(("sgdbm_get_node_label(%llu)\n", nodeid));

	return data.dptr;
}


void sgdbm_get_leaf_nodeids_in_range(ULONG start_nodeid, ULONG end_nodeid, GHashTable *nodeid_table) {
	/** NOT IMPLEMENTED */
	return;
}




/*****************************************************************************
*
*	INSERT query
*
*****************************************************************************/

void sgdbm_insert_value(ULONG nodeid, UINT visit_count) {
	datum key, data;
	//ULONG *p_nodeid, *p_new_visit_count;

	DEBUG_TRACE(("sgdbm_insert_value: (%llu, %u)\n", nodeid, visit_count));

	/** build datum key */
	//p_nodeid = malloc(sizeof(ULONG));
	//*p_nodeid = nodeid;
	//p_nodeid = &nodeid;
	datum_set(key, (char *)&nodeid, sizeof(ULONG));


	/** build datum data */
	//p_new_visit_count = malloc(sizeof(UINT));
	//*p_new_visit_count = visit_count;
	//p_new_visit_count = &visit_count;
	datum_set(data, (char *)&visit_count, sizeof(UINT));


	/** insert */
	if(gdbm_store(gdbm_leaf_answer_file, key, data, GDBM_REPLACE) != 0) {
		fprintf (stderr, "sgdbm_insert_value(%llu)\n", nodeid);
		exit (2);
	}

	/** return */
	//free(p_nodeid);
	//free(p_new_visit_count);

	return;
}

void sgdbm_insert_label(ULONG nodeid, char *label_buffer, UINT label_buffer_len) {
	datum key, data;
	ULONG *p_nodeid;

	DEBUG_TRACE(("sgdbm_insert_label: %llu, %u\n", nodeid, label_buffer_len));

	/** build datum key */
	p_nodeid = (ULONG *)malloc(sizeof(ULONG));
	*p_nodeid = nodeid;
	datum_set(key, (char*)p_nodeid, sizeof(ULONG));


	/** build datum data */
	datum_set(data, label_buffer, label_buffer_len);


	/** insert */
	if(gdbm_store(gdbm_label_file, key, data, GDBM_REPLACE) != 0) {
		fprintf (stderr, "sgdbm_insert_label(%llu)\n", nodeid);
		exit (2);
	}

	/** return */
	free(p_nodeid);

	return;
}


#if 0

void sgdbm_add_node(ULONG nodeid, char *label_buffer, UINT label_buffer_len, BOOL isLeaf) {
	datum key, data;
	ULONG *p_nodeid;

	DEBUG_TRACE(("sgdbm_add_node: %llu, %u\n", nodeid, label_buffer_len));

	/** build datum key */
	p_nodeid = malloc(sizeof(ULONG));
	*p_nodeid = nodeid;
	datum_set(key, (char*)p_nodeid, sizeof(ULONG));


	/** build datum data */
	datum_set(data, label_buffer, label_buffer_len);


	/** insert */
	if(gdbm_store(gdbm_label_file, key, data, GDBM_REPLACE) != 0) {
		fprintf (stderr, "sgdbm_add_node(%llu)\n", nodeid);
		exit (2);
	}

	/** return */
	free(p_nodeid);

	return;
}


/*****************************************************************************
*
*	UPDATE query
*
*****************************************************************************/
void sgdbm_update_node(ULONG nodeid, UINT new_visit_count, char *label_buffer, UINT label_buffer_len, BOOL isLeaf) {
	datum key, data;
	ULONG *p_nodeid, *p_new_visit_count;

	DEBUG_TRACE(("sgdbm_update_node: (%llu, %u)\n", nodeid, new_visit_count));

	/** build datum key */
	p_nodeid = malloc(sizeof(ULONG));
	*p_nodeid = nodeid;
	datum_set(key, (char*)p_nodeid, sizeof(ULONG));


	/** build datum data */
	p_new_visit_count = malloc(sizeof(UINT));
	*p_new_visit_count = new_visit_count;
	datum_set(data, (char *)p_new_visit_count, sizeof(UINT));


	/** insert */
	if(gdbm_store(gdbm_leaf_answer_file, key, data, GDBM_REPLACE) != 0) {
		fprintf (stderr, "sgdbm_update_node(%llu)\n", nodeid);
		exit (2);
	}

	/** insert label */
	sgdbm_add_node(nodeid, label_buffer, label_buffer_len, isLeaf);


	/** return */
	free(p_nodeid);
	free(p_new_visit_count);

	return;
}
#endif









