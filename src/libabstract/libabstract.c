/*

	libabstract
	(c) 2009 Copyright Hyper-Active Systems, Australia

	Contact: 
		Clinton Webb
		webb.clint@gmail.com

	libabstract is a simple library intended to provide an interface to 
	maintain a master connection to a mysql database, and a number of slave 
	servers.   It will often get the configuration information from a file.  
	When a database connection is needed, it returns a valid and opened 
	MYSQL handle.   This handle can then be used and closed by the requestor.  
	The library will not maintain handles.

	The slaves will be kept in a round-robin queue, and will be rotated 
	evenly.  Although it is tempting to add complicated prioritization and 
	class functionality, in practice it probably wouldn't be that useful for 
	most situations.
*/


#include "abstract.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#if (LIBABSTRACT_VERSION != 0x00000200)
	#error "Incorrect header version."
#endif


//-----------------------------------------------------------------------------
// clear the internal content of the structure.  Assumes that the structure 
// contains garbage.
static void abstract_clear(abstract_t *a)
{
	a->filecontent = NULL;
	
	a->master.host = NULL;
	a->master.user = NULL;
	a->master.pass = NULL; 
	a->master.db = NULL;
	a->master.port = 0;
	
	a->slave.host = NULL;
	a->slave.user = NULL;
	a->slave.pass = NULL; 
	a->slave.db = NULL;
	a->slave.port = 0;
}

//-----------------------------------------------------------------------------
// Initialize the 'abstract' object.  If 'abstract' is NULL, then allocate 
// memory for it.  If an abstract pointer is provided, then initialise that.
abstract_t * abstract_init(abstract_t *abstract)
{
	abstract_t *a;
	
	if (abstract == NULL) {
		a = (abstract_t *) malloc(sizeof(abstract_t));
		a->internally_created = 1;
	}
	else {
		a = abstract;
		a->internally_created = 0;
	}

	abstract_clear(a);
	
	assert(a);
	return(a);
}



//-----------------------------------------------------------------------------
// Free the resources used by the abstract structure.  If abstract_init was 
// used to create the object, then this function will free it also.
void abstract_free(abstract_t *abstract)
{
	assert(abstract);

	if (abstract->filecontent) {
		free(abstract->filecontent);
	}
	
	// Technically we dont need to do this, but since we may be freeing an 
	// object that is controlled elsewhere, it might be better to clear it 
	// out, and potentially catch some bugs that would otherwise be hard to 
	// detect.
	abstract_clear(abstract);
	
	if (abstract->internally_created > 0) {
		free(abstract);
	}
}



//-----------------------------------------------------------------------------
// return the version number, so that programs can ensure that the compiled 
// library is the same as the header they are referencing.
// It should be used something like:
//	if (abstract_version() != LIBABSTRACT_VERSION) {
//		fprintf(stderr, "Incorrect header version for libabstract.\n");
//		exit(1);
//   }
//
unsigned int abstract_version(void)
{
	assert(LIBABSTRACT_VERSION > 0);
	return(LIBABSTRACT_VERSION);
}
   


