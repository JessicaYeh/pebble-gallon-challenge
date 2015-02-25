#ifndef GALLON_CHALLENGE_HEADER
#define GALLON_CHALLENGE_HEADER

static bool is_this_date_today(time_t date);
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

static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);

static void handle_hour_tick(struct tm *tick_time, TimeUnits units_changed);

static void settings_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t settings_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t settings_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t settings_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void settings_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void settings_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);

static void unit_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t unit_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t unit_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t unit_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void unit_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void unit_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);

static void load_persistent_storage();
static void save_persistent_storage();

static void window_load(Window *window);
static void window_unload(Window *window);
static void settings_menu_window_load(Window *window);
static void settings_menu_window_unload(Window *window);
static void unit_menu_window_load(Window *window);
static void unit_menu_window_unload(Window *window);
static void settings_menu_show();
static void unit_menu_show();

static void init(void);
static void deinit(void);

#endif