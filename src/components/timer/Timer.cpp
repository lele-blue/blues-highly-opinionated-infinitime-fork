#include "components/timer/Timer.h"
#include <libraries/log/nrf_log.h>
#include <lvgl/lvgl.h>

using namespace Pinetime::Controllers;

Timer::Timer(void* const timerData, TimerCallbackFunction_t timerCallbackFunction, Controllers::MotorController *motorController):

    motorController {motorController}

{
  timer = xTimerCreate("Timer", 1, pdFALSE, timerData, timerCallbackFunction);
}

void Timer::StartTimer(std::chrono::milliseconds duration) {
  totalTime = duration;
  xTimerChangePeriod(timer, pdMS_TO_TICKS(duration.count()), 0);
  xTimerStart(timer, 0);
  expiry = xTimerGetExpiryTime(timer);
  triggered = true;
}

// nullopt if timer stopped (StopTimer called / StartTimer not yet called)
// otherwise TimerStatus with the ticks until/since expiry (depending on state of expired flag)
std::optional<Timer::TimerStatus> Timer::GetTimerState() {
  if (IsRunning()) {
    TickType_t remainingTime = expiry - xTaskGetTickCount();
    return std::make_optional<Timer::TimerStatus>(
      {.distanceToExpiry = std::chrono::seconds(remainingTime / configTICK_RATE_HZ) +
                           std::chrono::milliseconds((remainingTime % configTICK_RATE_HZ) * 1000 / configTICK_RATE_HZ),
       .expired = false});
  }
  if (triggered) {
    TickType_t timeSinceExpiry = xTaskGetTickCount() - expiry;
    return std::make_optional<Timer::TimerStatus>(
      {.distanceToExpiry = std::chrono::seconds(timeSinceExpiry / configTICK_RATE_HZ) +
                           std::chrono::milliseconds((timeSinceExpiry % configTICK_RATE_HZ) * 1000 / configTICK_RATE_HZ),
       .expired = true});
  }
  return std::nullopt;
}

float Timer::GetFractionRemaining() {
  return static_cast<float>(this->GetTimeRemaining().count()) / static_cast<float>(totalTime.count());
}

void Timer::StopTimer() {
  xTimerStop(timer, 0);
  triggered = false;
}


static void timerCallback(lv_task_t* task) {
    auto *self = static_cast<Pinetime::Controllers::Timer*>(task->user_data);
    self->Ring();
}

void Timer::StartRinging() {
  auto *task = lv_task_create(timerCallback, pdMS_TO_TICKS(300), LV_TASK_PRIO_MID, this);
  lv_task_set_repeat_count(task, 5);
}

void Timer::Ring() {
  motorController->RunForDuration(150);
}

bool Timer::IsRunning() {
  return (xTimerIsTimerActive(timer) == pdTRUE);
}

void Timer::ResetExpiredTime() {
  triggered = false;
}