//-----------------------------------------------------------------------------
// Load the abstract config from the file.  Will return the number of hosts 
// that have been configured.  If it returns 0, then there are no hosts, and 
// there is a problem that should be corrected.
int abstract_loadfile(abstract_t *abstract, const char *filename)
{
	int hosts = 0;			// number of hosts found in the config file.
	long buflen = 0;		// length of the buffer needed to read the file.
	FILE *fp;				// file pointer to open the file with.
	char *next;				// when going through the file, this will point to the next line to process.
	char *line;				// a line from the file, to be processed.
	int len;				// length of the line being processed.
	char *key;				// when processing the line, split it into a key/value pair.
	char *value;			// the value of a key from a line being processed.
	
	assert(abstract);
	assert(filename);
	assert(abstract->filecontent == NULL);
	
	// open the file.
	fp = fopen(filename, "r");
	if (fp) {
		// find out the length of the file, so that we know how much memory to allocate.
		fseek(fp, 0, SEEK_END);
		buflen = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		assert(buflen > 0);
		
		// read all the data from the file into a string buffer.
		abstract->filecontent = (char *) malloc(buflen+1);
		fread(abstract->filecontent, buflen, 1, fp);
		abstract->filecontent[buflen] = 0;		// make sure the data is null-terminated

		// close file.
		fclose(fp);
		fp = NULL;
		
		// strsep on the line-feeds.
		next = abstract->filecontent;
		while (next != NULL && *next != '\0') {
			line = strsep(&next, "\n");
			assert(line);
			
// 			printf("abstract: line='%s'\n", line);

			// strip leading blanks or tabs
			while (*line == ' ' || *line == '\t') { line ++; }
			
			// check for comment
			if (*line != '#' && *line != ';') {
				// strip dos line ending if found.
				len = strlen(line);
				while (len > 0 && (line[len-1] == ' ' || line[len-1] == '\r' || line[len-1] == '\t')) {
					len --;
					line[len] = 0;
				}
				
				// check for blank line
				if (len > 0) {
				
					// strsep the '='.
					value = line;
					key = strsep(&value, "=");
					assert(key);
					assert(value);
					
// 					printf("abstract: key='%s', value='%s'\n", key, value);
					
					
					// compare the key against known paramaters.
					if (strcasecmp(key, "master_host") == 0) {
						assert(abstract->master.host == NULL);
						abstract->master.host = value;
						hosts++;
					}
					else if (strcasecmp(key, "master_user") == 0) {
						assert(abstract->master.user == NULL);
						abstract->master.user = value;
						hosts++;
					}
					else if (strcasecmp(key, "master_pass") == 0) {
						assert(abstract->master.pass == NULL);
						abstract->master.pass = value;
						hosts++;
					}
					else if (strcasecmp(key, "master_db") == 0) {
						assert(abstract->master.db == NULL);
						abstract->master.db = value;
						hosts++;
					}
					else if (strcasecmp(key, "master_port") == 0) {
						assert(abstract->master.port == 0);
						abstract->master.port = atoi(value);
						hosts++;
					}
					else if (strcasecmp(key, "slave_host") == 0) {
						assert(abstract->slave.host == NULL);
						abstract->slave.host = value;
						hosts++;
					}
					else if (strcasecmp(key, "slave_user") == 0) {
						assert(abstract->slave.user == NULL);
						abstract->slave.user = value;
						hosts++;
					}
					else if (strcasecmp(key, "slave_pass") == 0) {
						assert(abstract->slave.pass == NULL);
						abstract->slave.pass = value;
						hosts++;
					}
					else if (strcasecmp(key, "slave_db") == 0) {
						assert(abstract->slave.db == NULL);
						abstract->slave.db = value;
						hosts++;
					}
					else if (strcasecmp(key, "slave_port") == 0) {
						assert(abstract->slave.port == 0);
						abstract->slave.port = atoi(value);
						hosts++;
					}
					else {
						fprintf(stderr, "libabstract: Invalid config option.  '%s'=='%s'\n", key, value);
					}
				}
			}
		}
	}
	
	return(hosts);
}

//-----------------------------------------------------------------------------
// Return a MYSQL handle for a connection that can be used to read from the 
// database.  If there are no slave connections configured, this function will 
// return a connection to the master.
//
// It is the responsibility of the caller to close the mysql handle that is 
// returned.  The abstract library will not keep track of the database 
// connections, or close them.
MYSQL * abstract_reader(abstract_t *abstract)
{
	MYSQL *mysql = NULL;

	if (abstract->slave.host == NULL || abstract->slave.user == NULL) {
		mysql = abstract_writer(abstract);
	}
	else {
		assert(abstract->slave.host);
		assert(abstract->slave.user);
		assert(abstract->slave.pass);
		assert(abstract->slave.db);
		
		mysql = mysql_init(NULL);
		if (!mysql_real_connect(mysql, abstract->slave.host, abstract->slave.user, abstract->slave.pass, abstract->slave.db, abstract->slave.port, NULL, CLIENT_COMPRESS)) {
			mysql_close(mysql);
			mysql = abstract_writer(abstract);
		}
	}
	
	return(mysql);
}


//-----------------------------------------------------------------------------
// Return a MYSQL handle for a connection that can be used to write to the 
// database (and read).  If there are no master connections (or the connect 
// fails), this function will return a NULL.
MYSQL * abstract_writer(abstract_t *abstract)
{
	MYSQL *mysql = NULL;
	
	if (abstract->master.host && abstract->master.user) {
		assert(abstract->master.host);
		assert(abstract->master.user);
		assert(abstract->master.pass);
		assert(abstract->master.db);
		
		mysql = mysql_init(NULL);
		if (!mysql_real_connect(mysql, 
								abstract->master.host, 
								abstract->master.user, 
								abstract->master.pass, 
								abstract->master.db, 
								abstract->master.port, 
								NULL, CLIENT_COMPRESS)) {
			mysql_close(mysql);
			mysql = NULL;
		}
	}
	
	return(mysql);
}


