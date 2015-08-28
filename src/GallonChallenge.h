#ifndef GALLON_CHALLENGE_HEADER
#define GALLON_CHALLENGE_HEADER

// Key for saving the previous date that the user completed the day's challenge
// to determine if the streak count needs to be reset. Saved as a time_t.
#define LAST_STREAK_DATE_KEY 1000
// Key for saving streak count
#define STREAK_COUNT_KEY 1001
// Keys for saving current day's water volume intake
#define CURRENT_DATE_KEY 1002
#define CURRENT_OZ_KEY 1003
#define CURRENT_ML_KEY 1013
// Key for saving display unit type
#define UNIT_KEY 1004
// Key for saving the type of unit system
#define UNIT_SYSTEM_KEY 1011
// Keys for saving the custom drinking unit in oz/ml
#define CDU_OZ_KEY 1014
#define CDU_ML_KEY 1015
// Key for saving goal unit type
#define GOAL_KEY 1005
// Key for saving end of day
#define EOD_KEY 1006
// Key for saving start of day
#define SOD_KEY 1012
// Keys for saving profile info
#define TOTAL_CONSUMED_KEY 1007
#define LONGEST_STREAK_KEY 1008
#define DRINKING_SINCE_KEY 1009
// Key for saving the number of hours for inactivity reminder
#define REMINDER_KEY 1010

#define WAKEUP_REMINDER_REASON 2000
#define WAKEUP_REMINDER_ID_KEY 2001
#define WAKEUP_RESET_REASON 2002
#define WAKEUP_RESET_ID_KEY 2003

#define OZ_IN_CUP 8
#define OZ_IN_PINT 16
#define OZ_IN_QUART 32
#define OZ_IN_GAL 128
#define EXACT_ML_IN_OZ 29.5735
#define ML_IN_OZ 50 // approx
#define ML_IN_CUP 250 // approx
#define ML_IN_PINT 500 // approx
#define ML_IN_QUART 1000 // approx
#define ML_IN_GAL 4000 // approx
#define CUP_IN_GAL 16
#define PINT_IN_GAL 8
#define QUART_IN_GAL 4
#define ML_IN_L 1000

#define SEC_IN_HOUR 3600
#define SEC_IN_DAY 86400

typedef enum {
    OUNCE,
    CUP,
    PINT,
    QUART,
    HALF_GALLON,
    GALLON,
    CUSTOM
} Unit;

typedef enum {
    CUSTOMARY,
    METRIC
} UnitSystem;

static uint16_t half_gallon_height(float v);
static uint16_t gallon_height(float v);
static uint16_t container_height(float vol);
static const char* unit_system_to_string(UnitSystem us);
static const char* unit_to_string(Unit u);
static const char* hour_to_string(uint16_t hour);
static const char* reminder_to_string(uint16_t hour);
static bool are_dates_equal(time_t date1, time_t date2);
static bool should_vibrate();
static time_t now();
static time_t get_todays_date();
static time_t get_yesterdays_date();
static time_t get_next_reset_time();
static float hours_left_in_day();
static bool reset_current_date_and_volume_if_needed();
static uint16_t calc_current_volume();
static uint16_t get_unit_in_gal();
static uint16_t get_ml_in(Unit u);
static float get_goal_scale();
static uint16_t get_goal_vol(UnitSystem us);
static void set_image_for_goal();
static void update_volume_display();
static void update_streak_display();
static void increment_volume();
static void decrement_volume();
static void update_streak_count();
static void reset_profile();

static void cancel_app_exit_and_remove_notify_text();
static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);

static void CDU_select_click_handler(ClickRecognizerRef recognizer, void *context);
static void CDU_up_click_handler(ClickRecognizerRef recognizer, void *context);
static void CDU_down_click_handler(ClickRecognizerRef recognizer, void *context);
static void CDU_update_display();
static void CDU_click_config_provider(void *context);

static void handle_hour_tick(struct tm *tick_time, TimeUnits units_changed);
static void wakeup_handler(WakeupId id, int32_t reason);
static void reset_reminder();
static void schedule_reminder_if_needed();
static void schedule_reset_if_needed();
static void app_exit_callback();

static void load_persistent_storage();
static void save_persistent_storage();

static void window_load(Window *window);
static void window_unload(Window *window);

static void CDU_window_load(Window *window);
static void CDU_window_unload(Window *window);

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

static Window *unit_system_menu_window;
static MenuLayer *unit_system_menu_layer;
static void unit_system_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t unit_system_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t unit_system_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t unit_system_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void unit_system_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void unit_system_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void unit_system_menu_show();
static void unit_system_menu_window_load(Window *window);
static void unit_system_menu_window_unload(Window *window);

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

static Window *sod_menu_window;
static MenuLayer *sod_menu_layer;
static void sod_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t sod_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data);
static uint16_t sod_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static int16_t sod_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void sod_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
static void sod_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);
static void sod_menu_show();
static void sod_menu_window_load(Window *window);
static void sod_menu_window_unload(Window *window);

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