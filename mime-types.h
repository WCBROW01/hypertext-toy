#ifndef MIME_TYPES_H
#define MIME_TYPES_H

// Returns the extension of a file (if there is one)
const char *get_file_ext(const char *path);

// Looks up the mime type associated with a file extension
// Returns the mime type on success, or NULL if it is unknown.
const char *lookup_mime_type(const char *ext);

#endif