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
}

std::chrono::milliseconds Timer::GetTimeRemaining() {
  if (IsRunning()) {
    TickType_t remainingTime = xTimerGetExpiryTime(timer) - xTaskGetTickCount();
    return std::chrono::milliseconds(remainingTime * 1000 / configTICK_RATE_HZ);
  }
  return std::chrono::milliseconds(0);
}

float Timer::GetFractionRemaining() {
  return static_cast<float>(this->GetTimeRemaining().count()) / static_cast<float>(totalTime.count());
}

void Timer::StopTimer() {
  xTimerStop(timer, 0);
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
