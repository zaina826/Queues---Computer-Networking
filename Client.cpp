//
// Created by alperen on 27.09.2023.
//

#include "Client.h"


Client::Client(string const& _id, string const& _ip, string const& _mac) {
    client_id = _id;
    client_ip = _ip;
    client_mac = _mac;
}

ostream &operator<<(ostream &os, const Client &client) {
    os << "client_id: " << client.client_id << " client_ip: " << client.client_ip << " client_mac: "
       << client.client_mac << endl;
    return os;
}

std::string Client::activityTypeToString(ActivityType type) {
    switch(type) {
        case ActivityType::MESSAGE_RECEIVED:
            return "Message Received";
        case ActivityType::MESSAGE_FORWARDED:
            return "Message Forwarded";
        case ActivityType::MESSAGE_SENT:
            return "Message Sent";
        case ActivityType::MESSAGE_DROPPED:
            return "Message Dropped";
    }
}

void Client::printlog(){
    int logn=1;
    if (!log_entries.empty()){
        std::cout<<"Client "<<client_id<<" Logs:"<<std::endl;
    }
    for (const auto& log : log_entries) {
        std::cout<<"--------------"<<std::endl;
        std::cout<<"Log Entry #"<< logn++<<":"<<std::endl;
        std::cout<<"Activity: "<<activityTypeToString(log.activity_type)<<std::endl;
        std::cout<<"Timestamp: "<<log.timestamp<<std::endl;;
        std::cout<<"Number of frames: "<<log.number_of_frames<<std::endl;
        std::cout<<"Number of hops: "<<log.number_of_hops<<std::endl;
        std::cout<<"Sender ID: "<<log.sender_id<<std::endl;
        std::cout<<"Receiver ID: "<<log.receiver_id<<std::endl;
        std::cout<<"Success: "<<(log.success_status?"Yes":"No")<<std::endl;
        if (log.activity_type == ActivityType::MESSAGE_RECEIVED || log.activity_type == ActivityType::MESSAGE_SENT )
        std::cout << "Message: \"" << log.message_content << "\"" << std::endl;
    }
}

Client::~Client() {
    while (!outgoing_queue.empty()) {
        std::stack<Packet*>& stack = outgoing_queue.front();
        while (!stack.empty()) {
            Packet* layer = stack.top();
            delete layer;
            stack.pop();
        }
        outgoing_queue.pop();
    }
    while (!incoming_queue.empty()) {
        std::stack<Packet*>& stack = incoming_queue.front();
        while (!stack.empty()) {
            Packet* layer = stack.top();
            delete layer;
            stack.pop();
        }
        incoming_queue.pop();
    }
}