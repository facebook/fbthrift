namespace cpp2 facebook.tutorials.thrift.chatroomservice

exception ChatRoomServiceException {
  1: string message
}

struct ChatRoomServiceMessage {
  1: string message
  2: string sender
  3: i64 timestamp
}

struct ChatRoomServiceIndexToken {
  1: i64 index
}

struct ChatRoomServiceGetMessagesRequest {
  // This token is a pointer into the messages queue that the server maintains,
  // and it marks a particular client's place in the queue. Hence, if you have
  // 10 messages in the queue, and the token's index is 5, you know that the
  // client has already received messages 0-5.
  1: optional ChatRoomServiceIndexToken token;
}

struct ChatRoomServiceGetMessagesResponse {
  1: list<ChatRoomServiceMessage> messages
  2: ChatRoomServiceIndexToken token
}

struct ChatRoomServiceSendMessageRequest {
  1: string message
  2: string sender
}

service ChatRoomService {
  /**
   * Get the last few chat messages
   */
  ChatRoomServiceGetMessagesResponse getMessages(
      1: ChatRoomServiceGetMessagesRequest request)
      throws (1: ChatRoomServiceException e)

  /**
   * Send a message
   */
  void sendMessage(1: ChatRoomServiceSendMessageRequest req)
      throws (1: ChatRoomServiceException e)
}
