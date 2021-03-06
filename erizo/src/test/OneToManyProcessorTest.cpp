#include <gmock/gmock.h>
#include <gtest/gtest.h>

#include <rtp/RtpHeaders.h>
#include <MediaDefinitions.h>
#include <OneToManyProcessor.h>
#include <string>

using testing::_;
using testing::Return;
using testing::Eq;
using erizo::dataPacket;

static const char kArbitraryPeerId[] = "111";

class MockPublisher: public erizo::MediaSource, public erizo::FeedbackSink {
 public:
  MockPublisher() {
    video_source_ssrc_list_[0] = 1;
    audio_source_ssrc_ = 2;
    source_fb_sink_ = this;
  }
  ~MockPublisher() {}
  void close() override {}
  int sendPLI() override { return 0; }
  int deliverFeedback_(std::shared_ptr<dataPacket> packet) override {
    return internalDeliverFeedback_(packet);
  }

  MOCK_METHOD1(internalDeliverFeedback_, int(std::shared_ptr<dataPacket>));
};

class MockSubscriber: public erizo::MediaSink, public erizo::FeedbackSource {
 public:
  MockSubscriber() {
    sink_fb_source_ = this;
  }
  ~MockSubscriber() {}
  void close() override {}
  int deliverAudioData_(std::shared_ptr<dataPacket> packet) override {
    return internalDeliverAudioData_(packet);
  }
  int deliverVideoData_(std::shared_ptr<dataPacket> packet) override {
    return internalDeliverVideoData_(packet);
  }

  MOCK_METHOD1(internalDeliverAudioData_, int(std::shared_ptr<dataPacket>));
  MOCK_METHOD1(internalDeliverVideoData_, int(std::shared_ptr<dataPacket>));
};

class OneToManyProcessorTest : public ::testing::Test {
 protected:
  virtual void SetUp() {
    publisher = std::make_shared<MockPublisher>();
    subscriber = std::make_shared<MockSubscriber>();
    otm.setPublisher(publisher);
    otm.addSubscriber(subscriber, kArbitraryPeerId);
  }
  virtual void TearDown() {}

  std::shared_ptr<MockSubscriber> getSubscriber(std::string peer_id) {
    if (otm.subscribers.find(peer_id) != otm.subscribers.end()) {
      return std::dynamic_pointer_cast<MockSubscriber>(
                        otm.subscribers.find(peer_id)->second);
    }
    return std::shared_ptr<MockSubscriber>();
  }
  std::shared_ptr<MockPublisher> publisher;
  std::shared_ptr<MockSubscriber> subscriber;
  erizo::OneToManyProcessor otm;
};

TEST_F(OneToManyProcessorTest, setPublisher_Success_WhenCalled) {
  EXPECT_THAT(otm.publisher.get(), Eq(publisher.get()));
}

TEST_F(OneToManyProcessorTest, addSubscriber_Success_WhenAddingNewSubscribers) {
  EXPECT_THAT(getSubscriber(kArbitraryPeerId), Eq(subscriber));
}

TEST_F(OneToManyProcessorTest, removeSubscriber_RemovesSubscribers_WhenSubscribersExist) {
  otm.removeSubscriber(kArbitraryPeerId);

  EXPECT_EQ(nullptr, getSubscriber(kArbitraryPeerId).get());
}

TEST_F(OneToManyProcessorTest, addSubscriber_Substitutes_WhenRepeated) {
  auto new_subscriber = std::make_shared<MockSubscriber>();

  otm.addSubscriber(new_subscriber, kArbitraryPeerId);

  EXPECT_THAT(getSubscriber(kArbitraryPeerId).get(), Eq(new_subscriber.get()));
}

TEST_F(OneToManyProcessorTest, deliverFeedback_CallsPublisher_WhenCalled) {
  erizo::RtpHeader header;
  header.setSeqNumber(12);

  EXPECT_CALL(*publisher.get(), internalDeliverFeedback_(_)).Times(1).WillOnce(Return(0));
  otm.deliverFeedback(std::make_shared<dataPacket>(0, reinterpret_cast<char*>(&header),
                      sizeof(erizo::RtpHeader), erizo::VIDEO_PACKET));
}

TEST_F(OneToManyProcessorTest, deliverVideoData_CallsSubscriber_whenCalled) {
  erizo::RtpHeader header;
  header.setSeqNumber(12);

  EXPECT_CALL(*subscriber, internalDeliverVideoData_(_)).Times(1).WillOnce(Return(0));
  otm.deliverVideoData(std::make_shared<dataPacket>(0, reinterpret_cast<char*>(&header),
                       sizeof(erizo::RtpHeader), erizo::VIDEO_PACKET));
}

TEST_F(OneToManyProcessorTest, deliverAudioData_CallsSubscriber_whenCalled) {
  erizo::RtpHeader header;
  header.setSeqNumber(12);

  EXPECT_CALL(*subscriber, internalDeliverAudioData_(_)).Times(1).WillOnce(Return(0));
  otm.deliverAudioData(std::make_shared<dataPacket>(0, reinterpret_cast<char*>(&header),
                       sizeof(erizo::RtpHeader), erizo::AUDIO_PACKET));
}
