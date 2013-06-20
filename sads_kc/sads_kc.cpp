#include <stdio.h>

#include <glib.h>
#include <kchashdb.h>

#include "sads_kc.h"
#include "sads_common.h"


using namespace std;
using namespace kyotocabinet;

//#define datum_set(um, buf, size) { um.dptr = buf; um.dsize = size; }

/*****************************************************************************
*
*	Variables
*
*****************************************************************************/
HashDB kc_label_db;
HashDB kc_leaf_answer_db;


/*****************************************************************************
*
*	interface to MySQL database
*
*****************************************************************************/
void kc_init() {

	// open the database
	if (!kc_label_db.open(KC_LABEL_FILENAME, HashDB::OWRITER | HashDB::OCREATE | HashDB::OTRUNCATE)) {
		cerr << "open error: " << kc_label_db.error().name() << endl;
		exit(-1);
	}

	if (!kc_leaf_answer_db.open(KC_LEAF_ANSWER_FILENAME, HashDB::OWRITER | HashDB::OCREATE | HashDB::OTRUNCATE)) {
		cerr << "open error: " << kc_leaf_answer_db.error().name() << endl;
		exit(-1);
	}

	DEBUG_TRACE(("sgdbm_init() done.\n"));

}

void kc_close() {
	// close the database
	if (!kc_label_db.close()) {
		cerr << "close error: " << kc_label_db.error().name() << endl;
		exit(-1);
	}

	if (!kc_leaf_answer_db.close()) {
		cerr << "close error: " << kc_leaf_answer_db.error().name() << endl;
		exit(-1);
	}

}

/*****************************************************************************
*
*	SELECT Queries
*
*****************************************************************************/
ULONG kc_get_leaf_val(ULONG nodeid) {
	// retrieve a record
	ULONG retVal = 0;

	if (!kc_leaf_answer_db.get((char*)&nodeid, sizeof(ULONG), (char*)&retVal, sizeof(ULONG))) {
		retVal = 0;
	}

	DEBUG_TRACE(("kc_get_leaf_val(%llu, %u)\n", nodeid, retVal));

	return retVal;

}

char *kc_get_node_label(ULONG nodeid, UINT buffer_len) {

	char *label_buffer = (char *)malloc(buffer_len);

	// retrieve a record
	if ((UINT)kc_label_db.get((char*)&nodeid, sizeof(ULONG), label_buffer, buffer_len) != buffer_len) {
		free(label_buffer);
		label_buffer = NULL;
	}

	return label_buffer;
}


#if 0
void sgdbm_get_leaf_nodeids_in_range(ULONG start_nodeid, ULONG end_nodeid, GHashTable *nodeid_table) {
	/** NOT IMPLEMENTED */
	return;
}
#endif



/*****************************************************************************
*
*	INSERT query
*
*****************************************************************************/

void kc_insert_value(ULONG nodeid, ULONG visit_count) {
	DEBUG_TRACE(("kc_insert_value: (%llu, %u)\n", nodeid, visit_count));

	if (!kc_leaf_answer_db.set((char *)&nodeid, sizeof(nodeid), (char *)&visit_count, sizeof(visit_count)) ) {
		cerr << "set error: " << kc_leaf_answer_db.error().name() << endl;
		exit(-1);
	}

	return;
}

void kc_insert_label(ULONG nodeid, char *label_buffer, UINT label_buffer_len) {
	DEBUG_TRACE(("kc_insert_label: %llu, %u\n", nodeid, label_buffer_len));

	if (!kc_label_db.set((char *)&nodeid, sizeof(nodeid), label_buffer, label_buffer_len) ) {
		cerr << "set error: " << kc_label_db.error().name() << endl;
		exit(-1);
	}

	return;
}









