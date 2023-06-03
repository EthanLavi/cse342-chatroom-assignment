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

using std::thread;
using std::string;
using std::cin;
using std::cout;

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

/// @brief Connect to a socket and send over your username
/// @param sock_fd the socket to connect to
/// @param port_number the port to connect on
void connect_socket(int sock_fd, short port_number) {
    // TODO: Connect to socket
    
    char username[CLIENT_NAME_SIZE];
    printf("Enter your name: ");
    cin.getline(username, CLIENT_NAME_SIZE);
    
    // TODO: Send server your username
}

/// @brief Send a message (both direct and broadcast) to the server
/// @param sock_fd the socket to send on
/// @param msg_type the type of message
/// @param receiver_ip the receiving ip
/// @param msg_to_send the message to send
void send_msg(int sock_fd, uint8_t msg_type, uint32_t receiver_ip, char *msg_to_send) {
    char buffer[PACKET_SIZE];
    
    // TODO: Set the packet headers into the buffer
    
    int bytes_sent;
    if (msg_type == BROADCAST_PKT) {
        // TODO: Set the data of the packet to be just the message
        // TODO: Send the packet
    }
    else {
        // TODO: Set the ip addresses into the packet 
        // TODO: Set the data of the packet to be the message
        // TODO: Send the packet
    }
}

/// @brief Communicate the filename with the server
/// @param sock_fd the socket to send on
/// @param filename the filename to send
/// @param type the header to use to denote a filename
void send_filename(int sock_fd, char* filename, uint8_t type) {
    // Create a buffer
    char filename_pkt[PACKET_SIZE];
    
    // TODO: Populate the buffer with the packet
    // TODO: Send the packet
}

/// @brief Send a file over a socket
/// @param sock_fd the socket to send on
/// @param filename the file to send
void send_file(int sock_fd, char* filename) {
    FILE *fp;
    fp = fopen(filename, "r");

    if(fp) {
        char buffer[PACKET_SIZE];
        
        // TODO: fill packet headers into buffer
        
        send_filename(sock_fd, filename, NOTIFY_UPLOAD_FILE_PKT);
        printf("filename sent\n");

        // Read the size of the file
        fseek(fp, 0, SEEK_END); // progress to end
        size_t fsize = ftell(fp); // determine file size
        fseek(fp, 0, SEEK_SET); // go back to start
        printf("filesize read\n");

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

/// @brief Save a file received on the socket
/// @param sock_fd socket to receive on
/// @param filename filename to save it as
void save_file(int sock_fd, char* filename) {
    FILE *fp;
    fp = fopen(filename, "w");

    if (fp) {
        // TODO: Receive into buffer
        char recv_buffer[PACKET_SIZE];
        
        while(true) {
            // TODO: Receive into buffer
            int bytes_recv = 0; // The number of bytes received

            // If defining the debugging flag, will print out each packet
            #ifdef DEBUGGING
            // TODO: Change 0 to size of data read
            pkt_hex_dump((unsigned char*) recv_buffer, 0);
            #endif

            // TODO: Validate packet type
            // TODO: Extract file contents and write into file
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
    printf("File saved.\n");
    fclose(fp);
}

/// @brief User options
void print_options() {
    cout << "\nOptions\n"
            "1. Broadcast Message\n"
            "2. Direct Message\n"
            "3. File Upload\n"
            "4. File Download\n"
            "Choose an Option: ";
    fflush(stdout);
}

/// Flag to communicate "finished" to different threads. Set this when the socket is detected as closed
volatile bool is_done = false;

/// @brief Handler for user input
/// @param sock_fd
void is_sending(int sock_fd) {
    int user_choice;
    while (true) {
        if (is_done) break;
        print_options();
        cin >> user_choice;
        cin.ignore(1,'\n');

        switch (user_choice) {
        case BROADCAST_PKT: {
            // TODO: Create a buffer for your packet

            cout << "\nMessage: ";
            // TODO: read data
            // cin.getline(msg_to_send, max_char_amount);
            
            // Send the message
            send_msg(sock_fd, BROADCAST_PKT, 0x00000000, "buffer");
            break;
        }
        case DIRECTMESSAGE_PKT: {
            // TODO: Create a buffer for your packet

            cout << "\nMessage: ";
            // TODO: read data
            // cin.getline(msg_to_send, max_char_amount);
            
            // Read in the desired IP
            char client_ip[16];
            cout << "Receiver IP: ";
            cin.getline(client_ip, 16);
            // Convert IP as string to network byte order
            uint32_t client_ip_addr;
            inet_aton(client_ip,(in_addr*)&client_ip_addr);

            send_msg(sock_fd, DIRECTMESSAGE_PKT, client_ip_addr, "buffer");
            break;
        }
        case NOTIFY_UPLOAD_FILE_PKT: {
            // Read a file name
            char filename[FILENAME_MAX];
            memset(filename, 0, FILENAME_MAX);
            cout << "\nEnter Filename: ";
            cin.getline(filename, FILENAME_MAX);

            // Send the file
            send_file(sock_fd, filename);
            break;
        }
        case REQUEST_DOWNLOAD_FILE_PKT: {
            // Read a filename
            char filename[FILENAME_MAX];
            memset(filename, 0, FILENAME_MAX);
            cout << "\nEnter Filename: ";
            cin.getline(filename, FILENAME_MAX);

            // Request the filename
            send_filename(sock_fd, filename, REQUEST_DOWNLOAD_FILE_PKT);
            break;
        }
        default:
            break;
        }
    }
}

/// @brief Handler for receiving messages
void is_receiving(int sock_fd) {
    char buffer[PACKET_SIZE];
    
    while(true) {
        // reset the buffer
        memset(buffer, 0, PACKET_SIZE);

        // TODO: Receive data into the buffer

        int recv_type = 0;
        switch(recv_type) {
        case BROADCAST_PKT: {
            // TODO: extract message
            printf("\n%s\n", "Receive message");
            print_options();
            break;
        }
        case DIRECTMESSAGE_PKT: {
            // TODO: Extract message and ip of sender
            printf("\n[DIRECT MESSAGE](%u.%u.%u.%u) %s\n", "1", "2", "3", "4", "Receive message");
            print_options();
            break;
        }
        case SERVER_SENDING_FILE_PKT: {
            // TODO: Extract filename from packet
            save_file(sock_fd, "filename");
            break;
        }
        default: {
            printf("Random packet detected\n");
            // Can print out packets if defining the DEBUGGING flag
            #ifdef DEBUGGING
            // TODO: Replace the 0 for the number of bytes in the packet
            pkt_hex_dump((unsigned char*) buffer, 0);
            #endif
            break;
        }
        }
    }
}

int main() {
    // Initialize a socket and connect to the server
    int sock_fd = socket(AF_INET, SOCK_STREAM, 0);
    error_check(sock_fd, "Socket Initialization");
    connect_socket(sock_fd, SERVER_PORT_NUMBER);

    // Start a sending thread
    std::thread sending_thread(is_sending, sock_fd);
    sending_thread.detach();

    // Start a receiving thread
    std::thread recvThread(is_receiving, sock_fd);
    recvThread.detach();

    // Wait for user input (CTRL-C to stop)
    while(true);
}
