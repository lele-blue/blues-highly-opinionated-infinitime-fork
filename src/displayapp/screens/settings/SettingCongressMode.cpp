#include <lvgl/lvgl.h>
#include <nrf_log.h>
#include <chrono>
#include <ctime>
#include "displayapp/DisplayApp.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/screens/Styles.h"

// hack eeeeehhhh should not need this
#include "displayapp/screens/settings/SettingCongressMode.h"
// #include "displayapp/screens/Screen.h"
#include "displayapp/screens/Symbols.h"

using namespace Pinetime::Applications::Screens;
using namespace std::chrono_literals;


namespace {
  constexpr int16_t POS_X_DAY = -72;
  constexpr int16_t POS_X_MONTH = 0;
  constexpr int16_t POS_X_YEAR = 72;
  constexpr int16_t POS_Y_TEXT = -6;

  void ValueChangedHandler([[maybe_unused]] void* userData) {
    auto* scm = static_cast<SettingCongressMode*>(userData);
    scm->HandleChange();
  }
  void event_handler([[maybe_unused]]lv_obj_t* obj, [[maybe_unused]]lv_event_t event) {
    auto* scm = static_cast<SettingCongressMode*>(obj->user_data);
    scm->HandleChange();
  }
  std::chrono::system_clock::time_point make_date_only(std::chrono::system_clock::time_point tp) {
    // Convert time_point to time_t
    std::time_t time = std::chrono::system_clock::to_time_t(tp);

    // Convert time_t to tm structure
    std::tm* tm = std::localtime(&time);

    tm->tm_hour = 6;
    tm->tm_min = 0;  // Reset minutes
    tm->tm_sec = 0;  // Reset seconds

    // Convert back to time_t
    std::time_t new_time = std::mktime(tm);

    // Convert back to time_point
    return std::chrono::system_clock::from_time_t(new_time);
  }
}

SettingCongressMode::SettingCongressMode(Pinetime::Controllers::Settings& settingsController, Pinetime::Controllers::DateTime& dateTimeController)
  : dateTimeController {dateTimeController}, settingsController{settingsController} {
  static constexpr lv_color_t bgColor = Colors::bgAlt;

  lv_obj_t* textEnable = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(textEnable, "Enable");
  lv_label_set_align(textEnable, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(textEnable, lv_scr_act(), LV_ALIGN_IN_BOTTOM_RIGHT, -30, -15);

  lv_obj_t* textDay1 = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(textDay1, "Day #\ntoday\n<-\n\nLength\n->");
  lv_label_set_align(textDay1, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(textDay1, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 0, 55);

  lv_obj_t* title = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_text_static(title, "Your Congress");
  lv_label_set_align(title, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(title, lv_scr_act(), LV_ALIGN_IN_TOP_MID, 15, 15);

  lv_obj_t* icon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, LV_COLOR_ORANGE);

  lv_label_set_text_static(icon, Symbols::ccc);
  lv_obj_set_style_local_text_font(icon, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &lv_font_sys_48);
  lv_label_set_align(icon, LV_LABEL_ALIGN_CENTER);
  lv_obj_align(icon, title, LV_ALIGN_OUT_LEFT_MID, -10, 0);

  currentDayCounter.SetValueChangedEventCallback(this, ValueChangedHandler);
  currentDayCounter.Create();

  auto currentSettings = settingsController.GetCongressMode();

  auto days_diff =std::chrono::duration_cast<std::chrono::days>(dateTimeController.CurrentDateTime() - currentSettings.day_0).count();
  // if (days_diff < 0) days_diff--;
  currentDayCounter.SetValue(days_diff);
  lv_obj_align(currentDayCounter.GetObject(), nullptr, LV_ALIGN_CENTER, POS_X_DAY, POS_Y_TEXT);

  lengthCounter.SetValueChangedEventCallback(this, ValueChangedHandler);
  lengthCounter.Create();
  lengthCounter.SetValue(currentSettings.length);
  lv_obj_align(lengthCounter.GetObject(), nullptr, LV_ALIGN_CENTER, POS_X_YEAR, POS_Y_TEXT);

  enableSwitch = lv_switch_create(lv_scr_act(), nullptr);
  enableSwitch->user_data = this;
  lv_obj_set_event_cb(enableSwitch, event_handler);
  lv_obj_set_size(enableSwitch, 100, 50);
  // Align to the center of 115px from edge
  lv_obj_align(enableSwitch, lv_scr_act(), LV_ALIGN_IN_BOTTOM_LEFT, 7, 0);
  lv_obj_set_style_local_bg_color(enableSwitch, LV_SWITCH_PART_BG, LV_STATE_DEFAULT, bgColor);

  if (currentSettings.enabled) {
    lv_switch_on(enableSwitch, LV_ANIM_OFF);
  } else {
    lv_switch_off(enableSwitch, LV_ANIM_OFF);
  }
}

SettingCongressMode::~SettingCongressMode() {
  lv_obj_clean(lv_scr_act());
}

void SettingCongressMode::HandleChange() {
  const uint16_t length = lengthCounter.GetValue();
  const int8_t current = currentDayCounter.GetValue();

  auto start = make_date_only(dateTimeController.CurrentDateTime());

  
  start = start - (24h * current); 
  std::time_t now_c = std::chrono::system_clock::to_time_t(start);
  NRF_LOG_INFO(std::ctime(&now_c));
  this->settingsController.SetCongressMode(lv_switch_get_state(this->enableSwitch), start, length);
}


void SettingCongressMode::CheckDay() {
}
