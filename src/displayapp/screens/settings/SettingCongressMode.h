#pragma once

#include <array>
#include <cstdint>
#include <lvgl/lvgl.h>

#include "components/datetime/DateTimeController.h"
#include "components/settings/Settings.h"
#include "displayapp/screens/Screen.h"
#include "displayapp/widgets/Counter.h"

namespace Pinetime {

  namespace Applications {
    namespace Screens {

      class SettingCongressMode : public Screen {
      public:
        SettingCongressMode(Pinetime::Controllers::Settings& settingsController, Pinetime::Controllers::DateTime& dateTimeController);
        ~SettingCongressMode() override;

        void HandleChange();
        void HandleChange(void *userData);
        void CheckDay();

      private:
        Controllers::DateTime& dateTimeController;
        Controllers::Settings& settingsController;

        Widgets::Counter currentDayCounter = Widgets::Counter(-256, 256, jetbrains_mono_bold_20);
        Widgets::Counter lengthCounter = Widgets::Counter(1, 30, jetbrains_mono_bold_20);
        lv_obj_t *enableSwitch;
        lv_obj_t *title;
        lv_obj_t *icon;
        lv_obj_t *textDay1;
        lv_obj_t *textEnable;
    };
  }
}
}
