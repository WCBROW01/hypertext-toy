/**
 * @file mime-types.h
 * @author Will Brown
 * @brief MIME type definitions
 * @version 0.1
 * @date 2023-03-05
 *
 * @copyright Copyright (c) 2023 Will Brown
 */

#ifndef MIME_TYPES_H
#define MIME_TYPES_H

void load_mime_type_list(void);

/**
 * @brief Get the extension of a file (if there is one)
 * 
 * @param path the file path to get the extension of
 * @return the file extension, or NULL if there isn't one
 */
const char *get_file_ext(const char *path);

/**
 * @brief Looks up the mime type associated with a file extension
 * 
 * @param ext the extension to look up
 * @return the mime type on success, or NULL if it is unknown.
 */
const char *lookup_mime_type(const char *ext);

#endif
