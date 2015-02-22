#ifndef GALLON_CHALLENGE_HEADER
#define GALLON_CHALLENGE_HEADER

static float calc_current_volume();
static void update_gallon();
static void increment_volume();
static void decrement_volume();

static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);

static void load_persistent_storage();
static void save_persistent_storage();

static void window_load(Window *window);
static void window_unload(Window *window);
static void init(void);
static void deinit(void);

#endif