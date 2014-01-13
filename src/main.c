
#include "pebble_os.h"
#include "pebble_app.h"
#include "pebble_fonts.h"
#include "http.h"
#include "util.h"
#include "progress_bar.h"

//Set this to true to compile for Android
#define ANDROID false
 
#if ANDROID
#define MY_UUID { 0xA3, 0x4E, 0x23, 0x60, 0xF2, 0x77, 0x4E, 0xF2, 0x99, 0x2D, 0xF1, 0x1E, 0xA7, 0xCB, 0xEF, 0x10 }
#else
#define MY_UUID HTTP_UUID
#endif

//Unique Random Stuff to identify one app from another with Httpebble
#define HTTP_COOKIE 42695678
	
PBL_APP_INFO(MY_UUID, "CANAL+ Time", "CANAL+", 1, 0, RESOURCE_ID_IMAGE_MENU_ICON, APP_INFO_WATCH_FACE);

//Declarations 

void handle_init(AppContextRef ctx);
void http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context);
void http_failure(int32_t request_id, int http_status, void* context);
void httpebble_error(int error_code);
void window_load(Window* me);
void window_unload(Window* me);
Window window;
TextLayer textlayer;
TextLayer programlayer;
PblTm g_Now;       
TextLayer g_DateLayer;
TextLayer backlayer;
int ELAPSED = 0;
int DURATION = 0;
//Do I need to fetch current program again ?
bool REFRESH = true;

//progress bar attempt
static ProgressBarLayer progress_bar;

//Date formatting
#define _DATE_BUF_LEN 26
static char _DATE_BUFFER[_DATE_BUF_LEN];
#define DATE_FORMAT "%R"

	//Handles API returns : gives you current program name, elapsed time since start and duration {"1":"Program Name","2":elapsed,"3":duration}
void http_success(int32_t request_id, int http_status, DictionaryIterator* received, void* context) {
  if (request_id != HTTP_COOKIE) {
    return;
  }
	Tuple* tuple1 = dict_find(received, 1);
	Tuple* tuple2 = dict_find(received, 2);
	Tuple* tuple3 = dict_find(received, 3);
	DURATION = tuple3->value->int32;
	ELAPSED = tuple2->value->int32;
	progress_bar_layer_set_range(&progress_bar, 0, DURATION);
    progress_bar_layer_set_value(&progress_bar, ELAPSED);
	text_layer_set_text(&programlayer,tuple1->value->cstring);
	
	REFRESH = false;
}

void get_current_program() {

	text_layer_set_text(&programlayer, "Loading");
	DictionaryIterator* dict;
  	HTTPResult  result = http_out_get("http://japansio.info/api/program.php", HTTP_COOKIE, &dict);
	if (result != HTTP_OK) {
		httpebble_error(result);
		//Something went wrong with the request, fecth program again next minute
		REFRESH = true;
    	return;
	}
	
	dict_write_cstring(dict, 1, "1");
	result = http_out_send();
	
  	if (result != HTTP_OK) {
		//Something went wrong with the request, fecth program again next minute
		REFRESH = true;
    	httpebble_error(result);
    return;
  }
}


void handle_minute_tick(AppContextRef ctx, PebbleTickEvent *t) {
  (void)ctx;
  (void)t;
	
	// update our current time
  get_time(&g_Now);
 
  // formant and render the date string on the date layer
  string_format_time(_DATE_BUFFER, _DATE_BUF_LEN, DATE_FORMAT, &g_Now);
  text_layer_set_text(&g_DateLayer, _DATE_BUFFER);
  
  ELAPSED +=1;
  progress_bar_layer_set_value(&progress_bar, ELAPSED);
//if it is time to get next program or previosu httprequest failed, get the current program
  if((ELAPSED>=DURATION)||REFRESH) {
		get_current_program();
	}
}

void window_load(Window* me) {
	
	text_layer_init(&backlayer, GRect(0, 138, 144, 30));
  text_layer_set_text_color(&backlayer, GColorClear);
  text_layer_set_background_color(&backlayer, GColorBlack);
  text_layer_set_text_alignment(&backlayer, GTextAlignmentCenter);
  text_layer_set_font(&backlayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_CANALBOLD_16)));
  

	progress_bar_layer_init(&progress_bar, GRect(22, 148, 100, 10));
    
	progress_bar_layer_set_range(&progress_bar, 0, 100);
	
  text_layer_init(&textlayer, GRect(0, 00, 144, 40));
  text_layer_set_text_color(&textlayer, GColorClear);
  text_layer_set_background_color(&textlayer, GColorBlack);
  text_layer_set_text_alignment(&textlayer, GTextAlignmentCenter);
  text_layer_set_font(&textlayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_CANALBOLD_22)));
  text_layer_set_text(&textlayer, "CANAL+");
  
	
