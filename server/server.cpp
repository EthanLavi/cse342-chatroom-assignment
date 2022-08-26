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

using std::vector;
using std::thread;
using std::string;

struct socket_info {
    int client_number;
    int sock_fd;
    sockaddr_in client_addr;
    char client_name[24];
};
vector<socket_info> current_connections;

void error_check(int err, const char *error_arrival) {
    if (err == -1) {
        perror(error_arrival);
        printf("Error Number: %d\n", errno);
        exit(-1);
    }
}

void bind_socket(int sock_fd, short port_number) {
    sockaddr_in b_addr;
    b_addr.sin_addr.s_addr = INADDR_ANY;
    b_addr.sin_family = AF_INET;
    b_addr.sin_port = htons(port_number);

    int err = bind(sock_fd, (sockaddr*)&b_addr, sizeof(b_addr));
    error_check(err, "Socket Binding");
}

void send_msg(uint8_t msg_type, uint32_t receiver_ip, socket_info sender_info, char *recv_msg) {
    char buffer[PACKET_SIZE];
    
    our_header *oh = (our_header*)buffer;
    oh->type = msg_type;

    char *msg;
    
    string client_name_string(sender_info.client_name);
    string msg_string(recv_msg);
    string out(string("\n") + client_name_string + string(" : ") + msg_string);
    
    if (receiver_ip == 0XFFFFFFFF) {
        msg = (char*)oh + szOH;
        strcpy(msg, out.c_str());

        for (int i = 0; i < current_connections.size(); i++) {
            if(current_connections[i].sock_fd == sender_info.sock_fd)
                continue;

            send(current_connections[i].sock_fd, buffer, szOH + out.length(), 0);
        }
    }
    else {
        dm_header *dmh = (dm_header*)(oh + szOH);
        dmh->sender_ip = sender_info.client_addr.sin_addr.s_addr;
        dmh->receiver_ip = receiver_ip;

        msg = (char*)dmh + szDMH;
        strcpy(msg, out.c_str());

        for (int i = 0; i < current_connections.size(); i++) {
            if(current_connections[i].client_addr.sin_addr.s_addr != receiver_ip)
                continue;

            send(current_connections[i].sock_fd, buffer, szOH + szDMH + out.length(), 0);
        }
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

void save_file(socket_info s_info, char* filename) {
    FILE *fp;
    fp = fopen(filename, "w");

    if (fp) {
        char file_buffer[PACKET_SIZE];
        char *file_contents = file_buffer + szOH;

        our_header* oh = (our_header*)file_buffer; 
        while(true) {
            int bytes_to_write = recv(s_info.sock_fd, file_buffer, PACKET_SIZE, 0);
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
    fclose(fp);
}

void send_file(socket_info s_info, char* filename) {
    FILE *fp;
    fp = fopen(filename, "r");

    if(fp) {
        char buffer[PACKET_SIZE];
        
        our_header *oh = (our_header*)buffer;
        oh->type = FILE_DATA_PKT;

        char *file_data = buffer + szOH;
        
        fseek(fp, 0, SEEK_END);
        size_t fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        send_filename(s_info.sock_fd, filename, SERVER_SENDING_FILE_PKT);
        int curr_iter = 0;
        while(true) {
            int bytes_to_send = (fsize-curr_iter)>=(PACKET_SIZE-szOH)?(PACKET_SIZE-szOH):fsize-curr_iter;
            fread(file_data, bytes_to_send, 1, fp);

            int bytes_sent = send(s_info.sock_fd, buffer, szOH + bytes_to_send, 0);
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

void is_receiving(socket_info s_info) {
    char buffer[PACKET_SIZE];
    our_header *oh = (our_header*)buffer;
    char *recv_msg;
    while(true) {
        memset(buffer, 0, PACKET_SIZE);

        int recv_len = recv(s_info.sock_fd, buffer, PACKET_SIZE, 0);
        error_check(recv_len, "Socket Receiving");
        switch(oh->type) {
            case BROADCAST_PKT: {
                recv_msg = buffer + szOH;
                send_msg(oh->type, 0XFFFFFFFF, s_info, recv_msg);
                printf("Broadcast message received: %s\n", recv_msg);
                break;
            }
            case DIRECTMESSAGE_PKT: {
                dm_header *dmh = (dm_header*)(buffer + szOH);
                recv_msg = (char*)dmh + szDMH;
                send_msg(oh->type, dmh->receiver_ip, s_info, recv_msg);
                printf("Direct message received for 0x%08X from %s: %s\n", htonl(dmh->receiver_ip), s_info.client_name, recv_msg);
                break;
            }
            case NOTIFY_UPLOAD_FILE_PKT: {
                printf("Notify Upload packet detected\n");
                char *filename = buffer + szOH;
                printf("File Uploading: %s\n", filename);
                save_file(s_info, filename);
                break;
            }
            case REQUEST_DOWNLOAD_FILE_PKT: {
                printf("Request Download packet detected\n");
                char *filename = buffer + szOH;
                printf("File Requested: %s\n", filename);
                send_file(s_info, filename);
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

void is_listening(int sock_fd) {
    int err;
    
    while (true) {
        static int current_client_number = 0;
        err = listen(sock_fd, BACKLOG);
        error_check(err, "Socket Listening");

        sockaddr_in c_addr;
        socklen_t c_len;

        int server_client_socket = accept(sock_fd, (sockaddr*)&c_addr, &c_len);
        error_check(server_client_socket, "Socket Accepting");
        current_client_number++;

        socket_info s_info;
        s_info.sock_fd = server_client_socket;
        s_info.client_addr = c_addr;
        s_info.client_number = current_client_number;

        err = recv(server_client_socket, s_info.client_name, CLIENT_NAME_SIZE, 0);
        error_check(err, "Socket Receiving Name");

        printf("%s connected to the server.\n", s_info.client_name);
        printf("IP Address %s\n",inet_ntoa(s_info.client_addr.sin_addr));
        current_connections.push_back(s_info);

        thread receiving_thread(is_receiving, s_info);
        receiving_thread.detach();
    }
}

int main() {
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    error_check(sock_fd, "Socket Initialization");

    bind_socket(sock_fd, SERVER_PORT_NUMBER);

    thread listening_thread(is_listening,sock_fd);
    listening_thread.detach();

    while(true);
}
