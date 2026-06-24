#pragma once

#include "components/motor/MotorController.h"
#include <FreeRTOS.h>
#include <timers.h>

#include <chrono>
#include <optional>

namespace Pinetime {
  namespace Controllers {
    class Timer {
    public:
      Timer(void* timerData, TimerCallbackFunction_t timerCallbackFunction, Controllers::MotorController *motorController);
      struct TimerStatus {
        std::chrono::milliseconds distanceToExpiry;
        bool expired;
      };


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
      std::optional<TimerStatus> GetTimerState();

      void ResetExpiredTime();

      TickType_t expiry;
      bool triggered = false;
    };
  }
}
