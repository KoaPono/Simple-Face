#include <pebble.h>

// Static Variables
static Window *s_main_window;
static Layer *s_battery_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static GFont s_time_font;
static GFont s_date_font;
static GFont s_wthr_font;
static int 	 s_battery_level;
static bool  pirulenIsSet;

static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
	// Store incoming information
	static char temperature_buffer[8];
	static char conditions_buffer[32];
	static char weather_layer_buffer[32];
	
	// Read tuples for data
	Tuple *temp_tuple = dict_find(iterator, MESSAGE_KEY_TEMPERATURE);
	Tuple *conditions_tuple = dict_find(iterator, MESSAGE_KEY_CONDITIONS);

	// If all data is available, use it
	if(temp_tuple || conditions_tuple) {
  		snprintf(temperature_buffer, sizeof(temperature_buffer), "%d°F", (int)temp_tuple->value->int32);
  		snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", conditions_tuple->value->cstring);
		
		// Assemble full string and display
		//snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s %s", temperature_buffer, conditions_buffer);
		snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s", temperature_buffer);
		text_layer_set_text(s_weather_layer, weather_layer_buffer);
	}
}

static void inbox_dropped_callback(AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
  APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
  APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

static void battery_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	
	//Find the width of the bar
	int width = (int)(float)(((float)s_battery_level / 100.0f) * 124.0f);
	
	// Draw the background
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
	
	//Draw the bar
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, GCornerNone);
}

static void update_time(){
	// Get a tm struct
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	// Write the current hours and minutes into the buffer
	static char s_buffer[8];
	strftime(s_buffer, sizeof(s_buffer), clock_is_24h_style() ? "%H:%M" :
		"%I:%M", tick_time);
	
	//Dispay this time on the TextLayer
	text_layer_set_text(s_time_layer, s_buffer);
}

static void update_date() {
	// Get a tm struct
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	//bool isSecondsEven = atoi("%S") % 2 == 0;
	
	// Write the current hours and minutes into the buffer
	static char s_buffer[16];
	strftime(s_buffer, sizeof(s_buffer), "%a %b %d", tick_time);
	
	//Dispay this time on the TextLayer
	text_layer_set_text(s_date_layer, s_buffer);
}

static void battery_callback(BatteryChargeState state) {
	// revord the new battery level
	s_battery_level = state.charge_percent;
	
	// Update meter
	layer_mark_dirty(s_battery_layer);
}

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
	if ((units_changed & MINUTE_UNIT) != 0) {
		update_time();
	}
	
	if ((units_changed & HOUR_UNIT) != 0) {
		vibes_long_pulse();
	}
	
	if ((units_changed & DAY_UNIT) != 0) {
		update_date();
	}
	
	// Get weather update every 30 minutes
	if(tick_time->tm_min % 30 == 0) {
  		// Begin dictionary
  		DictionaryIterator *iter;
 	 	app_message_outbox_begin(&iter);

  		// Add a key-value pair
  		dict_write_uint8(iter, 0, 0);

  		// Send the message!
  		app_message_outbox_send();
	}
}

static void main_window_load(Window *window) {
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	// Change background color
	window_set_background_color(window, GColorBlack);
	
	if (pirulenIsSet) {
		// Create the Layers
		s_time_layer = text_layer_create(GRect(0, 31, bounds.size.w , 45));
		s_battery_layer = layer_create(GRect(10, 69, 124, 2));
		layer_set_update_proc(s_battery_layer, battery_update_proc);
		s_date_layer = text_layer_create(GRect(0, 70, bounds.size.w , 15));
		s_weather_layer = text_layer_create(GRect(0, 101, bounds.size.w, 25));
		
		// Create GFont
		s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIRULEN_FONT_35));
		s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIRULEN_FONT_12));
		s_wthr_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_PIRULEN_FONT_20));
	} else {
		// Create the Layers
		s_time_layer = text_layer_create(GRect(0, 25, bounds.size.w , 45));
		s_battery_layer = layer_create(GRect(10, 69, 124, 2));
		layer_set_update_proc(s_battery_layer, battery_update_proc);
		s_date_layer = text_layer_create(GRect(0, 69, bounds.size.w , 15));
		s_weather_layer = text_layer_create(GRect(0, 100, bounds.size.w, 25));
		
		// Create GFont
		s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SOLARIA_FONT_40));
		s_date_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SOLARIA_FONT_12));
		s_wthr_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_SOLARIA_FONT_20));
	}
	
	// Change timeLayers defaults
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_font(s_time_layer, s_time_font);
	text_layer_set_text(s_time_layer, "00:00");
	text_layer_set_text_alignment(s_time_layer, GTextAlignmentCenter);
	
	// Change dateLayers defaults
	text_layer_set_background_color(s_date_layer, GColorClear);
	text_layer_set_text_color(s_date_layer, GColorWhite);
	text_layer_set_font(s_date_layer, s_date_font);
	text_layer_set_text_alignment(s_date_layer, GTextAlignmentCenter);
	
	// Style the text
	text_layer_set_background_color(s_weather_layer, GColorClear);
	text_layer_set_text_color(s_weather_layer, GColorWhite);
	text_layer_set_text_alignment(s_weather_layer, GTextAlignmentCenter);
	text_layer_set_font(s_weather_layer, s_wthr_font);
	text_layer_set_text(s_weather_layer, "Loading...");
	
	// Add it as a child layer to the Window's root layer
	layer_add_child(window_layer, text_layer_get_layer(s_time_layer));
	layer_add_child(window_layer, text_layer_get_layer(s_date_layer));
	layer_add_child(window_layer, s_battery_layer);
	layer_add_child(window_layer, text_layer_get_layer(s_weather_layer));
}

static void main_window_unload(Window *window) {
	// Destroy TextLayer
	text_layer_destroy(s_time_layer);
	text_layer_destroy(s_date_layer);
	text_layer_destroy(s_weather_layer);
	layer_destroy(s_battery_layer);
	
	// Unload GFont
	fonts_unload_custom_font(s_time_font);
	fonts_unload_custom_font(s_date_font);
	fonts_unload_custom_font(s_wthr_font);
}

static void init() {
	pirulenIsSet = false;
	
	// Create main Window element and assign to pointer
	s_main_window = window_create();
	
	// Set handlers to manage the elements inside the Window
	window_set_window_handlers(s_main_window, (WindowHandlers) {
		.load = main_window_load,
		.unload = main_window_unload
	});
	
	// Show the Window on the watch, with animated = true
	window_stack_push(s_main_window, true);
	
	// Register with tickTimerService
	tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT | HOUR_UNIT, tick_handler);
	
	// Make sure the time is displayed from the start
	update_time();
	update_date();
	
	// Register for battery level updates
	battery_state_service_subscribe(battery_callback);
	
	// Ensure battery level is displayed from the start
	battery_callback(battery_state_service_peek());
	
	// Register callbacks
	app_message_register_inbox_received(inbox_received_callback);
	
	// Open AppMessage
	const int inbox_size = 128;
	const int outbox_size = 128;
	app_message_open(inbox_size, outbox_size);
	
	app_message_register_inbox_dropped(inbox_dropped_callback);
	app_message_register_outbox_failed(outbox_failed_callback);
	app_message_register_outbox_sent(outbox_sent_callback);

}

static void deinit() {
	// Destroy Window
	window_destroy(s_main_window);
}

int main(void) {
	init();
	app_event_loop();
	deinit();
}