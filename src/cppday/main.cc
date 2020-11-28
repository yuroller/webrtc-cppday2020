#include "examples/cppday/client.h"
#include "examples/cppday/fake_audio_capture_module.h"

#include <iostream>
#include <string>
#include <utility>

#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/create_peerconnection_factory.h"
#include "api/scoped_refptr.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"
#include "rtc_base/thread.h"

namespace {
class MyClientObserver : public cppday::ClientObserver {
 public:
  MyClientObserver(rtc::scoped_refptr<cppday::Client> caller,
                   rtc::scoped_refptr<cppday::Client> receiver)
      : caller_(std::move(caller)), receiver_(std::move(receiver)) {
    RTC_DCHECK(caller_);
    RTC_DCHECK(receiver_);
  }

 protected:
  void OnLocalSdpAvailable(int from_client_id,
                           const std::string& sdp,
                           webrtc::SdpType type) override {
    std::cout << "Sdp from " << from_client_id << '\n' << sdp << '\n';
    if (from_client_id == caller_->id() && type == webrtc::SdpType::kOffer) {
      std::cout << "Setting offer sdp on receiver\n";
      receiver_->SetRemoteSdp(sdp, type);
    } else if (from_client_id == receiver_->id() &&
               type == webrtc::SdpType::kAnswer) {
      std::cout << "Setting answer sdp on caller\n";
      caller_->SetRemoteSdp(sdp, type);
    }
  }

  void OnIceCandidate(int from_client_id,
                      const cppday::IceCandidate& iceCandidate) override {
    std::cout << "Found ice candidate for " << from_client_id << ' '
              << iceCandidate.sdp_line() << '\n';
    if (from_client_id == caller_->id()) {
      std::cout << "Setting candidate on receiver\n";
      receiver_->AddIceCandidate(iceCandidate);
    } else if (from_client_id == receiver_->id()) {
      std::cout << "Setting candidate on caller\n";
      caller_->AddIceCandidate(iceCandidate);
    }
  }

  void OnDataChannelMessage(int from_client_id,
                            const std::string& message) override {
    std::cout << "Message received on " << from_client_id << ": " << message
              << '\n';
    if (from_client_id == receiver_->id()) {
      std::cout << "Replying\n";
      receiver_->SendMessage("Reply to message '" + message + "'");
    }
  }

 private:
  rtc::scoped_refptr<cppday::Client> caller_;
  rtc::scoped_refptr<cppday::Client> receiver_;
};

}  // namespace

int main(int argc, char* argv[]) {
  std::cout << "Start\n";
  // rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_VERBOSE);
  // rtc::LogMessage::LogToDebug(rtc::LoggingSeverity::LS_INFO);

  std::unique_ptr<rtc::Thread> worker_thread =
      rtc::Thread::CreateWithSocketServer();
  std::unique_ptr<rtc::Thread> signaling_thread = rtc::Thread::Create();
  worker_thread->Start();
  signaling_thread->Start();

  std::cout << "Creating peer_connection_factory\n";
  rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface>
      peer_connection_factory = webrtc::CreatePeerConnectionFactory(
          worker_thread.get(), /* network_thread */
          worker_thread.get(), signaling_thread.get(),
          rtc::scoped_refptr<webrtc::AudioDeviceModule>(
              FakeAudioCaptureModule::Create()),
          webrtc::CreateBuiltinAudioEncoderFactory(),
          webrtc::CreateBuiltinAudioDecoderFactory(),
          webrtc::CreateBuiltinVideoEncoderFactory(),
          webrtc::CreateBuiltinVideoDecoderFactory(), nullptr /* audio_mixer */,
          nullptr /* audio_processing */);
  if (!peer_connection_factory) {
    std::cout << "Error creating peer_connection_factory\n";
    return 1;
  }

  std::cout << "Creating caller and receiver clients\n";
  rtc::scoped_refptr<cppday::Client> caller =
      new rtc::RefCountedObject<cppday::Client>(1);
  rtc::scoped_refptr<cppday::Client> receiver =
      new rtc::RefCountedObject<cppday::Client>(2);

  MyClientObserver dialog(caller, receiver);
  caller->RegisterObserver(&dialog);
  receiver->RegisterObserver(&dialog);

  std::cout << "Creating peer_connections for caller and receiver\n";
  if (!caller->CreatePeerConnection(peer_connection_factory) ||
      !receiver->CreatePeerConnection(peer_connection_factory)) {
    std::cout << "Error creating peer_connection";
    return 2;
  }

  std::cout << "Making call\n";
  caller->MakeCall();

  std::string line;
  while (std::getline(std::cin, line)) {
    if (line == "q") {
      break;
    }

    caller->SendMessage(line);
  }

  std::cout << "Closing clients\n";
  caller->Close();
  receiver->Close();
  std::cout << "End\n";
  return 0;
}