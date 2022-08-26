#include <iostream>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "../protocol_header.h"
#include "../debugging.h"

#define BACKLOG 10
#define CLIENT_IP 0x7F000001

using std::thread;
using std::string;
using std::cin;
using std::cout;

void error_check(int err, const char *error_arrival) {
    if (err == -1) {
        perror(error_arrival);
        printf("Error Number: %d\n", errno);
        exit(-1);
    }
}

void connect_socket(int sock_fd, short port_number) {
    sockaddr_in s_addr;
    s_addr.sin_addr.s_addr = INADDR_ANY;
    s_addr.sin_family = AF_INET;
    s_addr.sin_port = htons(port_number);

    int err = connect(sock_fd, (sockaddr*)&s_addr, sizeof(s_addr));
    error_check(err, "Socket Connecting");
    
    char username[CLIENT_NAME_SIZE];
    printf("Enter your name: ");
    cin.getline(username, CLIENT_NAME_SIZE);
    
    err = send(sock_fd, username, CLIENT_NAME_SIZE, 0);
    error_check(err, "Socket Sending Name");
}

void send_msg(int sock_fd, uint8_t msg_type, uint32_t receiver_ip, char *msg_to_send) {
    char buffer[PACKET_SIZE];
    
    our_header *oh = (our_header*)buffer;
    oh->type = msg_type;
    
    int bytes_sent;
    if (msg_type == BROADCAST_PKT) {
        char *pkt_msg = buffer + szOH;
        strcpy(pkt_msg, msg_to_send);

        bytes_sent = send(sock_fd, buffer, szOH + strlen(pkt_msg), 0);
        error_check(bytes_sent, "Socket Sending Message");
    }
    else {
        dm_header *dmh = (dm_header*)(oh + szOH);
        dmh->receiver_ip = receiver_ip;
        dmh->sender_ip = htonl(CLIENT_IP);

        char *pkt_msg = (char*)dmh + szDMH;
        strcpy(pkt_msg, msg_to_send);
        
        bytes_sent = send(sock_fd, buffer, szOH + szDMH + strlen(pkt_msg), 0);
        error_check(bytes_sent, "Socket Sending Message");
    }
}

void send_filename(int sock_fd, char* filename, uint8_t type) {
    char filename_pkt[PACKET_SIZE];
    
    our_header *filename_pkt_oh = (our_header*)filename_pkt;
    filename_pkt_oh->type = type;
    
    char *fname = filename_pkt + szOH;
    strcpy(fname, filename);
    
    int filename_sent = send(sock_fd, filename_pkt, PACKET_SIZE, 0);
    error_check(filename_sent, "Socket Sending Filename");
}

void send_file(int sock_fd, char* filename) {
    FILE *fp;
    fp = fopen(filename, "r");

    if(fp) {
        char buffer[PACKET_SIZE];
        
        our_header *oh = (our_header*)buffer;
        oh->type = FILE_DATA_PKT;

        char *file_data = buffer + szOH;
        
        send_filename(sock_fd, filename, NOTIFY_UPLOAD_FILE_PKT);
        printf("filename sent\n");

        fseek(fp, 0, SEEK_END);
        printf("reached end\n");
        size_t fsize = ftell(fp);
        printf("filesize read\n");

        fseek(fp, 0, SEEK_SET);
            printf("reached start\n");

        int curr_iter = 0;
        while(true) {
            int bytes_to_send = (fsize-curr_iter)>=(PACKET_SIZE-szOH)?(PACKET_SIZE-szOH):fsize-curr_iter;
            //printf("data read\n");

            fread(file_data, bytes_to_send, 1, fp);

            int bytes_sent = send(sock_fd, buffer, szOH + bytes_to_send, 0);
            error_check(bytes_sent, "Socket Sending Data");

            curr_iter += bytes_to_send;
            if(bytes_sent != PACKET_SIZE || curr_iter == fsize)
                break;
        }
    }
    else {
        fprintf(stderr, "Error opening file.\n");
        return;
    }
    fclose(fp);
}

