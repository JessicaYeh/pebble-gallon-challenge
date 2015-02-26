#include <pebble.h>
#include "GallonChallenge.h"

// Key for saving the previous date that the user completed the day's challenge
// to determine if the streak count needs to be reset. Saved as a time_t.
#define LAST_STREAK_DATE_KEY 1000
// Key for saving streak count
#define STREAK_COUNT_KEY 1001
// Keys for saving current day's water volume intake
#define CURRENT_DATE_KEY 1002
#define CURRENT_OZ_KEY 1003
// Key for saving display unit type
#define UNIT_KEY 1004
// Key for saving goal unit type
#define GOAL_KEY 1005
// Key for saving end of day
#define EOD_KEY 1006
// Keys for saving profile info
#define TOTAL_CONSUMED_KEY 1007
#define LONGEST_STREAK_KEY 1008
#define DRINKING_SINCE_KEY 1009

#define OZ_IN_CUP 8
#define OZ_IN_PINT 16
#define OZ_IN_QUART 32
#define OZ_IN_GAL 128
#define CUP_IN_GAL 16
#define PINT_IN_GAL 8
#define QUART_IN_GAL 4


static Window *window;

static GBitmap *action_icon_plus;
static GBitmap *action_icon_settings;
static GBitmap *action_icon_minus;
static GBitmap *gallon_filled_image;
static GBitmap *gallon_image;
static GBitmap *star;

static ActionBarLayer *action_bar;

static TextLayer *streak_text_layer;
static TextLayer *text_layer;

static TextLayer *white_layer;
static BitmapLayer *gallon_filled_layer;
static BitmapLayer *gallon_layer;
static BitmapLayer *star_layer;

static Unit goal;
static Unit unit;
static time_t current_date;
static uint16_t current_oz;
static time_t last_streak_date;
static uint16_t streak_count;
static uint16_t end_of_day;

static uint32_t total_consumed;
static uint16_t longest_streak;
static time_t drinking_since;

static const char* unit_to_string(Unit u) {
    switch (u) {
        case OUNCE:       return "Ounces";
        case CUP:         return "Cups";
        case PINT:        return "Pints";
        case QUART:       return "Quarts";
        case HALF_GALLON: return "Half Gallon";
        case GALLON:      return "One Gallon";
        default:          return "";
    }
}

static const char* eod_to_string(uint16_t hour) {
    switch (hour) {
        case 0:  return "12:00 AM";
        case 3:  return "3:00 AM";
        case 6:  return "6:00 AM";
        case 9:  return "9:00 AM";
        case 12: return "12:00 PM";
        case 15: return "3:00 PM";
        case 18: return "6:00 PM";
        case 21: return "9:00 PM";
        default: return "";
    }
}

static bool are_dates_equal(time_t date1, time_t date2) {
    struct tm *old_date = localtime(&date1);
    uint16_t old_mday = old_date->tm_mday;
    uint16_t old_mon = old_date->tm_mon;
    uint16_t old_year = old_date->tm_year;
    
    struct tm *new_date = localtime(&date2);
    uint16_t new_mday = new_date->tm_mday;
    uint16_t new_mon = new_date->tm_mon;
    uint16_t new_year = new_date->tm_year;
    
    if (old_mday == new_mday && old_mon == new_mon && old_year == new_year) {
        return true;
    }
    
    return false;
}

static time_t get_todays_date() {
    return time(NULL) - end_of_day * 3600;
}

static time_t get_yesterdays_date() {
    return time(NULL) - end_of_day * 3600 - 86400;
}

static void reset_current_date_and_volume_if_needed() {
    time_t today = get_todays_date();
    if (!are_dates_equal(current_date, today)) {
        current_date = today;
        current_oz = 0;
        
        // Reset the streak if needed
        if (!are_dates_equal(last_streak_date, get_yesterdays_date())) {
            streak_count = 0;
        }
    }
    
    update_streak_count();
    update_volume_display();
}

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

static float get_goal_scale() {
    switch (goal) {
        case HALF_GALLON: return 0.5;
        default:          return 1.0;
    }
}

