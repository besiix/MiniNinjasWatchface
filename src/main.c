#include <pebble.h>
    
#define KEY_TEMPERATURE 0
#define KEY_CONDITIONS 1

static Window *s_main_window;
static TextLayer *s_hour_layer;
static TextLayer *s_minute_layer;
static TextLayer *s_weather_layer;
static GFont s_time_font;
static GFont s_weather_font;
static BitmapLayer *s_background_layer;
static GBitmap *s_background_bitmap;
 
//==================== UPDATE TIME ====================
static void update_time() {
    //Get a tm structure
    time_t temp = time(NULL);
    struct tm *tick_time = localtime(&temp);
    
    //Create a long-lived buffer
    static char hours[] = "00:";
    static char minutes[] = "00";
    
    //Write the current hours and minutes into the buffer
    if (clock_is_24h_style() == true) {
        //Use 24 hour format
        strftime(hours, sizeof("00:"), "%H:", tick_time);
    } else {
        //Use 12 hour format
        strftime(hours, sizeof("00:"), "%I:", tick_time);
    }
    //Get minutes
    strftime(minutes, sizeof("00"), "%M", tick_time);
    
    //Display this time on the TextLayer
    text_layer_set_text(s_hour_layer, hours);
    text_layer_set_text(s_minute_layer, minutes);
}

//==================== WINDOW LOAD ====================
static void main_window_load(Window *window) {
    //Create GFonts
    s_time_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BONZAI_60));
    s_weather_font = fonts_load_custom_font(resource_get_handle(RESOURCE_ID_FONT_BONZAI_26));
    
    //Create GBitmap, then set to created BitmapLayer
    s_background_bitmap = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_BACKGROUND);
    s_background_layer = bitmap_layer_create(GRect(0, 0, 144, 168));
    bitmap_layer_set_bitmap(s_background_layer, s_background_bitmap);
    layer_add_child(window_get_root_layer(window), bitmap_layer_get_layer(s_background_layer));
    
    //Create and setup hour TextLayer
    s_hour_layer = text_layer_create(GRect(83, -20, 70, 60));
    text_layer_set_background_color(s_hour_layer, GColorClear);
    text_layer_set_text_color(s_hour_layer, GColorBlack);
    text_layer_set_text(s_hour_layer, "00:");
    text_layer_set_font(s_hour_layer, s_time_font);
    
    text_layer_set_text_alignment(s_hour_layer, GTextAlignmentLeft);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_hour_layer));
    
    //Create and setup minute TextLayer
    s_minute_layer = text_layer_create(GRect(83, 15, 60, 60));
    text_layer_set_background_color(s_minute_layer, GColorClear);
    text_layer_set_text_color(s_minute_layer, GColorBlack);
    text_layer_set_text(s_minute_layer, "00");
    text_layer_set_font(s_minute_layer, s_time_font);
    
    text_layer_set_text_alignment(s_minute_layer, GTextAlignmentLeft);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_minute_layer));
    
    //Create and setup temperature TextLayer
    s_weather_layer = text_layer_create(GRect(5, 138, 144, 30));
    text_layer_set_background_color(s_weather_layer, GColorClear);
    text_layer_set_text_color(s_weather_layer, GColorWhite);
    text_layer_set_text(s_weather_layer, ""); 
    text_layer_set_font(s_weather_layer, s_weather_font);
    
    text_layer_set_text_alignment(s_weather_layer, GTextAlignmentLeft);
    layer_add_child(window_get_root_layer(window), text_layer_get_layer(s_weather_layer));
    
    //Make sure the time is displayed from the start
    update_time();
}

//==================== WINDOW UNLOAD ====================
static void main_window_unload(Window *window) {
    //Destroy time elements
    fonts_unload_custom_font(s_time_font);
    text_layer_destroy(s_hour_layer);
    text_layer_destroy(s_minute_layer);
    
    //Destroy background bitmap elements
    gbitmap_destroy(s_background_bitmap);
    bitmap_layer_destroy(s_background_layer);
   
    //Destroy weather elements
    text_layer_destroy(s_weather_layer);
    fonts_unload_custom_font(s_weather_font);
}

//==================== TICK HANDLER ====================
static void tick_handler(struct tm *tick_time, TimeUnits units_changed) {
    update_time();
    
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

//==================== APPMESSAGE CALLBACKS ====================
static void inbox_received_callback(DictionaryIterator *iterator, void *context) {
    // Store incoming information
    static char temperature_buffer[8];
    static char conditions_buffer[32];
    static char weather_layer_buffer[32];
    
    // Read first item
    Tuple *t = dict_read_first(iterator);

    // For all items
    while(t != NULL) {
        // Which key was received?
        switch(t->key) {
        case KEY_TEMPERATURE:
            snprintf(temperature_buffer, sizeof(temperature_buffer), "%dC", (int)t->value->int32);
            break;
        case KEY_CONDITIONS:
            snprintf(conditions_buffer, sizeof(conditions_buffer), "%s", t->value->cstring);
            break;
        default:
            APP_LOG(APP_LOG_LEVEL_ERROR, "Key %d not recognized!", (int)t->key);
            break;
        }
        // Look for next item
        t = dict_read_next(iterator);
    }
    
    // Assemble full string and display
    snprintf(weather_layer_buffer, sizeof(weather_layer_buffer), "%s, %s", temperature_buffer, conditions_buffer);
    text_layer_set_text(s_weather_layer, weather_layer_buffer);
}

static void inbox_dropped_callback(AppMessageResult reason, void * context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Message dropped!");
}

static void outbox_failed_callback(DictionaryIterator *iterator, AppMessageResult reason, void *context) {
    APP_LOG(APP_LOG_LEVEL_ERROR, "Outbox send failed!");
}

static void outbox_sent_callback(DictionaryIterator *iterator, void *context) {
    APP_LOG(APP_LOG_LEVEL_INFO, "Outbox send success!");
}

//==================== INIT ====================
static void init() {
    //Create main Window element and assign to pointer
    s_main_window = window_create();
    
    //Set handlers to manage the elements inside the Window
    window_set_window_handlers(s_main_window, (WindowHandlers) {
        .load = main_window_load,
        .unload = main_window_unload
    });
    
    //Show the Window on the watch, with animated=true
    window_stack_push(s_main_window, true);
    
    //Register with TickTimerService
    tick_timer_service_subscribe(MINUTE_UNIT, tick_handler);
    
    // Register callbacks
    app_message_register_inbox_received(inbox_received_callback);
    app_message_register_inbox_dropped(inbox_dropped_callback);
    app_message_register_outbox_failed(outbox_failed_callback);
    app_message_register_outbox_sent(outbox_sent_callback);
    
    // Open AppMessage with largest possible inbox and outbox
    app_message_open(app_message_inbox_size_maximum(), app_message_outbox_size_maximum());
}

//==================== DE-INIT ====================
static void deinit() {
    //Destroy Window
    window_destroy(s_main_window);
}

//==================== MAIN ====================
int main(void) {
    init();
    app_event_loop();
    deinit();
}