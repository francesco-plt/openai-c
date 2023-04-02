#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include "../lib/jsmn/jsmn.h"
#define MAX_RESPONSE_LENGTH 4096
#define API_KEY_LENGTH 51
#define MAX_PROMPT_LENGTH 512
#define MAX_JSON_LENGTH 4096
#define MAX_TOKENS 256

int validate_json(const char *payload)
{
    int ret = 0;

    jsmn_parser parser;
    jsmntok_t tokens[128];

    jsmn_init(&parser);
    int num_tokens = jsmn_parse(&parser, payload, strlen(payload), tokens, sizeof(tokens) / sizeof(tokens[0]));

    if (num_tokens < 0)
    {
        fprintf(stderr, "Failed to parse JSON: %d\n", num_tokens);
        ret = -1;
    }
    else if (num_tokens < 1 || tokens[0].type != JSMN_OBJECT)
    {
        fprintf(stderr, "Invalid JSON payload\n");
        ret = -1;
    }

    return ret;
}

char *get_content(const char *json)
{
    jsmn_parser parser;
    jsmntok_t tokens[MAX_TOKENS];
    int num_tokens;

    jsmn_init(&parser);
    num_tokens = jsmn_parse(&parser, json, strlen(json), tokens, MAX_TOKENS);

    if (num_tokens < 0)
    {
        fprintf(stderr, "Failed to parse JSON: %d\n", num_tokens);
        return NULL;
    }

    for (int i = 0; i < num_tokens; i++)
    {
        if (tokens[i].type == JSMN_STRING && tokens[i].size == 1 && strncmp(json + tokens[i].start, "content", 7) == 0)
        {
            return strndup(json + tokens[i + 1].start, tokens[i + 1].end - tokens[i + 1].start);
        }
    }

    return NULL;
}

size_t write_callback(char *ptr, size_t size, size_t nmemb, void *userdata)
{
    // printf("%.*s", (int)(size * nmemb), ptr);
    printf("GPT-3: %s\n", get_content(ptr));
    return size * nmemb;
}

int https_post(const char *url, const char *payload, const char *api_key)
{
    CURL *curl_handle;
    CURLcode res;
    struct curl_slist *headers = NULL;

    curl_global_init(CURL_GLOBAL_DEFAULT);

    curl_handle = curl_easy_init();

    if (curl_handle)
    {
        // Set the URL
        curl_easy_setopt(curl_handle, CURLOPT_URL, url);

        // Set the callback function
        curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, write_callback);

        // Set the POST data
        curl_easy_setopt(curl_handle, CURLOPT_POSTFIELDS, payload);

        // Set the Authorization header
        char auth_header[256];
        snprintf(auth_header, sizeof(auth_header), "Authorization: Bearer %s", api_key);

        // Set up Content-Type and Authorization headers
        headers = curl_slist_append(headers, "Content-Type: application/json");
        headers = curl_slist_append(headers, auth_header);
        curl_easy_setopt(curl_handle, CURLOPT_HTTPHEADER, headers);

        // Perform the request
        res = curl_easy_perform(curl_handle);

        // Check for errors
        if (res != CURLE_OK)
            fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));

        // Cleanup
        curl_easy_cleanup(curl_handle);
        curl_slist_free_all(headers);
    }
    else
    {
        fprintf(stderr, "curl_easy_init() failed\n");
        return 1;
    }

    curl_global_cleanup();

    return 0;
}

int main(int argc, char *argv[])
{
    char *host = "api.openai.com";
    char *endpoint = "/v1/chat/completions";
    char api_key[API_KEY_LENGTH + 1];
    char *model = "gpt-3.5-turbo";
    char url[92];
    char *url_fmt = "https://%s%s";
    // char *chat_payload_fmt = "{\"model\":\"%s\",\"messages\":%s}";
    // char *chat_msg_array_fmt = "[%s]";
    // char *chat_msg_fmt = "{\"role\":\"user\",\"content\":\"%s\"}";

    if (getenv("OPENAI_API_KEY") == NULL)
    {
        fprintf(stderr, "OPENAI_API_KEY environment variable not set\n");
        return -1;
    }
    else if (strlen(getenv("OPENAI_API_KEY")) != API_KEY_LENGTH)
    {
        fprintf(stderr, "OPENAI_API_KEY environment variable is not the correct length\n");
        return -1;
    }
    strcpy(api_key, getenv("OPENAI_API_KEY"));

    printf("[Q to quit] [Max length: %d characters]\n", MAX_PROMPT_LENGTH);
    do
    {
        size_t prompt_len;
        char prompt[MAX_PROMPT_LENGTH];

        printf("You: ");
        fgets(prompt, MAX_PROMPT_LENGTH, stdin);

        prompt_len = strlen(prompt);
        if (prompt_len < 0 || prompt_len > MAX_PROMPT_LENGTH)
        {
            fprintf(stderr, "Prompt length must be between 0 and %d characters\n", MAX_PROMPT_LENGTH);
            continue;
        }

        if (prompt[prompt_len - 1] == '\n')
        {
            prompt[prompt_len - 1] = '\0';
        }
        if (strcmp(prompt, "Q") == 0 || strcmp(prompt, "q") == 0)
        {
            break;
        }

        char payload[256 + strlen(prompt)];
        sprintf(payload, "{\"model\":\"%s\",\"messages\":[{\"role\":\"user\",\"content\":\"%s\"}]}", model, prompt);

        sprintf(url, url_fmt, host, endpoint);

        if (validate_json(payload) == 0)
        {
            if (https_post(url, payload, api_key) != 0)
            {
                fprintf(stderr, "Error in getting the response\n");
            }
        }
        else
        {
            fprintf(stderr, "Invalid JSON payload\n");
        }
    } while (1);
    return 0;
}