static void set_image_for_goal() {
    // Free the old image before creating the new image
    gbitmap_destroy(gallon_filled_image);
    gbitmap_destroy(gallon_image);
    
    switch (goal) {
        case HALF_GALLON:
            gallon_filled_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HALF_GALLON_FILLED);
            gallon_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_HALF_GALLON_BLACK);
            break;
        default:
            gallon_filled_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GALLON_FILLED);
            gallon_image = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_GALLON_BLACK);
            break;
    }
    
    bitmap_layer_set_bitmap(gallon_filled_layer, gallon_filled_image);
    bitmap_layer_set_bitmap(gallon_layer, gallon_image);
}

static void update_volume_display() {
    static char body_text[20];
    
    uint16_t numerator = calc_current_volume();
    uint16_t denominator = get_unit_in_gal() * get_goal_scale();
    snprintf(body_text, sizeof(body_text), "%u/%u %s", numerator, denominator, unit_to_string(unit));
    text_layer_set_text(text_layer, body_text);
    
    layer_set_frame(text_layer_get_layer(white_layer), GRect(0, 37, 124, (1 - (float)current_oz / (float)OZ_IN_GAL / get_goal_scale()) * 93));
    
    // Only show the star if the goal is met
    bool is_star_visible = !layer_get_hidden(bitmap_layer_get_layer(star_layer));
    if (numerator == denominator && !is_star_visible) {
        layer_set_hidden(bitmap_layer_get_layer(star_layer), false);
    } else if (numerator != denominator && is_star_visible) {
        layer_set_hidden(bitmap_layer_get_layer(star_layer), true);
    }
}

static void update_streak_display() {
    static char streak_text[20];
    snprintf(streak_text, sizeof(streak_text), "%u day streak!", streak_count);
    text_layer_set_text(streak_text_layer, streak_text);
}

// Increase the current volume by one unit
static void increment_volume() {
    uint16_t volume_increase = 0;
    switch (unit) {
        case CUP:
            volume_increase = OZ_IN_CUP;
            break;
        case PINT:
            volume_increase = OZ_IN_PINT;
            break;
        case QUART:
            volume_increase = OZ_IN_QUART;
            break;
        default:
            volume_increase = 1;
            break;
    }
    
    uint16_t oz_in_goal = OZ_IN_GAL * get_goal_scale();
    if (current_oz + volume_increase > oz_in_goal) {
        total_consumed += oz_in_goal - current_oz;
    } else if (current_oz < oz_in_goal) {
        total_consumed += volume_increase;
    }
    current_oz += volume_increase;
    
    update_streak_count();
    update_volume_display();
}

// Decrease the current volume by one unit
static void decrement_volume() {
    uint16_t volume_decrease = 0;
    
    switch (unit) {
        case CUP:
            volume_decrease = OZ_IN_CUP;
            break;
        case PINT:
            volume_decrease = OZ_IN_PINT;
            break;
        case QUART:
            volume_decrease = OZ_IN_QUART;
            break;
        default:
            volume_decrease = 1;
            break;
    }
    
    if (current_oz < volume_decrease) {
        total_consumed -= volume_decrease - current_oz;
        current_oz = 0;
    } else {
        total_consumed -= volume_decrease;
        current_oz -= volume_decrease;
    }
    
    update_streak_count();
    update_volume_display();
}

static void update_streak_count() {
    uint16_t goal_oz = OZ_IN_GAL * get_goal_scale();
    time_t today = get_todays_date();
    
    if (current_oz >= goal_oz) {
        // Restrict the max ounces to the goal ounces
        current_oz = goal_oz;
        
        // If the last streak date is not today's date, then set that date to
        // today's date and increment the streak count.
        if (!are_dates_equal(last_streak_date, today)) {
            last_streak_date = today;
            streak_count++;
            if (streak_count > longest_streak) {
                longest_streak = streak_count;
            }
        }
    } else if (current_oz < goal_oz && are_dates_equal(last_streak_date, today)) {
        // If the last streak date is today's date, since the goal is now no longer
        // met, set the last streak date to the previous date and decrement the
        // streak count.
        last_streak_date = get_yesterdays_date();
        if (streak_count == longest_streak) {
            longest_streak--;
        }
        streak_count--;
    }
    
    update_streak_display();
}

