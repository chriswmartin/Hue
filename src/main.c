#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <math.h>
#include <getopt.h>
#include <curl/curl.h>
#include "../lib/cJSON/cJSON.h"

/* ---- GROUP INDEX ----
    0   all
    1   Dining room
    3   Bedroom
    4   Living room
-------------------- */

char *ALL_GROUP = "0";
char *DINING_ROOM_GROUP = "1";
char *BEDROOM_GROUP = "3";
char *LIVING_ROOM_GROUP = "4";

char *USERNAME = NULL;
char *ADDRESS = NULL;

struct string {
  char *ptr;
  size_t len;
};

struct color {
  float x;
  float y;
};

struct user {
  char user[128];
  char ip_address[20];
};

// ----- global variables for command line flags -----
int RED = 255;
int GREEN = 207;
int BLUE = 120;

char *GROUP = "0";
// ---------------------------------------------------

struct user get_user(char *file, char *path);
struct color get_xy_color(float r, float g, float b);
int change_group_color(float x, float y, char *group);
char* get_group(char *group_id);
bool group_any_on(const char *json_input);
int set_group_state(char *group, char *state);
int toggle_group(char *group_id);
void init_string(struct string *s);
size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s);
size_t curl_write_silent(void *buffer, size_t size, size_t nmemb, void *userp);
void print_usage(void);
//char* parse_json(const char *json_input, const char *key);
//int print_json(const char *json_input);

int main (int argc, char **argv) {
  struct user current_user = get_user("env", argv[0]);
  USERNAME = current_user.user;
  ADDRESS = current_user.ip_address;

  int i;
    while (1) {
      static struct option long_options[] =
        {
          {"all",    no_argument, 0, 'a'},
          {"living-room",    no_argument, 0, 'l'},
          {"dining-room",    no_argument, 0, 'd'},
          {"bedroom",    no_argument, 0, 'b'},
          {"red",    required_argument, 0, 'r'},
          {"green",    required_argument, 0, 'g'},
          {"blue",    required_argument, 0, 'u'},
          {"toggle",    no_argument, 0, 't'},
          {"on",    no_argument, 0, 'o'},
          {"off",    no_argument, 0, 'f'},
          {"color",    no_argument, 0, 'c'},
          {"help",    no_argument, 0, 'h'},
          {0, 0, 0, 0}
        };
      int option_index = 0;
      i = getopt_long (argc, argv, "aldbrgutofch", long_options, &option_index);

      if (i == -1) {
        break;
      }

      switch (i) {
        case 0:
          if (long_options[option_index].flag != 0)
            break;
          printf ("option %s", long_options[option_index].name);
          if (optarg)
            printf (" with arg %s", optarg);
          printf ("\n");
          break;

        case 'a':
          GROUP = ALL_GROUP;
          break;

        case 'l':
          GROUP = LIVING_ROOM_GROUP;
          break;

        case 'd':
          GROUP = DINING_ROOM_GROUP;
          break;

        case 'b':
          GROUP = BEDROOM_GROUP;
          break;

        case 'r':
          if (atoi(optarg) >= 0 && atoi(optarg) <= 255){
            RED = atoi(optarg);
          } else {
              printf("ERROR: argument must be a number between 0-255\n");
          }
          break;

        case 'g':
          if (atoi(optarg) >= 0 && atoi(optarg) <= 255){
            GREEN = atoi(optarg);
          } else {
              printf("ERROR: argument must be a number between 0-255\n");
          }
          break;

        case 'u':
          if (atoi(optarg) >= 0 && atoi(optarg) <= 255){
            BLUE = atoi(optarg);
          } else {
              printf("ERROR: argument must be a number between 0-255\n");
          }
          break;

        case 't':
          toggle_group(GROUP);
          break;

        case 'o':
          set_group_state(GROUP, "true");
          break;

        case 'f':
          set_group_state(GROUP, "false");
          break;

        case 'c':
          if (1){ // FIX
            struct color current_color = get_xy_color(RED, GREEN, BLUE);
            change_group_color(current_color.x, current_color.y, GROUP);
          }
          break;

        case 'h':
          print_usage();
          break;

        case '?':
          print_usage();
          break;

        default:
          print_usage();
          abort ();
      }
    }


  if (optind < argc) {
    printf ("non-option ARGV-elements: ");
    while (optind < argc){
      printf ("%s ", argv[optind++]);
    }
    putchar ('\n');
    print_usage();
  }

  if (argc == 1) {
    print_usage();
  }

  exit (0);
}

