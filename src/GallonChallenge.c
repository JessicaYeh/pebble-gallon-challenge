#include <pebble.h>
#include "PDUtils.h"
#include "GallonChallenge.h"

static Window *window, *custom_drink_unit_window;

static GBitmap *action_icon_plus, *action_icon_settings, *action_icon_check, *action_icon_minus, *gallon_filled_image, *gallon_image, *star;

static ActionBarLayer *action_bar, *CDU_action_bar;
static TextLayer *streak_text_layer, *text_layer, *white_layer, *notify_text_layer, *CDU_header_text_layer, *CDU_text_layer;
static BitmapLayer *gallon_filled_layer, *gallon_layer, *star_layer;

static UnitSystem unit_system;
static Unit goal, unit;

static time_t current_date, last_streak_date, drinking_since;
static uint16_t current_oz, current_ml, streak_count, start_of_day, end_of_day, inactivity_reminder_hours, longest_streak, temp_cdu_oz, temp_cdu_ml, cdu_oz, cdu_ml;
static uint32_t total_consumed;

static WakeupId wakeup_reminder_id, wakeup_reset_id;

static AppTimer *quit_timer, *reset_reminder_timer, *remove_notify_timer;

static bool launched = false;

static uint16_t width, x_shift, y_shift;


static uint16_t half_gallon_height(float v) {
    if (unit_system == CUSTOMARY) {
        return -2.648947948*10e-7*v*v*v*v + 1.57399393*10e-5*v*v*v - 3.197706522*10e-5*v*v - 1.227965142*v + 84.21612388;
    } else {
        return -2.777623239*10e-13*v*v*v*v + 5.157663293*10e-10*v*v*v - 3.27445129*10e-8*v*v - 3.929488456*10e-3*v + 84.21612388;
    }
}

static uint16_t gallon_height(float v) {
    if (unit_system == CUSTOMARY) {
        return -1.308363868*10e-8*v*v*v*v + 3.373611245*10e-7*v*v*v + 1.708355542*10e-4*v*v - 6.136166056*10e-2*v + 81.11469866;
    } else {
        return -1.371918968*10e-14*v*v*v*v + 1.105465059*10e-11*v*v*v + 1.749356045*10e-7*v*v - 1.963573136*10e-3*v + 81.11469866;
    }
}

static uint16_t container_height(float vol) {
    if (vol == 0) {
        return 93;
    }
    
    if (vol >= get_goal_vol(unit_system)) {
        return 0;
    }
    
    switch (goal) {
        case HALF_GALLON:
            return half_gallon_height(vol);
            break;
            
        default:
            return gallon_height(vol);
            break;
    }
}

static const char* unit_system_to_string(UnitSystem us) {
    switch (us) {
        case CUSTOMARY:   return "Customary (Gallons)";
        case METRIC:      return "Metric (Liters)";
        default:          return "";
    }
}

static const char* unit_to_string(Unit u) {
    switch (unit_system) {
        case CUSTOMARY:
            switch (u) {
                case OUNCE:       return "Ounces";
                case CUP:         return "Cups";
                case PINT:        return "Pints";
                case QUART:       return "Quarts";
                case HALF_GALLON: return "Half Gallon";
                case GALLON:      return "One Gallon";
                case CUSTOM:      return "Ounces";
                default:          return "";
            }
        case METRIC:
            switch (u) {
                case OUNCE:       return "50 mL";
                case CUP:         return "250 mL";
                case PINT:        return "500 mL";
                case QUART:       return "1 Liter";
                case HALF_GALLON: return "2 Liters";
                case GALLON:      return "4 Liters";
                case CUSTOM:      return custom_unit_to_string();
                default:          return "";
            }
        default:
            return "";
    }
}

static const char* custom_unit_to_string() {
    static char string[10];
    switch (unit_system) {
        case CUSTOMARY:
            snprintf(string, sizeof(string), "%u oz", cdu_oz);
            return string;
        case METRIC:
            snprintf(string, sizeof(string), "%u mL", cdu_ml);
            return string;
        default:
            return "";
    }
}

static const char* hour_to_string(uint16_t hour) {
    switch (hour) {
        case 0:  return "12:00 AM";
        case 1:  return "1:00 AM";
        case 2:  return "2:00 AM";
        case 3:  return "3:00 AM";
        case 4:  return "4:00 AM";
        case 5:  return "5:00 AM";
        case 6:  return "6:00 AM";
        case 7:  return "7:00 AM";
        case 8:  return "8:00 AM";
        case 9:  return "9:00 AM";
        case 10: return "10:00 AM";
        case 11: return "11:00 AM";
        case 12: return "12:00 PM";
        case 13: return "1:00 PM";
        case 14: return "2:00 PM";
        case 15: return "3:00 PM";
        case 16: return "4:00 PM";
        case 17: return "5:00 PM";
        case 18: return "6:00 PM";
        case 19: return "7:00 PM";
        case 20: return "8:00 PM";
        case 21: return "9:00 PM";
        case 22: return "10:00 PM";
        case 23: return "11:00 PM";
        default: return "";
    }
}

