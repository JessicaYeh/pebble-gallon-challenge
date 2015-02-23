#include <pebble.h>
#include "GallonChallenge.h"

// Key for saving the previous date that the user completed the day's challenge
// to determine if the streak count needs to be reset
#define LAST_STREAK_DATE_KEY 1000
// Key for saving streak count
#define STREAK_COUNT_KEY 1001
// Keys for saving current day's water volume intake
#define CURRENT_DATE_KEY 1002
#define CURRENT_OZ_KEY 1003
// Key for saving display unit type
#define UNIT_KEY 1004

#define OZ_IN_CUP 8
#define OZ_IN_PINT 16
#define OZ_IN_QUART 32
#define OZ_IN_GAL 128
#define CUP_IN_GAL 16
#define PINT_IN_GAL 8
#define QUART_IN_GAL 4

typedef enum {
    OUNCE = 0,
    CUP = 1,
    PINT = 2,
    QUART = 3
} Unit;

static Window *window;

static Window *menu_window;
static MenuLayer *menu_layer;

static GBitmap *action_icon_plus;
static GBitmap *action_icon_settings;
static GBitmap *action_icon_minus;

static ActionBarLayer *action_bar;

static TextLayer *text_layer;

static uint16_t current_oz = 0;
static Unit unit = CUP;

// Uses the current_oz and the chosen display unit to calculate the volume of
// liquid consumed in the current day
static uint16_t calc_current_volume() {
    switch (unit) {
        case CUP:   return current_oz / OZ_IN_CUP;
        case PINT:  return current_oz / OZ_IN_PINT;
        case QUART: return current_oz / OZ_IN_QUART;
        default:    return current_oz;
    }
}

static uint16_t get_unit_in_gal() {
    switch (unit) {
        case CUP:   return CUP_IN_GAL;
        case PINT:  return PINT_IN_GAL;
        case QUART: return QUART_IN_GAL;
        default:    return OZ_IN_GAL;
    }
}

static void update_display() {
    static char body_text[20];
    switch (unit) {
        case CUP:
            snprintf(body_text, sizeof(body_text), "%u/%u cups", calc_current_volume(), get_unit_in_gal());
            break;
        case PINT:
            snprintf(body_text, sizeof(body_text), "%u/%u pints", calc_current_volume(), get_unit_in_gal());
            break;
        case QUART:
            snprintf(body_text, sizeof(body_text), "%u/%u quarts", calc_current_volume(), get_unit_in_gal());
            break;
        default:
            snprintf(body_text, sizeof(body_text), "%u/%u ounces", calc_current_volume(), get_unit_in_gal());
            break;
    }
    
    text_layer_set_text(text_layer, body_text);

}

// Increase the current volume by one unit
static void increment_volume() {
    switch (unit) {
        case CUP:
            current_oz += OZ_IN_CUP;
            break;
        case PINT:
            current_oz += OZ_IN_PINT;
            break;
        case QUART:
            current_oz += OZ_IN_QUART;
            break;
        default:
            current_oz++;
            break;
    }
    
    if (current_oz > OZ_IN_GAL) {
        current_oz = OZ_IN_GAL;
    }
    
    update_display();
}

// Decrease the current volume by one unit
static void decrement_volume() {
    switch (unit) {
        case CUP:
            current_oz -= OZ_IN_CUP;
            break;
        case PINT:
            current_oz -= OZ_IN_PINT;
            break;
        case QUART:
            current_oz -= OZ_IN_QUART;
            break;
        default:
            current_oz--;
            break;
    }
    
    // Since unsigned, will wrap around to large numbers
    if (current_oz > OZ_IN_GAL) {
        current_oz = 0;
    }
    
    update_display();
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    set_selected_unit_in_menu();
    window_stack_push(menu_window, true);
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

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Set Drinking Unit");
}

static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return 4;
}

static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Use the row to specify which item we'll draw
    switch (cell_index->row) {
        case 0:
            menu_cell_basic_draw(ctx, cell_layer, "Ounces", NULL, NULL);
            break;
        case 1:
            menu_cell_basic_draw(ctx, cell_layer, "Cups", NULL, NULL);
            break;
        case 2:
            menu_cell_basic_draw(ctx, cell_layer, "Pints", NULL, NULL);
            break;
        case 3:
            menu_cell_basic_draw(ctx, cell_layer, "Quarts", NULL, NULL);
            break;
    }
}

void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    unit = cell_index->row;
    update_display();
    window_stack_pop(true);
}

static void load_persistent_storage() {
    current_oz = persist_exists(CURRENT_OZ_KEY) ? persist_read_int(CURRENT_OZ_KEY) : 0;
    unit = persist_exists(UNIT_KEY) ? persist_read_int(UNIT_KEY) : CUP;
}

static void save_persistent_storage() {
    persist_write_int(CURRENT_OZ_KEY, current_oz);
    persist_write_int(UNIT_KEY, unit);
}

static void set_selected_unit_in_menu() {
    menu_layer_set_selected_index(menu_layer, (MenuIndex) { .row = unit, .section = 0 }, MenuRowAlignCenter, false);
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
    
    Layer *menu_window_layer = window_get_root_layer(menu_window);
    GRect menu_bounds = layer_get_bounds(menu_window_layer);
    
    // Create the menu layer
    menu_layer = menu_layer_create(menu_bounds);
    
    // Set all the callbacks for the menu layer
    menu_layer_set_callbacks(menu_layer, NULL, (MenuLayerCallbacks){
        .draw_header = menu_draw_header_callback,
        .get_num_rows = menu_get_num_rows_callback,
        .draw_row = menu_draw_row_callback,
        .select_click = menu_select_callback,
    });
    
    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(menu_layer, menu_window);
    
    // Add it to the window for display
    layer_add_child(menu_window_layer, menu_layer_get_layer(menu_layer));
    
    
    update_display();
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
    
    menu_window = window_create();
    
    load_persistent_storage();
    
    window_stack_push(window, true);
}

static void deinit(void) {
    save_persistent_storage();
    
    window_destroy(window);
    window_destroy(menu_window);
    
    // Destroy the menu layer
    menu_layer_destroy(menu_layer);

    gbitmap_destroy(action_icon_plus);
    gbitmap_destroy(action_icon_settings);
    gbitmap_destroy(action_icon_minus);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}