static void reset_profile() {
    total_consumed = current_oz;
    longest_streak = streak_count;
    drinking_since = get_todays_date();
    layer_mark_dirty(menu_layer_get_layer(profile_menu_layer));
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    settings_menu_show();
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

static void handle_hour_tick(struct tm *tick_time, TimeUnits units_changed) {
    if (tick_time->tm_hour == 0) {
        reset_current_date_and_volume_if_needed();
    }
}

static void load_persistent_storage() {
    current_oz = persist_exists(CURRENT_OZ_KEY) ? persist_read_int(CURRENT_OZ_KEY) : 0;
    end_of_day = persist_exists(EOD_KEY) ? persist_read_int(EOD_KEY) : 0;
    goal = persist_exists(GOAL_KEY) ? persist_read_int(GOAL_KEY) : GALLON;
    unit = persist_exists(UNIT_KEY) ? persist_read_int(UNIT_KEY) : CUP;
    streak_count = persist_exists(STREAK_COUNT_KEY) ? persist_read_int(STREAK_COUNT_KEY) : 0;
    last_streak_date = persist_exists(LAST_STREAK_DATE_KEY) ? persist_read_int(LAST_STREAK_DATE_KEY) : get_yesterdays_date();
    current_date = persist_exists(CURRENT_DATE_KEY) ? persist_read_int(CURRENT_DATE_KEY) : get_todays_date();
    total_consumed = persist_exists(TOTAL_CONSUMED_KEY) ? persist_read_int(TOTAL_CONSUMED_KEY) : 0;
    longest_streak = persist_exists(LONGEST_STREAK_KEY) ? persist_read_int(LONGEST_STREAK_KEY) : 0;
    drinking_since = persist_exists(DRINKING_SINCE_KEY) ? persist_read_int(DRINKING_SINCE_KEY) : get_todays_date();
}

static void save_persistent_storage() {
    persist_write_int(CURRENT_OZ_KEY, current_oz);
    persist_write_int(EOD_KEY, end_of_day);
    persist_write_int(GOAL_KEY, goal);
    persist_write_int(UNIT_KEY, unit);
    persist_write_int(STREAK_COUNT_KEY, streak_count);
    persist_write_int(LAST_STREAK_DATE_KEY, (int)last_streak_date);
    persist_write_int(CURRENT_DATE_KEY, (int)current_date);
    persist_write_int(TOTAL_CONSUMED_KEY, total_consumed);
    persist_write_int(LONGEST_STREAK_KEY, longest_streak);
    persist_write_int(DRINKING_SINCE_KEY, (int)drinking_since);
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
    
    const int16_t width = bounds.size.w - ACTION_BAR_WIDTH - 7;
    
    streak_text_layer = text_layer_create(GRect(4, 0, width, 60));
    text_layer_set_font(streak_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(streak_text_layer, GTextAlignmentCenter);
    text_layer_set_background_color(streak_text_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(streak_text_layer));
    
    text_layer = text_layer_create(GRect(4, 134, width, 20));
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    text_layer_set_background_color(text_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(text_layer));
    
    gallon_filled_layer = bitmap_layer_create(GRect(0, 30, 124, 104));
    layer_add_child(window_layer, bitmap_layer_get_layer(gallon_filled_layer));
    
    white_layer = text_layer_create(GRect(0, 30, 124, 104));
    text_layer_set_background_color(white_layer, GColorWhite);
    layer_add_child(window_layer, text_layer_get_layer(white_layer));
    
    gallon_layer = bitmap_layer_create(GRect(0, 30, 124, 104));
    bitmap_layer_set_background_color(gallon_layer, GColorClear);
    bitmap_layer_set_compositing_mode(gallon_layer, GCompOpClear);
    layer_add_child(window_layer, bitmap_layer_get_layer(gallon_layer));
    
    star_layer = bitmap_layer_create(GRect(0, 30, 124, 104));
    bitmap_layer_set_bitmap(star_layer, star);
    layer_add_child(window_layer, bitmap_layer_get_layer(star_layer));
    
    set_image_for_goal();
    reset_current_date_and_volume_if_needed();
}

static void window_unload(Window *window) {
    text_layer_destroy(text_layer);
    text_layer_destroy(streak_text_layer);
    text_layer_destroy(white_layer);
    bitmap_layer_destroy(gallon_layer);
    bitmap_layer_destroy(gallon_filled_layer);
    bitmap_layer_destroy(star_layer);
    action_bar_layer_destroy(action_bar);
}

static void init(void) {
    load_persistent_storage();
    
    action_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
    action_icon_settings = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_SETTINGS);
    action_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);
    star = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STAR);

    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });
    
    settings_menu_window = window_create();
    window_set_window_handlers(settings_menu_window, (WindowHandlers) {
        .load = settings_menu_window_load,
        .unload = settings_menu_window_unload,
    });
    
    profile_menu_window = window_create();
    window_set_window_handlers(profile_menu_window, (WindowHandlers) {
        .load = profile_menu_window_load,
        .unload = profile_menu_window_unload,
    });
    
    goal_menu_window = window_create();
    window_set_window_handlers(goal_menu_window, (WindowHandlers) {
        .load = goal_menu_window_load,
        .unload = goal_menu_window_unload,
    });
    
    unit_menu_window = window_create();
    window_set_window_handlers(unit_menu_window, (WindowHandlers) {
        .load = unit_menu_window_load,
        .unload = unit_menu_window_unload,
    });
    
    eod_menu_window = window_create();
    window_set_window_handlers(eod_menu_window, (WindowHandlers) {
        .load = eod_menu_window_load,
        .unload = eod_menu_window_unload,
    });
    
    window_stack_push(window, true);
    
    tick_timer_service_subscribe(HOUR_UNIT, handle_hour_tick);
}

