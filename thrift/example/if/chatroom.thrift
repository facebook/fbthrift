namespace cpp2 tutorials.chatroom

struct Message {
  1: string message
  2: string sender
  3: i64 timestamp
}

struct IndexToken {
  1: i64 index
}

struct GetMessagesRequest {
  // This token is a pointer into the messages queue that the server maintains,
  // and it marks a particular client's place in the queue. Hence, if you have
  // 10 messages in the queue, and the token's index is 5, then the client
  // has already received messages 0-5.
  1: optional IndexToken token
}

struct GetMessagesResponse {
  1: list<Message> messages
  2: IndexToken token
}

struct SendMessageRequest {
  1: string message
  2: string sender
}

exception Exception {
  1: string message
}

service ChatRoomService {
  /**
   * Initialize the service
   */
  void initialize() throws (1: Exception e)

  /**
   * Get the last few chat messages
   */
  GetMessagesResponse getMessages(1: GetMessagesRequest req)
    throws (1: Exception e)

  /**
   * Send a message
   */
  void sendMessage(1: SendMessageRequest req) throws (1: Exception e)
}

service Echo {
 /**
  * Echo back the message
  */
  string echo(1: string message)
}
