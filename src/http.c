#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

struct MemoryChunk {
    char *memory;
    size_t size;
};

static size_t write_memory_callback(void *contents, size_t size, size_t nmemb, void *userp) {
    size_t realsize = size * nmemb;
    struct MemoryChunk *mem = (struct MemoryChunk *)userp;

    char *ptr = realloc(mem->memory, mem->size + realsize + 1);
    if (!ptr) {
        fprintf(stderr, "[Error] Not enough memory to allocate HTTP response buffer.\n");
        return 0;
    }

    mem->memory = ptr;
    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

char *http_get(const char *url) {
    CURL *curl;
    CURLcode res;
    struct MemoryChunk chunk;

    chunk.memory = malloc(1);
    chunk.size = 0;
    if (!chunk.memory) {
        return NULL;
    }
    chunk.memory[0] = '\0';

    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        free(chunk.memory);
        return NULL;
    }

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_memory_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "vardan-browser/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 10L);
#ifdef _WIN32
    curl_easy_setopt(curl, CURLOPT_SSL_OPTIONS, CURLSSLOPT_NATIVE_CA);
#endif

    res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        fprintf(stderr, "[Error] HTTP request failed: %s\n", curl_easy_strerror(res));
        free(chunk.memory);
        chunk.memory = NULL;
    }

    curl_easy_cleanup(curl);
    curl_global_cleanup();

    return chunk.memory;
}

char *http_url_encode(const char *str) {
    CURL *curl = curl_easy_init();
    if (!curl) {
        return NULL;
    }

    char *encoded = curl_easy_escape(curl, str, 0);
    if (!encoded) {
        curl_easy_cleanup(curl);
        return NULL;
    }

    // Duplicate string with standard malloc so caller can free with free()
    char *result = strdup(encoded);
    curl_free(encoded);
    curl_easy_cleanup(curl);

    return result;
}
