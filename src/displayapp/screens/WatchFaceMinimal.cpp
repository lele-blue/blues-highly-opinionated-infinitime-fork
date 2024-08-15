#include <lvgl/lvgl.h>
#include "displayapp/screens/WatchFaceMinimal.h"
#include "displayapp/screens/BatteryIcon.h"
#include "displayapp/screens/NotificationIcon.h"
#include "displayapp/screens/Symbols.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/motion/MotionController.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/screens/WeatherSymbols.h"
#include "components/ble/SimpleWeatherService.h"

using namespace Pinetime::Applications::Screens;

namespace {
  lv_color_t temperatureColor(int16_t temperature) {
    if (temperature <= 0) { // freezing
      return Colors::blue;
    } else if (temperature <= 400) { // ice
      return LV_COLOR_CYAN;
    } else if (temperature >= 2700) { // hot
      return Colors::deepOrange;
    }
    return Colors::orange; // normal
  }
}

WatchFaceMinimal::WatchFaceMinimal(Controllers::DateTime& dateTimeController,
                                     const Controllers::Battery& batteryController,
                                     const Controllers::Ble& bleController,
                                     Controllers::NotificationManager& notificationManager,
                                     Controllers::Settings& settingsController,
                                     Controllers::HeartRateController& heartRateController,
                                     Controllers::MotionController& motionController,
                                     Controllers::SimpleWeatherService& weatherService)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService} {
  batteryValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(batteryValue, true);
  lv_obj_align(batteryValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -00);

  seconds = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(seconds, true);
  lv_obj_align(seconds, lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -30, 00);

  connectState = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(connectState, true);
  lv_obj_align(connectState, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 60);

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 5, -0);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_date, true);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -20);

  label_prompt_1 = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_prompt_1, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -80);
  lv_label_set_text_static(label_prompt_1, "");

  label_prompt_2 = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_prompt_2, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 100);
  lv_label_set_text_static(label_prompt_2, "<3");

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_time, true);
  lv_obj_set_style_local_text_font(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, &jetbrains_mono_76);
  lv_obj_align(label_time, lv_scr_act(), LV_ALIGN_CENTER, -115, -65);

  heartbeatValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(heartbeatValue, true);
  lv_obj_align(heartbeatValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 40);

  stepValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(stepValue, true);
  lv_obj_align(stepValue, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 20);

  weather = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(weather, true);
  lv_obj_align(weather, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 80);

  taskRefresh = lv_task_create(RefreshTaskCallback, LV_DISP_DEF_REFR_PERIOD, LV_TASK_PRIO_MID, this);
  Refresh();
}

WatchFaceMinimal::~WatchFaceMinimal() {
  lv_task_del(taskRefresh);
  lv_obj_clean(lv_scr_act());
}

void WatchFaceMinimal::Refresh() {
  powerPresent = batteryController.IsPowerPresent();
  batteryPercentRemaining = batteryController.PercentRemaining();
  if (batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated()) {
    lv_label_set_text_fmt(batteryValue, "#387b54 %d%%", batteryPercentRemaining.Get());
    if (batteryController.IsPowerPresent()) {
      lv_label_ins_text(batteryValue, LV_LABEL_POS_LAST, " C");
    }
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated()) {
    if (!bleRadioEnabled.Get()) {
      lv_label_set_text_static(connectState, "#f082fc DISABLED#");
    } else {
      if (bleState.Get()) {
        lv_label_set_text_static(connectState, "#0082fc CON#");
      } else {
        lv_label_set_text_static(connectState, "#f082fc DIS#");
      }
    }
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (notificationState.IsUpdated()) {
    if (notificationState.Get()) {
      lv_label_set_text_static(notificationIcon, "+NEW+");
    } else {
      lv_label_set_text_static(notificationIcon, "-");
    }
  }

  currentWeather = weatherService.Current();
  if (currentWeather.IsUpdated()) {
    auto optCurrentWeather = currentWeather.Get();
    if (optCurrentWeather) {
      int16_t temp = optCurrentWeather->temperature;
      char tempUnit = 'C';
      lv_obj_set_style_local_text_color(weather, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, temperatureColor(temp));
      if (settingsController.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial) {
        temp = Controllers::SimpleWeatherService::CelsiusToFahrenheit(temp);
        tempUnit = 'F';
      }
      lv_label_set_text_fmt(weather, "%i°%c %s ", temp / 100, tempUnit, Symbols::GetSimpleCondition(optCurrentWeather->iconId));
    } else {
      lv_label_set_text(weather, "#ffffff No Weather");
      lv_obj_set_style_local_text_color(weather, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
    }
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  if (currentDateTime.IsUpdated()) {
    uint8_t hour = dateTimeController.Hours();
    uint8_t minute = dateTimeController.Minutes();
    uint8_t second = dateTimeController.Seconds();

    if (settingsController.GetClockType() == Controllers::Settings::ClockType::H12) {
      char ampmChar[3] = "AM";
      if (hour == 0) {
        hour = 12;
      } else if (hour == 12) {
        ampmChar[0] = 'P';
      } else if (hour > 12) {
        hour = hour - 12;
        ampmChar[0] = 'P';
      }
      lv_label_set_text_fmt(seconds, ":%02d %s", second, ampmChar);
      lv_label_set_text_fmt(label_time, "#11cc55 %02d:%02d:%02d %s#", hour, minute, second, ampmChar);
    } else {
      lv_label_set_text_fmt(seconds, ":%02d %s", second);
      lv_label_set_text_fmt(label_time, "#11cc55 %02d:%02d:%02d", hour, minute, second);
    }

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      uint16_t year = dateTimeController.Year();
      Controllers::DateTime::Months month = dateTimeController.Month();
      uint8_t day = dateTimeController.Day();
      lv_label_set_text_fmt(label_date, "#007fff %04d-%02d-%02d#", short(year), char(month), char(day));
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "HR#ee3311 %d bpm#", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "HR#ee3311 ---#");
    }
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "#ee3377 %lu steps#", stepCount.Get());
  }
}
