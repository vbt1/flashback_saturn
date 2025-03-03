#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// Reduced memory footprint configuration
#define TOTAL_BITS 16
#define LOOKAHEAD_BITS 7  // Back to 7 bits to reduce memory needs
#define WINDOW_BITS (TOTAL_BITS - LOOKAHEAD_BITS)
#define LOOKAHEAD_SIZE ((1 << LOOKAHEAD_BITS) - 1)
#define WINDOW_SIZE ((1 << WINDOW_BITS) - 1)
#define MIN_MATCH_LENGTH 3
#define MAX_BUFFER_SIZE 100000

// Flag to distinguish between match and literal
#define LITERAL_FLAG 0x80

// Optimization 1: Small sliding window hash table
#define HASH_SIZE 256  // Drastically reduced hash table size
#define HASH_MASK (HASH_SIZE - 1)
#define MAX_CHAIN_LENGTH 8  // Short chain length for memory efficiency
#define HASH_SHIFT 4  // Reduced shift for simpler hash

// Memory-efficient hash table structure
typedef struct {
    short head[HASH_SIZE];  // Using short instead of int
    short prev[WINDOW_SIZE];  // Circular buffer of previous positions
} SmallHashTable;

// Simple hash function suited for embedded systems
static inline unsigned char hash_function(const unsigned char *data) {
    return ((data[0] << HASH_SHIFT) ^ data[1]) & HASH_MASK;
}

// Initialize the small hash table
void init_small_hash_table(SmallHashTable *ht) {
    memset(ht->head, -1, sizeof(ht->head));
    memset(ht->prev, -1, sizeof(ht->prev));
}

// Insert a position into the small hash table
void insert_string_small(SmallHashTable *ht, const unsigned char *data, int pos) {
    unsigned char hash = hash_function(data);
    int window_pos = pos & (WINDOW_SIZE - 1);  // Position in circular buffer
    
    ht->prev[window_pos] = ht->head[hash];
    ht->head[hash] = window_pos;
}

// Improved token storage format
void store_token(unsigned char *dest, int *out_pos, unsigned short offset, unsigned char length, unsigned char next_byte) {
    if (length < MIN_MATCH_LENGTH) {
        // Store as literal
        dest[(*out_pos)++] = LITERAL_FLAG | next_byte;
    } else {
        // Store match with improved format
        unsigned short combined = ((offset & WINDOW_SIZE) << LOOKAHEAD_BITS) | (length & LOOKAHEAD_SIZE);
        dest[(*out_pos)++] = (combined >> 8) & 0xFF; // Store high byte
        dest[(*out_pos)++] = combined & 0xFF;        // Store low byte
        dest[(*out_pos)++] = next_byte;
    }
}

void read_token(const unsigned char *src, int *in_pos, unsigned short *offset, unsigned char *length, unsigned char *next_byte) {
    unsigned char first_byte = src[(*in_pos)++];
    
    if (first_byte & LITERAL_FLAG) {
        // This is a literal
        *offset = 0;
        *length = 0;
        *next_byte = first_byte & 0x7F;
    } else {
        // This is a match
        unsigned short combined = ((unsigned short)(first_byte) << 8) | src[(*in_pos)++];
        *offset = (combined >> LOOKAHEAD_BITS) & WINDOW_SIZE;
        *length = combined & LOOKAHEAD_SIZE;
        *next_byte = src[(*in_pos)++];
    }
}

// Memory-efficient compression algorithm
int lz77_compress(const unsigned char *input, unsigned char *output, int input_size) {
    int pos = 0, out_pos = 0;
    SmallHashTable ht;
    
    init_small_hash_table(&ht);
    
    while (pos < input_size && out_pos < MAX_BUFFER_SIZE - 4) {
        unsigned short best_offset = 0;
        unsigned char best_length = 0;
        
        // Skip if we don't have enough data for matching
        if (pos + MIN_MATCH_LENGTH <= input_size) {
            // Use small hash table for memory-efficient match finding
            unsigned char hash = hash_function(&input[pos]);
            int chain_length = MAX_CHAIN_LENGTH;
            short idx = ht.head[hash];
            
            while (idx >= 0 && chain_length-- > 0) {
                // Calculate actual buffer position
                int true_pos = pos - (((pos & (WINDOW_SIZE - 1)) - idx) & (WINDOW_SIZE - 1));
                if (pos - true_pos <= WINDOW_SIZE && true_pos >= 0) {
                    int length = 0;
                    // Find match length
                    while (length < LOOKAHEAD_SIZE && 
                           (pos + length < input_size) && 
                           input[true_pos + length] == input[pos + length]) {
                        length++;
                    }
                    
                    // Update best match if better
                    if (length > best_length) {
                        best_length = length;
                        best_offset = pos - true_pos;
                        // Early termination for very good matches
                        if (length >= 16)  // Lower threshold for early termination
                            break;
                    }
                }
                idx = ht.prev[idx];
            }
        }
        
        // Insert current string
        if (pos + MIN_MATCH_LENGTH <= input_size) {
            insert_string_small(&ht, &input[pos], pos);
        }
        
        // Process match or literal
        if (best_length >= MIN_MATCH_LENGTH) {
            // We have a good match
            unsigned char next_byte = (pos + best_length < input_size) ? 
                                     input[pos + best_length] : 0;
            
            store_token(output, &out_pos, best_offset, best_length, next_byte);
            
            // Skip ahead but only insert every other position to save memory operations
            for (int i = 2; i < best_length && pos + i + MIN_MATCH_LENGTH <= input_size; i += 2) {
                insert_string_small(&ht, &input[pos + i], pos + i);
            }
            
            pos += best_length + 1;
        } else {
            // Store as literal
            store_token(output, &out_pos, 0, 0, input[pos]);
            pos++;
        }
    }
    
    return out_pos;
}

// Decompressor function with buffer size check
int lz77_decompress(const unsigned char *input, unsigned char *output, int compressed_size) {
    int in_pos = 0;
    unsigned char *out_ptr = output;
    
    while (in_pos < compressed_size) {
        unsigned short offset;
        unsigned char length, next_byte;
        
        // Read a token
        read_token(input, &in_pos, &offset, &length, &next_byte);
        
        // Check for possible overflow while copying from the sliding window
        if (length >= MIN_MATCH_LENGTH) {
            // This is a match
            if (offset > (out_ptr - output)) {
                //printf("Error: Decompression offset exceeds decompressed data range.\n");
                return -1;  // Handle error gracefully
            }

            unsigned char *copy_src = out_ptr - offset;
            // Ensure we are within bounds for the decompressed data buffer
            if (copy_src < output || copy_src + length > out_ptr) {
                //printf("Error: Decompression copying out of bounds.\n");
                return -1;
            }

            for (int i = 0; i < length; i++) {
                *out_ptr++ = *copy_src++;
            }
        }
        
        // Always append the next byte (or literal)
        if (out_ptr >= output + MAX_BUFFER_SIZE) {
            //printf("Error: Decompression overflow detected.\n");
            return -1;  // Handle error gracefully
        }
        
        *out_ptr++ = next_byte;
    }
    
    return (out_ptr - output);
}
