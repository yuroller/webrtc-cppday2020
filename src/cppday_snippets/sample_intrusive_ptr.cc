#include "examples/cppday_snippets/sample_snippets.h"

#include <iostream>
#include <string>
#include <utility>

#include "api/scoped_refptr.h"
#include "rtc_base/ref_count.h"

namespace {
class IntrusiveNonAtomicSharedPtr : public rtc::RefCountInterface {
 protected:
  explicit IntrusiveNonAtomicSharedPtr(std::string data)
      : counter_(0), data_(std::move(data)) {
    std::cout << "Constructor\n";
  }

 public:
  static rtc::scoped_refptr<IntrusiveNonAtomicSharedPtr> Create(
      std::string data) {
    return new IntrusiveNonAtomicSharedPtr(std::move(data));
  }

  ~IntrusiveNonAtomicSharedPtr() { std::cout << "Destructor\n"; }

  void AddRef() const override {
    std::cout << "AddRef\n";
    ++counter_;
  }

  rtc::RefCountReleaseStatus Release() const override {
    std::cout << "Release\n";
    if (--counter_ == 0) {
      delete this;
      return rtc::RefCountReleaseStatus::kDroppedLastRef;
    }

    return rtc::RefCountReleaseStatus::kOtherRefsRemained;
  }

  void Print() const { std::cout << data_ << ": " << counter_ << '\n'; }

 private:
  mutable int counter_;
  std::string data_;
};
}  // namespace

static void PrintIsNull(const std::string& name, const void* ptr) {
  std::cout << name << " is " << (ptr == nullptr ? "" : "not") << "null\n";
}

void SampleIntrusivePtr() {
  rtc::scoped_refptr<IntrusiveNonAtomicSharedPtr> a =
      IntrusiveNonAtomicSharedPtr::Create("IntrusivePtr");

  a->Print();

  rtc::scoped_refptr<IntrusiveNonAtomicSharedPtr> b;
  b.swap(a);
  PrintIsNull("a0", a.get());
  PrintIsNull("b0", b.get());

  a = b;
  PrintIsNull("a1", a.get());
  PrintIsNull("b1", b.get());

  a->Print();
}