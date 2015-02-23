#ifndef GALLON_CHALLENGE_HEADER
#define GALLON_CHALLENGE_HEADER

static bool is_last_streak_date_today();
static time_t get_todays_date();
static time_t get_yesterdays_date();
static uint16_t calc_current_volume();
static uint16_t get_unit_in_gal();
static void update_volume_display();
static void update_streak_display();
static void increment_volume();
static void decrement_volume();

static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);

static void menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data);
static uint16_t menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data);
static void menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data);
void menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data);

static void load_persistent_storage();
static void save_persistent_storage();
static void set_selected_unit_in_menu();

static void window_load(Window *window);
static void window_unload(Window *window);
static void menu_window_load(Window *window);
static void menu_window_unload(Window *window);
static void init(void);
static void deinit(void);

#endif