static const char* reminder_to_string(uint16_t hour) {
    switch (hour) {
        case 0:  return "Off";
        case 1:  return "Auto";
        case 2:  return "1 Hour";
        case 3:  return "2 Hours";
        case 4:  return "3 Hours";
        case 5:  return "4 Hours";
        case 6:  return "5 Hours";
        case 7:  return "6 Hours";
        case 8:  return "7 Hours";
        case 9:  return "8 Hours";
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

static bool should_vibrate() {
    // Get the current hour
    time_t current_time = now();
    struct tm *time_struct = localtime(&current_time);
    uint16_t hour = time_struct->tm_hour;

    // Make adjustments to be able to calculate the silent hours
    uint16_t start_silent = (end_of_day + 24 - 2) % 24;
    uint16_t end_silent = (start_of_day + 24) % 24;
    if (start_silent > end_silent) {
        end_silent += 24;
    }
    if (hour < start_silent) {
        hour += 24;
    }

    // Whether the hour is inside the silent hours
    if (hour >= start_silent && hour <= end_silent) {
        return false;
    }
    return true;
}

static time_t now() {
    return time(NULL);
}

static time_t get_todays_date() {
    return now() - end_of_day * SEC_IN_HOUR;
}

static time_t get_yesterdays_date() {
    return now() - end_of_day * SEC_IN_HOUR - SEC_IN_DAY;
}

static time_t get_next_reset_time() {
    time_t current_time = now();

    // Set the reset time to be at the "end of day" of the current day
    struct tm *reset_time_struct = localtime(&current_time);
    reset_time_struct->tm_hour = end_of_day;
    reset_time_struct->tm_min = 0;
    reset_time_struct->tm_sec = 0;

    // If the reset time is earlier than the current time, add a day to reset time
    time_t reset_time = p_mktime(reset_time_struct);
    if ((int)reset_time < (int)current_time) {
        reset_time += SEC_IN_DAY;
    }

    return reset_time;
}

static float hours_left_in_day() {
    time_t current_time = now();
    time_t end_of_day_time = get_next_reset_time() - SEC_IN_HOUR * 2;

    float hours_left = (end_of_day_time - current_time) / 3600.0;
    if (hours_left < 0) hours_left = 0;
    return hours_left;
}

static bool reset_current_date_and_volume_if_needed() {
    bool reset = false;

    time_t today = get_todays_date();
    time_t yesterday = get_yesterdays_date();
    if (!are_dates_equal(current_date, today)) {
        current_date = today;
        current_oz = 0;
        current_ml = 0;
        reset_reminder();
        reset = true;
        
        // Reset the streak if needed
        if (!are_dates_equal(last_streak_date, yesterday)) {
            if (streak_count > longest_streak) {
                longest_streak = streak_count;
            }
            streak_count = 0;
        }
    }
    
    update_streak_count();
    update_volume_display();

    return reset;
}

// Uses the current_oz/current_ml and the chosen display unit to calculate the 
// volume of liquid consumed in the current day
static uint16_t calc_current_volume() {
    switch (unit_system) {
        case CUSTOMARY:
            switch (unit) {
                case CUP:   return current_oz / OZ_IN_CUP;
                case PINT:  return current_oz / OZ_IN_PINT;
                case QUART: return current_oz / OZ_IN_QUART;
                default:    return current_oz;
            }
        case METRIC:
            return current_ml;
        default:
            return 0;
    }
}

static uint16_t get_unit_in_gal(bool forAutoReminder) {
    switch (unit_system) {
        case CUSTOMARY:
            switch (unit) {
                case CUP:    return CUP_IN_GAL;
                case PINT:   return PINT_IN_GAL;
                case QUART:  return QUART_IN_GAL;
                case CUSTOM: if (forAutoReminder) return OZ_IN_GAL / cdu_oz;
                default:     return OZ_IN_GAL;
            }
        case METRIC:
            return ML_IN_GAL;
        default:
            return 0;
    }
}

static uint16_t get_ml_in(Unit u) {
    switch (u) {
        case OUNCE:       return ML_IN_OZ;
        case CUP:         return ML_IN_CUP;
        case PINT:        return ML_IN_PINT;
        case QUART:       return ML_IN_QUART;
        case HALF_GALLON: return ML_IN_GAL * 0.5;
        case GALLON:      return ML_IN_GAL;
        case CUSTOM:      return ML_IN_GAL / cdu_ml;
        default:          return 0;
    }
}

static float get_goal_scale() {
    switch (goal) {
        case HALF_GALLON: return 0.5;
        default:          return 1.0;
    }
}

static uint16_t get_goal_vol(UnitSystem us) {
    switch (us) {
        case CUSTOMARY: return OZ_IN_GAL * get_goal_scale();
        case METRIC:    return ML_IN_GAL * get_goal_scale();
        default:        return 0;
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
    uint16_t denominator = get_unit_in_gal(false) * get_goal_scale();

    // Ounces needs to be abbreviated to not be cut off
    const char* unit_string = unit_to_string(unit);
    if (strcmp(unit_string, "Ounces") == 0) unit_string = "oz";

    snprintf(body_text, sizeof(body_text), "%u/%u %s", numerator, denominator, 
        (unit_system == CUSTOMARY) ? unit_string : "mL");
    text_layer_set_text(text_layer, body_text);
    
    uint16_t height = container_height((unit_system == CUSTOMARY) ? current_oz : current_ml);
    layer_set_frame(text_layer_get_layer(white_layer), GRect(0, 34 + y_shift, width, height));

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
    uint16_t oz_vol_inc, ml_vol_inc;
    switch (unit) {
        case CUP:
            oz_vol_inc = OZ_IN_CUP;
            ml_vol_inc = ML_IN_CUP;
            break;
        case PINT:
            oz_vol_inc = OZ_IN_PINT;
            ml_vol_inc = ML_IN_PINT;
            break;
        case QUART:
            oz_vol_inc = OZ_IN_QUART;
            ml_vol_inc = ML_IN_QUART;
            break;
        case CUSTOM:
            oz_vol_inc = cdu_oz;
            ml_vol_inc = cdu_ml;
            break;
        default:
            oz_vol_inc = 1;
            ml_vol_inc = ML_IN_OZ;
            break;
    }

    // Make the conversion more exact
    oz_vol_inc = (unit_system == CUSTOMARY) ? oz_vol_inc : (int)((float)ml_vol_inc / EXACT_ML_IN_OZ);
    ml_vol_inc = (unit_system == METRIC) ? ml_vol_inc : (int)((float)oz_vol_inc * EXACT_ML_IN_OZ);
    
    // maybe not needed
    uint16_t oz_in_goal, ml_in_goal;
    switch (unit_system) {
        case CUSTOMARY:
            oz_in_goal = OZ_IN_GAL * get_goal_scale();
            if (current_oz + oz_vol_inc > oz_in_goal) {
                total_consumed += oz_in_goal - current_oz;
            } else if (current_oz < oz_in_goal) {
                total_consumed += oz_vol_inc;
            }
            break;
        case METRIC:
            ml_in_goal = ML_IN_GAL * get_goal_scale();
            if (current_ml + ml_vol_inc > ml_in_goal) {
                total_consumed += (ml_in_goal - current_ml) / EXACT_ML_IN_OZ;
            } else if (current_ml < ml_in_goal) {
                total_consumed += ml_vol_inc / EXACT_ML_IN_OZ;
            }
            break;
        default:
            break;
    }

    current_oz += oz_vol_inc;
    current_ml += ml_vol_inc;
    
    update_streak_count();
    update_volume_display();

    // In case of repeating clicks, don't immediately reset the reminder
    app_timer_cancel(reset_reminder_timer);
    reset_reminder_timer = app_timer_register(666, reset_reminder, NULL);
}

// Decrease the current volume by one unit
static void decrement_volume() {
    uint16_t oz_vol_dec, ml_vol_dec;
    
    switch (unit) {
        case CUP:
            oz_vol_dec = OZ_IN_CUP;
            ml_vol_dec = ML_IN_CUP;
            break;
        case PINT:
            oz_vol_dec = OZ_IN_PINT;
            ml_vol_dec = ML_IN_PINT;
            break;
        case QUART:
            oz_vol_dec = OZ_IN_QUART;
            ml_vol_dec = ML_IN_QUART;
            break;
        case CUSTOM:
            oz_vol_dec = cdu_oz;
            ml_vol_dec = cdu_ml;
            break;
        default:
            oz_vol_dec = 1;
            ml_vol_dec = ML_IN_OZ;
            break;
    }

    // Make the conversion more exact
    oz_vol_dec = (unit_system == CUSTOMARY) ? oz_vol_dec : (int)((float)ml_vol_dec / EXACT_ML_IN_OZ);
    ml_vol_dec = (unit_system == METRIC) ? ml_vol_dec : (int)((float)oz_vol_dec * EXACT_ML_IN_OZ);
    
    // maybe not needed
    switch (unit_system) {
        case CUSTOMARY:
            (current_oz < oz_vol_dec) ? (total_consumed -= current_oz) : (total_consumed -= oz_vol_dec);
            break;
        case METRIC:
            (current_ml < ml_vol_dec) ? (total_consumed -= current_ml/EXACT_ML_IN_OZ) : (total_consumed -= ml_vol_dec/EXACT_ML_IN_OZ);
            break;
        default:
            break;
    }

    (current_oz < oz_vol_dec) ? (current_oz = 0) : (current_oz -= oz_vol_dec);
    (current_ml < ml_vol_dec) ? (current_ml = 0) : (current_ml -= ml_vol_dec);
    
    update_streak_count();
    update_volume_display();

    // In case of repeating clicks, don't immediately reset the reminder
    app_timer_cancel(reset_reminder_timer);
    reset_reminder_timer = app_timer_register(666, reset_reminder, NULL);
}

static void update_streak_count() {
    // Restrict the max volumes to the goal volumes
    if (current_oz >= get_goal_vol(CUSTOMARY)) current_oz = get_goal_vol(CUSTOMARY);
    if (current_ml >= get_goal_vol(METRIC)) current_ml = get_goal_vol(METRIC);

    uint16_t goal_vol = get_goal_vol(unit_system);
    uint16_t current_vol = (unit_system == CUSTOMARY) ? current_oz : current_ml;
    time_t today = get_todays_date();
    
    if (current_vol >= goal_vol) {
        // If the last streak date is not today's date, then set that date to
        // today's date and increment the streak count.
        if (!are_dates_equal(last_streak_date, today)) {
            last_streak_date = today;
            streak_count++;
        }
    } else if (current_vol < goal_vol && are_dates_equal(last_streak_date, today)) {
        // If the last streak date is today's date, since the goal is now no longer
        // met, set the last streak date to the previous date and decrement the
        // streak count.
        last_streak_date = get_yesterdays_date();
        if (streak_count > 0) {
            streak_count--;
        }
    }
    
    update_streak_display();
}

static void reset_profile() {
    total_consumed = current_oz;
    longest_streak = 0;
    drinking_since = now();
    layer_mark_dirty(menu_layer_get_layer(profile_menu_layer));
}

static void cancel_app_exit_and_remove_notify_text() {
    if (!layer_get_hidden(text_layer_get_layer(notify_text_layer))) {
        app_timer_cancel(quit_timer);
        layer_set_hidden(text_layer_get_layer(notify_text_layer), true);
    }
}

static void select_click_handler(ClickRecognizerRef recognizer, void *context) {
    cancel_app_exit_and_remove_notify_text();
    settings_menu_show();
}

static void up_click_handler(ClickRecognizerRef recognizer, void *context) {
    cancel_app_exit_and_remove_notify_text();
    increment_volume();
}

static void down_click_handler(ClickRecognizerRef recognizer, void *context) {
    cancel_app_exit_and_remove_notify_text();
    decrement_volume();
}

static void click_config_provider(void *context) {
    const uint16_t repeat_interval_ms = 100;
    window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, up_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, select_click_handler);
}

static void CDU_select_click_handler(ClickRecognizerRef recognizer, void *context) {
    unit = CUSTOM;
    cdu_oz = temp_cdu_oz;
    cdu_ml = temp_cdu_ml;
    update_volume_display();
    reset_reminder();
    window_stack_pop(true);
    window_stack_pop(true);
}

static void CDU_up_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (unit_system == CUSTOMARY) {
        if (temp_cdu_oz < OZ_IN_GAL * get_goal_scale()) {
            temp_cdu_oz++;
            CDU_update_display();
        }
    } else if (unit_system == METRIC) {
        if (temp_cdu_ml < ML_IN_GAL * get_goal_scale()) {
            temp_cdu_ml += 50;
            CDU_update_display();
        }
    }
}

static void CDU_down_click_handler(ClickRecognizerRef recognizer, void *context) {
    if (unit_system == CUSTOMARY) {
        if (temp_cdu_oz > 1) {
            temp_cdu_oz--;
            CDU_update_display();
        }
    } else if (unit_system == METRIC) {
        if (temp_cdu_ml > 50) {
            temp_cdu_ml -= 50;
            CDU_update_display();
        }
    }
}

static void CDU_update_display() {
    static char body_text[10];
    uint16_t cdu = (unit_system == CUSTOMARY) ? temp_cdu_oz : temp_cdu_ml;
    if (unit_system == CUSTOMARY) {
        snprintf(body_text, sizeof(body_text), "%u oz", cdu);
    } else {
        snprintf(body_text, sizeof(body_text), "%u mL", cdu);
    }
    text_layer_set_text(CDU_text_layer, body_text);
}

static void CDU_click_config_provider(void *context) {
    const uint16_t repeat_interval_ms = 50;
    window_single_repeating_click_subscribe(BUTTON_ID_UP, repeat_interval_ms, CDU_up_click_handler);
    window_single_repeating_click_subscribe(BUTTON_ID_DOWN, repeat_interval_ms, CDU_down_click_handler);
    window_single_click_subscribe(BUTTON_ID_SELECT, CDU_select_click_handler);
}

static void handle_hour_tick(struct tm *tick_time, TimeUnits units_changed) {
    if (tick_time->tm_hour == end_of_day) {
        reset_current_date_and_volume_if_needed();
    }
}

static void wakeup_handler(WakeupId id, int32_t reason) {
    if (reason == WAKEUP_REMINDER_REASON) {
        persist_delete(WAKEUP_REMINDER_ID_KEY);
        wakeup_reminder_id = 0;
        app_timer_cancel(remove_notify_timer);
        text_layer_set_text(notify_text_layer, "Drink water!");
        layer_set_hidden(text_layer_get_layer(notify_text_layer), false);

        if (should_vibrate()) {
            vibes_short_pulse();
        }

        if (launch_reason() == APP_LAUNCH_WAKEUP && !launched) {
            launched = true;
            app_timer_register(1000, reset_reminder, NULL);
            // Auto exit the app after 2 minutes if it was awoken from a reminder
            quit_timer = app_timer_register(120000, app_exit_callback, NULL);
        } else {
            reset_reminder();
        }
    } else if (reason == WAKEUP_RESET_REASON) {
        persist_delete(WAKEUP_RESET_ID_KEY);
        wakeup_reset_id = 0;
        text_layer_set_text(notify_text_layer, "New day!");
        layer_set_hidden(text_layer_get_layer(notify_text_layer), false);
        if (launch_reason() == APP_LAUNCH_WAKEUP && !launched) {
            launched = true;
            app_timer_register(1000, reset_reminder, NULL);
            //app_timer_register(1000, schedule_reset_if_needed, NULL);
            // Auto exit the app after 2 minutes if it was awoken for a reset
            quit_timer = app_timer_register(120000, app_exit_callback, NULL);
        } else {
            reset_reminder();
            //schedule_reset_if_needed();
            remove_notify_timer = app_timer_register(120000, cancel_app_exit_and_remove_notify_text, NULL);
        }
    }
}

static void reset_reminder() {
    // Cancel the reminder timer if one is present
    wakeup_cancel_all();
    persist_delete(WAKEUP_REMINDER_ID_KEY);
    persist_delete(WAKEUP_RESET_ID_KEY);
    wakeup_reminder_id = 0;
    wakeup_reset_id = 0;

    // Restart the reminder timer if the goal isn't met
    schedule_reminder_if_needed();
    schedule_reset_if_needed();
}

static void schedule_reminder_if_needed() {
    // Reminders are off, don't schedule a reminder
    if (inactivity_reminder_hours == 0) {
        return;
    }

    // Goal is met, so don't schedule a reminder
    if (calc_current_volume() == get_unit_in_gal(false) * get_goal_scale()) {
        return;
    }

    bool wakeup_scheduled = false;

    // Check if we have already scheduled a wakeup event
    if (persist_exists(WAKEUP_REMINDER_ID_KEY)) {
        wakeup_reminder_id = persist_read_int(WAKEUP_REMINDER_ID_KEY);
        // query if event is still valid
        if (wakeup_query(wakeup_reminder_id, NULL)) {
            wakeup_scheduled = true;
        } else {
            persist_delete(WAKEUP_REMINDER_ID_KEY);
            wakeup_reminder_id = 0;
        }
    }

    // Check to see if we were launched by a wakeup event
    if (launch_reason() == APP_LAUNCH_WAKEUP && !launched) {
        WakeupId id = 0;
        int32_t reason = 0;
        if (wakeup_get_launch_event(&id, &reason)) {
            wakeup_handler(id, reason);
        }
    } else if (!wakeup_scheduled) {
        float hours = 0;
        if (inactivity_reminder_hours == 1) {
            // Auto reminders based on how many hours are left in the day and 
            // how much you still need to drink
            hours = hours_left_in_day() / (get_unit_in_gal(true) * get_goal_scale() - calc_current_volume());
            // If using the metric unit system, scale the amount by drinking unit
            if (unit_system == METRIC) {
                hours *= get_ml_in(unit);
            }
        } else {
            hours = inactivity_reminder_hours - 1;
        }
        if (hours < 0.5) hours = 0.5;
        time_t future_time = now() + hours * SEC_IN_HOUR;
        // Avoid time conflict with reset time
        if (future_time == get_next_reset_time()) future_time += 0.5 * SEC_IN_HOUR;

        // Repeatedly try to schedule the wakeup in case of conflicting wakeup times
        wakeup_reminder_id = 0;
        uint16_t attempts = 0;
        while ((!wakeup_reminder_id || wakeup_reminder_id == E_RANGE || wakeup_reminder_id == E_INTERNAL) && attempts < 100) {
            // Add a minute to wakeup timer if the error was for time range
            if (wakeup_reminder_id == E_RANGE) {
                future_time = future_time + 60;
            }

            // Schedule wakeup event and keep the WakeupId in case it needs to be queried
            wakeup_reminder_id = wakeup_schedule(future_time, WAKEUP_REMINDER_REASON, false);
            attempts++;
        }

        // Persist to allow wakeup query after the app is closed.
        persist_write_int(WAKEUP_REMINDER_ID_KEY, wakeup_reminder_id);
    }
}

static void schedule_reset_if_needed() {
    bool wakeup_scheduled = false;

    // Check if we have already scheduled a wakeup event
    if (persist_exists(WAKEUP_RESET_ID_KEY)) {
        wakeup_reset_id = persist_read_int(WAKEUP_RESET_ID_KEY);
        // query if event is still valid
        if (wakeup_query(wakeup_reset_id, NULL)) {
            wakeup_scheduled = true;
        } else {
            persist_delete(WAKEUP_RESET_ID_KEY);
            wakeup_reset_id = 0;
        }
    }

    // Check to see if we were launched by a wakeup event
    if (launch_reason() == APP_LAUNCH_WAKEUP && !launched) {
        WakeupId id = 0;
        int32_t reason = 0;
        if (wakeup_get_launch_event(&id, &reason)) {
            wakeup_handler(id, reason);
        }
    } else if (!wakeup_scheduled) {
        time_t future_time = get_next_reset_time();

        // Repeatedly try to schedule the wakeup in case of conflicting wakeup times
        wakeup_reset_id = 0;
        uint16_t attempts = 0;
        while ((!wakeup_reset_id || wakeup_reset_id == E_RANGE || wakeup_reset_id == E_INTERNAL) && attempts < 100) {
            // Add a minute to wakeup timer if the error was for time range
            if (wakeup_reset_id == E_RANGE) {
                future_time = future_time + 60;
            }

            // Schedule wakeup event and keep the WakeupId in case it needs to be queried
            wakeup_reset_id = wakeup_schedule(future_time, WAKEUP_RESET_REASON, true);
            attempts++;
        }

        // Persist to allow wakeup query after the app is closed.
        persist_write_int(WAKEUP_RESET_ID_KEY, wakeup_reset_id);
    }
}

static void app_exit_callback() {
    window_stack_pop_all(true);
}

static void load_persistent_storage() {
    current_oz = persist_exists(CURRENT_OZ_KEY) ? persist_read_int(CURRENT_OZ_KEY) : 0;
    current_ml = persist_exists(CURRENT_ML_KEY) ? persist_read_int(CURRENT_ML_KEY) : 0;
    start_of_day = persist_exists(SOD_KEY) ? persist_read_int(SOD_KEY) : 9;
    end_of_day = persist_exists(EOD_KEY) ? persist_read_int(EOD_KEY) : 0;
    inactivity_reminder_hours = persist_exists(REMINDER_KEY) ? persist_read_int(REMINDER_KEY) : 0;
    unit_system = persist_exists(UNIT_SYSTEM_KEY) ? persist_read_int(UNIT_SYSTEM_KEY) : CUSTOMARY;
    goal = persist_exists(GOAL_KEY) ? persist_read_int(GOAL_KEY) : GALLON;
    unit = persist_exists(UNIT_KEY) ? persist_read_int(UNIT_KEY) : CUP;
    streak_count = persist_exists(STREAK_COUNT_KEY) ? persist_read_int(STREAK_COUNT_KEY) : 0;
    last_streak_date = persist_exists(LAST_STREAK_DATE_KEY) ? persist_read_int(LAST_STREAK_DATE_KEY) : get_yesterdays_date();
    current_date = persist_exists(CURRENT_DATE_KEY) ? persist_read_int(CURRENT_DATE_KEY) : get_todays_date();
    total_consumed = persist_exists(TOTAL_CONSUMED_KEY) ? persist_read_int(TOTAL_CONSUMED_KEY) : 0;
    longest_streak = persist_exists(LONGEST_STREAK_KEY) ? persist_read_int(LONGEST_STREAK_KEY) : 0;
    cdu_oz = persist_exists(CDU_OZ_KEY) ? persist_read_int(CDU_OZ_KEY) : 8;
    cdu_ml = persist_exists(CDU_ML_KEY) ? persist_read_int(CDU_ML_KEY) : 250;
    drinking_since = persist_exists(DRINKING_SINCE_KEY) ? persist_read_int(DRINKING_SINCE_KEY) : now();
}

static void save_persistent_storage() {
    persist_write_int(CURRENT_OZ_KEY, current_oz);
    persist_write_int(CURRENT_ML_KEY, current_ml);
    persist_write_int(SOD_KEY, start_of_day);
    persist_write_int(EOD_KEY, end_of_day);
    persist_write_int(REMINDER_KEY, inactivity_reminder_hours);
    persist_write_int(UNIT_SYSTEM_KEY, unit_system);
    persist_write_int(GOAL_KEY, goal);
    persist_write_int(UNIT_KEY, unit);
    persist_write_int(STREAK_COUNT_KEY, streak_count);
    persist_write_int(LAST_STREAK_DATE_KEY, (int)last_streak_date);
    persist_write_int(CURRENT_DATE_KEY, (int)current_date);
    persist_write_int(TOTAL_CONSUMED_KEY, total_consumed);
    persist_write_int(LONGEST_STREAK_KEY, longest_streak);
    persist_write_int(CDU_OZ_KEY, cdu_oz);
    persist_write_int(CDU_ML_KEY, cdu_ml);
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
    
    width = bounds.size.w - ACTION_BAR_WIDTH;
    x_shift = (width - 114) / 2;
    uint16_t height = bounds.size.h;
    y_shift = (height - 152) / 2;

    gallon_filled_layer = bitmap_layer_create(GRect(x_shift, 29 + y_shift, 114, 92));
    layer_add_child(window_layer, bitmap_layer_get_layer(gallon_filled_layer));
    
    white_layer = text_layer_create(GRect(0, 29 + y_shift, 114, 92));
    text_layer_set_background_color(white_layer, GColorWhite);
    layer_add_child(window_layer, text_layer_get_layer(white_layer));
    
    gallon_layer = bitmap_layer_create(GRect(x_shift, 29 + y_shift, 114, 92));
    bitmap_layer_set_background_color(gallon_layer, GColorClear);
    bitmap_layer_set_compositing_mode(gallon_layer, GCompOpClear);
    layer_add_child(window_layer, bitmap_layer_get_layer(gallon_layer));
    
    streak_text_layer = text_layer_create(GRect(0, y_shift, width, 60));
    text_layer_set_font(streak_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24));
    text_layer_set_text_alignment(streak_text_layer, GTextAlignmentCenter);
    text_layer_set_background_color(streak_text_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(streak_text_layer));
    
    text_layer = text_layer_create(GRect(0, 116 + y_shift, width, 60));
    text_layer_set_font(text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
    text_layer_set_background_color(text_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(text_layer));

    star_layer = bitmap_layer_create(GRect(width/2-13, 72 + y_shift, 26, 24));
    bitmap_layer_set_bitmap(star_layer, star);
    layer_add_child(window_layer, bitmap_layer_get_layer(star_layer));

    notify_text_layer = text_layer_create(GRect(0, 62 + y_shift, width, 38));
    text_layer_set_font(notify_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_text_alignment(notify_text_layer, GTextAlignmentCenter);
    text_layer_set_background_color(notify_text_layer, GColorWhite);
    layer_add_child(window_layer, text_layer_get_layer(notify_text_layer));
    layer_set_hidden(text_layer_get_layer(notify_text_layer), true);
    
    set_image_for_goal();
    reset_current_date_and_volume_if_needed();
}

static void window_unload(Window *window) {
    text_layer_destroy(text_layer);
    text_layer_destroy(streak_text_layer);
    text_layer_destroy(white_layer);
    text_layer_destroy(notify_text_layer);
    bitmap_layer_destroy(gallon_layer);
    bitmap_layer_destroy(gallon_filled_layer);
    bitmap_layer_destroy(star_layer);
    action_bar_layer_destroy(action_bar);
}

static void CDU_window_load(Window *window) {
    CDU_action_bar = action_bar_layer_create();
    action_bar_layer_add_to_window(CDU_action_bar, custom_drink_unit_window);
    action_bar_layer_set_click_config_provider(CDU_action_bar, CDU_click_config_provider);
    action_bar_layer_set_icon(CDU_action_bar, BUTTON_ID_UP, action_icon_plus);
    action_bar_layer_set_icon(CDU_action_bar, BUTTON_ID_SELECT, action_icon_check);
    action_bar_layer_set_icon(CDU_action_bar, BUTTON_ID_DOWN, action_icon_minus);

    Layer *window_layer = window_get_root_layer(window);
    
    CDU_header_text_layer = text_layer_create(GRect(4, 0, width, 60));
    text_layer_set_font(CDU_header_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_24_BOLD));
    text_layer_set_background_color(CDU_header_text_layer, GColorClear);
    text_layer_set_text(CDU_header_text_layer, "Set Custom Drinking Unit");
    layer_add_child(window_layer, text_layer_get_layer(CDU_header_text_layer));
    
    CDU_text_layer = text_layer_create(GRect(4, 60, width, 60));
    text_layer_set_font(CDU_text_layer, fonts_get_system_font(FONT_KEY_GOTHIC_28_BOLD));
    text_layer_set_background_color(CDU_text_layer, GColorClear);
    layer_add_child(window_layer, text_layer_get_layer(CDU_text_layer));

    temp_cdu_oz = cdu_oz;
    temp_cdu_ml = cdu_ml;
    CDU_update_display();
}

static void CDU_window_unload(Window *window) {
    action_bar_layer_destroy(CDU_action_bar);
    text_layer_destroy(CDU_header_text_layer);
    text_layer_destroy(CDU_text_layer);
}

static void init(void) {
    load_persistent_storage();
    
    action_icon_plus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_PLUS);
    action_icon_settings = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_SETTINGS);
    action_icon_minus = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_MINUS);
    action_icon_check = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_ACTION_ICON_CHECK);
    star = gbitmap_create_with_resource(RESOURCE_ID_IMAGE_STAR);

    window = window_create();
    window_set_click_config_provider(window, click_config_provider);
    window_set_window_handlers(window, (WindowHandlers) {
        .load = window_load,
        .unload = window_unload,
    });

    custom_drink_unit_window = window_create();
    window_set_click_config_provider(custom_drink_unit_window, CDU_click_config_provider);
    window_set_window_handlers(custom_drink_unit_window, (WindowHandlers) {
        .load = CDU_window_load,
        .unload = CDU_window_unload,
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
    
    unit_system_menu_window = window_create();
    window_set_window_handlers(unit_system_menu_window, (WindowHandlers) {
        .load = unit_system_menu_window_load,
        .unload = unit_system_menu_window_unload,
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
    
    sod_menu_window = window_create();
    window_set_window_handlers(sod_menu_window, (WindowHandlers) {
        .load = sod_menu_window_load,
        .unload = sod_menu_window_unload,
    });

    eod_menu_window = window_create();
    window_set_window_handlers(eod_menu_window, (WindowHandlers) {
        .load = eod_menu_window_load,
        .unload = eod_menu_window_unload,
    });
    
    reminder_menu_window = window_create();
    window_set_window_handlers(reminder_menu_window, (WindowHandlers) {
        .load = reminder_menu_window_load,
        .unload = reminder_menu_window_unload,
    });
    
    window_stack_push(window, true);
    
    tick_timer_service_subscribe(HOUR_UNIT, handle_hour_tick);
    wakeup_service_subscribe(wakeup_handler);
    schedule_reminder_if_needed();
    schedule_reset_if_needed();
}

static void deinit(void) {
    save_persistent_storage();
    
    gbitmap_destroy(action_icon_plus);
    gbitmap_destroy(action_icon_settings);
    gbitmap_destroy(action_icon_minus);
    gbitmap_destroy(action_icon_check);
    gbitmap_destroy(gallon_filled_image);
    gbitmap_destroy(gallon_image);
    gbitmap_destroy(star);
    
    window_destroy(window);
    window_destroy(settings_menu_window);
    window_destroy(profile_menu_window);
    window_destroy(unit_system_menu_window);
    window_destroy(goal_menu_window);
    window_destroy(unit_menu_window);
    window_destroy(sod_menu_window);
    window_destroy(eod_menu_window);
    window_destroy(reminder_menu_window);
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
            return 6;
            
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
                    menu_cell_basic_draw(ctx, cell_layer, "Unit System", unit_system_to_string(unit_system), NULL);
                    break;
                case 1:
                    menu_cell_basic_draw(ctx, cell_layer, "Daily Goal", unit_to_string(goal), NULL);
                    break;
                case 2:
                    menu_cell_basic_draw(ctx, cell_layer, "Drinking Unit", (unit == CUSTOM) ? custom_unit_to_string() : unit_to_string(unit), NULL);
                    break;
                case 3:
                    menu_cell_basic_draw(ctx, cell_layer, "Start of Day", hour_to_string(start_of_day), NULL);
                    break;
                case 4:
                    menu_cell_basic_draw(ctx, cell_layer, "End of Day", hour_to_string(end_of_day), NULL);
                    break;
                case 5:
                    menu_cell_basic_draw(ctx, cell_layer, "Drink Reminders", reminder_to_string(inactivity_reminder_hours), NULL);
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
                    unit_system_menu_show();
                    break;
                case 1:
                    goal_menu_show();
                    break;
                case 2:
                    unit_menu_show();
                    break;
                case 3:
                    sod_menu_show();
                    break;
                case 4:
                    eod_menu_show();
                    break;
                case 5:
                    reminder_menu_show();
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
    uint16_t streak;
    // Determine which section we're going to draw in
    switch (cell_index->section) {
        case 0:
            // Use the row to specify which item we'll draw
            switch (cell_index->row) {
                case 0:
                    if (unit_system == CUSTOMARY) {
                        if (total_consumed == OZ_IN_GAL) {
                            snprintf(buffer, sizeof(buffer), "1.0 Gallon");
                        } else {
                            snprintf(buffer, sizeof(buffer), "%u.%01u Gallons", (int)(double)total_consumed/OZ_IN_GAL, (int)((double)total_consumed/OZ_IN_GAL*10)%10);
                        }
                    } else {
                        int total_consumed_ml = total_consumed*EXACT_ML_IN_OZ;
                        if (total_consumed_ml == ML_IN_L) {
                            snprintf(buffer, sizeof(buffer), "1.0 Liter");
                        } else {
                            snprintf(buffer, sizeof(buffer), "%u.%01u Liters", (int)(double)total_consumed_ml/ML_IN_L, (int)((double)total_consumed_ml/ML_IN_L*10)%10);
                        }
                    }
                    menu_cell_basic_draw(ctx, cell_layer, "Total Consumed", buffer, NULL);
                    break;
                case 1:
                    streak = (streak_count > longest_streak) ? streak_count : longest_streak;
                    if (streak == 1) {
                        snprintf(buffer, sizeof(buffer), "1 Day");
                    } else {
                        snprintf(buffer, sizeof(buffer), "%u Days", streak);
                    }
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



// Unit system menu stuff
static void unit_system_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Change Unit System");
}

static uint16_t unit_system_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t unit_system_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return 2;
}

static int16_t unit_system_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void unit_system_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    menu_cell_basic_draw(ctx, cell_layer, unit_system_to_string(cell_index->row), NULL, NULL);
}

static void unit_system_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    unit_system = cell_index->row;
    update_streak_count();
    update_volume_display();
    reset_reminder();
    window_stack_pop(true);
}

static void unit_system_menu_show() {
    window_stack_push(unit_system_menu_window, true);
    
    // Sets the selected unit system in the menu
    menu_layer_set_selected_index(unit_system_menu_layer, (MenuIndex) { .row = unit_system, .section = 0 }, MenuRowAlignCenter, false);
}

static void unit_system_menu_window_load(Window *window) {
    Layer *menu_window_layer = window_get_root_layer(window);
    GRect menu_bounds = layer_get_bounds(menu_window_layer);
    
    // Create the menu layer
    unit_system_menu_layer = menu_layer_create(menu_bounds);
    
    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(unit_system_menu_layer, window);
    
    // Setup callbacks
    menu_layer_set_callbacks(unit_system_menu_layer, NULL, (MenuLayerCallbacks){
        .get_header_height = unit_system_menu_get_header_height_callback,
        .draw_header = unit_system_menu_draw_header_callback,
        .get_num_sections = unit_system_menu_get_num_sections_callback,
        .get_num_rows = unit_system_menu_get_num_rows_callback,
        .draw_row = unit_system_menu_draw_row_callback,
        .select_click = unit_system_menu_select_callback,
    });
    
    // Add it to the window for display
    layer_add_child(menu_window_layer, menu_layer_get_layer(unit_system_menu_layer));
}

static void unit_system_menu_window_unload(Window *window) {
    menu_layer_destroy(unit_system_menu_layer);
}
// End unit system menu stuff



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
    if (cdu_oz > OZ_IN_GAL * get_goal_scale()) {
        cdu_oz = OZ_IN_GAL * get_goal_scale();
    }
    if (cdu_ml > ML_IN_GAL * get_goal_scale()) {
        cdu_ml = ML_IN_GAL * get_goal_scale();
    }
    set_image_for_goal();
    update_streak_count();
    update_volume_display();
    reset_reminder();
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
    return 5;
}

static int16_t unit_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void unit_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Use the row to specify which item we'll draw
    if (cell_index->row != 4) {
        menu_cell_basic_draw(ctx, cell_layer, unit_to_string(cell_index->row), NULL, NULL);
    } else {
        menu_cell_basic_draw(ctx, cell_layer, "Custom", custom_unit_to_string(), NULL);
    }
}

