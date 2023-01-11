#include"inf151753_inf151247_comm.h"
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <errno.h>
#include <string.h>

int fromFork;
int publicQueue;
int privateQueue;
char loggedin[16] = "niezalogowany";

void welcomeUser(int signum){
    printf("Witamy! Aby zobaczyć liczbę komend wpisz \"help\"\n");
}

void handleSrvInt(int signum){
    printf("Wylogowano\n");
    kill(fromFork, 9);
    exit(0);
}

void noSpaceError(int signum){
    printf("Błąd: Aktualnie zalogowana jest maksymalna liczba użytkowników. Spróbój ponownie później\n");
}
struct comm getMsg(int idIPC){
    struct msgbuf message;
    int sizeOfMsg = msgrcv(idIPC, &message, sizeof(message), 2, 0);
    if(sizeOfMsg == -1){
        struct comm noMsg;
        noMsg.pid = -1;
        return noMsg;
    }

    return message.msgContent;
}

void sendSuccess(int queue, char* mess){
    struct comm contents = {0};
    contents.pid = 0;
    contents.commType = 0;
    strncpy(contents.mess, mess, sizeof(char)*256);

    struct msgbuf message = {0};
    message.msgRecipient = 2;
    message.msgContent = contents;

    msgsnd(queue, &message, sizeof(message), 0);
}

void sendError(int queue, char* mess){
    struct comm contents = {0};
    contents.pid = 0;
    contents.commType = -1;
    strncpy(contents.mess, mess, sizeof(char)*256);

    struct msgbuf message = {0};
    message.msgRecipient = 2;
    message.msgContent = contents;

    msgsnd(queue, &message, sizeof(message), 0);
}

int login(){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 1;

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(publicQueue, &message, sizeof(message), 0);
    pause();

    if(msgsndOutput == 0){
        privateQueue = msgget(getpid(), 0666);
        return 0;
    }

    return 1;
}

