#include"inf151753_inf151247_comm.h"
#include <fcntl.h>
#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <wait.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/msg.h>
#include <string.h>
#include <stdbool.h>

struct activeUser{
    int pid;
    int queue;
    char nick[16];
};

struct activeGroupUser{
    char nick[16];
    char group[16];
    int queue;
};

volatile sig_atomic_t runProgram = 1;

struct activeUser emptyUser;
struct activeGroupUser emptyGroupUser;

struct activeUser activeUsers[16] = { 0 };
struct activeGroupUser activeGroupUsers[64]  = { 0 };

char usedNicks[16][16] = { 0 };
char groupNames[16][16] = { 0 };

void handleSigInt(int signum){
    runProgram = 0;
}

struct comm getMsg(int idIPC){
    struct msgbuf message;
    int sizeOfMsg = msgrcv(idIPC, &message, sizeof(message), 1, IPC_NOWAIT);
    message.msgContent.group[strcspn(message.msgContent.group, "\r\n")] = 0;
    message.msgContent.nick[strcspn(message.msgContent.nick, "\r\n")] = 0;
    message.msgContent.mess[strcspn(message.msgContent.mess, "\r\n")] = 0;
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

void sendWakeUpSignal(int childPid){
    kill(childPid, SIGUSR1);
}

void sendNoSpaceSignal(int childPid){
    kill(childPid, SIGUSR2);
}

int createPrivateQueue(int childPid){
    int privateQueue = msgget(childPid, 0666 | IPC_CREAT);
    printf("Private %d\n", privateQueue);

    return privateQueue;
}

void addUserToActiveUsers(struct comm msgContents, int privateQueue){
    int firstFreeIndex = -1;

    for (int i = 0; i<16; i++){
        if(activeUsers[i].pid == 0){
            firstFreeIndex = i;
            break;
        }
    }

    if(firstFreeIndex == -1)
        sendNoSpaceSignal(msgContents.pid);

    activeUsers[firstFreeIndex].pid = msgContents.pid;
    activeUsers[firstFreeIndex].queue = privateQueue;
}

void addNickToActiveUser(int userInd, char nick[16]){
    if(strlen(nick) == 0){
        sendError(activeUsers[userInd].queue, "Nick nie mo??e by?? pusty");
        return;
    }

    bool nickExists = false;
    for (int i = 0; i<16; i++){
        if(strcmp(usedNicks[i], nick) == 0){
            nickExists = true;
        }
    }

    if(nickExists == false){
        sendError(activeUsers[userInd].queue, "Podane konto nie istnieje");
        return;
    }
    

    for(int i=0; i<16; i++){
        if(strcmp(activeUsers[i].nick, nick) == 0){
            sendError(activeUsers[userInd].queue, "Ten u??ytkownik jest ju?? zalogowany");
            return;
        }
    }

    strncpy(activeUsers[userInd].nick, nick, sizeof(char)*16);

    for(int i=0; i<64; i++){
        if(strcmp(activeGroupUsers[i].nick, activeUsers[userInd].nick) == 0){
            activeGroupUsers[i].queue = activeUsers[userInd].queue;
        }
    }

    struct comm contents = {0};
    contents.pid = 0;
    contents.commType = 2;
    strncpy(contents.nick, nick, sizeof(char)*16);

    struct msgbuf message = {0};
    message.msgRecipient = 2;
    message.msgContent = contents;

    msgsnd(activeUsers[userInd].queue, &message, sizeof(message), 0);
}



void removeUserFromAllGroups(int userInd){
    for(int i=0; i<64; i++){
        if(activeGroupUsers[i].nick == activeUsers[userInd].nick){
            activeGroupUsers[i] = emptyGroupUser;
        }
    }
}

void logoutActiveUser(int childPid){
    for(int i=0; i<16; i++){
        if(activeUsers[i].pid == childPid){
            msgctl(activeUsers[i].queue, IPC_RMID, 0);
            removeUserFromAllGroups(i);
            activeUsers[i] = emptyUser;
            kill(childPid, SIGQUIT);
            return;
        }
    }
}

void sendListOfUsers(int userInd){
    char list[256] = { 0 };
    int lastPos = 0;
    for(int i=0; i<16; i++){
        if(strcmp(usedNicks[i], "") != 0){
            strncpy(list+lastPos, usedNicks[i], sizeof(char)*16);
            lastPos += 16;
        }
    }

    struct comm contents = {0};
    contents.pid = 0;
    contents.commType = 4;
    memcpy(contents.mess, list, sizeof(list));

    struct msgbuf message = {0};
    message.msgRecipient = 2;
    message.msgContent = contents;

    msgsnd(activeUsers[userInd].queue, &message, sizeof(message), 0);
}

void sendListOfGroupUsers(int userInd, char groupName[16]){
    if(strlen(groupName) == 0){
        sendError(activeUsers[userInd].queue, "Grupa nie mo??e by?? pusta");
        return;
    }

    char list[256] = {0};
    int lastPos = 0;
    for(int i=0; i<16; i++){
        if(strcmp(activeGroupUsers[i].nick, "") != 0){
            strncpy(list+lastPos, activeGroupUsers[i].nick, sizeof(char)*16);
            lastPos += 16;
        }
    }

    struct comm contents = {0};
    contents.pid = 0;
    contents.commType = 5;
    strncpy(contents.group, groupName, sizeof(char)*16);
    memcpy(contents.mess, list, sizeof(char)*256);

    struct msgbuf message = {0};
    message.msgRecipient = 2;
    message.msgContent = contents;

    msgsnd(activeUsers[userInd].queue, &message, sizeof(message), 0);
}

void addUserToGroup(int userInd, char groupName[16]){
    if(strlen(groupName) == 0){
        sendError(activeUsers[userInd].queue, "Grupa nie mo??e by?? pusta");
        return;
    }

    bool groupExists = false;
    int firstFreeIndex = -1;

    for(int i=0; i<16; i++){
        if(strcmp(groupNames[i],groupName) == 0){
            groupExists = true;
        }
    }

    if(groupExists == false){
        sendError(activeUsers[userInd].queue, "Taka grupa nie istnieje");
        return;
    }

    for(int i=0; i<64; i++){
        if(strcmp(activeGroupUsers[i].nick, "") == 0){
            firstFreeIndex = i;
            break;
        }else if(strcmp(activeGroupUsers[i].nick, activeUsers[userInd].nick) == 0){
            sendError(activeUsers[userInd].queue, "Ju?? nale??ysz do tej grupy");
            return;
        }
    }

    if(firstFreeIndex == -1){
        sendError(activeUsers[userInd].queue, "Maksymalna liczba u??ytkownik??w jest ju?? dodana do grup");
        return;
    }

    strncpy(activeGroupUsers[firstFreeIndex].nick, activeUsers[userInd].nick, sizeof(char)*16);
    strncpy(activeGroupUsers[firstFreeIndex].group, groupName, sizeof(char)*16);
    activeGroupUsers[firstFreeIndex].queue =  activeUsers[userInd].queue;

    sendSuccess(activeUsers[userInd].queue, "Dodano do grupy");
}

void removeUserFromGroup(int userInd, char groupName[16]){
    if(strlen(groupName) == 0){
        sendError(activeUsers[userInd].queue, "Grupa nie mo??e by?? pusta");
        return;
    }

    for(int i=0; i<64; i++){
        if(activeGroupUsers[i].group == groupName && activeGroupUsers[i].nick == activeUsers[userInd].nick){
            activeGroupUsers[i] = emptyGroupUser;
        }
    }
    sendSuccess(activeUsers[userInd].queue, "Usuni??to z grupy");
}

void sendGroupList(int userInd){
    char list[256] = { 0 };
    int lastPos = 0;
    for(int i=0; i<16; i++){
        if(strcmp(groupNames[i], "") != 0){
            strncpy(list+lastPos, groupNames[i], sizeof(char)*16);
            lastPos += 16;
        }
    }

    if(lastPos == 0){
        sendError(activeUsers[userInd].queue, "Nie istnieje ??adna grupa");
        return;
    }

    struct comm contents = {0};
    contents.pid = 0;
    contents.commType = 8;
    memcpy(contents.mess, list, sizeof(list));

    struct msgbuf message = {0};
    message.msgRecipient = 2;
    message.msgContent = contents;

    msgsnd(activeUsers[userInd].queue, &message, sizeof(message), 0);
}

void sendPrivateMessage(int userInd, char targetNick[16], char mess[256]){
    if(strlen(targetNick) == 0){
        sendError(activeUsers[userInd].queue, "Nick nie mo??e by?? pusty");
        return;
    }

    int queue = -1;

    for(int i=0; i<16; i++){
        if(strcmp(activeUsers[i].nick, targetNick)==0){
            queue = activeUsers[i].queue;
            break;
        }
    }

    if(queue == -1){
        sendError(activeUsers[userInd].queue, "Ten u??ytkownik nie jest zalogowany");
        return;
    }
    
    struct comm contents = {0};
    contents.pid = 0;
    contents.commType = 9;
    strncpy(contents.nick, activeUsers[userInd].nick, sizeof(char)*16);
    memcpy(contents.mess, mess, sizeof(char)*256);
    
    struct msgbuf message = {0};
    message.msgRecipient = 2;
    message.msgContent = contents;

    if(msgsnd(queue, &message, sizeof(message), 0) == -1){
        sendError(activeUsers[userInd].queue, "Wiadomo???? nie zosta??a dostarczona");
        return;
    }
}

void sendGroupMessage(int userInd, char groupName[16], char mess[256]){
    if(strlen(groupName) == 0){
        sendError(activeUsers[userInd].queue, "Grupa nie mo??e by?? pusta");
        return;
    }

    bool errorOccured = false;

    for(int i=0; i<16; i++){
        if(strcmp(activeGroupUsers[i].group, groupName)==0){
            struct comm contents = {0};
            contents.pid = 0;
            contents.commType = 10;
            strncpy(contents.nick, activeUsers[userInd].nick, sizeof(char)*16);
            strncpy(contents.group, groupName, sizeof(char)*16);
            strncpy(contents.mess, mess, sizeof(char)*256);
            
            struct msgbuf message = {0};
            message.msgRecipient = 2;
            message.msgContent = contents;

            if(msgsnd(activeGroupUsers[i].queue, &message, sizeof(message), 0)==-1){
                errorOccured = true;
            }
        }
    }
    
    if(errorOccured){
        sendError(activeUsers[userInd].queue, "Podczas wysy??ania wyst??pi?? b????d. Cz?????? u??ytkownik??w mog??a nie odebra?? wiadomo??ci");
        return;
    }
}

void addNickToUsedNicks(char nick[16]){  
    if(strlen(nick) == 0){
        printf("Nick nie mo??e by?? pusty\n");
        return;
    }
    int firstFreeIndex = -1;

    for(int i=0; i<16; i++){
        if(strcmp(usedNicks[i], "") == 0){
            firstFreeIndex = i;
            break;
        }else if(strcmp(usedNicks[i], nick) == 0){
            printf("Ten nick: %s, jest ju?? u??ywany\n", nick);
            return;
        }
    }

    strncpy(usedNicks[firstFreeIndex], nick, sizeof(char)*16);
    printf("Dodano u??ytkownika %s\n", nick);
}

void addGroupToGroupNames(char groupName[16]){
    if(strlen(groupName) == 0){
        printf("Grupa nie mo??e by?? pusta\n");
        return;
    }

    int firstFreeIndex = -1;

    for(int i=0; i<16; i++){
        if(strcmp(groupNames[i], "") == 0){
            firstFreeIndex = i;
            break;
        }else if(strcmp(groupNames[i], groupName) == 0){
            printf("Ta grupa: %s, ju?? istnieje\n", groupName);
            return;
        }
    }

    strncpy(groupNames[firstFreeIndex], groupName, sizeof(char)*16);
    printf("Dodano grup?? %s\n", groupName);
}


void configure(char* confFileName){
    FILE* fd = fopen(confFileName, "r");

    if(fd == NULL){
        printf("Nie uda??o si?? odczyta?? pliku konfiguracyjnego\n");
        return;
    }

    char option[16] = {0};
    char value[16] = {0};

    while(fscanf(fd, "%s = %s\n", option, value) == 2){
        if(strstr(option, "user") != NULL){
            addNickToUsedNicks(value);
        }else if(strstr(option, "group") != NULL){
            addGroupToGroupNames(value);
        }else{
            printf("Nieznana opcja: %s, nie uda??o si?? wczyta?? linii konfiguracyjnej\n", option);
        }
        
    }
}

void handlePrivateMsg(int userInd){
    struct comm privateMsgContents = getMsg(activeUsers[userInd].queue);
    if (privateMsgContents.pid != -1){
        if(privateMsgContents.commType == 2){
            printf("Pr??ba zalogowania u??ytkownika %s\n", privateMsgContents.nick);
            addNickToActiveUser(userInd, privateMsgContents.nick);
        }else if(privateMsgContents.commType == 3){
            printf("Pr??ba wylogowania u??ytkownika %s\n", activeUsers[userInd].nick);
            logoutActiveUser(privateMsgContents.pid);
        }
        else{
            if(strcmp(activeUsers[userInd].nick, "") != 0){
                switch(privateMsgContents.commType){
                    case 4:
                        printf("Pr??ba wys??ania listy u??ytkownik??w do %s\n", activeUsers[userInd].nick);
                        sendListOfUsers(userInd);
                        break;
                    case 5:
                        printf("Pr??ba wys??ania listy u??ytkownik??w w grupie\n");
                        sendListOfGroupUsers(userInd, privateMsgContents.group);
                        break;
                    case 6:
                        printf("Pr??ba zapisu do grupy\n");
                        addUserToGroup(userInd, privateMsgContents.group);
                        break;
                    case 7:
                        printf("Pr??ba wypisania z grupy\n");
                        removeUserFromGroup(userInd, privateMsgContents.group);
                        break;
                    case 8:
                        printf("Pr??ba wys??ania listy grup\n");
                        sendGroupList(userInd);
                        break;
                    case 9:
                        printf("Pr??ba wys??ania wiadomo??ci\n");
                        sendPrivateMessage(userInd, privateMsgContents.nick, privateMsgContents.mess);
                        break;
                    case 10:
                        printf("Pr??ba wys??ania wiadomo??ci grupowej\n");
                        sendGroupMessage(userInd, privateMsgContents.group, privateMsgContents.mess);
                        break;
                }
            }else{
                printf("Pr??ba wywo??ania komendy bez nicku\n");
                sendError(activeUsers[userInd].queue, "Aby wykonywa?? komendy nale??y si?? zalogowa?? (komenda \"login\")");
            }
        }
    }
}

int main(){
    signal(SIGINT, handleSigInt);
    int publicQueue = msgget(151, 0666 | IPC_CREAT);
    printf("Public %d\n", publicQueue);

    configure("init.conf");

    while(runProgram){
        struct comm publicMsgContents = getMsg(publicQueue);
        if (publicMsgContents.pid != -1){
            int newPrivateQueue = createPrivateQueue(publicMsgContents.pid);
            sendWakeUpSignal(publicMsgContents.pid);
            addUserToActiveUsers(publicMsgContents, newPrivateQueue);
            printf("Dodano pid: %d\n", publicMsgContents.pid);
        }

        for(int i=0; i<16; i++){
            if(activeUsers[i].pid != 0){
                handlePrivateMsg(i);
            }
        }
    }

    for(int i=0; i<16; i++){
        if(activeUsers[i].queue != 0){
            logoutActiveUser(activeUsers[i].pid);
            msgctl(activeUsers[i].queue, IPC_RMID, 0);
        }
            
    }
    msgctl(publicQueue, IPC_RMID, 0);

    return 0;
}