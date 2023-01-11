struct comm {
    int pid; // pid
    int commType; // numer komunikatu
    char nick[16]; 
    char group[16];
    char mess[256]; // treść wiadomości
};

struct msgbuf {
    long msgRecipient; // 1 - serwer, 2 - klient
    struct comm msgContent;
};