int sendNick(char nick[16]){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 2;
    strncpy(contents.nick, nick, sizeof(char)*16);

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

int logout(){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 3;

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

void logoutFromSig(int signum){
    logout();
}

int askForActiveUserList(){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 4;

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

int askForGroupUserList(char groupName[16]){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 5;
    strncpy(contents.group, groupName, sizeof(char)*16);

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

int joinGroup(char groupName[16]){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 6;
    strncpy(contents.group, groupName, sizeof(char)*16);

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

int leaveGroup(char groupName[16]){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 7;
    strncpy(contents.group, groupName, sizeof(char)*16);

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

int askForGroupList(){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 8;

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

int askForHelp(){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 11;

    struct msgbuf message = {0};
    message.msgRecipient = 2;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

int sendPrivateMessage(char nick[16], char mess[256]){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 9;
    strncpy(contents.nick, nick, sizeof(char)*16);
    strncpy(contents.mess, mess, sizeof(char)*256);

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

int sendGroupMessage(char groupName[16], char mess[256]){
    struct comm contents = {0};
    contents.pid = getpid();
    contents.commType = 10;
    strncpy(contents.group, groupName, sizeof(char)*16);
    strncpy(contents.mess, mess, sizeof(char)*256);

    struct msgbuf message = {0};
    message.msgRecipient = 1;
    message.msgContent = contents;

    int msgsndOutput = msgsnd(privateQueue, &message, sizeof(message), 0);

    return msgsndOutput;
}

void printList(char mess[256]){
    for(int i=0; i<16; i++){
            char string[16];
            strncpy(string, &mess[16*i], sizeof(char)*16);
            if(strlen(string) > 0)
                printf("%s\n", string);
        }   
}

void handleChoice(char choice[16]){
    int out;
    if(strstr(choice, "help") != NULL){
        out = askForHelp();
    }else if(strstr(choice, "logout") != NULL){
        out = logout();
    }else if(strstr(choice, "login") != NULL){
        printf("Podaj swój nick: ");
        char nick[16];
        fgets(nick, 16, stdin);

        out = sendNick(nick);
    }else if(strstr(choice, "groupuserlist") != NULL){
        printf("Podaj nazwę grupy: ");
        char group[16];
        fgets(group, 16, stdin);
        
        out = askForGroupUserList(group);
    }else if(strstr(choice, "userlist") != NULL){
        out = askForActiveUserList();
    }else if(strstr(choice, "grouplist") != NULL){
        out = askForGroupList();
    }else if(strstr(choice, "join") != NULL){
        printf("Podaj nazwę grupy: ");
        char group[16];
        fgets(group, 16, stdin);
        
        out = joinGroup(group);
    }else if(strstr(choice, "leave") != NULL){
        printf("Podaj nazwę grupy: ");
        char group[16];
        fgets(group, 16, stdin);
        
        out = leaveGroup(group);
    }else if(strstr(choice, "sendgroup") != NULL){
        printf("Podaj nazwę grupy: ");
        char group[16];
        fgets(group, 16, stdin);
        
        printf("Wpisz wiadomość: ");
        char mess[256];
        fgets(mess, 256, stdin);

        out = sendGroupMessage(group, mess);
    }else if(strstr(choice, "send") != NULL){
        printf("Podaj nick odbiorcy: ");
        char nick[16];
        fgets(nick, 16, stdin);

        printf("Wpisz wiadomość: ");
        char mess[256];
        fgets(mess, 256, stdin);

        out = sendPrivateMessage(nick, mess);
    }else{
        sendError(privateQueue, "Nie ma takiej komendy. Skorzystaj z \"help\", aby uzyskać listę komend.");
    }

    if(out == -1){
        sendError(privateQueue, "Wystąpił błąd podczas wysyłania wiadomości.");
    }
}

int handleIncoming(struct comm msgContent){
    if(msgContent.commType == 0){
        printf("Sukces: %s\n", msgContent.mess);
    }else if(msgContent.commType == -1){
        printf("Błąd: %s\n", msgContent.mess);
    }else if(msgContent.commType == 2){
        printf("Sukces: Zalogowano jako %s\n", msgContent.nick);
        strncpy(loggedin,msgContent.nick, sizeof(char)*16);
    }else if(msgContent.commType == 4){
        printf("Lista użytkowników:\n");
        printList(msgContent.mess);
    }else if(msgContent.commType == 5){
        printf("Lista użytkowników w grupie %s:\n", msgContent.group);
        printList(msgContent.mess);
    }else if(msgContent.commType == 8){
        printf("Lista grup:\n");
        printList(msgContent.mess);
    }else if(msgContent.commType == 9){
        printf("%s: ", msgContent.nick);
        printf("%s\n", msgContent.mess);
    }else if(msgContent.commType == 10){
        printf("[%s] ",msgContent.group);
        printf("%s: ", msgContent.nick);
        printf("%s\n", msgContent.mess);
    }else if(msgContent.commType == 11){
        printf("Lista komend:\n");
        printf("login - logowanie do serwera\n");
        printf("logout - wylogowanie z serwera\n");
        printf("userlist - lista użytkowników \n");
        printf("grouplist - lista grup\n");
        printf("join - dołączenie do grupy\n");
        printf("leave - opuszczenie grupy\n");
        printf("groupuserlist - lista użytkowników w danej grupie\n");
        printf("send - wysłanie wiadomości prywatnej\n");
        printf("sendgroup - wysłanie wiadomości grupowej\n");
    }else{
        return 1;
    }

    return 0;
}


int main(){
    signal(SIGUSR1, welcomeUser);
    signal(SIGUSR2, noSpaceError);
    publicQueue = msgget(151, 0666);

    if(publicQueue == -1){
        if(errno == ENOENT){
            printf("Błąd: Serwer nie jest gotowy do komunikacji, spróbuj ponownie później.\n");
        }else{
            printf("Błąd: Wystąpił niespodziewany błąd, spróbuj ponownie później.\n");
        }

        return 1;
    }

    login(publicQueue);

    fromFork = fork();

    if(fromFork!=0){
        signal(SIGINT, logoutFromSig);
        signal(SIGQUIT, handleSrvInt);
        printf("%s: ", loggedin);
                fflush(stdout);
        while(1){
            char choice[16];
            fgets(choice, 16, stdin);
            handleChoice(choice);
        }
    }else{
        while(1){
            struct msgbuf message = {0};
            
            if(msgrcv(privateQueue, &message, sizeof(message), 2, 0) > 0){
                handleIncoming(message.msgContent);
                printf("%s: ", loggedin);
                fflush(stdout);
            }
        }
    }

}