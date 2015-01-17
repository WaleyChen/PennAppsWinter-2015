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
  
  //INITIAL PROCESSING (SEARCHING FOR ARTICLE)  
  struct MemoryStruct* ms = malloc(sizeof(struct MemoryStruct));
  ms->memory = malloc(sizeof(char)*10000);
  CURL *curl_handle;
           
  curl_global_init(CURL_GLOBAL_ALL);
  curl_handle = curl_easy_init();
  char buff[2000];
  strcpy(buff, "http://en.wikipedia.org/w/index.php?title=Special:Search&profile=default&search=");
  strcat(buff, argv[1]);
  strcat(buff, "&fulltext=Search&profile=advanced");
  int i;
  for (i = 0; i < 2000; i++) {
      if (buff[i] == ' ') buff[i] = '+';
  }
  curl_easy_setopt(curl_handle, CURLOPT_URL, buff);
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
      
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, callback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, ms);
               
  curl_easy_perform(curl_handle);
  if (ms->memory == NULL) printf("false");

  if (strstr(ms->memory, "There were no results") != NULL) {
      /**
       *
       * CALL THE STOCK PAGE, AS THERE IS NO
       * WIKI ARTICLE
       */
      return 0;
  }
  
  //BEGIN SEARCHING FOR ARTICLE LINK ON PAGE
  char* linkptr = strstr(ms->memory, "<div class='mw-search-result-heading'><a href=\"");
  if (linkptr == NULL) {printf("SOMETHING HORRIBLE WENT WRONG.\n"); return 0;}
  char* linkend = strstr(linkptr, "\" title");
  char* adr = malloc(255);
  if (linkend != NULL) {
      //All Good
      memcpy(adr, linkptr + 47, (int)(linkend-linkptr)-46);
      adr[(int)(linkend-linkptr)-47]='\0';
  } else printf("SOMETHING MOR EHORRIBLE\n");
  //curl_easy_cleanup(curl_handle);

  strcpy(buff, "http://en.wikipedia.org");
  strcat(buff, adr);
  //printf("%s\n", buff);

  //THE ADDRESS IS IN 'buff' NOW GO TO ADDRESS AND TAKE INFO
  ms = malloc(sizeof(struct MemoryStruct));
  ms->memory = malloc(sizeof(char)*10000);
  //curl_handle = curl_easy_init();
  curl_easy_setopt(curl_handle, CURLOPT_URL, buff);
  curl_easy_setopt(curl_handle, CURLOPT_NOPROGRESS, 1L);
      
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, callback);
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, ms);
               
  curl_easy_perform(curl_handle);
  //printf("%s\n", ms->memory);
  linkptr = strstr(ms->memory, "<table class=\"info");
  if (linkptr == NULL) {printf("ERROR 1\n"); return 1;}
  linkend = strstr(linkptr, "</table>");
  if (linkend == NULL) {printf("ERROR 2\n"); return 1;}



  char newbuff[10000];
  memcpy(newbuff, linkptr, (int)(linkend-linkptr));
  strcat(newbuff, "</table>");
  FILE* output = fopen("out.html", "w");
  fprintf(output, "%s", newbuff);
  fclose(output);
  strcpy(buff, "wkhtmltoimage out.html out.jpg");
  system(buff);

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
