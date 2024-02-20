#include "Network.h"
#include <fstream>
#include <iostream>
#include <sstream>
#include <ctime>
#include <chrono>
#include <iomanip>

Network::Network() {

}

std::string Network::getTimestamp() {
    auto now = std::chrono::system_clock::now();
    std::time_t currentTime = std::chrono::system_clock::to_time_t(now);

    std::stringstream ss;

    // Formatting the time directly into the stringstream
    ss << std::put_time(std::localtime(&currentTime), "%Y-%m-%d %H:%M:%S");

    // Returning the formatted time as a string
    return ss.str();
}

void Network::newMessage(string senderId, string receiverId, string messageContent, int message_limit, std::string sender_port, std::string receiver_port,vector<Client> &clients) {
    //We need to first find the clients:
    Client* Sender = nullptr;
    Client* Reciever = nullptr;
    Client* currentHop= nullptr;

    for (int i = 0; i < numberOfClients; ++i) {
       if (clients[i].client_id==senderId){
            Sender= &clients[i];
       }
       else if (clients[i].client_id==receiverId){
            Reciever=&clients[i];
       }
    }

    std::vector<std::string> subMessages;
    
    //First splitting the message:
    for (int i = 0; i < messageContent.size(); i += message_limit) {
     subMessages.push_back(messageContent.substr(i, message_limit));
    }

    string nextHopID=Sender->routing_table[receiverId];
    for (int i = 0; i < numberOfClients; ++i) {
       if (clients[i].client_id==nextHopID){
            currentHop=&clients[i];
       }
    }
    //Now we make a frame for each fragment of the message
    std::cout << "Message to be sent: \"" << messageContent << "\"\n\n";
    int counter=1;
    for (const std::string& subMessage : subMessages) {
        std::cout<<"Frame #"<<counter++<<std::endl;
        std::stack<Packet*> Frame;

        //For the Physical Layer:
        PhysicalLayerPacket* PhysLayer= new PhysicalLayerPacket(3,Sender->client_mac,currentHop->client_mac);
        PhysLayer->frame_num=counter;
        PhysLayer->print();
        Frame.push(PhysLayer);

        if (subMessage.find("!") != std::string::npos||
        subMessage.find("?") != std::string::npos||
        subMessage.find(".") != std::string::npos){
            PhysLayer->isLast=true;
        }
        //For the Network Layer:
        NetworkLayerPacket* NetLayer= new NetworkLayerPacket(2,Sender->client_ip, Reciever->client_ip);
        NetLayer->print();
        Frame.push(NetLayer);

        //For the Transporat Layer:
        TransportLayerPacket* TransLayer= new TransportLayerPacket(1,sender_port,receiver_port);
        TransLayer->print();
        Frame.push(TransLayer);
       
        //For the Application Layer:
        ApplicationLayerPacket* AppLayer = new ApplicationLayerPacket(0,senderId,receiverId,subMessage);
        AppLayer->print();
        Frame.push(AppLayer);

        std::stack<Packet*> reversedFrame;

        // Now we should reverse the stack
        while (!Frame.empty()) {
            Packet* packet = Frame.top();
            reversedFrame.push(packet);
            Frame.pop();
        }

        Frame = reversedFrame;
        Sender->outgoing_queue.push(Frame);
        std::cout << "Message chunk carried: \"" << subMessage << "\"" << std::endl;
        std::cout<<"Number of hops so far: "<<PhysLayer->numOfHopsSoFar<<std::endl;
        std::cout<<"--------"<<std::endl;

        if (PhysLayer->isLast){
            Log sent(getTimestamp(), messageContent, PhysLayer->frame_num - 1, PhysLayer->numOfHopsSoFar, AppLayer->sender_ID, AppLayer->receiver_ID, true, ActivityType::MESSAGE_SENT);
            Sender->log_entries.push_back(sent);}
    }

}



Packet* Network::indexStack(std::stack<Packet*>& stack, int index) {
   if (index >= stack.size() || index < 0) {
        return nullptr;
   }
   std::stack<Packet*> tempStack = stack;
   std::stack<Packet*> reverseStack;
  
   // Accessing elements from a stack is easier from the bottom, so we reverse:
   while (!tempStack.empty()) {
       reverseStack.push(tempStack.top());
       tempStack.pop();
   }


   for (int i = 0; i < index; ++i) {
       reverseStack.pop();
   }
   return reverseStack.top();
}


