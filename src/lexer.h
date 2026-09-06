//
// Created by nkinder on 9/4/26.
//

#pragma once
#include "TokenList.h"
#include <stddef.h>


/**
 * *****************************************************************************
 * SECTION NAME: Lexing primitives(stream reading)
 * *****************************************************************************
 */

extern const int EOI; /**< Representation of the End-of-input state. */

/**
 * Models a stream of characters with a known length.
 */
typedef struct CharStream {
    const char* data; /**< The underlying buffer of character data. */
    size_t      pos;  /**< The current position of the stream. */
    size_t      len;  /**< The length of the underlying buffer.*/
} CharStream;

/**
 * Allocates a new CharStream. This constructor copies the given buffer, so that the stream
 * owns its own buffer.
 *
 * @param data The data buffer to use
 * @return A new CharStream object if successful, nullptr otherwise.
 */
CharStream* cs_new(const char* data);

/**
 * Destructor for CharStream.
 *
 * @param stream The stream to delete
 */
void cs_delete(CharStream* stream);

/**
 * Returns the next character(or -1) without advancing the stream forward.
 *
 * @param stream
 * @return The next character in the stream, or -1 on error or EOI
 */
int cs_peek(CharStream* stream);

/**
 * Peeks n chars ahead in the stream.
 *
 * @param stream
 * @param n The number of chars to read ahead.
 * @return The char(or -1) n positions ahead in the stream.
 */
int cs_peek_ahead(CharStream* stream, int n);

/**
 * Return the next char in the stream while also advancing the stream forward.
 *
 * @param stream
 * @return The next char in the stream. Or -1 on failure.
 */
int cs_read(CharStream* stream);

/**
 * Check if the stream is at the end.
 * @param stream
 * @return True if the stream is at EOI, false otherwise.
 */
bool cs_eoi(CharStream* stream);

/**
 * This checks if the next character in the stream is c. If it is, the stream is advanced
 * forward. If not, the function returns false and the position doesn't change.
 *
 * @param stream
 * @param c The char to match
 * @return True if the stream matched and advanced forward
 */
bool cs_match(CharStream* stream, int c);

/**
 * Resets the stream back to the beginning.
 *
 * @param stream
 */
void cs_reset(CharStream* stream);

/**
 * *****************************************************************************
 * SECTION NAME: Lexing
 * *****************************************************************************
 */


/**
 * @brief Tracks the current quoting state while tokenizing input.
 *
 * @c DOUBLE_QUOTED is defined as @c 0x3 so that it overlaps the bit
 * pattern of @c SINGLE_QUOTED (0x1); code that only needs to know "some
 * kind of quoting is active" can test with a bitwise AND against
 * @c SINGLE_QUOTED.
 */
typedef enum QuoteFlagE {
  UNQUOTED,           /**< No quoting is currently active. */
  SINGLE_QUOTED,      /**< Inside a single-quoted (`'...'`) span. */
  DOUBLE_QUOTED = 0x3 /**< Inside a double-quoted (`"..."`) span. */
} QuoteFlagE;

/**
 * Scans the input until it has reached the end of a token and then returns its type.
 * Begin and end are two output parameters. After this function returns begin will be
 * pointing at the start of a token of the returned type, and end will be pointed one past that.
 * That does mean that, if this function scans the last token in the stream, end will pointed
 * at one past the stream buffer. So it is not safe to dereference end after this function is called.
 */
TokenType lx_scan(CharStream* stream, char** begin, char** end);

/**
 * Pulls a token from the stream if it can. If the stream is at the end or it otherwise
 * encounters an erroneous state, this will return a nullptr. Otherwise, it returns an
 * allocated Token.
 */
Token* lx_token(CharStream* stream);

/**
 * Takes an input stream and tokenizes it. The function will consume the entire stream
 * and use it to insert tokens into dest. If the function fails for any reason, it will
 * return with an errno. This function clears errno at the start, clearing it outside
 * the function is not necessary. The function, if it fails, will clean up any tokens
 * it has created. It will not clean up anything that was passed to it (the stream and
 * the list will not be freed). If you want to change this behavior, use the function
 * *set_cleanup_policy*, which will toggle a static variable that controls cleanup behavior.
 *
 * @param dest A list to insert tokens into
 * @param stream The stream to read.
 * @return 0 upon success, anything else indicates some sort of failure.
 */
int lx_tokenize(TokenList* dest, CharStream* stream);

typedef enum CleanupPolicy {
  LX_CLEANUP,
  LX_NOCLEANUP
} CleanupPolicy;

void set_cleanup_policy(CleanupPolicy new_policy);
#define lx_clean set_cleanup_policy(true)
#define lx_noclean set_cleanup_policy(false)

/**
 * Determines if the given character, in combination with the current flag state,
 * delimits the end of a token region.
 *
 * @param c The current char
 * @param flag The current quote flag
 * @return true if this has reached the end of a token. False if it can keep reading.
 */
bool lx_end_of_token(int c, QuoteFlagE flag);