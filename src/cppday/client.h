#ifndef EXAMPLES_CPPDAY_CLIENT_H_
#define EXAMPLES_CPPDAY_CLIENT_H_

#include <string>
#include <utility>

#include "api/data_channel_interface.h"
#include "api/jsep.h"
#include "api/peer_connection_interface.h"
#include "api/rtc_error.h"
#include "api/scoped_refptr.h"

namespace cppday {

class IceCandidate {
 public:
  IceCandidate(std::string mid_name, int m_line_index, std::string sdp_line)
      : mid_name_(std::move(mid_name)),
        m_line_index_(m_line_index),
        sdp_line_(std::move(sdp_line)) {}

  std::string mid_name() const { return mid_name_; }
  int m_line_index() const { return m_line_index_; }
  std::string sdp_line() const { return sdp_line_; }

 private:
  std::string mid_name_;
  int m_line_index_;
  std::string sdp_line_;
};

struct ClientObserver {
  virtual void OnLocalSdpAvailable(int from_client_id,
                                   const std::string& sdp,
                                   webrtc::SdpType type) = 0;
  virtual void OnIceCandidate(int from_client_id,
                              const IceCandidate& iceCandidate) = 0;
  virtual void OnDataChannelMessage(int from_client_id,
    const std::string& message) = 0;

 protected:
  virtual ~ClientObserver() {}
};

class Client : public webrtc::PeerConnectionObserver,
               public webrtc::CreateSessionDescriptionObserver,
               public webrtc::DataChannelObserver {
 public:
  explicit Client(int id);
  ~Client();

  int id() const { return id_; }

  void RegisterObserver(ClientObserver* callback);
  bool CreatePeerConnection(
      webrtc::PeerConnectionFactoryInterface* peer_connection_factory);
  void MakeCall();
  void SetRemoteSdp(const std::string& sdp, webrtc::SdpType type);
  void AddIceCandidate(const IceCandidate& candidate);
  bool SendMessage(const std::string& message);
  void Close();

 protected:
  // PeerConnectionObserver implementation.
  void OnSignalingChange(
      webrtc::PeerConnectionInterface::SignalingState new_state) override {}
  void OnDataChannel(
      rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override;
  void OnRenegotiationNeeded() override {}
  void OnConnectionChange(
      webrtc::PeerConnectionInterface::PeerConnectionState new_state) override;
  void OnIceGatheringChange(
      webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
  void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;

  // CreateSessionDescriptionObserver implementation.
  void OnSuccess(
      webrtc::SessionDescriptionInterface* desc) override;  // on local sdp
  void OnFailure(webrtc::RTCError error) override;

  // DataChannelObserver implementation.
  void OnStateChange() override;
  void OnMessage(const webrtc::DataBuffer& buffer) override;

 private:
  int id_;
  ClientObserver* callback_;
  rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
  rtc::scoped_refptr<webrtc::DataChannelInterface> data_channel_;
};

}  // namespace cppday

#endif  // EXAMPLES_CPPDAY_CLIENT_H_