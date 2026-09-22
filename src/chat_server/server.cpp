#include <iostream>
#include <fstream>
#include <string>
#include <thread>
#include <mutex>
#include <unordered_map>
#include <unordered_set>
#include <sstream>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <vector>

#define PORT 12345
#define BUFFER_SIZE 1024
#define USERS_FILE "../../config/users.txt"


std :: unordered_map < int, std :: string> clients ; // Client socket ->username 
std::unordered_map<std::string, std::string> user_credentials; //username -> passwords
std :: unordered_map < std :: string , std::unordered_set<int>> groups ; // Group name -> client sockets
std::mutex client_mutex; //used for locking

// Function to remove a client safely from all data structures
void remove_client(int client_socket) {
    if (clients.find(client_socket) != clients.end()) {
        std::string username = clients[client_socket];

        // Remove client from groups
        for (auto &group : groups) {
            group.second.erase(client_socket);
        }

        // Remove client from active users
        clients.erase(client_socket);
        std::cout << username << " has disconnected.\n";
    }
}

// Function to send a message to a client
void send_message(int client_socket, const std::string &message) {
    send(client_socket, message.c_str(), message.length(), 0);
}

// Function for sending private messages
void send_private_message(int client_socket, std::string username, std::string message){
    int new_socket=-1;
    
    // If username is not found, give error to the client
    if(user_credentials.find(username) == user_credentials.end()){
        send_message(client_socket, "[Error]:Invalid username.\n");
        return;
    }
    for(auto client: clients){
        if(client.second == username){
            new_socket = client.first;
            break;
        }
    }
    if(new_socket ==-1){
        send_message(client_socket, "[Error]: Message not sent as user is currently offline.\n");
        return;
    }
    send_message(new_socket, message);
    send_message(client_socket, "Message has been sent successfully.\n");
    return;
}

//send a message to all active clients
void send_broadcast(std:: string message, int client_socket){
    std::string intro = "[Broadcast]";
    for(auto active_client : clients){
        send_message(active_client.first, intro+message);
    }
    return;
}

//the client with socket number client_socket wants to join a group group_name 
void join_group(std::string group_name, int client_socket){
    // Checking if the group exists
    if(groups.find(group_name) == groups.end()){
        send_message(client_socket, "[Error]:No such group found.\n");
        return;
    }
    groups[group_name].insert(client_socket);
    send_message(client_socket,"You have joined the group.\n");
    return;
}

// Function for sending message in a group
void message_group(std::string group_name, std::string message, int client_socket){
    // Checking if the group exists
    if(groups.find(group_name) == groups.end()){
        send_message(client_socket, "[Error]:No such group found.\n");
        return;
    }
    // Checking if the client is a member of the group
    else if(groups[group_name].find(client_socket)==groups[group_name].end()){
        send_message(client_socket,"[Error]:You are not a member of this group !\n");
        return;
    }
    std::string intro = "[";
    intro+=group_name;intro+="]";
    for(auto mem : groups[group_name]){
        send_message(mem, intro + message);
    }
    return;
}

//Function for leavng the group
void leave_group(std::string group_name, int client_socket){
    if(groups.find(group_name) == groups.end()){
        send_message(client_socket, "[Error]:No such group found.\n");
        return;
    }
    if(groups[group_name].find(client_socket) == groups[group_name].end()){
        send_message(client_socket, "[Error]:You do not belong to this group. \n");
        return;
    }
    groups[group_name].erase(client_socket);
    send_message(client_socket,"Successfully, exited the group. You will no longer receive messages from this group\n");
    return;
}

//Function for making a group
void create_group(std::string group_name, int client_socket){
    if(groups.find(group_name) == groups.end()){
        std::unordered_set<int> mems;
    mems.insert(client_socket);
    groups[group_name]=mems;
    send_message(client_socket,"Group created successfully\n");
    return;
    }
    send_message(client_socket,"[Error]:A group with this name already exists\n");
    return;
}

// Function to load user credentials from file
void load_users(const std::string &filename) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "[Error]: Could not open users file.\n";
        exit(1);
    }

    std::string line;
    while (std::getline(file, line)) {
        size_t delimiter_pos = line.find(':');
        if (delimiter_pos != std::string::npos) {
            std::string username = line.substr(0, delimiter_pos);
            std::string password = line.substr(delimiter_pos + 1);
            user_credentials[username] = password;
        }
    }

    file.close();
    std::cout << "Loaded " << user_credentials.size() << " users from file.\n";
}

// Function for authenticating client
void authenticate_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    std::string username, password;
    bool logged_in=false;
    while (true) {
        // Prompting the client to enter username
        send_message(client_socket, "Enter username: ");
        memset(buffer, 0, BUFFER_SIZE);
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        username = buffer;

        // Checking validity of username
        if (user_credentials.find(username) == user_credentials.end()) {
            send_message(client_socket, "[Error]: Invalid username\n");
            continue;
        }
        logged_in=false;

        // Checking if the user has already logged in 
        for(auto client:clients){
            if(client.second==username){
                logged_in=true;
                break;
            }
        }
        if(logged_in){
            send_message(client_socket,"[Error]: User already logged in.\n");
            continue;
        }

        // Prompting the client to enter password
        send_message(client_socket, "Enter password: ");
        memset(buffer, 0, BUFFER_SIZE);
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        password = buffer;

        // Checking validity of username
        if (user_credentials[username] != password) {
            send_message(client_socket, "[Error]: Incorrect password\n");
            continue;
        }

        // Sucess ! Boost is the secret of my energy
        send_message(client_socket, "Welcome to the server!\n");
        std::cout << username << " has logged in.\n";

        //Adding the new client to the global map
        clients.insert({client_socket, username});
        break;
    }
}