struct user get_user(char *file, char *path){
  struct user current_user;

  char *del = &path[strlen(path)];
  while (del > path && *del != '/'){
    del--;
  }
  if (*del== '/') {
    *del= '\0';
  }

  char absolute_path[128] = {0};
  strcat(absolute_path, path);
  strcat(absolute_path, "/");
  strcat(absolute_path, file);

  FILE *file_pointer = fopen (absolute_path, "r");
  if (file_pointer == NULL) {
    printf("ERROR: no such file\n");
  }

  fscanf(file_pointer, "%s %s", current_user.user, current_user.ip_address);

  return current_user;
}

int toggle_group(char *group_id){
  char *group = get_group(group_id);
  if (group == NULL){
    printf("GROUP IS NULL\n");
    free(group);
    return 1;
  } else {
      bool group_any_on_state = group_any_on(group);
      if (group_any_on_state == true){ // lights on are on
        set_group_state(group_id, "false"); // turn lights off
      } else { // lights are off
          set_group_state(group_id, "true"); // turn lights on
      }
    }

  free(group);
  return 0;
}

char* get_group(char *group_id) {
  char *result = NULL;

  CURL *curl;
  CURLcode res;

  curl = curl_easy_init();
  if(curl) {
    struct string s;
    init_string(&s);

    char url[128] = {0};
    strcat(url, "http://");
    strcat(url, ADDRESS);
    strcat(url, "/api/");
    strcat(url, USERNAME);
    strcat(url, "/groups/");
    strcat(url, group_id);

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, writefunc);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &s);

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
    }

    result = s.ptr;

    curl_easy_cleanup(curl);
  } else {
      printf("GET GROUP FAILED - CURL NOT AVAILABLE\n");
  }

  return result;
}

bool group_any_on(const char *json_input) {
  bool return_value = false;

  cJSON *json = cJSON_Parse(json_input);
  cJSON *state = cJSON_GetObjectItemCaseSensitive(json, "state");
  cJSON *any_on = cJSON_GetObjectItemCaseSensitive(state, "any_on");

  if (cJSON_IsTrue(any_on)){
    return_value = true;
  } else {
      return_value = false;
  }

  cJSON_Delete(json);
  return return_value;
}

int set_group_state(char *group, char *state){
  CURL *curl;
  CURLcode res;

  char postthis[15] = {0};
  strcat(postthis, "{\"on\":");
  strcat(postthis, state);
  strcat(postthis, "}");

  curl = curl_easy_init();
  if(curl) {
    char url[100] = {0};
    strcat(url, "http://");
    strcat(url, ADDRESS);
    strcat(url, "/api/");
    strcat(url, USERNAME);
    strcat(url, "/groups/");
    strcat(url, group);
    strcat(url, "/action");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_silent);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(postthis));

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      return 1;
    }

    curl_easy_cleanup(curl);
  }

  return 0;
}

struct color get_xy_color(float r, float g, float b){
  struct color return_color;

  float red = r / 255;
  float green = g / 255;
  float blue = b / 255;

  // gamma correction
  float redCorrected = (red > 0.04045f)
                                 ? pow((red + 0.055f) / (1.0f + 0.055f), 2.4f)
                                 : (red / 12.92f);
  float greenCorrected =
      (green > 0.04045f) ? pow((green + 0.055f) / (1.0f + 0.055f), 2.4f)
                         : (green / 12.92f);
  float blueCorrected = (blue > 0.04045f)
                                  ? pow((blue + 0.055f) / (1.0f + 0.055f), 2.4f)
                                  : (blue / 12.92f);