static void unit_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    if (cell_index->row != 4) {
        unit = cell_index->row;
        update_volume_display();
        reset_reminder();
        window_stack_pop(true);
    } else {
        window_stack_push(custom_drink_unit_window, true);
    }
}

static void unit_menu_show() {
    window_stack_push(unit_menu_window, true);
    
    // Sets the selected unit in the menu
    uint16_t row = (unit == CUSTOM) ? 4 : unit;
    menu_layer_set_selected_index(unit_menu_layer, (MenuIndex) { .row = row, .section = 0 }, MenuRowAlignCenter, false);
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



// Start of day menu stuff
static void sod_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Change Start of Day");
}

static uint16_t sod_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t sod_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return 24;
}

static int16_t sod_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void sod_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Use the row to specify which item we'll draw
    menu_cell_basic_draw(ctx, cell_layer, hour_to_string(cell_index->row), NULL, NULL);
}

static void sod_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    start_of_day = cell_index->row;
    reset_reminder();
    window_stack_pop(true);
}

static void sod_menu_show() {
    window_stack_push(sod_menu_window, true);
    
    // Sets the selected unit in the menu
    menu_layer_set_selected_index(sod_menu_layer, (MenuIndex) { .row = start_of_day, .section = 0 }, MenuRowAlignCenter, false);
}

