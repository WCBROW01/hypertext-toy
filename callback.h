/**
 * @file callback.h
 * @author Will Brown
 * @brief Default callbacks for HTTP server
 * @version 0.1
 * @date 2023-08-31
 *
 * @copyright Copyright (c) 2023 Will Brown
 */

#ifndef HTT_CALLBACK_H
#define HTT_CALLBACK_H

#include "connection.h"

/**
 * @brief Service a HTTP request for a connection
 * @details This will recover a HTTP header, parse it, and prepare the server to respond
 *
 * @param conn the connection to service the request for
 * @return 0 on success, -1 on failure
 */
int http_request_callback(htt_connection_t *conn);

/**
 * @brief Send the header for a HTTP response
 * @details This will send chunks of the response until there is no more to send.
 *
 * @param conn the connection to send the response for
 * @return 0
 */
int http_response_header_callback(htt_connection_t *conn);

/**
 * @brief Send the content for a HTTP response
 * @details This will send chunks of the response until there is no more to send.
 *
 * @param conn the connection to send the response for
 * @return 0
 */
int http_response_content_callback(htt_connection_t *conn);

#endif // HTT_CALLBACK_H