text_layer_init(&g_DateLayer, GRect(0, 40, 144, 30));
text_layer_set_text_color(&g_DateLayer, GColorClear);
text_layer_set_background_color(&g_DateLayer, GColorBlack);
text_layer_set_text_alignment(&g_DateLayer, GTextAlignmentCenter);
text_layer_set_font(&g_DateLayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_CANALBOLD_22)));

	
text_layer_init(&programlayer, GRect(0, 70, 144, 68));
text_layer_set_text_color(&programlayer, GColorClear);
text_layer_set_background_color(&programlayer, GColorBlack);
text_layer_set_text_alignment(&programlayer, GTextAlignmentCenter);
text_layer_set_font(&programlayer, fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_CUSTOM_CANALBOLD_16)));

	
}

void window_appear(Window* me) {
	
	layer_add_child(&window.layer, &backlayer.layer);
	layer_add_child(&window.layer, (Layer*)&progress_bar);
	layer_add_child(&window.layer, &textlayer.layer);
	layer_add_child(&window.layer, &g_DateLayer.layer);
	layer_add_child(&window.layer, &programlayer.layer);
	
}

void window_unload(Window* me) {
	
	layer_remove_child_layers(window_get_root_layer(me));
}


// Standard app initialisation

void handle_init(AppContextRef ctx) {
  (void)ctx;
  
  http_register_callbacks((HTTPCallbacks) {
    .success = http_success,
    .failure = http_failure
  }, NULL);
	
  resource_init_current_app(&APP_RESOURCES);
  window_init(&window, "CANAL+ Time");
  window_set_window_handlers(&window, (WindowHandlers) {.load = window_load,.unload = window_unload, .appear = window_appear});
  window_stack_push(&window, true /* Animated */);
  window_set_fullscreen(&window, true);
  handle_minute_tick(ctx,NULL);
}

void http_failure(int32_t request_id, int http_status, void* context) {
  	REFRESH = true;
	httpebble_error(http_status >= 1000 ? http_status - 1000 : http_status);
}

void pbl_main(void *params) {
    http_set_app_id(76782702);
	PebbleAppHandlers handlers = {
    .init_handler = &handle_init,
		.tick_info = {
      	.tick_handler = &handle_minute_tick,
      .tick_units = MINUTE_UNIT
    },
	.messaging_info = {
      .buffer_sizes = {
        .inbound = 256,
        .outbound = 256,
      }
    }
  };
  app_event_loop(params, &handlers);
}

void httpebble_error(int error_code) {

  static char error_message[] = "UNKNOWN_HTTP_ERRROR_CODE_GENERATED";

  switch (error_code) {
    case HTTP_SEND_TIMEOUT:
      strcpy(error_message, "HTTP_SEND_TIMEOUT");
    break;
    case HTTP_SEND_REJECTED:
      strcpy(error_message, "HTTP_SEND_REJECTED");
    break;
    case HTTP_NOT_CONNECTED:
      strcpy(error_message, "HTTP_NOT_CONNECTED");
    break;
    case HTTP_BRIDGE_NOT_RUNNING:
      strcpy(error_message, "HTTP_BRIDGE_NOT_RUNNING");
    break;
    case HTTP_INVALID_ARGS:
      strcpy(error_message, "HTTP_INVALID_ARGS");
    break;
    case HTTP_BUSY:
      strcpy(error_message, "HTTP_BUSY");
    break;
    case HTTP_BUFFER_OVERFLOW:
      strcpy(error_message, "HTTP_BUFFER_OVERFLOW");
    break;
    case HTTP_ALREADY_RELEASED:
      strcpy(error_message, "HTTP_ALREADY_RELEASED");
    break;
    case HTTP_CALLBACK_ALREADY_REGISTERED:
      strcpy(error_message, "HTTP_CALLBACK_ALREADY_REGISTERED");
    break;
    case HTTP_CALLBACK_NOT_REGISTERED:
      strcpy(error_message, "HTTP_CALLBACK_NOT_REGISTERED");
    break;
    case HTTP_NOT_ENOUGH_STORAGE:
      strcpy(error_message, "HTTP_NOT_ENOUGH_STORAGE");
    break;
    case HTTP_INVALID_DICT_ARGS:
      strcpy(error_message, "HTTP_INVALID_DICT_ARGS");
    break;
    case HTTP_INTERNAL_INCONSISTENCY:
      strcpy(error_message, "HTTP_INTERNAL_INCONSISTENCY");
    break;
    case HTTP_INVALID_BRIDGE_RESPONSE:
      strcpy(error_message, "HTTP_INVALID_BRIDGE_RESPONSE");
    break;
    default: {
      strcpy(error_message, "HTTP_ERROR_UNKNOWN");
    }
  }
  text_layer_set_text(&programlayer, error_message);
}