void save_file(int sock_fd, char* filename) {
    FILE *fp;
    fp = fopen(filename, "w");

    if (fp) {
        char file_buffer[PACKET_SIZE];
        char *file_contents = file_buffer + szOH;

        our_header* oh = (our_header*)file_buffer; 
        while(true) {
            int bytes_to_write = recv(sock_fd, file_buffer, PACKET_SIZE, 0);
            error_check(bytes_to_write, "Socket Receiving Data");

#ifdef DEBUGGING
pkt_hex_dump((unsigned char*)file_buffer, bytes_to_write);
#endif        

            if (oh->type != FILE_DATA_PKT) {
                printf("Expecting Data Packets atm.\n");
                continue;
            }

            fwrite(file_contents, bytes_to_write - szOH, 1, fp);
            if(bytes_to_write != PACKET_SIZE)
                break;
        }
    }
    else {
        fprintf(stderr, "Error opening file.\n");
        return;
    }
    printf("File saved.\n");
    fclose(fp);
}

void print_options() {
    cout << "\nOptions\n"
            "1. Broadcast Message\n"
            "2. Direct Message\n"
            "3. File Upload\n"
            "4. File Download\n"
            "Choose an Option: ";
    fflush(stdout);
}

void is_sending(int sock_fd) {
    int user_choice;
    while (true) {
        print_options();
        cin >> user_choice;
        cin.ignore(1,'\n');

        switch (user_choice) {
        case BROADCAST_PKT: {
            char msg_to_send[PACKET_SIZE - szOH];
            memset(msg_to_send, 0, PACKET_SIZE - szOH);

            cout << "\nMessage: ";
            cin.getline(msg_to_send, PACKET_SIZE - szOH);
            
            send_msg(sock_fd, BROADCAST_PKT, 0x00000000, msg_to_send);
            break;
        }
        case DIRECTMESSAGE_PKT: {
            char msg_to_send[PACKET_SIZE - szOH - szDMH];
            memset(msg_to_send, 0, PACKET_SIZE - szOH - szDMH);

            cout << "\nMessage: ";
            cin.getline(msg_to_send, PACKET_SIZE - szOH);
            
            char client_ip[16];
            cout << "Receiver IP: ";
            cin.getline(client_ip, 16);
            uint32_t client_ip_addr;
            inet_aton(client_ip,(in_addr*)&client_ip_addr);

            send_msg(sock_fd, DIRECTMESSAGE_PKT, client_ip_addr, msg_to_send);
            break;
        }
        case NOTIFY_UPLOAD_FILE_PKT: {
            char filename[FILENAME_MAX];
            memset(filename, 0, FILENAME_MAX);

            cout << "\nEnter Filename: ";
            cin.getline(filename, FILENAME_MAX);
            send_file(sock_fd, filename);
            break;
        }
        case REQUEST_DOWNLOAD_FILE_PKT: {
            char filename[FILENAME_MAX];
            memset(filename, 0, FILENAME_MAX);

            cout << "\nEnter Filename: ";
            cin.getline(filename, FILENAME_MAX);
            send_filename(sock_fd, filename, REQUEST_DOWNLOAD_FILE_PKT);
            break;
        }
        default:
            break;
        }
    }
}

void is_receiving(int sock_fd) {
    char buffer[PACKET_SIZE];
    our_header *oh = (our_header*)buffer;
    char *recv_msg;
    while(true) {
        memset(buffer, 0, PACKET_SIZE);

        int recv_len = recv(sock_fd, buffer, PACKET_SIZE, 0);
        error_check(recv_len, "Socket Receiving");
        switch(oh->type) {
        case BROADCAST_PKT: {
            recv_msg = buffer + szOH;
            printf("\n%s\n", recv_msg);
            print_options();
            break;
        }
        case DIRECTMESSAGE_PKT: {
            dm_header *dmh = (dm_header*)(buffer + szOH);
            unsigned char *sender_ip = (unsigned char*)&dmh->sender_ip;
            recv_msg = (char*)dmh + szDMH;
            printf("\n[DIRECT MESSAGE](%u.%u.%u.%u) %s\n", (int)sender_ip[0], (int)sender_ip[1], (int)sender_ip[2], (int)sender_ip[3], recv_msg);
            print_options();
            break;
        }
        case SERVER_SENDING_FILE_PKT: {
            char* filename = (char*)oh + szOH;
            save_file(sock_fd, filename);
            break;
        }
        default: {
            printf("Random packet detected\n");
#ifdef DEBUGGING
    pkt_hex_dump((unsigned char*)buffer, recv_len);
#endif
            break;
        }
        }
    }
}

int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    error_check(sock_fd, "Socket Initialization");

    connect_socket(sock_fd, SERVER_PORT_NUMBER);

    std::thread sending_thread(is_sending, sock_fd);
    sending_thread.detach();

    std::thread recvThread(is_receiving, sock_fd);
    recvThread.detach();

    while(true);
}
