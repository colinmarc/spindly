#include "spdy_nv_block.h"

#include <stdlib.h>
#include <string.h>
#include <netinet/in.h>
#include <assert.h>

/**
 * Parse a Name/Value block payload.
 * @param nv_block - Target block.
 * @param data - Data to parse.
 * @see spdy_nv_block
 * @todo Replace mallocs with a single one. (Speed up!)
 * @todo Freeing in the loop.
 * @todo Multiple value support.
 * @return 0 on success, -1 on failure.
 */
int spdy_nv_block_parse(spdy_nv_block *nv_block, char *data) {
	// Read the 16 bit integer containing the number of name/value pairs.
	nv_block->count = ntohs(*((uint16_t*) data));
	assert(nv_block->count > 0);

	// Allocate memory for Name/Value pairs.
	nv_block->pairs = calloc(nv_block->count, sizeof(spdy_nv_pair));
	// Malloc failed
	if(!nv_block->pairs) {
		return -1;
	}

	// Move forward by two bytes.
	data += 2;

	uint16_t item_length;
	size_t size;
	// Loop through all pairs
	for(int i=0; i < nv_block->count; i++) {
		spdy_nv_pair *pair = &nv_block->pairs[i];

		// Read Name
		// Read length of name
		item_length = ntohs(*((uint16_t*) data));
		data += 2;
		// Allocate space for name
		size = (sizeof(char)*item_length)+1;
		pair->name = malloc(size);
		if(!pair->name) {
			return -1;
		}
		memcpy(pair->name, data, item_length);
		pair->name[item_length] = '\0';
		data += item_length;

		// Read Values
		// Read length of value
		item_length = ntohs(*((uint16_t*) data));
		data += 2;
		// Allocate space for values
		size = (sizeof(char)*item_length)+1;
		pair->values = malloc(size);
		if(!pair->name) {
			return -1;
		}
		memcpy(pair->values, data, item_length);
		pair->values[item_length] = '\0';
		data += item_length;
	}

	return 0;
}

/**
 * Pack a Name/Value block into a payload for transmitting.
 * @param dest - Destination buffer.
 * @param dest_size - Pointer for storing the size of the destination buffer.
 * @param nv_block - NV block to pack.
 * @see spdy_nv_block
 * @see spdy_nv_block_parse
 * @todo Multiple value support.
 * @return 0 on succes, -1 on failure.
 */
int spdy_nv_block_pack(char **dest, size_t *dest_size, spdy_nv_block *nv_block) {
	*dest = NULL;

	// Two bytes for the number of pairs.
	*dest_size = 2;
	// Calculate the size needed for the ouput buffer.
	for(int i=0; i < nv_block->count; i++) {
		// Two bytes (length) + stringlength
		*dest_size += 2+strlen(nv_block->pairs[i].name);
		*dest_size += 2+strlen(nv_block->pairs[i].values);
	}

	// Allocate memory for dest
	*dest = malloc(sizeof(char)*(*dest_size));
	if(!*dest) {
		return -1;
	}
	char *cursor = *dest;

	*((uint16_t*)cursor) = htons(nv_block->count);
	cursor += 2;
	uint16_t length;
	for(int i=0; i < nv_block->count; i++) {
		length = strlen(nv_block->pairs[i].name);
		*((uint16_t*)cursor) = htons(length);
		memcpy(
				cursor+2,
				nv_block->pairs[i].name,
				length);
		cursor += length+2;
		length = strlen(nv_block->pairs[i].values);
		*((uint16_t*)cursor) = htons(length);
		memcpy(
				cursor+2,
				nv_block->pairs[i].values,
				*((uint16_t*)cursor));
		cursor += length+2;
	}
	return 0;
}