static void deinit(void) {
    save_persistent_storage();
    
    gbitmap_destroy(action_icon_plus);
    gbitmap_destroy(action_icon_settings);
    gbitmap_destroy(action_icon_minus);
    gbitmap_destroy(gallon_filled_image);
    gbitmap_destroy(gallon_image);
    gbitmap_destroy(star);
    
    window_destroy(window);
    window_destroy(settings_menu_window);
    window_destroy(goal_menu_window);
    window_destroy(unit_menu_window);
    window_destroy(eod_menu_window);
}

int main(void) {
    init();
    app_event_loop();
    deinit();
}


// Settings menu stuff
static void settings_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0:
            menu_cell_basic_header_draw(ctx, cell_layer, "Profile");
            break;
        case 1:
            menu_cell_basic_header_draw(ctx, cell_layer, "Settings");
            break;
    }
}

static uint16_t settings_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 2;
}

static uint16_t settings_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0:
            return 1;
            
        case 1:
            return 3;
            
        default:
            return 0;
    }
}

static int16_t settings_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void settings_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Determine which section we're going to draw in
    switch (cell_index->section) {
        case 0:
            // Use the row to specify which item we'll draw
            switch (cell_index->row) {
                case 0:
                    menu_cell_basic_draw(ctx, cell_layer, "View Profile", NULL, NULL);
                    break;
            }
            break;
            
        case 1:
            switch (cell_index->row) {
                case 0:
                    menu_cell_basic_draw(ctx, cell_layer, "Daily Goal", unit_to_string(goal), NULL);
                    break;
                case 1:
                    menu_cell_basic_draw(ctx, cell_layer, "Drinking Unit", unit_to_string(unit), NULL);
                    break;
                case 2:
                    menu_cell_basic_draw(ctx, cell_layer, "End of Day", eod_to_string(end_of_day), NULL);
                    break;
            }
            break;
    }
}

static void settings_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    switch (cell_index->section) {
        case 0:
            switch (cell_index->row) {
                case 0:
                    profile_menu_show();
                    break;
            }
            break;
        case 1:
            switch (cell_index->row) {
                case 0:
                    goal_menu_show();
                    break;
                case 1:
                    unit_menu_show();
                    break;
                case 2:
                    eod_menu_show();
                    break;
            }
            break;
    }
}

static void settings_menu_show() {
    window_stack_push(settings_menu_window, true);
}

static void settings_menu_window_load(Window *window) {
    Layer *menu_window_layer = window_get_root_layer(window);
    GRect menu_bounds = layer_get_bounds(menu_window_layer);
    
    // Create the menu layer
    settings_menu_layer = menu_layer_create(menu_bounds);
    
    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(settings_menu_layer, window);
    
    // Setup callbacks
    menu_layer_set_callbacks(settings_menu_layer, NULL, (MenuLayerCallbacks){
        .get_header_height = settings_menu_get_header_height_callback,
        .draw_header = settings_menu_draw_header_callback,
        .get_num_sections = settings_menu_get_num_sections_callback,
        .get_num_rows = settings_menu_get_num_rows_callback,
        .draw_row = settings_menu_draw_row_callback,
        .select_click = settings_menu_select_callback,
    });
    
    // Add it to the window for display
    layer_add_child(menu_window_layer, menu_layer_get_layer(settings_menu_layer));
}

