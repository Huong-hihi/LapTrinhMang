#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#include "../libs/protocol.h"
#include "../libs/tool.h"
#include "../libs/valid.h"

#define BUFF_SIZE 1024
// Hướng dẫn đăng nhập
void loginTutorial()
{
    printf("-------------------Đấu trường 100-------------------\n");
    printf("\nVui Lòng Đăng Nhập! ");
    printf("\nNếu Bạn Đã Có Tài Khoản! ");
    printf("\n\tĐăng Nhập: USER tên đăng nhập của bạn");
    printf("\n\tMật Khẩu: PASS mật khẩu");
    printf("\nNếu Chưa Có Tài Khoản! ");
    printf("\n\tĐăng Ký: REGISTER ten_dang_nhap mat_khau");
    printf("\n-------------------Đấu trường 100-------------------");
    printf("\nVui Lòng Nhập Lựa Chọn Của Bạn: \n");
}//hiển thị cho người chơi 
void gamePlayForNormalTutorial()
{
    printf("-------------------Đấu trường 100-------------------\n");
    printf("\nChọn Câu Trả Lời Của Bạn:ANSWER Đáp án ");
    printf("\n-------------------Đấu trường 100-------------------");
    printf("\nVui Lòng Nhập Lựa Chọn Của Bạn: \n");
}
void chooseTopicLevel()
{
    printf("-------------------Đấu trường 100-------------------\n");
    printf("\nChọn Mức Độ Của Câu Hỏi: TOPIC mứcđộ (mứcđộ: EASY, NORMAL, HARD)");
    printf("\n--------------------------------------------------");
    printf("\nVui Lòng Nhập Lựa Chọn Của Bạn: \n");
}
//hiển thị cho người chơi chính
void gamePlayForSpecialTutorial()
{
    printf("-------------------Đấu trường 100-------------------\n");
    printf("\nChọn câu trả lời: ANSWER answer");
    printf("\n\tSử dụng trợ giúp : HELP");
    printf("\n--------------------------------------------------");
    printf("\nVui Lòng Nhập Lựa Chọn Của Bạn: \n");
}
void showQuestion(Question *question)
{
    printf("\n%s", question->question);
    printf("\n%s", question->answer1);
    printf("\n%s", question->answer2);
    printf("\n%s", question->answer3);
    printf("\n%s", question->answer4);
}
int main(int argc, char const *argv[])
{
    int client_sock, servPort;
    char buff[BUFF_SIZE], username[BUFF_SIZE], luckyPlayer[BUFF_SIZE], topic[BUFF_SIZE];
    struct sockaddr_in server_addr; /* server's address information */
    int msg_len, bytes_sent, bytes_received;
    char code[BUFF_SIZE], data[BUFF_SIZE];
    GAMEPLAY_STATUS status = UNAUTH;
    Question *ques = (Question *)malloc(sizeof(Question));
    Request *request = (Request *)malloc(sizeof(Request));
    Response *response = (Response *)malloc(sizeof(Response));
    Information *infor = (Information *)malloc(sizeof(Information));
    int lucky = FALSE, existQuestion = FALSE, help = FALSE;
    int questionNumber = 0;
    float score = 0;
    int inforamation = TRUE;
    int gameStatus = GAME_PLAYING;
    if (argc != 3)
    {
        printf("\nThông số không chính xác\n");
    }
    else
    {
        //Check input : IP address & Port
        //Kiểm tra đầu vào : địa chỉ IP và cổng
        if (checkIPAndPort(argv[1], argv[2]) != 0)
        {
            //B1: xây dựng socket
            client_sock = socket(AF_INET, SOCK_STREAM, 0);

            //B2: Xác định địa chỉ server
            servPort = atoi(argv[2]);

            server_addr.sin_family = AF_INET;
            server_addr.sin_port = htons(servPort);
            server_addr.sin_addr.s_addr = inet_addr(argv[1]);
            //B3: Yêu cầu kết nối với server
            if (connect(client_sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) < 0)
            {
                printf("\nLỗi không thể kết nối với sever,  người dùng đã thoát  ");
                return 0;
            }
            while (1)
            {
                switch (status)
                {
                case UNAUTH:
                    //Gửi yêu cầu
                    loginTutorial();
                    memset(buff, '\0', (strlen(buff) + 1));
                    fgets(buff, BUFF_SIZE, stdin);
                    buff[strlen(buff) - 1] = '\0';
                    setOpcodeRequest(request, buff);
                    sendRequest(client_sock, request, sizeof(Request), 0);
                    //Nhận yêu cầu
                    receiveResponse(client_sock, response, sizeof(Response), 0);
                    readMessageResponse(response);
                    status = response->status;
                    if (response->data != NULL)
                    {
                        memset(username, '\0', (strlen(username) + 1));
                        strcpy(username, response->data);
                    }
                    break;
                case WAITING_PLAYER:
                    //Gửi yêu cầu 
                    requestGet(client_sock);
                    receiveResponse(client_sock, response, sizeof(Response), 0);
                    status = response->status;
                    if (status == WAITING_QUESTION)
                    {
                        //Đoc message
                        readMessageResponse(response);
                        memset(luckyPlayer, '\0', (strlen(luckyPlayer) + 1));
                        strcpy(luckyPlayer, response->data);
                        printf("Người Chơi May Mắn: %s\n", luckyPlayer);
                        if (strcmp(luckyPlayer, username) == 0)
                            lucky = TRUE;
                        else
                            lucky = FALSE;
                    }
                    break;
                case WAITING_QUESTION:
                    if (lucky == TRUE)
                    {
                        if (inforamation == FALSE)
                        {
                            //Yêu cầu lấy thông tin game
                            requestCheckInformation(client_sock);
                            //Nhận thông tin của game
                            receiveInformation(client_sock, infor, sizeof(Information), 0);
                            if (infor->status == TRUE)
                            {
                                inforamation = TRUE;
                                if (help == FALSE)
                                {
                                    score = score + infor->score;
                                    printf("Số người trả lời sai câu hỏi trên: %d\n", infor->playerAnswerWrong);
                                    printf("Số điểm bạn nhận được: %.1f\n", infor->score);
                                    printf("Số người cùng chơi: %d\n", infor->playerPlaying);
                                    printf("Số điểm của bạn hiện tại: %.1f\n", score);
                                }
                                else
                                {
                                    score = score - infor->score;
                                    printf("Bạn đã sử dụng trợ giúp\n");
                                    printf("Số điểm bạn nhận bị trừ: %.1f\n", infor->score);
                                    printf("Số người trả lời sai câu hỏi trên: %d\n", infor->playerAnswerWrong);
                                    printf("Số người cùng chơi: %d\n", infor->playerPlaying);
                                    printf("Số điểm của bạn hiện tại: %.1f\n", score);
                                    help = FALSE;
                                }
                            }
                        }
                        else
                        {
                            //Kiểm tra trạng thái của game là sẽ tiếp tục hay kết thúc
                            requestGet(client_sock);
                            receiveResponse(client_sock, response, sizeof(Response), 0);
                            inforamation = FALSE;
                            if (response->status == END_GAME)
                            {
                                status = response->status;
                                readMessageResponse(response);
                            }
                            else
                            {
                                //Chọn mức độ khó của câu hỏi
                                chooseTopicLevel();
                                memset(buff, '\0', (strlen(buff) + 1));
                                fgets(buff, BUFF_SIZE, stdin);
                                buff[strlen(buff) - 1] = '\0';
                                setOpcodeRequest(request, buff);
                                sendRequest(client_sock, request, sizeof(Request), 0);
                                //Nhận phản hồi
                                receiveResponse(client_sock, response, sizeof(Response), 0);
                                status = response->status;
                                if (status == PLAYING)
                                {
                                    strcpy(topic, response->data);
                                    readMessageResponse(response);
                                }
                                if (status == WAITING_QUESTION)
                                {
                                    readMessageResponse(response);
                                    inforamation = TRUE;
                                }
                            }
                        }
                    }
                    else
                    {
                        //Kiểm tra trạng thái của game 
                        requestGet(client_sock);
                        receiveResponse(client_sock, response, sizeof(Response), 0);
                        if (response->status == END_GAME)
                        {
                            status = response->status;
                            readMessageResponse(response);
                        }
                        else
                        {
                            //rNhận phản hồi từ server
                            requestGet(client_sock);
                            receiveResponse(client_sock, response, sizeof(Response), 0);
                            status = response->status;
                            if (status == PLAYING)
                            {
                                strcpy(topic, response->data);
                                readMessageResponse(response);
                            }
                        }
                    }
                    break;
                case PLAYING:
                    if (lucky == TRUE)
                    {
                        if (existQuestion == TRUE)
                        {
                            printf("\nCâu hỏi số %d\n", questionNumber);
                            printf("Mức độ: %s", topic);
                            showQuestion(ques);
                            printf("\nCâu trả lời: \n");
                            gamePlayForNormalTutorial();
                            //check if request->code == HELP
                            
                            // else
                            // {
                                //send request
                                memset(buff, '\0', (strlen(buff) + 1));
                                fgets(buff, BUFF_SIZE, stdin);
                                buff[strlen(buff) - 1] = '\0';
                                if (strcmp(buff, "HELP") == 0)
                                {
                                requestGetHelp(client_sock);
                                } else {
                                    setOpcodeRequest(request, buff);
                                    sendRequest(client_sock, request, sizeof(Request), 0);
                                }

                                
                            // }
                            //rcv request
                            receiveResponse(client_sock, response, sizeof(Response), 0);

                            status = response->status;
                            readMessageResponse(response);
                            if (status == WAITING_QUESTION) // Lỗi
                            {
                                existQuestion = FALSE;
                            }
                            if (response->code == USER_USED_HINT_SUCCESS) //Sử dụng trợ giúp
                            {    
                                help = TRUE;
                            }
                        }
                        else
                        {
                            //Yêu cầu lấy câu hỏi 
                            requestGet(client_sock);
                            //Nhận câu hỏi
                            receiveQuestion(client_sock, ques, sizeof(Question), 0);
                            existQuestion = TRUE;
                            questionNumber++;
                        }
                    }
                    else
                    {
                        if (existQuestion == TRUE)
                        {
                            printf("\nCâu hỏi số %d\n", questionNumber);
                            printf("Mức độ: %s", topic);
                            showQuestion(ques);
                            printf("\nCâu trả lời: \n");
                            gamePlayForNormalTutorial();
                            //Gửi đáp án 
                            memset(buff, '\0', (strlen(buff) + 1));
                            fgets(buff, BUFF_SIZE, stdin);
                            buff[strlen(buff) - 1] = '\0';
                            setOpcodeRequest(request, buff);
                            sendRequest(client_sock, request, sizeof(Request), 0);
                            //Nhận phản hồi
                            receiveResponse(client_sock, response, sizeof(Response), 0);
                            status = response->status;
                            if (status == WAITING_QUESTION)
                            {
                                existQuestion = FALSE;
                            }

                            readMessageResponse(response);
                        }
                        else
                        {
                            requestGet(client_sock);
                            receiveQuestion(client_sock, ques, sizeof(Question), 0);
                            existQuestion = TRUE;
                            questionNumber++;
                        }
                    }
                    break;
                case END_GAME:
                    if (lucky == TRUE)
                    {
                        if (inforamation == FALSE)
                        {
                            //lấy kết quả
                            requestCheckInformation(client_sock);
                            receiveInformation(client_sock, infor, sizeof(Information), 0);
                            if (infor->status == TRUE)
                            {
                                printf("Số điểm bạn nhận được là: %1.f\n", infor->score);
                                inforamation = TRUE;
                            }
                        }
                        else
                        {
                            //yêu cầu thoát game
                            requestLogout(client_sock, username);
                            receiveResponse(client_sock, response, sizeof(Response), 0);
                            status = response->status;
                            readMessageResponse(response);
                            gameStatus = GAME_END;
                        }
                    }
                    else
                    {
                        if (inforamation == TRUE)
                        {
                            inforamation = FALSE;
                            // lấy thông tin
                            requestCheckInformation(client_sock);
                            receiveInformation(client_sock, infor, sizeof(Information), 0);
                            printf("Số điểm bạn nhận được là: %1.f\n", infor->score);
                        }
                        else
                        {
                            //Yêu cầu thoát game
                            requestLogout(client_sock, username);
                            receiveResponse(client_sock, response, sizeof(Response), 0);
                            status = response->status;
                            readMessageResponse(response);
                            gameStatus = GAME_END;
                        }
                    }
                    break;
                }
                if (gameStatus == GAME_END)
                    break;
            }
            //B5: đóng socket
            close(client_sock);
            return 0;
        }
    }
}