void Network::show_frame_info(string clientId, std::string queue_selection, int frame_number, vector<Client> &clients) {
    // Locating the exact frame:
    Client* client=nullptr;
    for (int i = 0; i < numberOfClients; ++i) {
        if (clients[i].client_id == clientId) {
            client = &clients[i];
        }
    }
    
    std::queue<std::stack<Packet*>> tempQueue = queue_selection=="out"?client->outgoing_queue:client->incoming_queue;

    int current_frame = 0;
    //Defining the frame:
    std::stack<Packet*> Frame; 
    while (!tempQueue.empty()) {
        if (current_frame == frame_number-1) {
            Frame = tempQueue.front();
            break;
        }
        tempQueue.pop();
        current_frame++;
    }

    if (indexStack(Frame,0)==nullptr) {
        //If the stack isn't available.
        std::cout<<"No such frame."<<std::endl;
        return;
    }
    else{
    //We will need theses in the printing:
    Packet* layer = indexStack(Frame,0); 
    ApplicationLayerPacket* appLayer = dynamic_cast<ApplicationLayerPacket*>(layer);

    Packet* layer3 = indexStack(Frame,3); 
    PhysicalLayerPacket* PhysLayer = dynamic_cast<PhysicalLayerPacket*>(layer3);

    //Printing:
    std::cout<<"Current Frame #"<<frame_number<<" on the "<<((queue_selection=="out")?"outgoing":"incoming")<<" queue of client "<<clientId<<std::endl;
    std::cout<<"Carried Message: \""<<appLayer->message_data<< "\""<<std::endl;
    std::cout<<"Layer 0 info: ";
    indexStack(Frame,0)->print();

    std::cout<<"Layer 1 info: ";
    indexStack(Frame,1)->print();

    std::cout<<"Layer 2 info: ";
    indexStack(Frame,2)->print();

    std::cout<<"Layer 3 info: ";
    indexStack(Frame,3)->print();

    std::cout<<"Number of hops so far: "<<PhysLayer->numOfHopsSoFar<<std::endl;}
};


void Network::show_q_info(string clientId,std::string queue_selection, vector<Client> &clients){
    // Locating the exact queue:
    Client* client=nullptr;
    for (int i = 0; i < numberOfClients; ++i) {
        if (clients[i].client_id == clientId) {
            client = &clients[i];
        }
    }
    
    std::queue<std::stack<Packet*>> tempQueue = queue_selection=="out"?client->outgoing_queue:client->incoming_queue;
    std::queue<std::stack<Packet*>> anotherone = queue_selection=="out"?client->outgoing_queue:client->incoming_queue;

    //Keep popping until the queue is empty, to count the number of frame stacks stored in the queue:
    int numOfFrames=0;
    while (!tempQueue.empty()) {
        tempQueue.pop();
        numOfFrames++;
    }

    //Printing:
    std::cout<<"Client "<<client->client_id<<" "<< (queue_selection=="out"?"Outgoing":"Incoming") <<" Queue Status"<<std::endl;
    std::cout<<"Current total number of frames: "<<numOfFrames<<std::endl;
    
};

void Network::send(std::vector<Client>& clients) {
    for (int i = 0; i < numberOfClients; i++) {
        Client* client = &clients[i];
        while(!client->outgoing_queue.empty()) {
            std::stack<Packet*> stack;
            stack = client->outgoing_queue.front();
            

            Packet* physlayer = indexStack(stack, 3);
            PhysicalLayerPacket* physLayer = dynamic_cast<PhysicalLayerPacket*>(physlayer);
            physLayer->numOfHopsSoFar++;

            Packet* layer = indexStack(stack,0); 
            ApplicationLayerPacket* appLayer = dynamic_cast<ApplicationLayerPacket*>(layer);
            string message=appLayer->message_data;

            // Find the next receiver based on MAC address
            Client* next_receiver = nullptr;
            for (int j = 0; j < numberOfClients; ++j) {
                if (clients[j].client_mac == physLayer->receiver_MAC_address) {
                    next_receiver = &clients[j];
                    break; 
                }
            }
            std::cout<<"Client "<<client->client_id<<" sending frame #"<<physLayer->frame_num-1<<" to client "<<next_receiver->client_id<<std::endl;
            indexStack(stack,3)->print();
            indexStack(stack,2)->print();
            indexStack(stack,1)->print();
            indexStack(stack,0)->print();
            std::cout << "Message chunk carried: \"" << message << "\"" << std::endl;
            std::cout<<"Number of hops so far: "<<physLayer->numOfHopsSoFar<<std::endl;
            std::cout<<"--------"<<std::endl;
            // Move the stack to the next reciever
            next_receiver->incoming_queue.push(stack);
            // Pop the queue
            client->outgoing_queue.pop();
        }
    }
}

