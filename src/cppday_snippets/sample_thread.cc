#include "examples/cppday_snippets/sample_snippets.h"

#include <functional>
#include <iostream>
#include <thread>

#include "rtc_base/event.h"
#include "rtc_base/message_handler.h"
#include "rtc_base/thread.h"

namespace {
class Signaler : public rtc::MessageHandler {
 public:
  explicit Signaler(rtc::Event* finished) : finished_(finished) {}

  void OnMessage(rtc::Message* pmsg) override {
    std::cout << "Signaler on thread " << std::this_thread::get_id()
              << '\n';
    std::cout << "from: " << pmsg->posted_from.ToString()
              << " id: " << pmsg->message_id << '\n';
    finished_->Set();
  }

 private:
  rtc::Event* finished_;
};
}  // namespace

void SampleThread() {
  rtc::Event finished(true, false);

  Signaler signaler(&finished);

  std::cout << "main on thread " << std::this_thread::get_id() << '\n';

  auto t1 = rtc::Thread::Create();
  t1->SetName("t1", nullptr);
  t1->Start();

  // using rtc::Thread as a message queue
  t1->Post(RTC_FROM_HERE, nullptr, 55);
  rtc::Message msg;
  if (t1->Get(&msg, 0)) {
    std::cout << "received " << msg.message_id << '\n';
  } else {
    std::cerr << "error: no message\n";
  }

  // executing tasks on a rtc::Thread
  // t1->Post(RTC_FROM_HERE, &signaler);
  t1->PostDelayed(RTC_FROM_HERE, 1000, &signaler, 783);  // delay 1s, id=783
  int ret = t1->Invoke<int>(RTC_FROM_HERE, []() {        // this is sync
    std::cout << "invoked synchronously on thread "
              << std::this_thread::get_id() << '\n';
    return 42;
  });
  std::cout << "returned " << ret << '\n';

  finished.Wait(rtc::Event::kForever);
}
