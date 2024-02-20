#ifndef NETWORK_H
#define NETWORK_H

#include <vector>
#include <iostream>
#include "Packet.h"
#include "Client.h"

using namespace std;

class Network {
public:
    Network();
    ~Network();

    // Executes commands given as a vector of strings while utilizing the remaining arguments.
    void process_commands(vector<Client> &clients, vector<string> &commands, int message_limit, const string &sender_port,
                     const string &receiver_port);

    // Initialize the network from the input files.
    vector<Client> read_clients(string const &filename);
    void read_routing_tables(vector<Client> & clients, string const &filename);
    vector<string> read_commands(const string &filename); 

    //Added:
    int numberOfClients, numOfCommands;
    void newMessage(string senderId, string receiverId, string messageContent, int message_limit, std::string sender_port, std::string receiver_port,vector<Client> &clients);
    void show_frame_info(string clientId,std::string queue_selection,int frame_number,vector<Client> &clients);
    void show_q_info(string clientId,std::string queue_selection,vector<Client> &clients);
    void send(vector<Client> &clients);
    void receive(vector<Client> &clients);
    void printLog(vector<Client> &clients, string client_ID);
    std::string getTimestamp();
    Packet* indexStack(std::stack<Packet*>& stack, int index) ;

};

#endif  // NETWORK_H