static void settings_menu_window_unload(Window *window) {
    menu_layer_destroy(settings_menu_layer);
}
// End settings menu stuff



// Profile menu stuff
static void profile_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0:
            menu_cell_basic_header_draw(ctx, cell_layer, "Your Profile");
            break;
        case 1:
            menu_cell_basic_header_draw(ctx, cell_layer, "Actions");
            break;
    }
}

static uint16_t profile_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 2;
}

static uint16_t profile_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    switch (section_index) {
        case 0:
            return 3;
            
        case 1:
            return 1;
            
        default:
            return 0;
    }
}

static int16_t profile_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void profile_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    char buffer[20];
    // Determine which section we're going to draw in
    switch (cell_index->section) {
        case 0:
            // Use the row to specify which item we'll draw
            switch (cell_index->row) {
                case 0:
                    snprintf(buffer, sizeof(buffer), "%u Gallons", (int)(total_consumed/(OZ_IN_GAL*get_goal_scale())));
                    menu_cell_basic_draw(ctx, cell_layer, "Total Consumed", buffer, NULL);
                    break;
                case 1:
                    snprintf(buffer, sizeof(buffer), "%u Days", longest_streak);
                    menu_cell_basic_draw(ctx, cell_layer, "Longest Streak", buffer, NULL);
                    break;
                case 2:
                    strftime(buffer, 20, "%b %e, %Y", localtime(&drinking_since));
                    menu_cell_basic_draw(ctx, cell_layer, "Drinking Since", buffer, NULL);
                    break;
            }
            break;
            
        case 1:
            switch (cell_index->row) {
                case 0:
                    menu_cell_basic_draw(ctx, cell_layer, "Reset Profile", "Can't be undone!", NULL);
                    break;
            }
            break;
    }
}

static void profile_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    switch (cell_index->section) {
        case 1:
            switch (cell_index->row) {
                case 0:
                    reset_profile();
                    break;
            }
            break;
    }
}

static void profile_menu_show() {
    window_stack_push(profile_menu_window, true);
}

static void profile_menu_window_load(Window *window) {
    Layer *menu_window_layer = window_get_root_layer(window);
    GRect menu_bounds = layer_get_bounds(menu_window_layer);
    
    // Create the menu layer
    profile_menu_layer = menu_layer_create(menu_bounds);
    
    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(profile_menu_layer, window);
    
    // Setup callbacks
    menu_layer_set_callbacks(profile_menu_layer, NULL, (MenuLayerCallbacks){
        .get_header_height = profile_menu_get_header_height_callback,
        .draw_header = profile_menu_draw_header_callback,
        .get_num_sections = profile_menu_get_num_sections_callback,
        .get_num_rows = profile_menu_get_num_rows_callback,
        .draw_row = profile_menu_draw_row_callback,
        .select_click = profile_menu_select_callback,
    });
    
    // Add it to the window for display
    layer_add_child(menu_window_layer, menu_layer_get_layer(profile_menu_layer));
}

static void profile_menu_window_unload(Window *window) {
    menu_layer_destroy(profile_menu_layer);
}
// End settings menu stuff



// Goal menu stuff
static void goal_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Change Daily Goal");
}

static uint16_t goal_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t goal_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return 2;
}

static int16_t goal_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void goal_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Use the row to specify which item we'll draw
    menu_cell_basic_draw(ctx, cell_layer, unit_to_string(cell_index->row + 4), NULL, NULL);
}

static void goal_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    goal = cell_index->row + 4;
    set_image_for_goal();
    update_streak_count();
    update_volume_display();
    window_stack_pop(true);
}

static void goal_menu_show() {
    window_stack_push(goal_menu_window, true);
    
    // Sets the selected goal in the menu
    menu_layer_set_selected_index(goal_menu_layer, (MenuIndex) { .row = goal - 4, .section = 0 }, MenuRowAlignCenter, false);
}

static void goal_menu_window_load(Window *window) {
    Layer *menu_window_layer = window_get_root_layer(window);
    GRect menu_bounds = layer_get_bounds(menu_window_layer);
    
    // Create the menu layer
    goal_menu_layer = menu_layer_create(menu_bounds);
    
    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(goal_menu_layer, window);
    
    // Setup callbacks
    menu_layer_set_callbacks(goal_menu_layer, NULL, (MenuLayerCallbacks){
        .get_header_height = goal_menu_get_header_height_callback,
        .draw_header = goal_menu_draw_header_callback,
        .get_num_sections = goal_menu_get_num_sections_callback,
        .get_num_rows = goal_menu_get_num_rows_callback,
        .draw_row = goal_menu_draw_row_callback,
        .select_click = goal_menu_select_callback,
    });
    
    // Add it to the window for display
    layer_add_child(menu_window_layer, menu_layer_get_layer(goal_menu_layer));
}