// Handle client authentication
void handle_client(int client_socket) {
    authenticate_client(client_socket);
    char buffer[BUFFER_SIZE];

    // Handle all sorts of messages here
    while(true){
        std::string client_msg;
        memset(buffer, 0, BUFFER_SIZE);
        recv(client_socket, buffer, BUFFER_SIZE, 0);
        client_msg=buffer;
        if(client_msg.empty())  continue;
        std::string cmd="";
        std::stringstream ss(client_msg);
        std::vector<std::string> tokens;
        std::string word;
        while(ss >> word)   tokens.push_back(word);
        if(tokens.empty())  continue;
      
        cmd=tokens[0];
        //Private message
        if(cmd=="/msg"){  
            std::lock_guard<std::mutex> lock(client_mutex);
            if(tokens.size()<2){
                send_message(client_socket,"[Error]:Receiver not specified\n");
                continue;
            }
            else if(tokens.size()==2){
                send_message(client_socket,"[Error]:Message not entered\n");
                continue;
            }
            std::string username= tokens[1];
            std::string message="";
            message+="[";
            message+=clients[client_socket];
            message+="]";
            message+=": ";
            int i;
            for(i=2;i<(int)tokens.size();i++){
                message=message+tokens[i];message+=" ";
            }
            send_private_message(client_socket, username, message);
            
        }
        //Broadcast
        else if(cmd=="/broadcast"){
            std::lock_guard<std::mutex> lock(client_mutex);
            if(tokens.size()<2){
                send_message(client_socket,"[Error]:Broadcast message not specified\n");
                continue;
            }
            std::string message="";
            message+="[";
            message+=clients[client_socket];
            message+="]";
            message+=": ";
            int i;
            for(i=1;i<(int)tokens.size();i++){
                message=message+tokens[i];message+=" ";
            }
            send_broadcast(message, client_socket);
        }
        //Join Group
        else if(cmd=="/join_group"){
            std::lock_guard<std::mutex> lock(client_mutex);
            if(tokens.size()>2){send_message(client_socket,"[Error]:Group name cannot have spaces in it.\n");}
            else {
            std::string group_name=tokens[1];
            join_group(group_name, client_socket);
            }
            
        }
        //Group Message
        else if(cmd=="/group_msg"){
            std::lock_guard<std::mutex> lock(client_mutex);
            if(tokens.size()<2){
                send_message(client_socket,"[Error]:Group name not specified\n");
                continue;
            }
            else if(tokens.size()==2){
                send_message(client_socket,"[Error]:Message not entered\n");
                continue;
            }
            std::string group_name=tokens[1];
            std::string message="";
            message+="[";
            message+=clients[client_socket];
            message+="]";
            message+=": ";
            int i;
            for(i=2;i<(int)tokens.size();i++){
                message+=tokens[i]+" ";
            }
            message_group(group_name, message, client_socket);
        }
        //Leave group
        else if(cmd=="/leave_group"){
            std::lock_guard<std::mutex> lock(client_mutex);
            if(tokens.size()<2){
                send_message(client_socket,"[Error]:Group name not specified\n");
                continue;
            }
            if(tokens.size()>2){send_message(client_socket,"[Error]:Group name cannot have spaces in it.\n");}
            else {
                std::string group_name=tokens[1];
                leave_group(group_name, client_socket);
            } 
        }
        //Create group
        else if(cmd=="/create_group"){
            std::lock_guard<std::mutex> lock(client_mutex);
            if(tokens.size()<2){
                send_message(client_socket,"[Error]:Group name not specified\n");
                continue;
            }
            if(tokens.size()>2){send_message(client_socket,"[Error]:Group name cannot have spaces in it.\n");}
            else {
                std::string group_name=tokens[1];
                create_group(group_name, client_socket);
            }
        }
        //Exit
        else if(cmd=="/exit"){
            std::lock_guard<std::mutex> lock(client_mutex);
            send_message(client_socket,"You are logged out.\n");
            remove_client(client_socket);
        }
        else {
            send_message(client_socket, "[Error]:Oops...Invalid command\n");
        }
    }
}

int main() {
    // Load users from file
    load_users(USERS_FILE);
    int server_socket;
    struct sockaddr_in server_address;

    server_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (server_socket == -1) {
        std::cerr << "[Error]:Socket creation failed...\n";
        exit(1);
    }
    std::cout << "Socket successfully created..\n";

    bzero(&server_address, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(PORT);
    server_address.sin_addr.s_addr = inet_addr("127.0.0.1");

    if (bind(server_socket, (struct sockaddr *)&server_address, sizeof(server_address)) != 0) {
        std::cerr << "[Error]:Socket bind failed...\n";
        exit(1);
    }
    std::cout << "Socket successfully binded..\n";

    if (listen(server_socket, 100) != 0) {
        std::cerr << "[Error]:Listen failed...\n";
        exit(1);
    }
    std::cout << "Server listening..\n";

    while (true) {
        struct sockaddr_in client_address;
        socklen_t client_len = sizeof(client_address);
        int client_socket = accept(server_socket, (struct sockaddr *)&client_address, &client_len);

        if (client_socket < 0) {
            std::cerr << "[Error]:Client connection failed...\n";
            continue;
        }

        std::cout << "New client connected.\n";
        std::thread(handle_client, client_socket).detach();
    }

    close(server_socket);
    return 0;
}