void Network::receive(vector<Client> &clients){
    bool first = true;

    for (int i = 0; i < numberOfClients; i++) {
        Client* client = &clients[i];
        string message;

        while (!client->incoming_queue.empty()) {
            //For every frame in this clients incoming queue:
            std::stack<Packet*> stack;
            stack = client->incoming_queue.front();
            
            Packet* thirdLayer = indexStack(stack, 3);
            PhysicalLayerPacket* physLayer = dynamic_cast<PhysicalLayerPacket*>(thirdLayer);

            Packet* layer0 = indexStack(stack, 0);
            ApplicationLayerPacket* appLayer = dynamic_cast<ApplicationLayerPacket*>(layer0);

            string og_sender_id= appLayer->sender_ID;
            Client* og_sender = nullptr;

            for (int j = 0; j < numberOfClients; ++j) {
                if (clients[j].client_id ==og_sender_id) {
                    og_sender = &clients[j];
                    break; 
                }
            }

            string receiver_id= appLayer->receiver_ID;
            Client* end_receiver = nullptr;

            for (int j = 0; j < numberOfClients; ++j) {
                if (clients[j].client_id ==receiver_id) {
                    end_receiver = &clients[j];
                    break; 
                }
            }

            string sender_mac= physLayer->sender_MAC_address;
            Client* sender = nullptr;

            string next_client_ID= client->routing_table[end_receiver->client_id];
            Client* next_client = nullptr;

            for (int j = 0; j < numberOfClients; ++j) {
                if (clients[j].client_id ==next_client_ID) {
                    next_client = &clients[j];
                    break; 
                }
            }   

            for (int j = 0; j < numberOfClients; ++j) {
                if (clients[j].client_mac ==sender_mac) {
                    sender = &clients[j];
                    break; 
                }
            }

            if (end_receiver==client){

                //In case it has reached its intended client
                if (physLayer->frame_num-1==1){
                    //If it's a new message:
                    message="";
                } 
                first = false;
                std::cout<<"Client "<<client->client_id<<" receiving frame #"<<physLayer->frame_num-1<<" from client "<<sender->client_id<<", originating from client "<<og_sender->client_id<<std::endl;
                indexStack(stack,3)->print();
                indexStack(stack,2)->print();
                indexStack(stack,1)->print();
                indexStack(stack,0)->print();
                std::cout << "Message chunk carried: \"" << appLayer->message_data << "\"" << std::endl;
                std::cout<<"Number of hops so far: "<<physLayer->numOfHopsSoFar<<std::endl;
                std::cout<<"--------"<<std::endl;
                message.append(appLayer->message_data);

                //If it is the last chunk of the message:
                if (physLayer->isLast){
                    std::cout << "Client " << end_receiver->client_id << " received the message \"" << message << "\" from client " << og_sender->client_id << "."<<std::endl;
                    std::cout<<"--------"<<std::endl;
                    Log receiver_log(getTimestamp(), message, physLayer->frame_num - 1, physLayer->numOfHopsSoFar, og_sender->client_id, end_receiver->client_id, true, ActivityType::MESSAGE_RECEIVED);
                    client->log_entries.push_back(receiver_log);
                }
                //Now in this case the stacks aren't being redirected, they are being popped later
                //Since the use of them is done, we need to free the memory allocated by each layer like so:
                while (!stack.empty()) {
                    Packet* packet = stack.top();
                    stack.pop();
                    delete packet;
                }                
            }
            else{

                //If the next reciever is null (not because it is the end receiver)
                if (next_client==nullptr){
                   
                    std::cout<<"Client "<<client->client_id<<" receiving frame #"<<physLayer->frame_num-1<<" from client "<<sender->client_id<<", but intended for client "<<end_receiver->client_id<<". Forwarding... "<<std::endl;
                    std::cout<<"Error: Unreachable destination. Packets are dropped after "<<physLayer->numOfHopsSoFar<<" hops!"<<std::endl;
                if (physLayer->isLast){
                    std::cout<<"--------"<<std::endl;
                }
                    if (physLayer->isLast){
                    Log dropped(getTimestamp(), message, physLayer->frame_num - 1, physLayer->numOfHopsSoFar, sender->client_id, end_receiver->client_id, false, ActivityType::MESSAGE_DROPPED);
                    client->log_entries.push_back(dropped);}

                    while (!stack.empty()) {
                    Packet* packet = stack.top();
                    stack.pop();
                    delete packet;
                    }
                }
                else{
                string next_client_mac= next_client->client_mac;

            
                if (physLayer->frame_num-1==1){
                    std::cout<<"Client "<<client->client_id<<" receiving a message from client "<<sender->client_id<<", but intended for client "<<end_receiver->client_id<<". Forwarding... "<<std::endl;
                
                }
                first = false;

                std::cout<<"Frame #"<<physLayer->frame_num-1<<" MAC address change: New sender MAC "<<client->client_mac<<", new receiver MAC "<<next_client->client_mac<<std::endl;
                if (physLayer->isLast){
                    std::cout<<"--------"<<std::endl;
                }
                //Then we edit the physical layer:
                physLayer->sender_MAC_address=client->client_mac;
                physLayer->receiver_MAC_address=next_client_mac;

                //Then we need to push
                client->outgoing_queue.push(stack);

                //Create the log 
                //Should the message actually be updated? I dont think so
                if (physLayer->isLast){
                Log Forwarded(getTimestamp(), message, physLayer->frame_num - 1, physLayer->numOfHopsSoFar, sender->client_id, end_receiver->client_id, true, ActivityType::MESSAGE_FORWARDED);
                client->log_entries.push_back(Forwarded);}}
            }
            // Pop the queue
            client->incoming_queue.pop();
        }
    }

}

