/**
 * @file config.h
 * @author Will Brown
 * @brief Mime type definitions
 * @version 0.1
 * @date 2023-03-05
 *
 * @copyright Copyright (c) 2023 Will Brown
 */

#ifndef MIME_TYPES_H
#define MIME_TYPES_H

// Returns the extension of a file (if there is one)
const char *get_file_ext(const char *path);

// Looks up the mime type associated with a file extension
// Returns the mime type on success, or NULL if it is unknown.
const char *lookup_mime_type(const char *ext);

#endif