  float X = redCorrected * 0.664511f + greenCorrected * 0.154324f +
                  blueCorrected * 0.162028f;
  float Y = redCorrected * 0.283881f + greenCorrected * 0.668433f +
                  blueCorrected * 0.047685f;
  float Z = redCorrected * 0.000088f + greenCorrected * 0.072310f +
                  blueCorrected * 0.986039f;

  return_color.x = X / (X + Y + Z);
  return_color.y = Y / (X + Y + Z);

  return return_color;
}

int change_group_color(float x, float y, char *group){
  CURL *curl;
  CURLcode res;

  char *x_string = (char *)malloc(20*sizeof(char));
  sprintf(x_string, "%f", x);

  char *y_string = (char *)malloc(20*sizeof(char));
  sprintf(y_string, "%f", y);

  char postthis[50] = {0};
  strcat(postthis, "{\"xy\":[");
  strcat(postthis, x_string);
  strcat(postthis, ",");
  strcat(postthis, y_string);
  strcat(postthis, "]}");

  curl = curl_easy_init();
  if(curl) {
    char url[100] = {0};
    strcat(url, "http://");
    strcat(url, ADDRESS);
    strcat(url, "/api/");
    strcat(url, USERNAME);
    strcat(url, "/groups/");
    strcat(url, group);
    strcat(url, "/action");

    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curl_write_silent);
    curl_easy_setopt(curl, CURLOPT_POSTFIELDS, postthis);
    curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "PUT");
    curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (long)strlen(postthis));

    res = curl_easy_perform(curl);
    if(res != CURLE_OK) {
      fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
      curl_easy_cleanup(curl);
      return 1;
    }

    curl_easy_cleanup(curl);
  }

  return 0;

}

void init_string(struct string *s) {
  s->len = 0;
  s->ptr = malloc(1);

  if (s->ptr == NULL) {
    fprintf(stderr, "malloc() failed\n");
    exit(EXIT_FAILURE);
  }
  s->ptr[0] = '\0';
}

size_t writefunc(void *ptr, size_t size, size_t nmemb, struct string *s) {
  size_t new_len = s->len + size*nmemb;
  s->ptr = realloc(s->ptr, new_len+1);
  if (s->ptr == NULL) {
    fprintf(stderr, "realloc() failed\n");
    exit(EXIT_FAILURE);
  }
  memcpy(s->ptr+s->len, ptr, size*nmemb);
  s->ptr[new_len] = '\0';
  s->len = new_len;

  return size*nmemb;
}

size_t curl_write_silent(void *buffer, size_t size, size_t nmemb, void *userp) {
   return size * nmemb;
}

void print_usage(void){
  printf(" ----------------\n");
  printf("|      USAGE     |\n");
  printf("|________________|\n\n");

  printf("./hue [group] [action]\n\n");

  printf("toggle a group's lights on/off\n");
  printf("./hue --living-room --toggle\n\n");

  printf("turn all lights in a group off\n");
  printf("./hue --living-room --off\n\n");

  printf("turn all lights in a group on\n");
  printf("./hue --living-room --on\n\n");

  printf("change the color of all lights in a group\n");
  printf("./hue --living-room --red=0 --green=120 --blue=120 --color\n\n");

  printf("set all lights in a group to their default color\n");
  printf("./hue --living-room --color\n");
}

/*
char* parse_json(const char *json_input, const char *key) {
  char *result_string = NULL;

  cJSON *json = cJSON_Parse(json_input);

  // ------------- DEBUG ---------------
  char *init = cJSON_Print(json);
  printf("%s\n", init);
  // -----------------------------------

  cJSON *result_json =  cJSON_DetachItemFromObjectCaseSensitive(json, key);

  result_string = cJSON_Print(result_json);

  if (result_string == NULL){
    result_string = "PARSE_ERROR";
    cJSON_Delete(json);
    cJSON_Delete(result_json);
    return result_string;
  } else {
      cJSON_Delete(json);
      cJSON_Delete(result_json);

      return result_string;
  }
}

int print_json(const char *json_input) {
  cJSON *json = cJSON_Parse(json_input);

  char *result = cJSON_Print(json);
  printf("%s\n", result);

  cJSON_Delete(json);

  return 0;
}
*/