void Network::printLog(vector<Client> &clients, string client_ID){
    Client* client= nullptr;
    for (int j = 0; j < numberOfClients; ++j) {
        if (clients[j].client_id ==client_ID) {
           client = &clients[j];
            break; 
        }
    }
    if (client==nullptr){
        std::cout<<"Client does not exist!"<<std::endl;
    }
    else{
        client->printlog();
    }
}
void Network::process_commands(vector<Client> &clients, vector<string> &commands, int message_limit,
                      const string &sender_port, const string &receiver_port) {
    
    //Iterating over the commands:
    for (string command : commands) {

        std::cout << std::setw(command.length()+9) << std::setfill('-') << '-' << std::endl;
        std::cout << "Command: " << command << std::endl;
        std::cout << std::setw(command.length()+9) << std::setfill('-') << '-' << std::endl;

        //MESSAGE
        if (command.substr(0, 7) == "MESSAGE") {
            std::istringstream iss(command);
            std::string commandName;
            iss >> commandName;

            //First word:
            std::string senderId;
            iss >> senderId;
        
            //Second word:
            std::string recieverId;
            iss >> recieverId;

            //The message:
            size_t start = command.find('#');
            size_t stop = command.find('#', start + 1);
            string message = command.substr(start + 1, stop - start - 1);

            newMessage(senderId,recieverId,message,message_limit,sender_port,receiver_port,clients);
  
        }

        //SHOW_FRAME_INFO
        else if (command.substr(0, 15) == "SHOW_FRAME_INFO") {
            std::istringstream iss(command);
            std::string commandName;
            iss >> commandName;

            //First word:
            std::string client_ID;
            iss >> client_ID;
        
            //Second word:
            std::string queue_selection;
            iss >> queue_selection;

            //Third word:
            int frame_number;
            iss >> frame_number;

            show_frame_info(client_ID,queue_selection,frame_number,clients);
        }

        //SHOW_Q_INFO
        else if (command.substr(0, 11) == "SHOW_Q_INFO") {
            std::istringstream iss(command);
            std::string commandName;
            iss >> commandName;

            //First word:
            std::string client_ID;
            iss >> client_ID;
        
            //Second word:
            std::string queue_selection;
            iss >> queue_selection;

            show_q_info(client_ID,queue_selection,clients);
        }

        //SEND
        else if (command.substr(0, 4) == "SEND"){
            send(clients);
        }

        //RECEIVE
        else if(command.substr(0, 7) == "RECEIVE"){
            receive(clients);
        }
        
        //PRINT_LOG
        else if(command.substr(0, 9) == "PRINT_LOG"){
            std::istringstream iss(command);
            std::string commandName;
            iss >> commandName;

            //First word:
            std::string client_ID;
            iss >> client_ID;

            printLog(clients,client_ID );
        }

        //INVALID COMMANDS
        else{
            std::cout<<"Invalid command."<<std::endl;
        }

    }
            std::cout<<std::endl;
    // TODO: Execute the commands given as a vector of strings while utilizing the remaining arguments.
    /* Don't use any static variables, assume this method will be called over and over during testing.
     Don't forget to update the necessary member variables after processing each command. For example,
     after the MESSAGE command, the outgoing queue of the sender must have the expected frames ready to send. */
}

