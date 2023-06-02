#define PACKET_SIZE 1024
#define SERVER_PORT_NUMBER 8080
#define CLIENT_NAME_SIZE 24
#define BROADCAST_PKT 1
#define DIRECTMESSAGE_PKT 2
#define NOTIFY_UPLOAD_FILE_PKT 3
#define REQUEST_DOWNLOAD_FILE_PKT 4
#define FILE_DATA_PKT 5
#define SERVER_SENDING_FILE_PKT 6

struct our_header {
    int type;
};

struct dm_header {
    int sender_ip;
    int receiver_ip;
};

#define szOH sizeof(our_header)
#define szDMH sizeof(dm_header)