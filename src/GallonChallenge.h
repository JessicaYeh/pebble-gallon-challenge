#ifndef GALLON_CHALLENGE_HEADER
#define GALLON_CHALLENGE_HEADER

static void select_click_handler(ClickRecognizerRef recognizer, void *context);
static void up_click_handler(ClickRecognizerRef recognizer, void *context);
static void down_click_handler(ClickRecognizerRef recognizer, void *context);
static void click_config_provider(void *context);

static void window_load(Window *window);
static void window_unload(Window *window);
static void init(void);
static void deinit(void);

#endif