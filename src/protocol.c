#include "../libs/protocol.h"

int receiveRequest(int socket, Request *buff, int size, int flags)
{
  int n;

  n = recv(socket, buff, size, flags);                                                          // nhận vào Nhận dữ liệu từ SOCKET 
  if (n < 0)
    perror("Error: ");                                                                                //Thất bại: Trả về:SOCKET_ERROR
  return n;                                                                                       //Thành công: Trả về:kích thước dữ liệu đã nhận (byte)
}

int sendRequest(int socket, Request *buff, int size, int flags)
{
  int n;

  n = send(socket, buff, size, flags);                                                            //Gửi dữ liệu ra SOCKET
  if (n < 0)
    perror("Error: ");                                                                      //Thất bại: SOCKET_ERROR
  return n;                                                                         //Thành công: kích thước dữ liệu đã gửi đi (byte)
}

int sendResponse(int socket, Response *msg, int size, int flags)
{
  int n;
  n = send(socket, msg, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}

int receiveResponse(int socket, Response *msg, int size, int flags)
{
  int n;
  n = recv(socket, msg, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}

void setMessageResponse(Response *msg)
{
  if (msg->code != NULL)
  {
    switch (msg->code)
    {
    case SYNTAX_ERROR:
      strcpy(msg->message, "Lỗi cú pháp ");
      break;
    case INVALID_OPERATION:
      strcpy(msg->message, "Hoạt động không hợp lệ ");
      break;
    case USER_NAME_FOUND:
      strcpy(msg->message, "Tên người dùng đúng ");
      break;
    case USER_NAME_NOT_FOUND:
      strcpy(msg->message, "Không tìm thấy tài khoản ");
      break;
    case USER_NAME_BLOCKED:
      strcpy(msg->message, "Tài khoản đã bị chặn");
      break;
    case USER_NAME_IS_SIGNIN:
      strcpy(msg->message, "Chỉ đăng nhập một tài khoản ");
      break;
    case PASSWORD_CORRECT:
      strcpy(msg->message, "Đăng nhập thành công");
      break;
    case PASSWORD_INCORRECT:
      strcpy(msg->message, "Mật khẩu không đúng ");
      break;
    case PASSWORD_INCORRECT_THREE_TIMES:
      strcpy(msg->message, "Mật khẩu sai. Tài khoản đã bị chặn ");
      break;
    case LOGOUT_SUCCESS:
      strcpy(msg->message, "Bạn đã thoát khỏi game ");
      break;
    case REGISTER_SUCCESSFULL:
      strcpy(msg->message, "Đăng ký thành công");
      break;
    case REGISTER_USERNAME_EXISTED:
      strcpy(msg->message, "Tên người dùng đã tồn tại ");
      break;
    case PASSWORD_CORRECT_BUT_ACCOUNT_IS_SIGNINED_IN_ORTHER_CLIENT:
      strcpy(msg->message, "Tài khoản đang đăng nhập khách hàng khác");
      break;
    case ANSWER_IS_CORRECT:
      strcpy(msg->message, "Câu trả lời chính xác ");
      break;
    case ANSWER_IS_INCORRECT:
      strcpy(msg->message, "Câu trả lời sai \nKết thúc trò chơi");
      break;
    case ANSWER_IS_INVALID:
      strcpy(msg->message, "Câu trả lời sai ");
      break;
    case USER_USED_HINT_SUCCESS:
      strcpy(msg->message, "Người dùng sử dụng gợi ý thành công ! ");
      break;
    case USER_USED_HINT_FAIL:
      strcpy(msg->message, "Sử dụng gợi ý không thành công ( Bạn đã dùng hết gợi ý)");
      break;
    case TOPIC_USER_CHOOSE_LEVEL:
      strcpy(msg->message, "");
      break;
    case TOPIC_TYPE_INVALID:
      strcpy(msg->message, "Mức độ bạn chọn không hợp lệ ");
      break;
    case INFORMATION_SUCCESS:
      strcpy(msg->message, "");
      break;
    case INFORMATION_ORTHER_PLAYER_ANSWERING:
      strcpy(msg->message, "");
      break;
    case GAME_READY:
      strcpy(msg->message, "Trò chơi bắt đầu ");
      break;
    case GAME_NOT_READY:
      strcpy(msg->message, "Đang đợi người chơi khác ...");
      break;
    case TOPIC_USER_DONT_CHOOSE_LEVEL:
      strcpy(msg->message, "");
      break;
    case GAME_END_WIN:
      strcpy(msg->message, "Kết thúc trò chơi.\nBạn là người chiến thắng ");
      break;
    case GAME_IS_PLAYING_DONT_LOG_IN:
      strcpy(msg->message, "\nTrò chơi đang diễn ra bạn không thể đăng nhập\n");
      break;
    default:
      strcpy(msg->message, "Ngoại lệ ");
      break;
    }
  }
}

void readMessageResponse(Response *msg)
{
  if (msg->code != NULL)
  {
    printf("%s\n", msg->message);
    switch (msg->code)
    {
    case PASSWORD_CORRECT:
      printf("Chào mừng %s đã trở thành người chơi\n", msg->data);
      break;
    case LOGOUT_SUCCESS:
      printf("Tạm biệt %s\n", msg->data);
      break;
    case TOPIC_USER_CHOOSE_LEVEL:
      printf("Người dùng chọn mức độ câu hỏi %s\n", msg->data);
      break;
    default:
      break;
    }
  }
}

void setOpcodeRequest(Request *request, char *input)
{
  char code[BUFF_SIZE], data[BUFF_SIZE];
  splitMessage(input, code, data);
  strcpy(request->message, data);
  if (strcmp(code, "USER") == 0)
    request->code = USER;
  else if (strcmp(code, "PASS") == 0)
    request->code = PASS;
  else if (strcmp(code, "REGISTER") == 0)
    request->code = REGISTER;
  else if (strcmp(code, "LOGOUT") == 0)
    request->code = LOGOUT;
  else if (strcmp(code, "CHECK") == 0)
    request->code = CHECK;
  else if (strcmp(code, "INFORMATION") == 0)
    request->code = INFORMATION;
  else if (strcmp(code, "ANSWER") == 0)
    request->code = CHOOSE_ANWSER;
  else if (strcmp(code, "TOPIC") == 0)
    request->code = TOPIC_LEVEL;
  else if (strcmp(code, "HELP") == 0)
    request->code = HELP;
}

int sendQuestion(int socket, Question *question, int size, int flags)
{
  int n;
  n = send(socket, question, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}
int receiveQuestion(int socket, Question *question, int size, int flags)
{
  int n;
  n = recv(socket, question, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}

void requestGet(int socket)
{
  Request *request = (Request *)malloc(sizeof(Request));
  setOpcodeRequest(request, "CHECK check");
  sendRequest(socket, request, sizeof(Request), 0);
}
void requestCheckInformation(int socket)
{
  Request *request = (Request *)malloc(sizeof(Request));
  setOpcodeRequest(request, "INFORMATION information");
  sendRequest(socket, request, sizeof(Request), 0);
}
int sendInformation(int socket, Information *infor, int size, int flags)
{
  int n;
  n = send(socket, infor, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}
int receiveInformation(int socket, Information *infor, int size, int flags)
{
  int n;
  n = recv(socket, infor, size, flags);
  if (n < 0)
    perror("Error: ");
  return n;
}
void requestLogout(int socket, char *username)
{
  Request *request = (Request *)malloc(sizeof(Request));
  char buff[BUFF_SIZE];
  strcpy(buff, "LOGOUT ");
  strcat(buff, username);
  setOpcodeRequest(request, buff);
  sendRequest(socket, request, sizeof(Request), 0);
}
void requestGetHelp(int socket)
{
  Request *request = (Request *)malloc(sizeof(Request));
  setOpcodeRequest(request, "HELP help");
  sendRequest(socket, request, sizeof(Request), 0);
}