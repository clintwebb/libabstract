/*

	libabstract
	(c) 2009 Copyright Hyper-Active Systems, Australia

	Contact: 
	Clinton Webb
	webb.clint@gmail.com

	libabstract is a simple library intended to provide an interface to 
	maintain a master connection to a mysql database, and a slave server.
	It does not bother with more than one slave, although that capability 
	may be added in the future.
	
	It will often get the configuration information from a file.  
	When a database connection is needed, it returns a valid and opened 
	MYSQL handle.   This handle can then be used and closed by the requestor.  
	The library will not maintain handles.

	The slaves will be kept in a round-robin queue, and will be rotated 
	evenly.  Although it is tempting to add complicated prioritization and 
	class functionality, in practice it probably wouldn't be that useful for 
	most situations.

	It is released under GPL v2 or later license.  Details are included in the 
	LICENSE file.

*/

#ifndef __ABSTRACT_H
#define __ABSTRACT_H

#include <mysql/mysql.h>


#define LIBABSTRACT_VERSION 0x00000100
#define LIBABSTRACT_VERSION_TEXT "v0.01"


// The abstract data structure.  This will contain all the information loaded from the config file, and will be used to 
typedef struct {
	
	char *filecontent;	// buffer used to read the entire contents of the file.

	struct {
		const char *host;
		const char *user;
		const char *pass;
		const char *db;
		unsigned int port;
	} master, slave;
	
	char internally_created;
} abstract_t;


// Initialise an abstract_t object.   If 'abstract' parameter is NULL, then it 
// will allocation memory and initialise it, returning the pointer to it.  
// When using abstract_free it will free memory if it created it.
abstract_t * abstract_init(abstract_t *abstract);


// Free the resources used by the abstract structure.  If abstract_init was 
// used to create the object, then this function will free it also.
void abstract_free(abstract_t *abstract);


// return the version number, so that programs can ensure that the compiled 
// library is the same as the header they are referencing.
// It should be used something like:
//   if (abstract_version() != LIBABSTRACT_VERSION) {
//		fprintf(stderr, "Incorrect header version for libabstract.\n");
//		exit(1);
//   }
//
unsigned int abstract_version(void);


// Load the abstract config from the file.  Will return the number of hosts 
// that have been configured.  If it returns 0, then there are no hosts, and 
// there is a problem that should be corrected.
int abstract_loadfile(abstract_t *abstract, const char *filename);


// Return a MYSQL handle for a connection that can be used to read from the 
// database.  If there are no slave connections configured, this function will 
// return a connection to the master.
MYSQL * abstract_reader(abstract_t *abstract);


// Return a MYSQL handle for a connection that can be used to write to the 
// database (and read).  If there are no master connections (or the connect 
// fails), this function will return a NULL.
MYSQL * abstract_writer(abstract_t *abstract);


#endif
