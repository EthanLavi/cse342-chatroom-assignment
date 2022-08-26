# Multi-Party Chat Application

This repository contains code to an application for message &amp; file sharing using BSD sockets in Linux.

## Objective

We used the standard BSD sockets in Linux to develop a working multi-party chat.
Our approach consisted of a server in the middle keeping track of each client that connects.

After a client has connected, it will be able to send broadcast messages to all other clients and direct messages to particular clients as well. To share files between clients, each client can upload a file on the server from where every other client can request the file.

To figure out what sort of packet the server/client has received, we designed a protocol consisting of first 8 bits to define the type of packet, which we called as “our_header”. Based on the type of packet, we check if the next part is either a broadcast message, a direct message, file or the header for direct message.

### Connection
![image](/.images/1.png)

### Messaging
![image](/.images/2.png)

### File Uploading
![image](/.images/3.png)

### File Request & Downloading
![image](/.images/4.png)
