#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>

/**
 * Callback function for curl, allows the data from the curl read
 * to be saved to a file, rather than stdout or a file
 */
size_t callback(void *ptr, size_t size, size_t nmemb, void *data);

struct MemoryStruct {
    char *memory;
    size_t size;
};

int main(int argc, char** argv)
{
  /**
   * Web Parsing
   */
  struct MemoryStruct* ms = malloc(sizeof(struct MemoryStruct));
  ms->memory = malloc(sizeof(char)*10000);
  CURL *curl_handle;
           
  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();
  char buff[2000];
  strcpy(buff, "https://www.google.com/search?q=");
  strcat(buff, argv[1]);
  strcat(buff, "&hl=en&safe=off&tbo=d&gbv=2&tbas=0&sout=1&biw=1440&bih=796&tbs=ift:png&tbm=isch&ei=p34VUZn6JvSO2QWwpoGgDg&start=0&sa=N");
  printf("%s\n", buff);
  curl_easy_setopt(curl_handle, CURLOPT_URL, buff);
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
      
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, callback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, ms);
               
  curl_easy_perform(curl_handle);
  if (ms->memory == NULL) printf("false");
  printf("%s\n", ms->memory);
  curl_easy_cleanup(curl_handle);

  return 0;
}

size_t callback(void *ptr, size_t size, size_t nmemb, void *data)
{
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)data;

    mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
    if (mem->memory) {
        memcpy(&(mem->memory[mem->size]), ptr, realsize);
        mem->size += realsize;
        mem->memory[mem->size] = 0;
    }
    return realsize;
}
