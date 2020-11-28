#include "examples/cppday_snippets/sample_snippets.h"

#include <iostream>
#include <string>
#include <utility>
#include <cmath>

#include "api/scoped_refptr.h"
#include "rtc_base/ref_count.h"
#include "rtc_base/ref_counted_object.h"

// same technique as webrtc::CreateSessionDescriptionObserver
// for handling result of async operations

namespace {

  // reference counted interface for success/error handler
class OperationStatusObserver : public rtc::RefCountInterface {
 public:
  virtual void OnSuccess(double result) = 0;
  virtual void OnFailure(const std::string& error) = 0;

 protected:
  ~OperationStatusObserver() override = default;
};

// concrete implementation of success/error handler
class MyOpStatusObserver : public OperationStatusObserver {
 public:
  static OperationStatusObserver* Create() {
    return new rtc::RefCountedObject<MyOpStatusObserver>();
  }

  void OnSuccess(double result) override { std::cout << result << '\n'; }

  virtual void OnFailure(const std::string& error) override {
    std::cout << "Error: " << error << '\n';
  }
};

// worker class
class MySubject {
 public:
  explicit MySubject(OperationStatusObserver* observer) : observer_(observer) {}

  void Run(double n) {
    if (n < 0.0) {
      observer_->OnFailure("cannot get sqrt of a negative number");
    } else {
      observer_->OnSuccess(sqrt(n));
    }
  }

 private:
  rtc::scoped_refptr<OperationStatusObserver> observer_;
};
}  // namespace

void SampleObserver() {
  MySubject subj(MyOpStatusObserver::Create());
  subj.Run(2);
  subj.Run(-1);
}
