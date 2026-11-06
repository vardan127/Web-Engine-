#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "http.h"
#include "parser.h"

static void perform_search(const char *query) {
    if (!query || strlen(query) == 0) {
        printf("Please enter a valid search term.\n");
        return;
    }

    char *encoded_query = http_url_encode(query);
    if (!encoded_query) {
        fprintf(stderr, "[Error] Failed to encode search query.\n");
        return;
    }

    char url[1024];
    snprintf(url, sizeof(url), "https://api.duckduckgo.com/?q=%s&format=json&no_html=1&skip_disambig=0", encoded_query);
    free(encoded_query);

    printf("\nSearching FukFukGo for: \"%s\"...\n", query);

    char *json_data = http_get(url);
    if (!json_data) {
        fprintf(stderr, "[Error] Failed to fetch search results from DuckDuckGo API.\n");
        return;
    }

    SearchResponse *resp = parse_search_response(json_data);
    print_search_response(resp);

    free_search_response(resp);
    free(json_data);
}

int main(int argc, char *argv[]) {
    printf("======================================================\n");
    printf("              Vardan's Search Engine (C)              \n");
    printf("======================================================\n");

    if (argc > 1) {
        // Query passed directly via command line arguments
        char query_buf[512] = {0};
        for (int i = 1; i < argc; i++) {
            strncat(query_buf, argv[i], sizeof(query_buf) - strlen(query_buf) - 2);
            if (i < argc - 1) {
                strncat(query_buf, " ", sizeof(query_buf) - strlen(query_buf) - 1);
            }
        }
        perform_search(query_buf);
        return 0;
    }

    // Interactive REPL loop
    char input_buf[256];
    while (1) {
        printf("\nvardan> ");
        if (!fgets(input_buf, sizeof(input_buf), stdin)) {
            break;
        }

        // Strip trailing newline
        input_buf[strcspn(input_buf, "\r\n")] = '\0';

        if (strcmp(input_buf, "exit") == 0 || strcmp(input_buf, "quit") == 0) {
            printf("Exiting Vardan's Search. Goodbye!\n");
            break;
        }

        if (strlen(input_buf) == 0) {
            continue;
        }

        perform_search(input_buf);
    }

    return 0;
}
