#include <pebble.h>
#include <ctype.h>

// Static Variables
static Window *s_main_window;
static Layer *s_battery_layer;
static TextLayer *s_time_layer;
static TextLayer *s_date_layer;
static TextLayer *s_weather_layer;
static GFont s_time_font;
static GFont s_date_font;
static GFont s_wthr_font;
static int s_battery_level;

static void battery_update_proc(Layer *layer, GContext *ctx) {
	GRect bounds = layer_get_bounds(layer);
	
	//Find the width of the bar
	int width = (int)(float)(((float)s_battery_level / 100.0f) * 114.0f);
	
	// Draw the background
	graphics_context_set_fill_color(ctx, GColorBlack);
	graphics_fill_rect(ctx, bounds, 0, GCornerNone);
	
	//Draw the bar
	graphics_context_set_fill_color(ctx, GColorWhite);
	graphics_fill_rect(ctx, GRect(0, 0, width, bounds.size.h), 0, 
		GCornerNone);
}

static void main_window_load(Window *window) {
	// Get information about the Window
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	
	// Change background color
	window_set_background_color(window, GColorBlack);
	
	// Create the TextLayer with specific bounds
	s_time_layer = text_layer_create(GRect(0, 25,
		bounds.size.w , 45));
	
	// Create battery meter Layer
	s_battery_layer = layer_create(GRect(14, 69, 115, 2));
	layer_set_update_proc(s_battery_layer, battery_update_proc);
	
	//Create the date TextLayer
	s_date_layer = text_layer_create(GRect(0, 70, bounds.size.w , 15));
	
	// Create weather Layer
	s_weather_layer = text_layer_create(GRect(0, 
		100, bounds.size.w, 25));
	
	// Create GFont
	s_time_font = fonts_load_custom_font(resource_get_handle
		(RESOURCE_ID_SOLARIA_FONT_40));
	s_date_font = fonts_load_custom_font(resource_get_handle
		(RESOURCE_ID_SOLARIA_FONT_12));
	s_wthr_font = fonts_load_custom_font(resource_get_handle
		(RESOURCE_ID_SOLARIA_FONT_20));
	
	// Change timeLayers defaults
	text_layer_set_background_color(s_time_layer, GColorClear);
	text_layer_set_text_color(s_time_layer, GColorWhite);
	text_layer_set_font(s_time_layer, s_time_font);
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

static void battery_callback(BatteryChargeState state) {
	// revord the new battery level
	s_battery_level = state.charge_percent;
	
	// Update meter
	layer_mark_dirty(s_battery_layer);
}

static void update_time(){
	// Get a tm struct
	time_t temp = time(NULL);
	struct tm *tick_time = localtime(&temp);
	
	//bool isSecondsEven = atoi("%S") % 2 == 0;
	
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

static void tick_handler(struct tm *tick_time, TimeUnits units_changed){
	if ((units_changed & MINUTE_UNIT) != 0) {
		update_time();
	}
	
	if ((units_changed & HOUR_UNIT) != 0) {
		vibes_double_pulse();
	}
	
	if ((units_changed & DAY_UNIT) != 0) {
		update_date();
	}
}

static void init() {
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
	tick_timer_service_subscribe(MINUTE_UNIT | DAY_UNIT | HOUR_UNIT, 
		tick_handler);
	
	// Make sure the time is displayed from the start
	update_time();
	update_date();
	
	// Register for battery level updates
	battery_state_service_subscribe(battery_callback);
	
	// Ensure battery level is displayed from the start
	battery_callback(battery_state_service_peek());
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