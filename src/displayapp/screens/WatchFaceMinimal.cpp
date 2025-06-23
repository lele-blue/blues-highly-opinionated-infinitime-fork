#include <lvgl/lvgl.h>
#include <libraries/log/nrf_log.h>
#include "displayapp/screens/WatchFaceMinimal.h"
#include "components/battery/BatteryController.h"
#include "components/ble/BleController.h"
#include "components/ble/NotificationManager.h"
#include "components/heartrate/HeartRateController.h"
#include "components/alarm/AlarmController.h"
#include "components/motion/MotionController.h"
#include "components/ble/MusicService.h"
#include "components/settings/Settings.h"
#include "displayapp/InfiniTimeTheme.h"
#include "displayapp/screens/WeatherSymbols.h"
#include <cstring>
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
                                     Controllers::SimpleWeatherService& weatherService,
                                     Controllers::AlarmController& alarmController,
                                     Controllers::MusicService& musicService)
  : currentDateTime {{}},
    dateTimeController {dateTimeController},
    batteryController {batteryController},
    bleController {bleController},
    notificationManager {notificationManager},
    settingsController {settingsController},
    heartRateController {heartRateController},
    motionController {motionController},
    weatherService {weatherService},
    alarmController {alarmController},
    musicService {musicService} {
  batteryValue = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(batteryValue, true);
  lv_obj_align(batteryValue, lv_scr_act(), LV_ALIGN_IN_RIGHT_MID, 10, -20);

  seconds = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(seconds, true);
  lv_obj_align(seconds, lv_scr_act(), LV_ALIGN_IN_TOP_RIGHT, -30, 00);

  alarmStateLabel = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(alarmStateLabel, true);
  lv_obj_align(alarmStateLabel, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 60);
  lv_label_set_text_static(alarmStateLabel, "#ffaaff NA#");

  notificationIcon = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(notificationIcon, true);
  lv_obj_align(notificationIcon, nullptr, LV_ALIGN_IN_TOP_LEFT, 5, -0);

  label_date = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(label_date, true);
  lv_obj_align(label_date, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -20);

  label_prompt_1 = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_prompt_1, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, -80);
  lv_label_set_text_static(label_prompt_1, "");

  time_bg = lv_obj_create(lv_scr_act(), nullptr);
	lv_obj_set_width(time_bg, 500);
	lv_obj_set_height(time_bg, 65);
  lv_obj_align(time_bg, lv_scr_act(), LV_ALIGN_CENTER, -115, -65);
  lv_obj_set_style_local_bg_color(time_bg, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000) );

  label_bottom_music = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_align(label_bottom_music, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 100);
  lv_label_set_text_static(label_bottom_music, MINIMAL_BOTTOM_LINE);
  lv_label_set_long_mode(label_bottom_music, LV_LABEL_LONG_CROP);
  lv_label_set_recolor(label_bottom_music, true);

  label_time = lv_label_create(lv_scr_act(), nullptr);
  lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x11cc55));
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

  weatherLocation = lv_label_create(lv_scr_act(), nullptr);
  lv_label_set_recolor(weatherLocation, true);
  lv_obj_align(weatherLocation, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, 0, 80);
  lv_label_set_long_mode(weatherLocation, LV_LABEL_LONG_SROLL_CIRC);
  lv_label_set_anim_speed(weatherLocation, 60);


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
  auto batteryStateUpdated = batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated();
  if (batteryStateUpdated) {
    int batteryPercentRemainingVal = batteryPercentRemaining.Get();
    lv_label_set_text_fmt(batteryValue, "#%s %d%%", batteryController.IsPowerPresent()? "feff00":"387b54",batteryPercentRemainingVal);
    lv_obj_align(batteryValue, lv_scr_act(), LV_ALIGN_IN_RIGHT_MID, 0, -20);
  }

  bleState = bleController.IsConnected();
  bleRadioEnabled = bleController.IsRadioEnabled();

  alarmState = alarmController.IsEnabled();

  bool isAlarmUpdated = alarmState.IsUpdated();
  uint16_t hoursToAlarm = 0;
  if (alarmState.Get()) {
    hoursToAlarm = alarmController.SecondsToAlarm()/60/60;
  }

  notificationState = notificationManager.AreNewNotificationsAvailable();
  if (bleState.IsUpdated() || bleRadioEnabled.IsUpdated() || notificationState.IsUpdated() || batteryPercentRemaining.IsUpdated() || powerPresent.IsUpdated() || batteryStateUpdated || isAlarmUpdated) {
    lv_label_set_text_fmt(notificationIcon, "");
    if (notificationState.Get()) {
      lv_label_ins_text(notificationIcon, LV_LABEL_POS_LAST, "N");
    }
    if (!bleRadioEnabled.Get()) {
      lv_label_ins_text(notificationIcon, LV_LABEL_POS_LAST, "#ff4444 B#");
    } else {
      if (!bleState.Get()) {
        lv_label_ins_text(notificationIcon, LV_LABEL_POS_LAST, "#0082fc N#");
      }
    }
    if (batteryController.IsPowerPresent()) {
      lv_label_ins_text(notificationIcon, LV_LABEL_POS_LAST, "#feff00 C#");
    } else if (batteryPercentRemaining.Get() < 15){
      lv_label_ins_text(notificationIcon, LV_LABEL_POS_LAST, "#387b54 V#");
    }

    if (alarmState.Get() && hoursToAlarm <= 5) {
      lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffffff));
      lv_label_ins_text(notificationIcon, LV_LABEL_POS_LAST, "#ff3355 S!#");
      lv_obj_set_style_local_bg_color(time_bg, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xff3355) );
    }
    else if (alarmState.Get() && hoursToAlarm <= 8) {
      lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0xffaaff));
      lv_label_ins_text(notificationIcon, LV_LABEL_POS_LAST, "#ffaaff S#");
      lv_obj_set_style_local_bg_color(time_bg, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000) );
    }
    else {
      lv_obj_set_style_local_text_color(label_time, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x11cc55));
      lv_obj_set_style_local_bg_color(time_bg, LV_OBJ_PART_MAIN, LV_STATE_DEFAULT, lv_color_hex(0x000000) );
    }

  }

  currentWeather = weatherService.Current();
  if (currentWeather.IsUpdated()) {
    auto optCurrentWeather = currentWeather.Get();
    if (optCurrentWeather) {
      auto temp_o = optCurrentWeather->temperature;
      int16_t temp = temp_o.PreciseCelsius();
      char tempUnit = 'C';
      lv_obj_set_style_local_text_color(weather, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, temperatureColor(temp));
      lv_obj_set_style_local_text_color(weatherLocation, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, temperatureColor(temp));
      if (settingsController.GetWeatherFormat() == Controllers::Settings::WeatherFormat::Imperial) {
        temp = temp_o.PreciseFahrenheit();
        tempUnit = 'F';
      }
      lv_label_set_text_fmt(weather, "%i°%c %s", temp / 100, tempUnit, Symbols::GetSimpleCondition(optCurrentWeather->iconId));
      lv_point_t text_size;
      char text_weather[34];
      if (lv_snprintf(&text_weather[0], 34, "%i°%c %s", temp/100, tempUnit, Symbols::GetSimpleCondition(optCurrentWeather->iconId)) < 0) {
        text_weather[0] = 'E';
        text_weather[1] = '\0';
      };
      _lv_txt_get_size(&text_size, text_weather, &jetbrains_mono_bold_20, 0, 0, LV_COORD_MAX, LV_TXT_FLAG_EXPAND);
      lv_label_set_text_fmt(weatherLocation, " %s", &(optCurrentWeather->location));
      signed short WIDTH = 240;
      lv_obj_set_width(weatherLocation,(unsigned int) (WIDTH - text_size.x));
      lv_obj_align(weatherLocation, lv_scr_act(), LV_ALIGN_IN_LEFT_MID, text_size.x, 80);

    } else {
      lv_label_set_text(weather, "#ffffff No Weather");
      lv_label_set_text(weatherLocation, "");
      lv_obj_set_style_local_text_color(weather, LV_LABEL_PART_MAIN, LV_STATE_DEFAULT, Colors::lightGray);
    }
  }

  currentDateTime = std::chrono::time_point_cast<std::chrono::seconds>(dateTimeController.CurrentDateTime());
  bool isDateTimeUpdated = currentDateTime.IsUpdated();
  if (isDateTimeUpdated) {
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
      lv_label_set_text_fmt(label_time, "%02d:%02d:%02d %s", hour, minute, second, ampmChar);
    } else {
      lv_label_set_text_fmt(seconds, ":%02d", second);
      lv_label_set_text_fmt(label_time, "%02d:%02d:%02d", hour, minute, second);
    }

    currentDate = std::chrono::time_point_cast<std::chrono::days>(currentDateTime.Get());
    if (currentDate.IsUpdated()) {
      lv_label_set_text_fmt(label_date, "#007fff %s#", dateTimeController.FormattedDate().c_str());
    }
  }

  if (isAlarmUpdated || isDateTimeUpdated) {
    if (alarmState.Get()) {
      lv_label_set_text_fmt(alarmStateLabel, "#%s A%ih#", hoursToAlarm <= 8 ? "ff4444" :"ffaaff", hoursToAlarm);
      if (hoursToAlarm <= 4) {
        lv_label_ins_text(alarmStateLabel, LV_LABEL_POS_LAST, " #ff2222 >:-( #");
      }
    } else {
      lv_label_set_text_static(alarmStateLabel, "#ffaaff NA#");
    }
  }

  heartbeat = heartRateController.HeartRate();
  heartbeatRunning = heartRateController.State() != Controllers::HeartRateController::States::Stopped;
  if (heartbeat.IsUpdated() || heartbeatRunning.IsUpdated()) {
    if (heartbeatRunning.Get()) {
      lv_label_set_text_fmt(heartbeatValue, "HR#ee3311  %d bpm#", heartbeat.Get());
    } else {
      lv_label_set_text_static(heartbeatValue, "HR#ee3311 ---#");
    }
  }

  stepCount = motionController.NbSteps();
  if (stepCount.IsUpdated()) {
    lv_label_set_text_fmt(stepValue, "#ee3377 %lu steps#", stepCount.Get());
  }

  playingState = musicService.isPlaying();
  if (playingState.Get()) {
    lv_label_set_text_fmt(label_bottom_music, "#00fffb %s#", musicService.getTrack().c_str());
  } else {
    lv_label_set_text_static(label_bottom_music, MINIMAL_BOTTOM_LINE);
  }
}
