#ifndef MAIN_FN
#define MAIN_FN

#include <stdio.h>

#include "fparse.h"
#include "devtypes.h"
#include "usb-operations.h"
#include "http-operations.h"
#include "cJSON.h"

#define BUF_SIZE 0x10
#define MAX_DEVICES 1

void show_user_options()
{
  printf("\nPlease enter number corresponding to selection.\n");
  printf("0: Exit\n");
  printf("1: Get temperature reading\n");
  printf("2: Read device config\n");
}

int get_user_selection()
{
  char text[20];
  fputs("Enter selection: ", stdout);
  fflush(stdout);
  if ( fgets(text, sizeof text, stdin) )
    {
      int number;
      if ( sscanf(text, "%d", &number) == 1 )
	{
	  return number;
	}
    }
  return 0;
}


int main(void)
{
  FILE *dfile;
  int size;
  char *fbuffer;

  cJSON *root;
  cJSON *specifications;
  char *json_readings;

  int product_id;
  int vendor_id;
  char *url;

  int err;
  int looping;

  struct ftdi_context *ftHandle;

  int selection;

  int read_bytes;
  uint8_t inc_buf[280];

  SEM710_READINGS readings;
  CONFIG_DATA cal;

  char *content;

  ftHandle = ftdi_new();

  curl_global_init(CURL_GLOBAL_ALL);

  /* Get device ids from file: */
  dfile = fopen("product.json", "r");

  fseek(dfile, 0L, SEEK_END);
  size = ftell(dfile);
  fseek(dfile, 0L, SEEK_SET);

  fbuffer = (char *) malloc(size);
  get_file(dfile, fbuffer);
  fclose(dfile);

  root = cJSON_Parse(fbuffer);
  specifications = cJSON_GetObjectItem(root, "specifications");

  product_id = cJSON_GetObjectItem(specifications, "product_id")->valueint;
  vendor_id = cJSON_GetObjectItem(specifications, "vendor_id")->valueint;
  url = cJSON_GetObjectItem(specifications, "url")->valuestring;
  /* free(fbuffer); */
  /* cJSON_Delete(root); */

  /************************/
  /* 1: Connect to server */
  /************************/

  content = do_web_get(url);
  printf("GET Result: %s\n", content);

  /*************************/
  /* 2: Open SEM710 device */
  /*************************/

  err = detach_device_kernel(vendor_id, product_id);
  if (err) {
    printf("error detaching device kernel\n");
    return 1; 
  }

  err = open_device(ftHandle, vendor_id, product_id);
  if (err) { 
    printf("error opening device \n");
    return 1; 
  }

  err = prepare_device(ftHandle);
  if (err) { 
    printf("error preparing device \n");
    return 1;
  }

  printf("Device prepared.\n");
  /*****************************************/
  /* Communication to server */
  /*****************************************/
  
  /****************************************/
  /* 3: await instructions (skip for now) */
  /****************************************/

  looping = 1;
  while (looping) {
    show_user_options();
    selection = get_user_selection();
    printf("\n");

    switch(selection) {
    case 0:
      looping = 0;
      break;
    case 1: /* read process */
      read_bytes = read_device(ftHandle, SEM_COMMANDS_cREAD_PROCESS, inc_buf);
      if (read_bytes <= 0) {
  	printf("Read process failure.\n");
  	looping = 0;
      }
      else {
        get_readings(&readings, inc_buf, read_bytes);
  	display_readings(&readings);
	json_readings = pack_readings(&readings);
	printf("json_readings=%s\n", json_readings);
	do_web_put(url, json_readings);
      }
      break;
    case 2:  /* read config */
      read_bytes = read_device(ftHandle, SEM_COMMANDS_cREAD_CONFIG, inc_buf);
      if (read_bytes <= 0) {
    	printf("Read calibration failure.\n");
    	looping = 0;
      }
      else {
        get_config(&cal, inc_buf, read_bytes);
  	display_config(&cal);
      }
      break;
    default:
      printf("Selection invalid.\n");
      break;
    }
    usleep(500000);
  }
  /****************************/
  /* 4: read data from SEM710 */
  /****************************/

  /* 4.5: transmit data from SEM710 */

  /****************************************/
  /* Communication to server unimplemented*/
  /****************************************/

  /* 5: purge buffer; await further instructions */

  /* 6: close device */
  ftdi_usb_close(ftHandle);
  ftdi_free(ftHandle);
  free(fbuffer);
  cJSON_Delete(root);

  return 0;
}

#endif
