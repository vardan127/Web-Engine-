#ifndef FUKFUKGO_PARSER_H
#define FUKFUKGO_PARSER_H

typedef struct {
    char *title;
    char *snippet;
    char *url;
} SearchResult;

typedef struct {
    char *heading;
    char *abstract_text;
    char *abstract_url;
    SearchResult *results;
    int result_count;
} SearchResponse;

// Parses DuckDuckGo JSON response into structured search results.
SearchResponse *parse_search_response(const char *json_raw);

// Frees all heap memory allocated for the response.
void free_search_response(SearchResponse *response);

// Prints the search results cleanly to the terminal.
void print_search_response(const SearchResponse *response);

#endif // FUKFUKGO_PARSER_H
