#pragma once

#include "components/motor/MotorController.h"
#include <FreeRTOS.h>
#include <timers.h>

#include <chrono>

namespace Pinetime {
  namespace Controllers {
    class Timer {
    public:
      Timer(void* timerData, TimerCallbackFunction_t timerCallbackFunction, Controllers::MotorController *motorController);

      void StartTimer(std::chrono::milliseconds duration);

      void StopTimer();

      std::chrono::milliseconds GetTimeRemaining();
      float GetFractionRemaining();

      bool IsRunning();

      void StartRinging();
      void Ring();

    private:
      TimerHandle_t timer;
      std::chrono::milliseconds totalTime;
      MotorController *motorController;
    };
  }
}
