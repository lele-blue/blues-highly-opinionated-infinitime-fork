#pragma once

#include <lvgl/src/lv_core/lv_obj.h>
#include <chrono>
#include <cstdint>
#include <memory>
#include <displayapp/Controllers.h>
#include "displayapp/screens/Screen.h"
#include "components/datetime/DateTimeController.h"
#include "components/ble/SimpleWeatherService.h"
#include "utility/DirtyValue.h"

namespace Pinetime {
  namespace Controllers {
    class Settings;
    class Battery;
    class Ble;
    class NotificationManager;
    class HeartRateController;
    class MotionController;
  }

  namespace Applications {
    namespace Screens {

      class WatchFaceMinimal : public Screen {
      public:
        WatchFaceMinimal(Controllers::DateTime& dateTimeController,
                          const Controllers::Battery& batteryController,
                          const Controllers::Ble& bleController,
                          Controllers::NotificationManager& notificationManager,
                          Controllers::Settings& settingsController,
                          Controllers::HeartRateController& heartRateController,
                          Controllers::MotionController& motionController,
                          Controllers::SimpleWeatherService& weatherService,
                          Controllers::AlarmController& alarmController,
                          Controllers::MusicService& musicService,
                          Controllers::Timer& timer);
        ~WatchFaceMinimal() override;

        void Refresh() override;

      private:
        Utility::DirtyValue<int> batteryPercentRemaining {};
        Utility::DirtyValue<float> connectionLevel {};
        Utility::DirtyValue<bool> powerPresent {};
        Utility::DirtyValue<bool> bleState {};
        Utility::DirtyValue<bool> bleRadioEnabled {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::seconds>> currentDateTime {};
        Utility::DirtyValue<uint32_t> stepCount {};
        Utility::DirtyValue<uint8_t> heartbeat {};
        Utility::DirtyValue<bool> heartbeatRunning {};
        Utility::DirtyValue<bool> timerRunning {};
        Utility::DirtyValue<bool> notificationState {};
        Utility::DirtyValue<bool> alarmState {};
        Utility::DirtyValue<bool> playingState {};
        Utility::DirtyValue<std::string> playingTrack {};
        Utility::DirtyValue<std::chrono::time_point<std::chrono::system_clock, std::chrono::days>> currentDate;
        Utility::DirtyValue<std::optional<Controllers::SimpleWeatherService::CurrentWeather>> currentWeather {};

        lv_obj_t* label_time;
        lv_obj_t* timerLabel;
        lv_obj_t* time_bg;
        lv_obj_t* timer_bg;
        lv_obj_t* label_date;
        lv_obj_t* seconds;
        lv_obj_t* weather;
        lv_obj_t* weatherLocation;
        lv_obj_t* label_prompt_1;
        lv_obj_t* label_bottom_music;
        lv_obj_t* batteryValue;
        lv_obj_t* heartbeatValue;
        lv_obj_t* connLevelValue;
        lv_obj_t* stepValue;
        lv_obj_t* notificationIcon;
        lv_obj_t* alarmStateLabel;

        Controllers::DateTime& dateTimeController;
        const Controllers::Battery& batteryController;
        const Controllers::Ble& bleController;
        Controllers::NotificationManager& notificationManager;
        Controllers::Settings& settingsController;
        Controllers::HeartRateController& heartRateController;
        Controllers::MotionController& motionController;
        Controllers::SimpleWeatherService& weatherService;
        Controllers::AlarmController& alarmController;
        Controllers::MusicService& musicService;
        Controllers::Timer& timer;

        lv_task_t* taskRefresh;
      };
    }

    template <>
    struct WatchFaceTraits<WatchFace::Minimal> {
      static constexpr WatchFace watchFace = WatchFace::Minimal;
      static constexpr const char* name = "Minimal";

      static Screens::Screen* Create(AppControllers& controllers) {
        return new Screens::WatchFaceMinimal(controllers.dateTimeController,
                                              controllers.batteryController,
                                              controllers.bleController,
                                              controllers.notificationManager,
                                              controllers.settingsController,
                                              controllers.heartRateController,
                                              controllers.motionController,
                                              *controllers.weatherController,
                                              controllers.alarmController,
                                              *controllers.musicService,
                                              controllers.timer);
      };

      static bool IsAvailable(Pinetime::Controllers::FS& /*filesystem*/) {
        return true;
      }
    };
  }
}