static void sod_menu_window_load(Window *window) {
    Layer *menu_window_layer = window_get_root_layer(window);
    GRect menu_bounds = layer_get_bounds(menu_window_layer);
    
    // Create the menu layer
    sod_menu_layer = menu_layer_create(menu_bounds);
    
    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(sod_menu_layer, window);
    
    // Setup callbacks
    menu_layer_set_callbacks(sod_menu_layer, NULL, (MenuLayerCallbacks){
        .get_header_height = sod_menu_get_header_height_callback,
        .draw_header = sod_menu_draw_header_callback,
        .get_num_sections = sod_menu_get_num_sections_callback,
        .get_num_rows = sod_menu_get_num_rows_callback,
        .draw_row = sod_menu_draw_row_callback,
        .select_click = sod_menu_select_callback,
    });
    
    // Add it to the window for display
    layer_add_child(menu_window_layer, menu_layer_get_layer(sod_menu_layer));
}

static void sod_menu_window_unload(Window *window) {
    menu_layer_destroy(sod_menu_layer);
}
// End end of day menu stuff



// End of day menu stuff
static void eod_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Change End of Day");
}

static uint16_t eod_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t eod_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return 24;
}

static int16_t eod_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    // This is a define provided in pebble.h that you may use for the default height
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void eod_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    // Use the row to specify which item we'll draw
    menu_cell_basic_draw(ctx, cell_layer, hour_to_string(cell_index->row), NULL, NULL);
}