vector<Client> Network::read_clients(const string &filename) {
    vector<Client> clients;
    //Reading the data:
    std::ifstream clientdata(filename);
    if(!clientdata.is_open()){
            std::cerr<<"Error while opening commands data file...";
            std::vector<Client> emptyVector;
            return emptyVector;
    }

    std::string line;
    //Reading the number of clients:
    while (clientdata >> line) {
        //Get the number of clients:
            numberOfClients=stoi(line);
            break;
    }
    
    //Reading the clients and storing them in a vector:
    std::string client_id,client_ip,client_mac;
    while (clientdata >> client_id>> client_ip >> client_mac) {
        Client client(client_id, client_ip, client_mac);
        clients.push_back(client);
    }


    // TODO: Read clients from the given input file and return a vector of Client instances.
    // DONE 
    return clients;
}

void Network::read_routing_tables(vector<Client> &clients, const string &filename) {
    std::ifstream routing_data(filename);
    if(!routing_data.is_open()){
            std::cerr<<"Error while opening routing data file...";
            std::vector<Client> emptyVector;
            return;
    }

    std::string line;
    int clientnum=0;

    while (std::getline(routing_data, line)) {
        if (line.compare("-") == 0) {
            //If it is a - , then we should increase the clientnum index and move on.
            clientnum++;

        } else {
            //Split the line into 2, and put them in the respective dictionary.
            if (line.size() == 3 && line[1] == ' ') {

                std::string receiverID = line.substr(0, 1); 
                std::string nexthopID = line.substr(2, 1); 
                
                //Checking if the routing file has too much data.
                if (clientnum >= 0 && clientnum < numberOfClients) {
                    clients[clientnum].routing_table[receiverID] = nexthopID;
                } else {
                    std::cerr << "There are no more clients to set up for!";
                }
            }
        }

      
    }


    // TODO: Read the routing tables from the given input file and populate the clients' routing_table member variable.
    //DONE
}

// Returns a list of token lists for each command
vector<string> Network::read_commands(const string &filename) {
    vector<string> commands;

    //Reading the data:
    std::ifstream command_data(filename);
    if(!command_data.is_open()){
            std::cerr<<"Error while opening commands data file...";
            std::vector<string> emptyVector;
            return emptyVector;
    }

    std::string line;
    //Reading the number of clients:
    while (command_data >> line) {
        //Get the number of commands:
            numOfCommands=stoi(line);
            break;
    }

    //Append the lines to the vector:
    while (std::getline(command_data, line)) {
        if (!line.empty()){
        commands.push_back(line);
        }
    }

    // TODO: Read commands from the given input file and return them as a vector of strings.
    //DONE
    return commands;
}


Network::~Network() {
    // TODO: Free any dynamically allocated memory if necessary.
    
}
