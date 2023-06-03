#include <iostream>
#include <vector>
#include <unistd.h>
#include <string.h>
#include <thread>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <mutex>
#include "../protocol_header.h"
#include "../debugging.h"

using std::vector;
using std::thread;
using std::string;
using std::mutex;

/// A struct to represent a connection state
struct socket_info {
    int client_number;
    int sock_fd;
    sockaddr_in client_addr;
    char client_name[24];
};

/// Vector of current connections
vector<socket_info> current_connections;
mutex connection_lock;

/// @brief Validate the result of functions
/// @param err the output to validate
/// @param error_arrival an error message
void error_check(int err, const char *error_arrival) {
    if (err == -1) {
        perror(error_arrival);
        printf("Error Number: %d\n", errno);
        exit(-1);
    }
}

/// @brief Bind to the socket
/// @param sock_fd the socket
/// @param port_number the port number
void bind_socket(int sock_fd, short port_number) {
    // TODO: Bind to the socket
}

/// @brief Send a message
/// @param msg_type the message type
/// @param receiver_ip the receiver ip
/// @param sender_info the socket information
/// @param recv_msg the message
void send_msg(uint8_t msg_type, uint32_t receiver_ip, socket_info sender_info, char *recv_msg) {
    // Sending buffer
    char buffer[PACKET_SIZE];
    
    // TODO: Extract header and packet data
    
    if (receiver_ip == 0XFFFFFFFF) {
        // TODO: Send the message to each current connection
    }
    else {
        // TODO: Extract sender and receiver ip
        // TODO: Send a message to the receiver
    }
}

/// @brief Send a filename to the clients
/// @param sock_fd the socket to send on
/// @param filename the filename
/// @param type the type of packet to send in the header
void send_filename(int sock_fd, char* filename, uint8_t type) {
    // Create a buffer
    char filename_pkt[PACKET_SIZE];

    // TODO: Copy header into buffer
    // TODO: Copy data into buffer
    // TODO: Send buffer on socket
}

/// @brief Save a file on the server
/// @param s_info the socket information
/// @param filename the filename to save
void save_file(socket_info s_info, char* filename) {
    // Open the file
    FILE *fp;
    fp = fopen(filename, "w");

    if (fp) {
        // File buffer
        char file_buffer[PACKET_SIZE];

        int curr_iter = 0;
        while(true) {
            // TODO: Receive packet into the buffer
            int bytes_recv = 0; // The number of bytes received

            // TODO: Extract header and data

            // If defining the debugging flag, will print out each packet
            #ifdef DEBUGGING
            // TODO: Change 0 to size of data read
            pkt_hex_dump((unsigned char*) file_buffer, 0);
            #endif        

            // TODO: Validate packet type
            // TODO: Write the content into the file
            char* file_contents = "";
            fwrite(file_contents, bytes_recv, 1, fp);

            // Validate if file has more data
            if(bytes_recv != PACKET_SIZE)
                break;
        }
    }
    else {
        fprintf(stderr, "Error opening file.\n");
        return;
    }
    fclose(fp);
}

/// @brief Send a file from the server
/// @param s_info the socket information
/// @param filename the filename to send
void send_file(socket_info s_info, char* filename) {
    // Open the file
    FILE *fp;
    fp = fopen(filename, "r");

    if(fp) {
        // Send buffer
        char buffer[PACKET_SIZE];
        
        // TODO: Populate the send buffer
        
        // Calculate the size of the file
        fseek(fp, 0, SEEK_END);
        size_t fsize = ftell(fp);
        fseek(fp, 0, SEEK_SET);

        // Send the filename
        send_filename(s_info.sock_fd, filename, SERVER_SENDING_FILE_PKT);

        int curr_iter = 0;
        while(true) {
            // TODO: read acceptable number of bytes from file into data part of packet
            // fread(file_data, bytes_to_send, 1, fp);

            // TODO: send the packet, validate amount of bytes sent

            // Keep track of file contents already sent
            // curr_iter += bytes_to_send;
            
            // Break out of loop if file is completely read
            if(curr_iter == fsize)
                break;
        }
    }
    else {
        fprintf(stderr, "Error opening file.\n");
        return;
    }
    fclose(fp);
}

/// @brief Handler for receiving
/// @param s_info the socket info
void is_receiving(socket_info s_info) {
    // Receive buffer
    char buffer[PACKET_SIZE];

    while(true) {
        // Reset  buffer
        memset(buffer, 0, PACKET_SIZE);

        // TODO: Receive data
        int recv_len; // bytes received
        if (recv_len == 0){
            // thread safe way to remove the connection
            connection_lock.lock();
            printf("\nClient (%s) disconnected\n", s_info.client_name);
            for (auto it = current_connections.begin(); it != current_connections.end(); it++){
                if (it->client_number == s_info.client_number) {
                    current_connections.erase(it);
                    break;
                }
            }
            connection_lock.unlock();
            break;
        }

        int type; // header
        char* recv_msg; // message
        switch(type) {
            case BROADCAST_PKT: {
                send_msg(BROADCAST_PKT, 0XFFFFFFFF, s_info, recv_msg);
                printf("Broadcast message received: %s\n", recv_msg);
                break;
            }
            case DIRECTMESSAGE_PKT: {
                // TODO: Extract ip address and message from recv_msg
                int receiver_ip;
                send_msg(DIRECTMESSAGE_PKT, receiver_ip, s_info, recv_msg);
                printf("Direct message received for 0x%08X from %s: %s\n", htonl(receiver_ip), s_info.client_name, recv_msg);
                break;
            }
            case NOTIFY_UPLOAD_FILE_PKT: {
                printf("Notify Upload packet detected\n");
                char *filename = recv_msg;
                printf("File Uploading: %s\n", filename);
                save_file(s_info, filename);
                break;
            }
            case REQUEST_DOWNLOAD_FILE_PKT: {
                printf("Request Download packet detected\n");
                char *filename = recv_msg;
                printf("File Requested: %s\n", filename);
                send_file(s_info, filename);
                break;
            }
            default: {
                printf("Random packet detected\n");
                // If defining the debugging flag, will print out each packet
                #ifdef DEBUGGING
                // TODO: Change 0 to size of data read
                pkt_hex_dump((unsigned char*)buffer, recv_len);
                #endif
                break;
            }
        }
    }
}

static volatile int current_client_number = 0;

/// @brief A function that will start to accept connections
/// @param sock_fd 
void is_listening(int sock_fd) {
    int err;
    
    while (true) {
        // TODO: Listen on the socket (with a backlog of 10)
        // TODO: Accept a single connection
        // Increment client id
        current_client_number++;

        socket_info s_info;
        // TODO: Fill in the sock_fd and client_addr
        // s_info.sock_fd = ;
        // s_info.client_addr = ;
        s_info.client_number = current_client_number;

        // TODO: Receive the username
        // TODO: Add username to s_info

        printf("%s connected to the server.\n", s_info.client_name);
        printf("IP Address %s\n",inet_ntoa(s_info.client_addr.sin_addr));
        
        // Add the socket to the current connections
        current_connections.push_back(s_info);

        // Create a new thread
        thread receiving_thread(is_receiving, s_info);
        receiving_thread.detach();
    }
}

int main() {
    // Create a socket
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    error_check(sock_fd, "Socket Initialization");

    // Bind to the socket
    bind_socket(sock_fd, SERVER_PORT_NUMBER);

    // Starting a listening thread
    thread listening_thread(is_listening,sock_fd);
    listening_thread.detach();

    // Wait until CTRL-C is sent
    while(true);
}
