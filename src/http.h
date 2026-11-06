#ifndef FUKFUKGO_HTTP_H
#define FUKFUKGO_HTTP_H

// Performs an HTTP GET request.
// Returns a dynamically allocated string containing the response body.
// The caller must call free() on the returned buffer when finished.
// Returns NULL on failure.
char *http_get(const char *url);

// URL-encodes a query string so spaces and symbols are safe for URLs.
// The caller must call free() on the returned buffer when finished.
char *http_url_encode(const char *str);

#endif // FUKFUKGO_HTTP_H
