#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>

#include "../libs/account.h"
#include "../libs/tool.h"
#include "../libs/protocol.h"
#include "../libs/valid.h"
#include "../libs/question.h"
#include "../libs/gameplay.h"

#define BACKLOG 3 /* Số người trong 1 game */

#define BUFF_SIZE 1024
Account **head;
Question **headQuestion;
Help **headHelp;
/*
đọc dữ liệu từ file account.txt
input: account.txt
output: Account **head;
*/
int loadDataBase()
{
    FILE *fin;
    char username[30], password[20];
    int account, members = 0;
    char position[30];
    char c;
    head = createLinkList();
    fin = fopen("../data/account.txt", "r");
    Account *new = NULL;
    while (!feof(fin))
    {
        fscanf(fin, "%s%c%s%c%s%c%d%c", position, &c, username, &c, password, &c, &account, &c);
        members++;
        if (feof(fin))
            break;
        addAccount(head, username, password, account, atoi(position));
    }
    fclose(fin);
    return members;
}
/*
intput: Account **head
output: account.txt
*/

void exportDataToFile()
{
    FILE *fout;
    Account *ptr;
    fout = fopen("../data/account.txt", "w");
    for (ptr = *head; ptr != NULL; ptr = ptr->next)
    {
        fprintf(fout, "%d%c%s%c%s%c%d%c", ptr->position, '\t', ptr->username, '\t', ptr->password, '\t', ptr->accountStatus, '\n');
    }
    fclose(fout);
}