static void eod_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    uint16_t old_end_of_day = end_of_day;
    end_of_day = cell_index->row;
    uint32_t time_diff = (end_of_day - old_end_of_day) * SEC_IN_HOUR;
    current_date -= time_diff;
    last_streak_date -= time_diff;
    reset_current_date_and_volume_if_needed();
    reset_reminder();
    window_stack_pop(true);
}

static void eod_menu_show() {
    window_stack_push(eod_menu_window, true);
    
    // Sets the selected unit in the menu
    menu_layer_set_selected_index(eod_menu_layer, (MenuIndex) { .row = end_of_day, .section = 0 }, MenuRowAlignCenter, false);
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



// Reminder menu stuff
static void reminder_menu_draw_header_callback(GContext* ctx, const Layer *cell_layer, uint16_t section_index, void *data) {
    menu_cell_basic_header_draw(ctx, cell_layer, "Set Drink Reminders");
}

static uint16_t reminder_menu_get_num_sections_callback(MenuLayer *menu_layer, void *data) {
    return 1;
}

static uint16_t reminder_menu_get_num_rows_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return 9;
}

static int16_t reminder_menu_get_header_height_callback(MenuLayer *menu_layer, uint16_t section_index, void *data) {
    return MENU_CELL_BASIC_HEADER_HEIGHT;
}

