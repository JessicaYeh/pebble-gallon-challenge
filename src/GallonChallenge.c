#include <pebble.h>
#include "GallonChallenge.h"

// Key for saving the previous date that the user completed the day's challenge
// to determine if the streak count needs to be reset
#define LAST_STREAK_DATE_KEY 1000
// Key for saving streak count
#define STREAK_COUNT_KEY 1001
// Keys for saving current day's water intake in mL or oz (total for the day
// is the sum of both amounts, to preserve number precision)
#define CURRENT_DATE_KEY 1002
#define CURRENT_ML_KEY 1003
#define CURRENT_OZ_KEY 1004

#define ML_IN_OZ 29.5735
#define OZ_IN_GAL 128
#define ML_IN_GAL 3785

static Window *window;

static GBitmap *action_icon_plus;
static GBitmap *action_icon_settings;
static GBitmap *action_icon_minus;

static ActionBarLayer *action_bar;

static TextLayer *text_layer;

static uint32_t current_ml = 0;
static uint16_t current_oz = 0;

// Uses the current_ml and current_oz and the chosen display unit to calculate
// the volume of liquid consumed in the current day
static float calc_current_volume() {
    // Currently hardcode to display in cups
    return (current_ml / ML_IN_OZ + current_oz) / 8;
}

static void update_gallon() {
    static char body_text[10];
    snprintf(body_text, sizeof(body_text), "%u/16 cups", (int)calc_current_volume());
    text_layer_set_text(text_layer, body_text);

}

// Increase the current volume by one unit
static void increment_volume() {
    current_oz += 8;
    if (current_oz > OZ_IN_GAL) {
        current_oz = OZ_IN_GAL;
    }
    if (current_ml > ML_IN_GAL) {
        current_ml = ML_IN_GAL;
    }
    
    update_gallon();
}

// Decrease the current volume by one unit
static void decrement_volume() {
    if (current_oz - 8 >= 0) {
        current_oz -= 8;
    } else {
        current_oz = 0;
    }
    
    update_gallon();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    text_layer_set_text(text_layer, "Select");
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    increment_volume();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    decrement_volume();
}

static void click_config_provider(void *context) {
    const uint16_t repeat_interval_ms = 100;
    window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, up_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, down_click_handler);
    
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void load_persistent_storage() {
    current_ml = persist_exists(CURRENT_ML_KEY) ? persist_read_int(CURRENT_ML_KEY) : 0;
    current_oz = persist_exists(CURRENT_OZ_KEY) ? persist_read_int(CURRENT_OZ_KEY) : 0;
}

static void save_persistent_storage() {
    persist_write_int(CURRENT_ML_KEY, current_ml);
    persist_write_int(CURRENT_OZ_KEY, current_oz);
}

static void window_load(Window *window) {
    action_bar = action_bar_layer_create();
    action_bar_layer_add_to_window(action_bar, window);
    action_bar_layer_set_click_config_provider(action_bar, click_config_provider);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_UP, action_icon_plus);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_SELECT, action_icon_settings);
    action_bar_layer_set_icon(action_bar, BUTTON_ID_DOWN, action_icon_minus);

    Layer *window_layer = window_get_root_layer(window);
    GRect bounds = layer_get_bounds(window_layer);

    text_layer = text_layer_create((GRect) { .origin = { 0, 72 }, .size = { bounds.size.w - 20, 20 } });
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    layer_add_child(window_layer, text_layer_get_layer(text_layer));
    
    update_gallon();
}

static void window_unload(Window *window) {
    text_layer_destroy(text_layer);
}

static void init(void) {
    action_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
    action_icon_settings = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_SETTINGS);
    action_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);

    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    
    load_persistent_storage();
    
    window_stack_push(window, true);
}

static void deinit(void) {
    save_persistent_storage();
    
    window_destroy(window);

    gbitmap_destroy(action_icon_plus);
    gbitmap_destroy(action_icon_settings);
    gbitmap_destroy(action_icon_minus);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