static void goal_menu_window_unload(Window *window) {
    menu_layer_destroy(goal_menu_layer);
}
// End goal menu stuff



// Unit menu stuff
static void unit_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Change Drinking Unit");
}

static uint16_t unit_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t unit_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return 4;
}

static int16_t unit_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void unit_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Use the row to specify which item we'll draw
    menu_cell_basic_draw(ctx, cell_layer, unit_to_string(cell_index->row), NULL, NULL);
}

static void unit_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    unit = cell_index->row;
    update_volume_display();
    window_stack_pop(true);
}

static void unit_menu_show() {
    window_stack_push(unit_menu_window, true);
    
    // Sets the selected unit in the menu
    menu_layer_set_selected_index(unit_menu_layer, (MenuIndex) { .row = unit, .section = 0 }, MenuRowAlignCenter, false);
}

static void unit_menu_window_load(Window *window) {
    Layer *menu_window_layer = window_get_root_layer(window);
    GRect menu_bounds = layer_get_bounds(menu_window_layer);
    
    // Create the menu layer
    unit_menu_layer = menu_layer_create(menu_bounds);
    
    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(unit_menu_layer, window);
    
    // Setup callbacks
    menu_layer_set_callbacks(unit_menu_layer, NULL, (MenuLayerCallbacks){
        .get_header_height = unit_menu_get_header_height_callback,
        .draw_header = unit_menu_draw_header_callback,
        .get_num_sections = unit_menu_get_num_sections_callback,
        .get_num_rows = unit_menu_get_num_rows_callback,
        .draw_row = unit_menu_draw_row_callback,
        .select_click = unit_menu_select_callback,
    });
    
    // Add it to the window for display
    layer_add_child(menu_window_layer, menu_layer_get_layer(unit_menu_layer));
}

static void unit_menu_window_unload(Window *window) {
    menu_layer_destroy(unit_menu_layer);
}
// End unit menu stuff



// End of day menu stuff
static void eod_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Change End of Day");
}

static uint16_t eod_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t eod_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return 8;
}

static int16_t eod_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void eod_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Use the row to specify which item we'll draw
    menu_cell_basic_draw(ctx, cell_layer, eod_to_string(cell_index->row * 3), NULL, NULL);
}

static void eod_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    uint16_t old_end_of_day = end_of_day;
    end_of_day = cell_index->row * 3;
    uint32_t time_diff = (end_of_day - old_end_of_day) * 3600;
    current_date -= time_diff;
    last_streak_date -= time_diff;
    reset_current_date_and_volume_if_needed();
    window_stack_pop(true);
}

static void eod_menu_show() {
    window_stack_push(eod_menu_window, true);
    
    // Sets the selected unit in the menu
    menu_layer_set_selected_index(eod_menu_layer, (MenuIndex) { .row = end_of_day / 3, .section = 0 }, MenuRowAlignCenter, false);
}

static void eod_menu_window_load(Window *window) {
    Layer *menu_window_layer = window_get_root_layer(window);
    GRect menu_bounds = layer_get_bounds(menu_window_layer);
    
    // Create the menu layer
    eod_menu_layer = menu_layer_create(menu_bounds);
    
    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(eod_menu_layer, window);
    
    // Setup callbacks
    menu_layer_set_callbacks(eod_menu_layer, NULL, (MenuLayerCallbacks){
        .get_header_height = eod_menu_get_header_height_callback,
        .draw_header = eod_menu_draw_header_callback,
        .get_num_sections = eod_menu_get_num_sections_callback,
        .get_num_rows = eod_menu_get_num_rows_callback,
        .draw_row = eod_menu_draw_row_callback,
        .select_click = eod_menu_select_callback,
    });
    
    // Add it to the window for display
    layer_add_child(menu_window_layer, menu_layer_get_layer(eod_menu_layer));
}

static void eod_menu_window_unload(Window *window) {
    menu_layer_destroy(eod_menu_layer);
}
// End end of day menu stuff