static void reminder_menu_draw_row_callback(GContext* ctx, const Layer *cell_layer, MenuIndex *cell_index, void *data) {
    menu_cell_basic_draw(ctx, cell_layer, reminder_to_string(cell_index->row), NULL, NULL);
}

static void reminder_menu_select_callback(MenuLayer *menu_layer, MenuIndex *cell_index, void *data) {
    inactivity_reminder_hours = cell_index->row;

    reset_reminder();

    window_stack_pop(true);
}

static void reminder_menu_show() {
    window_stack_push(reminder_menu_window, true);
    
    // Sets the selected hour in the menu
    menu_layer_set_selected_index(reminder_menu_layer, (MenuIndex) { .row = inactivity_reminder_hours, .section = 0 }, MenuRowAlignCenter, false);
}

static void reminder_menu_window_load(Window *window) {
    Layer *menu_window_layer = window_get_root_layer(window);
    GRect menu_bounds = layer_get_bounds(menu_window_layer);
    
    // Create the menu layer
    reminder_menu_layer = menu_layer_create(menu_bounds);
    
    // Bind the menu layer's click config provider to the window for interactivity
    menu_layer_set_click_config_onto_window(reminder_menu_layer, window);
    
    // Setup callbacks
    menu_layer_set_callbacks(reminder_menu_layer, NULL, (MenuLayerCallbacks){
        .get_header_height = reminder_menu_get_header_height_callback,
        .draw_header = reminder_menu_draw_header_callback,
        .get_num_sections = reminder_menu_get_num_sections_callback,
        .get_num_rows = reminder_menu_get_num_rows_callback,
        .draw_row = reminder_menu_draw_row_callback,
        .select_click = reminder_menu_select_callback,
    });
    
    // Add it to the window for display
    layer_add_child(menu_window_layer, menu_layer_get_layer(reminder_menu_layer));
}

static void reminder_menu_window_unload(Window *window) {
    menu_layer_destroy(reminder_menu_layer);
}
// End reminder menu stuff
