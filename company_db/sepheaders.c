#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <curl/curl.h>
#include <mongoc.h>
#include <bson.h>

/**
 * Callback function for curl, allows the data from the curl read
 * to be saved to a file, rather than stdout or a file
 */
size_t callback(void *ptr, size_t size, size_t nmemb, void *data);

/**
 * Parses company data from wikipedia. This returns an array of
 * company names. This potentially will be rerouted to man-god-db
 */
char** parse_wiki(FILE* html);

/**
 * Parses company data from wikipedia. This returns an array of
 * company names, which, again, will likely be uploaded to mongodb
 */
char** parse_csv(FILE* csv);

void mongo_upload(char* name);

char* get_address(char* name);

char* clean(char* name);

struct MemoryStruct {
    char *memory;
    size_t size;
};

//MongoDB
mongoc_client_t *client;
mongoc_collection_t *collection;
mongoc_cursor_t *cursor;
bson_error_t error;

char alpha[26] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

int main(int argc, char** argv)
{
  /**
   * MongoDB
   */  
  mongoc_init();
  client = mongoc_client_new ("mongodb://localhost:27017/");
  collection = mongoc_client_get_collection (client, "Curiousity", "Companies");

  //CSV File stuff
  if (argc < 4) return 1;
  FILE* csv = fopen(argv[2], "r"); 
  FILE* html = fopen(argv[3], "r");

  //Process and generate from NASDAQ csv
  parse_csv(csv);
  
  parse_wiki(html);


  mongoc_collection_destroy (collection);
  mongoc_client_destroy (client);

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

char** parse_wiki(FILE* html) {
    fseek(html, 0, SEEK_END);
    long length = ftell(html);
    fseek(html, 0, SEEK_SET);
    char* body = malloc(length);
    if (body) {
        fread(body, 1, length, html);
    }
    fclose(html);

    if (body == NULL) printf("true\n");
    char* locate_start = strstr(body, "\"Current_companies\"");
    locate_start = strstr(body, "<li>");

    char* tok;
    char* tmp;
    tok = strtok(locate_start, "\n");

    int pastAlpha = 0;
    int currentAlpha = 0;

    while (tok != NULL) {
        int i = strstr(tok, "</a>") - strstr(tok, "\">");
        char* ptr = strstr(tok, "\">");
        if (ptr != NULL) {
            tmp = malloc(sizeof(char)*50);
            if (i > 2 && i < 75) {
                memcpy(tmp, ptr+2, i-1);
                tmp[i-2] = '\0';
                pastAlpha = currentAlpha;
                currentAlpha = (int)(*tmp);
                if (pastAlpha == 90 && currentAlpha < 90) break;
                //THIS IS WHERE I UPLOAD TO MONGO
                mongo_upload(tmp);
            }
        }

        tok = strtok(NULL, "\n");
    }
    

    return NULL;
}

char** parse_csv(FILE* csv) {
    char* tmp = malloc(sizeof(char)*1000);
    size_t len = 0;
    int num = 0;
    while (getline(&tmp, &len, csv) != -1) {
        //printf("%s\n", tmp);
        char* tok = strtok(tmp, "\"");
        int count = 0;
        while (tok != NULL && count < 3) {
            //UPLOAD TO MONGO HERE
            
            if (count == 2 && num > 0) mongo_upload(tok);

            tok = strtok(NULL, "\"");
            count++;
        }
        num++;
    }

    fclose(csv);

    return NULL;
}

void mongo_upload(char* name) {
    bson_t *doc;
    bson_oid_t oid;
    doc = bson_new ();
    bson_oid_init (&oid, NULL);
    BSON_APPEND_OID (doc, "_id", &oid);
    BSON_APPEND_UTF8 (doc, "Name", name);
    char* n = get_address(clean(name));
    if (n == NULL) return;
    BSON_APPEND_UTF8 (doc, "Logo", n);

    
    if (!mongoc_collection_insert (collection, MONGOC_INSERT_NONE, doc, NULL, &error)) {
        printf ("Oh no %s\n", error.message);
    }
  
    bson_destroy (doc);
}

char* get_address(char* name) {
    char buff[200];
    strcpy(buff, "./webparse ");
    strcat(buff, name);
    strcat(buff, "+logo");
    
    printf("QUERY: %s\n", buff);
 
    FILE* file = popen(buff, "r");
    char* body = malloc(100000);
    fread(body, 1, 100000, file);
    fclose(file);
     
    char* pos = strstr(body, "src=\"https://encrypted-tbn0.gstatic.com/images?q=");
    if (pos == NULL) return NULL;
    char* pos2 = strstr(pos, "\" ");
    if (pos2 == NULL) return NULL;
    char* three = malloc(2000);
    memcpy(three, pos+5, (int)(pos2-pos)-5);
   
    return three; 
}

char* clean(char* name) {
    //char* retur = malloc(255);
    char* mvr = name;
    int count = 0;

    while (*mvr != '\0') {
        if (*mvr == '(' || *mvr == ')' || *mvr == '\'' || *mvr == '&') {
            *mvr = ' ';
            //retur[count] = *name;
            count++;
        }
        mvr++;
    }
    //retur[count] = '\0';
    return name;
}
