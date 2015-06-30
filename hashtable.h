struct entry_s {
	char *key;
	char *value;
	struct entry_s *next;
};
 
typedef struct entry_s entry_t;
 
struct hashtable_s {
	int size;
	struct entry_s **table;	
};
 
typedef struct hashtable_s hashtable_t;

hashtable_t *ht_create( int size ); //creating hashtable
int ht_hash( hashtable_t *hashtable, char *key ); //hash function for hashtable
entry_t *ht_newpair( char *key, char *value ); 
void ht_set( hashtable_t *hashtable, char *key, char *value ); //creates and sets the values in the hashtable
char *ht_get( hashtable_t *hashtable, char *key ); //returns value corresponding to the key
void print_chain(hashtable_t *hashtable, char * key); //printing the chain of strings for a given node in hashtable.
void print_hashtable(hashtable_t *hashtable);
void remove_entry(char *value ,hashtable_t * hashtable);