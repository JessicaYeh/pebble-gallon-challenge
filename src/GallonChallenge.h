#ifndef GALLON_CHALLENGE_HEADER
#define GALLON_CHALLENGE_HEADER

typedef enum {
    OUNCE = 0,
    CUP = 1,
    PINT = 2,
    QUART = 3,
    HALF_GALLON = 4,
    GALLON = 5
} Unit;

static uint16_t half_gallon_height(float vol);
static uint16_t gallon_height(float vol);
static uint16_t container_height(float vol);
static const char* unit_to_string(Unit u);
static const char* eod_to_string(uint16_t hour);
static const char* reminder_to_string(uint16_t hour);
static bool are_dates_equal(time_t date1, time_t date2);
static time_t now();
static time_t get_todays_date();
static time_t get_yesterdays_date();
static void reset_current_date_and_volume_if_needed();
static uint16_t calc_current_volume();
static uint16_t get_unit_in_gal();
static float get_goal_scale();
static void set_image_for_goal();
static void update_volume_display();
static void update_streak_display();
static void increment_volume();
static void decrement_volume();
static void update_streak_count();
static void reset_profile();

static void cancel_app_exit_and_remove_reminder_text();
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);

static void handle_hour_tick(struct tm *tick_time, TimeUnits units_changed);
static void wakeup_handler(WakeupId id, int32_t reason);
static void schedule_wakeup_if_needed();
static void app_exit_callback();

static void load_persistent_storage();
static void save_persistent_storage();

static void window_load(Window *window);
static void window_unload(Window *window);
static void init(void);
static void deinit(void);

static Window *settings_menu_window;
static MenuLayer *settings_menu_layer;
static void settings_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t settings_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t settings_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t settings_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void settings_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void settings_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void settings_menu_show();
static void settings_menu_window_load(Window *window);
static void settings_menu_window_unload(Window *window);

static Window *profile_menu_window;
static MenuLayer *profile_menu_layer;
static void profile_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t profile_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t profile_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t profile_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void profile_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void profile_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void profile_menu_show();
static void profile_menu_window_load(Window *window);
static void profile_menu_window_unload(Window *window);

static Window *goal_menu_window;
static MenuLayer *goal_menu_layer;
static void goal_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t goal_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t goal_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t goal_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void goal_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void goal_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void goal_menu_show();
static void goal_menu_window_load(Window *window);
static void goal_menu_window_unload(Window *window);

static Window *unit_menu_window;
static MenuLayer *unit_menu_layer;
static void unit_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t unit_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t unit_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t unit_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void unit_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void unit_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void unit_menu_show();
static void unit_menu_window_load(Window *window);
static void unit_menu_window_unload(Window *window);

static Window *eod_menu_window;
static MenuLayer *eod_menu_layer;
static void eod_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t eod_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t eod_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t eod_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void eod_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void eod_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void eod_menu_show();
static void eod_menu_window_load(Window *window);
static void eod_menu_window_unload(Window *window);

static Window *reminder_menu_window;
static MenuLayer *reminder_menu_layer;
static void reminder_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t reminder_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t reminder_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t reminder_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void reminder_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void reminder_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void reminder_menu_show();
static void reminder_menu_window_load(Window *window);
static void reminder_menu_window_unload(Window *window);

#endif