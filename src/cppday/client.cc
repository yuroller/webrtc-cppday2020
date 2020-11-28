#include "examples/cppday/client.h"

#include <memory>

#include "rtc_base/checks.h"
#include "rtc_base/logging.h"
#include "rtc_base/ref_counted_object.h"

namespace {

class DummySetSessionDescriptionObserver
    : public webrtc::SetSessionDescriptionObserver {
 public:
  static DummySetSessionDescriptionObserver* Create() {
    return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
  }
  virtual void OnSuccess() { RTC_LOG(INFO) << __FUNCTION__; }
  virtual void OnFailure(webrtc::RTCError error) {
    RTC_LOG(INFO) << __FUNCTION__ << " " << ToString(error.type()) << ": "
                  << error.message();
  }
};

}  // namespace

namespace cppday {

Client::Client(int id) : id_(id), callback_(nullptr) {}

Client::~Client() {
  RTC_DCHECK(!peer_connection_);
  RTC_DCHECK(!data_channel_);
}

void Client::RegisterObserver(ClientObserver* callback) {
  RTC_DCHECK(!callback_);
  callback_ = callback;
}

bool Client::CreatePeerConnection(
    webrtc::PeerConnectionFactoryInterface* peer_connection_factory) {
  RTC_DCHECK(!peer_connection_);

  webrtc::PeerConnectionInterface::RTCConfiguration config;
  config.sdp_semantics = webrtc::SdpSemantics::kUnifiedPlan;
  config.enable_dtls_srtp = true;
  // config.enable_rtp_data_channel = true;

  // stun server
  webrtc::PeerConnectionInterface::IceServer stun_server;
  stun_server.uri = "stun:stun.l.google.com:19302";
  config.servers.push_back(stun_server);

  peer_connection_ = peer_connection_factory->CreatePeerConnection(
      config,
      nullptr,  // port allocator
      nullptr,  // cert_generator,
      this);    // PeerConnectionObserver

  return peer_connection_ != nullptr;
}

void Client::MakeCall() {
  RTC_DCHECK(!data_channel_);
  webrtc::DataChannelInit config;
  data_channel_ = peer_connection_->CreateDataChannel("Hello", &config);
  if (!data_channel_) {
    RTC_LOG(WARNING) << "Can't create data channel.";
  } else {
    data_channel_->RegisterObserver(this);
  }

  peer_connection_->CreateOffer(
      this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
}

void Client::SetRemoteSdp(const std::string& sdp, webrtc::SdpType type) {
  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::SessionDescriptionInterface> session_description =
      webrtc::CreateSessionDescription(type, sdp, &error);
  if (!session_description) {
    RTC_LOG(WARNING) << "Can't parse received session description message. "
                        "SdpParseError was: "
                     << error.description;
    return;
  }

  RTC_LOG(INFO) << " Received session description :" << session_description;
  peer_connection_->SetRemoteDescription(
      DummySetSessionDescriptionObserver::Create(),
      session_description.release());
  if (type == webrtc::SdpType::kOffer) {
    peer_connection_->CreateAnswer(
        this, webrtc::PeerConnectionInterface::RTCOfferAnswerOptions());
  }
}

void Client::AddIceCandidate(const IceCandidate& iceCandidate) {
  webrtc::SdpParseError error;
  std::unique_ptr<webrtc::IceCandidateInterface> candidate(
      webrtc::CreateIceCandidate(iceCandidate.mid_name(),
                                 iceCandidate.m_line_index(),
                                 iceCandidate.sdp_line(), &error));
  if (!candidate.get()) {
    RTC_LOG(WARNING) << "Can't parse received candidate message. "
                        "SdpParseError was: "
                     << error.description;
    return;
  }
  if (!peer_connection_->AddIceCandidate(candidate.get())) {
    RTC_LOG(WARNING) << "Failed to apply the received candidate";
    return;
  }
  RTC_LOG(INFO) << " Received candidate :" << candidate;
}

bool Client::SendMessage(const std::string& message) {
  webrtc::DataBuffer buffer(message);
  return data_channel_->Send(buffer);
}

void Client::Close() {
  data_channel_ = nullptr;
  peer_connection_ = nullptr;
}

//
// PeerConnectionObserver implementation.
//

void Client::OnDataChannel(
    rtc::scoped_refptr<webrtc::DataChannelInterface> channel) {
  RTC_LOG(INFO) << "Created data channel";
  RTC_DCHECK(!data_channel_);
  data_channel_ = channel;
  data_channel_->RegisterObserver(this);
}

void Client::OnConnectionChange(
    webrtc::PeerConnectionInterface::PeerConnectionState new_state) {
  RTC_LOG(INFO) << "ConnectionChange " << new_state;
}

void Client::OnIceCandidate(const webrtc::IceCandidateInterface* candidate) {
  std::string sdp;
  if (!candidate->ToString(&sdp)) {
    RTC_LOG(LS_ERROR) << "Failed to serialize candidate";
    return;
  }

  callback_->OnIceCandidate(
      id_,
      IceCandidate(candidate->sdp_mid(), candidate->sdp_mline_index(), sdp));
}

//
// CreateSessionDescriptionObserver implementation.
//

void Client::OnSuccess(webrtc::SessionDescriptionInterface* desc) {
  peer_connection_->SetLocalDescription(
      DummySetSessionDescriptionObserver::Create(), desc);

  std::string sdp;
  desc->ToString(&sdp);

  callback_->OnLocalSdpAvailable(id_, sdp, desc->GetType());
}

void Client::OnFailure(webrtc::RTCError error) {
  RTC_LOG(LERROR) << ToString(error.type()) << ": " << error.message();
}

//
// DataChannelObserver implementation.
//

void Client::OnStateChange() {}

void Client::OnMessage(const webrtc::DataBuffer& buffer) {
  size_t size = buffer.data.size();
  char* msg = new char[size + 1];
  memcpy(msg, buffer.data.data(), size);
  msg[size] = 0;
  callback_->OnDataChannelMessage(id_, msg);
  delete[] msg;
}

}  // namespace cppday