int main(int argc, char const *argv[])
{
    int i, maxi, maxfd, listenfd, connfd, sockfd, j;
    int nready, client[FD_SETSIZE];
    ssize_t ret;
    fd_set readfds, allset;
    char sendBuff[BUFF_SIZE], rcvBuff[BUFF_SIZE], topic[BUFF_SIZE];
    socklen_t clilen;
    struct sockaddr_in cliaddr, servaddr;
    char username[30], password[30];
    int numberQuestionArray[BUFF_SIZE];
    int numberPlayerArray[BACKLOG];
    int positionLuckyPlayer = 0;
    PHASE status[BACKLOG];
    GAMEPLAY_STATUS gamePlayStatus[BACKLOG];
    int result[BACKLOG];

    int numberArrayLength, number = 0;
    int countPlayerPlaying = 0, countAnswerWrong = 0, countPlayerLoser = 0;
    int existQuestion = FALSE;
    int help = FALSE;
    int helpNumber = 1;
    int questionNumber = 0;
    float score = 0, playerScore = 0, bonusScore = 0;

    Account *account[BACKLOG];
    Account *luckyPlayer = (Account *)malloc(sizeof(Account));
    Request *rcvRequest = (Request *)malloc(sizeof(Request));
    Response *response = (Response *)malloc(sizeof(Response));
    Information *infor = (Information *)malloc(sizeof(Information));

    headQuestion = createQuestionList();
    Question *ques = (Question *)malloc(sizeof(Question));

    headHelp = createHelpList();
    Help *hint = (Help *)malloc(sizeof(Help));

    int members;
    int gameStatus = GAME_END;
    if (argc != 2)
    {
        printf("Tham số không hợp lệ\n");
    }
    else
    {
        if (checkPort(argv[1]) == 1)
        {
            members = loadDataBase();
            //B1: Xây dựng một socket TCP để lắng nghe yêu cầu kết nối
            if ((listenfd = socket(AF_INET, SOCK_STREAM, 0)) == -1)
            { /* gọi socket */
                perror("\nError: ");
                return 0;
            }

            //B 2: Liên kết địa chỉ với socket
            bzero(&servaddr, sizeof(servaddr));
            servaddr.sin_family = AF_INET;
            servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
            servaddr.sin_port = htons(atoi(argv[1]));

            if (bind(listenfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) == -1)
            { 
                perror("\nError: ");
                return 0;
            }

            //B 3: Lắng nghe yêu cầu từ client
            if (listen(listenfd, BACKLOG) == -1)
            { 
                perror("\nError: ");
                return 0;
            }

            maxfd = listenfd; //khởi tạo 
            maxi = -1;        /* index into client[] array */
            for (i = 0; i < FD_SETSIZE; i++)
            {
                client[i] = -1; // cho biết mục nhập có sẵn 
                status[i] = NOT_LOGGED_IN;
                gamePlayStatus[i] = UNAUTH;
                account[i] = NULL;
                result[i] = LOSE;
            }

            FD_ZERO(&allset);
            FD_SET(listenfd, &allset);

            //B4: Giao tiếp với các client
            while (1)
            {
                readfds = allset; // phân công nhiệm vụ
                nready = select(maxfd + 1, &readfds, NULL, NULL, NULL);
                if (nready < 0)
                {
                    perror("\nError: ");
                    return 0;
                }

                if (FD_ISSET(listenfd, &readfds))
                { /* người dùng mới kết nối  */ 
                    clilen = sizeof(cliaddr);
                    if ((connfd = accept(listenfd, (struct sockaddr *)&cliaddr, &clilen)) < 0)
                        perror("\nError: ");
                    else
                    {
                        printf("Có một kết nối từ %s\n", inet_ntoa(cliaddr.sin_addr)); 
                        for (i = 0; i < FD_SETSIZE; i++)
                            if (client[i] < 0)
                            {
                                client[i] = connfd; /* lưu */
                                break;
                            }
                        if (i == FD_SETSIZE)
                        {
                            printf("\nQúa nhiều người chơi");
                            close(connfd);
                        }

                        FD_SET(connfd, &allset); /* add new descriptor to set */
                        if (connfd > maxfd)
                            maxfd = connfd; /* for select */
                        if (i > maxi)
                            maxi = i; /* max index in client[] array */

                        if (--nready <= 0)
                            continue; /* no more readable descriptors */
                    }
                }
                if (gameStatus == LOSE)
                {
                    if (countMemberOnline(head, numberPlayerArray) == BACKLOG && positionLuckyPlayer == 0)
                    {
                        //lấy người chơi may mắn
                        positionLuckyPlayer = randomNumberInArray(numberPlayerArray, BACKLOG);
                        luckyPlayer = findUserNameAccountByPosition(head, positionLuckyPlayer);
                        //init question
                        readQuestionFromFile(headQuestion);
                        readHelpFromFile(headHelp);
                        gameStatus = GAME_PLAYING;
                    }
                }
                for (i = 0; i <= maxi; i++)
                { /* Kiểm tra dữ liệu của tất cả khách hàng */
                    if ((sockfd = client[i]) < 0)
                        continue;
                    if (FD_ISSET(sockfd, &readfds))
                    {
                        ret = receiveRequest(sockfd, rcvRequest, sizeof(Request), 0);
                        if (ret <= 0)
                        {
                            FD_CLR(sockfd, &allset);
                            close(sockfd);
                            client[i] = -1;
                            printf("\nMột kết nối đã bị đóng!!\n");
                        }
                        else
                        {
                            switch (gamePlayStatus[i])
                            {
                            case UNAUTH:
                                if (rcvRequest->code == USER) // code = "USER"
                                {
                                    switch (status[i])
                                    {
                                    case NOT_LOGGED_IN:
                                        if (findUserNameAccount(head, rcvRequest->message) != NULL)
                                        {
                                            account[i] = findUserNameAccount(head, rcvRequest->message);
                                            if (account[i]->accountStatus == BLOCKED)
                                            {
                                                response->code = USER_NAME_BLOCKED;
                                            }
                                            else
                                            {
                                                status[i] = UNAUTHENTICATE;
                                                response->code = USER_NAME_FOUND;
                                            }
                                        }
                                        else
                                        {
                                            response->code = USER_NAME_NOT_FOUND;
                                        }
                                        break;
                                    case UNAUTHENTICATE:
                                        if (findUserNameAccount(head, rcvRequest->message) != NULL)
                                        {
                                            account[i] = findUserNameAccount(head, rcvRequest->message);
                                            if (account[i]->accountStatus == BLOCKED)
                                            {
                                                response->code = USER_NAME_BLOCKED;
                                            }
                                            else
                                            {
                                                response->code = USER_NAME_FOUND;
                                            }
                                        }
                                        else
                                        {
                                            status[i] = NOT_LOGGED_IN;
                                        }
                                        break;
                                    case AUTHENTICATED:
                                        response->code = USER_NAME_IS_SIGNIN;
                                        break;
                                    default:
                                        response->code = INVALID_OPERATION;
                                        break;
                                    }
                                }
                                else if (rcvRequest->code == PASS) //CODE = PASS
                                {
                                    switch (status[i])
                                    {
                                    case UNAUTHENTICATE:
                                        if (strcmp(rcvRequest->message, account[i]->password) != 0)
                                        {
                                            account[i]->countSignIn++;
                                            if (account[i]->countSignIn >= 3)
                                            {
                                                status[i] = NOT_LOGGED_IN;
                                                blockAccount(head, account[i]->username);
                                                response->code = PASSWORD_INCORRECT_THREE_TIMES;
                                                exportDataToFile();
                                            }
                                            else
                                            {
                                                response->code = PASSWORD_INCORRECT;
                                            }
                                        }
                                        else
                                        {
                                            if (account[i]->status == ONLINE)
                                            {
                                                response->code = PASSWORD_CORRECT_BUT_ACCOUNT_IS_SIGNINED_IN_ORTHER_CLIENT;
                                            }
                                            else
                                            {
                                                status[i] = AUTHENTICATED;
                                                signinAccount(head, account[i]->username);
                                                account[i]->countSignIn = 0;
                                                response->code = PASSWORD_CORRECT;
                                                memset(response->data, '\0', (strlen(response->data) + 1));
                                                strcpy(response->data, account[i]->username);
                                                gamePlayStatus[i] = WAITING_PLAYER;
                                            }
                                        }
                                        break;
                                    default:
                                        response->code = INVALID_OPERATION;
                                        break;
                                    }
                                }
                                else if (rcvRequest->code == REGISTER) //CODE = REGISTER
                                {
                                    switch (status[i])
                                    {
                                    case NOT_LOGGED_IN:
                                        if (checkSpace(rcvRequest->message))
                                        {
                                            splitMessageData(rcvRequest->message, username, password);
                                            if (findUserNameAccount(head, username) != NULL)
                                            {
                                                response->code = REGISTER_USERNAME_EXISTED;
                                            }
                                            else
                                            {
                                                addAccount(head, username, password, ACTIVE, members);
                                                response->code = REGISTER_SUCCESSFULL;
                                                exportDataToFile();
                                            }
                                        }
                                        else
                                        {
                                            response->code = SYNTAX_ERROR;
                                        }
                                        break;
                                    default:
                                        response->code = SYNTAX_ERROR;
                                        break;
                                    }
                                }
                                else
                                {
                                    response->code = SYNTAX_ERROR;
                                }
                                response->status = gamePlayStatus[i];
                                setMessageResponse(response);
                                sendResponse(sockfd, response, sizeof(Response), 0);
                                break;
                            case WAITING_PLAYER:
                                if (positionLuckyPlayer != 0)
                                {
                                    gamePlayStatus[i] = WAITING_QUESTION;
                                    response->code = GAME_READY;
                                    response->status = gamePlayStatus[i];
                                    memset(response->data, '\0', (strlen(response->data) + 1));
                                    strcpy(response->data, luckyPlayer->username);
                                    setMessageResponse(response);
                                    sendResponse(sockfd, response, sizeof(Response), 0);
                                }
                                else
                                {
                                    response->code = GAME_NOT_READY;
                                    response->status = gamePlayStatus[i];
                                    setMessageResponse(response);
                                    sendResponse(sockfd, response, sizeof(Response), 0);
                                }
                                break;
                            case WAITING_QUESTION:
                                if (strcmp(account[i]->username, luckyPlayer->username) == 0)
                                {
                                    if (rcvRequest->code == TOPIC_LEVEL) //codee == TOPIC_LEVEL
                                    {
                                        memset(topic, '\0', (strlen(topic) + 1));
                                        strcpy(topic, rcvRequest->message);
                                        if (convertLevel(topic) != ORTHER)
                                        {
                                            if (number == 0)
                                            {
                                                numberArrayLength = searchQuestionByLevel(headQuestion, convertLevel(topic), numberQuestionArray);
                                                number = randomNumberInArray(numberQuestionArray, numberArrayLength);
                                                ques = searchQuestionByStt(headQuestion, number);
                                            }
                                            else
                                            {
                                                deleteQuestion(headQuestion, number);
                                                numberArrayLength = searchQuestionByLevel(headQuestion, convertLevel(topic), numberQuestionArray);
                                                number = randomNumberInArray(numberQuestionArray, numberArrayLength);
                                                ques = searchQuestionByStt(headQuestion, number);
                                            }
                                            questionNumber++;
                                            existQuestion = TRUE;
                                            gamePlayStatus[i] = PLAYING;
                                            //set data response
                                            response->code = TOPIC_USER_CHOOSE_LEVEL;
                                            response->status = gamePlayStatus[i];
                                            memset(response->data, '\0', (strlen(response->data) + 1));
                                            strcpy(response->data, topic);
                                        }
                                        else
                                        {
                                            response->status = WAITING_QUESTION;
                                            response->code = TOPIC_TYPE_INVALID;
                                        }
                                        setMessageResponse(response);
                                        sendResponse(sockfd, response, sizeof(Response), 0);
                                    }
                                    else if (rcvRequest->code == INFORMATION)
                                    {
                                        //check all client answer
                                        if (countAnswerWrong + countPlayerPlaying == BACKLOG - 1 - countPlayerLoser)
                                        {
                                            if (help == TRUE)
                                            {
                                                score = hint->score;
                                                playerScore -= score;
                                                help = FALSE;
                                            }
                                            else
                                            {
                                                score = 1000 / (BACKLOG - 1 - countPlayerLoser) * countAnswerWrong;
                                                playerScore += score;
                                            }
                                            infor->score = score;
                                            infor->status = TRUE;
                                            infor->playerPlaying = countPlayerPlaying;
                                            infor->playerAnswerWrong = countAnswerWrong;
                                            //send information
                                            sendInformation(sockfd, infor, sizeof(Information), 0);
                                            countPlayerLoser += countAnswerWrong;
                                            countPlayerPlaying = 0;
                                            countAnswerWrong = 0;
                                        }
                                        else
                                        {
                                            infor->status = FALSE;
                                            sendInformation(sockfd, infor, sizeof(Information), 0);
                                        }
                                    }
                                    else if (rcvRequest->code == CHECK)
                                    {
                                        //check status of game
                                        if (BACKLOG - 1 - countPlayerLoser == 0)
                                        {
                                            gamePlayStatus[i] = END_GAME;
                                            response->code = GAME_END_WIN;
                                            result[i] = WIN;
                                        }
                                        else
                                        {
                                            gamePlayStatus[i] = WAITING_QUESTION;
                                        }
                                        response->status = gamePlayStatus[i];
                                        setMessageResponse(response);
                                        sendResponse(sockfd, response, sizeof(Response), 0);
                                    }
                                    else //syntax error
                                    {
                                        response->code = SYNTAX_ERROR;
                                        response->status = WAITING_QUESTION;
                                        setMessageResponse(response);
                                        sendResponse(sockfd, response, sizeof(Response), 0);
                                    }
                                }
                                else
                                {
                                    if (rcvRequest->code == CHECK)
                                    {
                                        if (luckyPlayer->status == ONLINE)
                                        {
                                            response->status = WAITING_QUESTION;
                                            sendResponse(sockfd, response, sizeof(Response), 0);
                                            receiveRequest(sockfd, rcvRequest, sizeof(Request), 0);
                                            if (existQuestion == TRUE) //exist question
                                            {
                                                response->code = TOPIC_USER_CHOOSE_LEVEL;
                                                gamePlayStatus[i] = PLAYING;
                                                response->status = gamePlayStatus[i];
                                                memset(response->data, '\0', (strlen(response->data) + 1));
                                                strcpy(response->data, topic);
                                                setMessageResponse(response);
                                                sendResponse(sockfd, response, sizeof(Response), 0);
                                            }
                                            else
                                            {
                                                response->code = TOPIC_USER_DONT_CHOOSE_LEVEL;
                                                response->status = gamePlayStatus[i];
                                                setMessageResponse(response);
                                                sendResponse(sockfd, response, sizeof(Response), 0);
                                            }
                                        }
                                        else //lucky lose->end game
                                        {
                                            response->code = GAME_END_WIN;
                                            response->status = END_GAME;
                                            gamePlayStatus[i] = END_GAME;
                                            setMessageResponse(response);
                                            sendResponse(sockfd, response, sizeof(Response), 0);
                                            result[i] = WIN;
                                        }
                                    }
                                }

                                break;
                            case PLAYING:
                                if (strcmp(account[i]->username, luckyPlayer->username) == 0)
                                {
                                    if (rcvRequest->code == CHOOSE_ANWSER)
                                    {
                                        if (convertListAnswer(rcvRequest->message) != E)
                                        {
                                            //check answer
                                            if (checkAnswer(ques->true_ans, convertListAnswer(rcvRequest->message)) == 1)
                                            {

                                                response->code = ANSWER_IS_CORRECT;
                                                setMessageResponse(response);
                                                gamePlayStatus[i] = WAITING_QUESTION;
                                                response->status = gamePlayStatus[i];
                                                sendResponse(sockfd, response, sizeof(Response), 0);
                                            }
                                            else
                                            {
                                                response->code = ANSWER_IS_INCORRECT;
                                                setMessageResponse(response);
                                                gamePlayStatus[i] = END_GAME;
                                                response->status = gamePlayStatus[i];
                                                sendResponse(sockfd, response, sizeof(Response), 0);
                                                // bonusScore = playerScore / (countMemberOnline(head, numberPlayerArray) - 1);
                                                result[i] = LOSE;
                                            }
                                        }
                                        else
                                        {
                                            response->code = ANSWER_IS_INVALID;
                                            response->status = gamePlayStatus[i];
                                            setMessageResponse(response);
                                            sendResponse(sockfd, response, sizeof(Response), 0);
                                        }
                                    }
                                    else if (rcvRequest->code == HELP) //uses help
                                    {
                                        hint = searchHelpByStt(headHelp, helpNumber);
                                        if (hint != NULL) //hint is exist
                                        {
                                            hint->score = playerScore * hint->value;
                                            //printf("\nscore: %f", hint->score);
                                            response->code = USER_USED_HINT_SUCCESS;
                                            setMessageResponse(response);
                                            gamePlayStatus[i] = WAITING_QUESTION;
                                            response->status = gamePlayStatus[i];
                                            help = TRUE;
                                            deleteHelp(headHelp, helpNumber++);
                                            sendResponse(sockfd, response, sizeof(Response), 0);
                                            
                                            
                                        }
                                        else
                                        {
                                            response->code = USER_USED_HINT_FAIL;
                                            setMessageResponse(response);
                                            gamePlayStatus[i] = PLAYING;
                                            response->status = gamePlayStatus[i];
                                            sendResponse(sockfd, response, sizeof(Response), 0);
                                        }
                                    }
                                    else if (rcvRequest->code == CHECK)
                                    {
                                        //send question
                                        sendQuestion(sockfd, ques, sizeof(Question), 0);
                                    }
                                    else
                                    {
                                        response->code = SYNTAX_ERROR;
                                        response->status = PLAYING;
                                        setMessageResponse(response);
                                        sendResponse(sockfd, response, sizeof(Response), 0);
                                    }
                                }
                                else
                                {
                                    if (rcvRequest->code == CHOOSE_ANWSER)
                                    {
                                        if (convertListAnswer(rcvRequest->message) != E)
                                        {
                                            //check answer true
                                            if (checkAnswer(ques->true_ans, convertListAnswer(rcvRequest->message)) == 1)
                                            {
                                                response->code = ANSWER_IS_CORRECT;
                                                setMessageResponse(response);
                                                gamePlayStatus[i] = WAITING_QUESTION;
                                                response->status = gamePlayStatus[i];
                                                sendResponse(sockfd, response, sizeof(Response), 0);
                                                countPlayerPlaying++;
                                            }
                                            else //false
                                            {
                                                response->code = ANSWER_IS_INCORRECT;
                                                setMessageResponse(response);
                                                gamePlayStatus[i] = END_GAME;
                                                response->status = gamePlayStatus[i];
                                                sendResponse(sockfd, response, sizeof(Response), 0);
                                                countAnswerWrong++;
                                                result[i] = LOSE;
                                            }
                                            existQuestion = FALSE;
                                        }
                                        else
                                        {
                                            response->code = SYNTAX_ERROR;
                                            response->status = PLAYING;
                                            setMessageResponse(response);
                                            sendResponse(sockfd, response, sizeof(Response), 0);
                                        }
                                    }
                                    else if (rcvRequest->code == CHECK)
                                    {
                                        //send question
                                        sendQuestion(sockfd, ques, sizeof(Question), 0);
                                    }
                                    else //syntax error
                                    {
                                        response->code = SYNTAX_ERROR;
                                        response->status = PLAYING;
                                        setMessageResponse(response);
                                        sendResponse(sockfd, response, sizeof(Response), 0);
                                    }
                                }
                                break;
                            case END_GAME:
                                if (strcmp(account[i]->username, luckyPlayer->username) == 0) //lucky player
                                {
                                    if (rcvRequest->code == LOGOUT) //code == LOGOUT
                                    {
                                        account[i]->status = OFFLINE;
                                        luckyPlayer->status = OFFLINE;
                                        response->code = LOGOUT_SUCCESS;
                                        setMessageResponse(response);
                                        memset(response->data, '\0', (strlen(response->data) + 1));
                                        strcpy(response->data, account[i]->username);
                                        sendResponse(sockfd, response, sizeof(Response), 0);
                                    }
                                    else if (rcvRequest->code == INFORMATION) //send result
                                    {
                                        if (countAnswerWrong + countPlayerPlaying == BACKLOG - 1 - countPlayerLoser)
                                        {
                                            if (result[i] == WIN)
                                            {
                                                infor->score = playerScore;
                                            }
                                            else
                                            {
                                                infor->score = 0;
                                            }
                                            infor->status = TRUE;
                                        }
                                        else
                                        {
                                            infor->status = FALSE;
                                        }
                                        sendInformation(sockfd, infor, sizeof(Information), 0);
                                    }
                                }
                                else
                                {
                                    if (rcvRequest->code == LOGOUT) // code ==LOGOUT
                                    {
                                        account[i]->status = OFFLINE;
                                        response->code = LOGOUT_SUCCESS;
                                        setMessageResponse(response);
                                        memset(response->data, '\0', (strlen(response->data) + 1));
                                        strcpy(response->data, account[i]->username);
                                        sendResponse(sockfd, response, sizeof(Response), 0);
                                    }
                                    else if (rcvRequest->code == INFORMATION) //send result
                                    {
                                        if (countAnswerWrong + countPlayerPlaying == BACKLOG - 1 - countPlayerLoser) //check all client answer
                                        {
                                            if (result[i] == WIN)
                                            {
                                                bonusScore = playerScore / countPlayerPlaying;
                                                infor->score = bonusScore;
                                            }
                                            else
                                            {
                                                infor->score = 0;
                                            }
                                            infor->status = TRUE;
                                        }
                                        else
                                        {
                                            infor->status = FALSE;
                                        }
                                        sendInformation(sockfd, infor, sizeof(Information), 0);
                                    }
                                }
                                break;
                            default:
                                break;
                            }
                        }

                        if (--nready <= 0)
                            break; /* no more readable descriptors */
                    }
                }
                if (gameStatus == GAME_PLAYING) //init new game
                {
                    if (countMemberOnline(head, numberPlayerArray) == 0)
                    {
                        gameStatus = GAME_END;
                        positionLuckyPlayer = 0;
                        number = 0;
                        countPlayerPlaying = 0;
                        countAnswerWrong = 0;
                        countPlayerLoser = 0;
                        existQuestion = FALSE;
                        help = FALSE;
                        helpNumber = 1;
                        questionNumber = 0;
                        score = 0;
                        playerScore = 0;
                        bonusScore = 0;
                    }
                }
            }

            return 0;
        }
    }
}
