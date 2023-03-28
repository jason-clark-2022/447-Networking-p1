// server.cpp
// Jason Clark
// 800617442

#include <iostream>
#include <fstream>
#include <math.h>
#include <string>
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>

#define MAXBUFLEN 1000
#define HOSTNAME ""
#define ROUND_TO 2  // How many decimals to round answers to

using namespace std;

struct connector_list
{
    string ip;
    int seq_flag;
    connector_list* next;
    connector_list* prev;
};

void sigchld_handler(int s)
{
    int saved_errno = errno;
    while(waitpid(-1, NULL, WNOHANG) > 0);
    errno = saved_errno;
}

void read_config(char* port)
{
    ifstream config;
    string line;
    string delimiter = "=";
    size_t position;
    
    config.open("server.conf");

    getline(config, line);
    position = line.find(delimiter); 
    line.erase(0, position+delimiter.length());
    strcpy(port, line.c_str());
}

string get_hostname_str()
{
    char c_hostname[50];
    int host_flag;
    //struct hostent* h;
    if((host_flag = gethostname(c_hostname, sizeof(c_hostname))) == -1)
    {
        perror("get_hostname_str");
        exit(EXIT_FAILURE);
    }
    string s_hostname = c_hostname;
    return s_hostname;

    //h = gethostbyname(hostname);

    //struct in_addr** p1 = NULL;
    //char ip_address[INET_ADDRSTRLEN];

    //memset(h, 0, sizeof h);
    //memset(p1, 0, sizeof(p1));

    
    
}

string process_client_msg(char* c_msg_in, connector_list *active_connectors, string client_ip, string connection_type)
{
    printf("msg:(%s)\n", c_msg_in);

    struct str_list
    {
        string data;
        str_list* next;
    };

    struct str_list* empty_str_list = new str_list;
    
    connector_list* t = active_connectors;
    
    bool new_connector = true;
    
    str_list* temp = new str_list;
    str_list *msg_in_lst = temp;

    string s_msg_in = c_msg_in;
    string delimiter = " ";
    string token;
    size_t position = 0;

    string msg_out;

    
    if(HOSTNAME != "")
    {
        const char* n_hostname = HOSTNAME;
        sethostname(n_hostname, sizeof n_hostname);
    }

    // populate msg_in_lst with (delimiter) seperated strings
    while((position = s_msg_in.find(delimiter)) != std::string::npos) 
    {
        token = s_msg_in.substr(0, position);// keep 

        temp->data = token;
        temp->next = new str_list;
        temp = temp->next;
        
        //cout << token << endl;
        s_msg_in.erase(0, position + delimiter.length()); 
    }
    temp->data = s_msg_in; // store the remaining string

    while(t->ip != "") // Check if new ip is entering sequence
    {
        if(t->ip == client_ip)
        {
            new_connector = false;
            break;
        }
        t=t->next;
    }
    
    if(msg_in_lst->data == "HELO") //first command issued
    {
        if(msg_in_lst->next != NULL && msg_in_lst->next != empty_str_list)
        {
            if(msg_in_lst->next->data == get_hostname_str())
            {
                if(new_connector == true) //correct sequence
                {
                    t->ip = client_ip;
                    t->seq_flag = 0;
                    t->next = new connector_list;
                    t->next->prev = t;
                    t->next->ip = "";
                
                    printf("Client: %s now in sequence\n", t->ip.c_str());
                    printf("Clients in sequence:\n");
                    for(connector_list* i = active_connectors; i->ip!=""; i=i->next)
                    {
                        printf("%s\n", i->ip.c_str());
                    }

                    msg_out += "200 HELO ";
                    msg_out += client_ip;
                    msg_out += "(";
                    msg_out += connection_type;
                    msg_out += ")\n";
                }
                else //client already connected
                {
                    msg_out += "200 HELO again... ";
                    msg_out += client_ip;
                    msg_out += "\n";
                }
            }
            else //invalid hostname
            {
                msg_out += "501 invalid hostname (try: HELO ";
                msg_out += get_hostname_str();
                msg_out += ")\n";
            }
        }
        else //no hostname provided
        {
            msg_out += "501 no hostname provided (try: HELO ";
            msg_out += get_hostname_str();
            msg_out += ")\n";
        }
        

    }
    else if(new_connector == false) // is proper connection established
    {
        if(msg_in_lst->data == "HELP") // anytime after HELO
        {
            if(msg_in_lst->next == NULL)
            {
                msg_out += "200\n";
                msg_out += "<-----------MENU----------->\n";
                msg_out += ":Commands: \n";
                msg_out += "CIRCLE \n";
                msg_out += "\tAREA <r> \n";
                msg_out += "\tCIRC <A> \n";
                
                msg_out += "SPHERE\n";
                msg_out += "\tVOL <r>\n";
                msg_out += "\tRAD <A>\n";
                
                msg_out += "CYLINDER\n";
                msg_out += "\tAREA<r><h>\n";
                msg_out += "\tHGT <V><r>\n";
                msg_out += "BYE <server-hostname>\n\n";

                msg_out += ":Responses: \n";
                msg_out += "200/210/220/230 Command Success\n";
                msg_out += "250 <answer> response for calculation\n";
                msg_out += "500 Syntax Error, command unrecognized\n";
                msg_out += "501 Syntax error in parameters or arguments\n";
                msg_out += "503 – Bad sequence of commands\n";

                msg_out += "<-------------------------->\n\n";
            }
            else
            {
                msg_out += "501 No arguments accepted for HELP\n";
            }
        }
        else if(msg_in_lst->data == "CIRCLE") //seq_flag = 1
        {
            if(msg_in_lst->next == NULL) //correct command issued
            {
                if(t->seq_flag == 0) //correct sequence 
                {
                    t->seq_flag = 1;
                    msg_out += "220 CIRCLE ready!\n";
                }
                else // bad sequence
                {
                    msg_out += "503 cannot call CIRCLE, client in sequence\n";
                }

            }
            else // no parameters accepted
            {
                msg_out += "501 No arguments accepted for CIRCLE\n";
            }
        }
        else if(msg_in_lst->data == "AREA") // part of CIRCLE and CYLINDER
        {
            if(t->seq_flag == 1) // FOR CIRCLE
            {
                if(msg_in_lst->next != NULL && msg_in_lst->next != empty_str_list)
                {
                    if(msg_in_lst->next->next == NULL) // correct sequence
                    {
                        try
                        {
                            double d = stod(msg_in_lst->next->data);
                            double value = M_PI * (d * d);
                            string rounded = to_string(value).substr(0, to_string(value).find(".")+(ROUND_TO+1));

                            msg_out += "250: ";
                            msg_out += rounded;
                            msg_out += "\n";

                            t->seq_flag = 0;
                        }
                        catch(invalid_argument&)// invalid argument
                        //catch(exception& e)
                        {
                            msg_out += "501 invalid arguments (expected: double double)\n";
                        }
                    }
                    else // bad arguments (more than 1)
                    {
                        msg_out += "501 Too many arguments provided (expected: 1)\n";
                    }
                }
                else // Bad arguments (no args)
                {
                    msg_out += "501 No arguments provided (expected: 1)\n";
                }
            }
            else if(t->seq_flag == 3) // for cylinder
            {
                if(msg_in_lst->next != NULL && msg_in_lst->next != empty_str_list) // check if 1 arg
                {
                    if(msg_in_lst->next->next != NULL && msg_in_lst->next->next != empty_str_list) // check for second arg
                    {
                        if(msg_in_lst->next->next->next == NULL) // correct sequence
                        {
                            try
                            {
                                double r = stod(msg_in_lst->next->data);
                                double h = stod(msg_in_lst->next->next->data);
                                
                                double value = (2*M_PI*r*h)+(2*M_PI*(r*r));
                                string rounded = to_string(value).substr(0, to_string(value).find(".")+(ROUND_TO+1));

                                msg_out += "250: ";
                                msg_out += rounded;
                                msg_out += "\n";

                                t->seq_flag = 0;
                            }
                            catch(invalid_argument&)// invalid argument
                            {
                                msg_out += "501 invalid argument (expected: double)\n";
                            }
                        }
                        else // bad arguments (more than 1)
                        {
                            msg_out += "501 Too many arguments provided (expected: 2)\n";
                        }
                    }
                    else // Bad arguments (only 1)
                    {
                        msg_out += "501 1 argument provided (expected: 2)\n";
                    }
                }
                else // Bad arguments (no args)
                {
                    msg_out += "501 No arguments provided (expected: 2)\n";
                }
            }
            else // wrong sequence
            {
                msg_out += "503 Wrong Command order: CYLINDER or CIRCLE before AREA\n";
            }
        }
        else if(msg_in_lst->data == "CIRC") // part of CIRCLE
        {
            if(t->seq_flag == 1) 
            {
                if(msg_in_lst->next != NULL && msg_in_lst->next != empty_str_list)
                {
                    if(msg_in_lst->next->next == NULL) // correct sequence
                    {
                        try
                        {
                            double d = stod(msg_in_lst->next->data);
                            
                            double value = 2*M_PI*(sqrt(d/M_PI));
                            string rounded = to_string(value).substr(0, to_string(value).find(".")+(ROUND_TO+1));

                            msg_out += "250: ";
                            msg_out += rounded;
                            msg_out += "\n";

                            t->seq_flag = 0;
                        }
                        catch(invalid_argument&)// invalid argument
                        {
                            msg_out += "501 invalid argument (expected: double)\n";
                        }
                    }
                    else // bad arguments (more than 1)
                    {
                        msg_out += "501 Too many arguments provided (expected: 1)\n";
                    }
                }
                else // Bad arguments (no args)
                {
                    msg_out += "501 No arguments provided (expected: 1)\n";
                }
            }
            else // wrong sequence
            {
                msg_out += "503 Wrong Command order: CIRCLE before CIRC\n";
            }
        }
        else if(msg_in_lst->data == "SPHERE") //seq_flag = 2
        {
            if(msg_in_lst->next == NULL) //correct command issued
            {
                if(t->seq_flag == 0) //correct sequence 
                {
                    t->seq_flag = 2;
                    msg_out += "220 SPHERE ready!\n";
                }
                else // bad sequence
                {
                    msg_out += "503 cannot call SPHERE, client in sequence\n";
                }

            }
            else // no parameters accepted
            {
                msg_out += "501 No arguments accepted for SPHERE\n";
            }
        }
        else if(msg_in_lst->data == "VOL") //part of SPHERE
        {
            if(t->seq_flag == 2) 
            {
                if(msg_in_lst->next != NULL && msg_in_lst->next != empty_str_list)
                {
                    if(msg_in_lst->next->next == NULL) // correct sequence
                    {
                        try
                        {
                            double d = stod(msg_in_lst->next->data);
                            
                            double value = (4/3)*(M_PI)*(d*d*d);
                            string rounded = to_string(value).substr(0, to_string(value).find(".")+(ROUND_TO+1));

                            msg_out += "250: ";
                            msg_out += rounded;
                            msg_out += "\n";

                            t->seq_flag = 0;
                        }
                        catch(invalid_argument&)// invalid argument
                        {
                            msg_out += "501 invalid argument (expected: double)\n";
                        }
                    }
                    else // bad arguments (more than 1)
                    {
                        msg_out += "501 Too many arguments provided (expected: 1)\n";
                    }
                }
                else // Bad arguments (no args)
                {
                    msg_out += "501 No arguments provided (expected: 1)\n";
                }
            }
            else // wrong sequence
            {
                msg_out += "503 Wrong Command order: SPHERE before VOL\n";
            }
        }
        else if(msg_in_lst->data == "RAD") // part of SPHERE
        {
            if(t->seq_flag == 2) 
            {
                if(msg_in_lst->next != NULL && msg_in_lst->next != empty_str_list)
                {
                    if(msg_in_lst->next->next == NULL) // correct sequence
                    {
                        try
                        {
                            double d = stod(msg_in_lst->next->data);
                            
                            double value = (1/2)*(sqrt(d/M_PI));
                            string rounded = to_string(value).substr(0, to_string(value).find(".")+(ROUND_TO+1));

                            msg_out += "250: ";
                            msg_out += rounded;
                            msg_out += "\n";

                            t->seq_flag = 0;
                        }
                        catch(invalid_argument&)// invalid argument
                        {
                            msg_out += "501 invalid argument (expected: double)\n";
                        }
                    }
                    else // bad arguments (more than 1)
                    {
                        msg_out += "501 Too many arguments provided (expected: 1)\n";
                    }
                }
                else // Bad arguments (no args)
                {
                    msg_out += "501 No arguments provided (expected: 1)\n";
                }
            }
            else // wrong sequence
            {
                msg_out += "503 Wrong Command order: SPHERE before RAD\n";
            }
        }
        else if(msg_in_lst->data == "CYLINDER") //seq_flag = 3
        {
            if(msg_in_lst->next == NULL) //correct command issued
            {
                if(t->seq_flag == 0) //correct sequence 
                {
                    t->seq_flag = 3;
                    msg_out += "220 CYLINDER ready!\n";
                }
                else // bad sequence
                {
                    msg_out += "503 cannot call CYLINDER, client in sequence\n";
                }
            }
            else // no parameters accepted
            {
                msg_out += "501 No arguments accepted for CYLINDER\n";
            }
        }
        else if(msg_in_lst->data == "HGT")
        {
            if(t->seq_flag == 3) 
            {
                if(msg_in_lst->next != NULL && msg_in_lst->next != empty_str_list) // check if 1 arg
                {
                    if(msg_in_lst->next->next != NULL && msg_in_lst->next->next != empty_str_list) // check for second arg
                    {
                        if(msg_in_lst->next->next->next == NULL) // correct sequence
                        {
                            try
                            {
                                double v = stod(msg_in_lst->next->data);
                                double r = stod(msg_in_lst->next->next->data);
                                
                                double value = (v)/(M_PI*(r*r));
                                string rounded = to_string(value).substr(0, to_string(value).find(".")+(ROUND_TO+1));

                                msg_out += "250: ";
                                msg_out += rounded;
                                msg_out += "\n";

                                t->seq_flag = 0;
                            }
                            catch(invalid_argument&)// invalid argument
                            {
                                msg_out += "501 invalid argument (expected: double double)\n";
                            }
                        }
                        else // bad arguments (more than 1)
                        {
                            msg_out += "501 Too many arguments provided (expected: 2)\n";
                        }
                    }
                    else // Bad arguments (only 1)
                    {
                        msg_out += "501 1 argument provided (expected: 2)\n";
                    }
                }
                else // Bad arguments (no args)
                {
                    msg_out += "501 No arguments provided (expected: 2)\n";
                }
            }
            else // wrong sequence
            {
                msg_out += "503 Wrong Command order: CYLINDER before HGT\n";
            }
        }
        else if(msg_in_lst->data == "BYE")
        {
            if(msg_in_lst->next != NULL && msg_in_lst->next != empty_str_list)
            {
                if(msg_in_lst->next->data == get_hostname_str()) // valid call and hostname
                {
                    msg_out += "200 BYE ";
                    msg_out += t->ip.c_str();
                    msg_out += "\n";

                    printf("Client: %s leaving sequence\n", t->ip.c_str());

                    if(t->prev == NULL)// if t is top of list
                    {
                        if(t->next->ip == "") // t is top with empty next
                        {
                            t->ip = "";
                            t->seq_flag = -1;
                            delete(t->next);
                            t->next = NULL;
                        }
                        else // t is top with not empty next
                        {
                            t->ip = t->next->ip;
                            t->seq_flag = t->next->seq_flag;
                            connector_list* del = t->next;
                            t->next = t->next->next;
                            delete(del);
                        }
                    }
                    else // t is not top (next could be empty or not empty)
                    {
                        if(t->next->ip == "")
                        {
                            t->ip = "";
                            t->seq_flag = -1;
                            delete(t->next);
                            t->next = NULL;
                        }
                        else
                        {
                            //connector_list* del = t;
                            t->prev->next = t->next;
                            t->next->prev = t->prev;
                            //delete(del);
                        }
                    }

                    printf("Clients in sequence:\n");
                    for(connector_list* i = active_connectors; i->ip!=""; i=i->next)
                    {
                        printf("%s\n", i->ip.c_str());
                    }
                }
                else // bad argument, wrong hostname
                {
                    msg_out += "501 invalid hostname (try: BYE ";
                    msg_out += get_hostname_str();
                    msg_out += ")\n";
                }
            }
            else // bad arguments, no args
            {
                msg_out += "501 no hostname provided (try: BYE ";
                msg_out += get_hostname_str();
                msg_out += ")\n";
            }
        }
        else // unrecognized command
        {
            msg_out += "500 Syntax Error, command unrecognized\n";
        }
    }
    else // client not in sequence
    {
        msg_out += "500 syntax error, client not in sequence (try HELO ";
        msg_out += get_hostname_str();
        msg_out += ")\n";
    }


    return msg_out;
}

int main(void)
{
    int numbytes, sockfd, addr_info_status;
    int yes=1;
    
    struct sigaction sig_act;
    struct addrinfo hints, *serv_info, *serv_info_ptr;
    struct sockaddr_storage client_addr;
    struct connector_list* active_connectors = new connector_list;
    active_connectors->ip = "";

    socklen_t client_addr_len = sizeof client_addr;

    string client_ip_str;
    char in_buf[MAXBUFLEN];
    char client_ip[INET6_ADDRSTRLEN];

    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_DGRAM;
    hints.ai_flags = AI_PASSIVE;
    char port[50];

    read_config(port);

    if ((addr_info_status = getaddrinfo(NULL, port, &hints, &serv_info)) != 0) 
    {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(addr_info_status));
        return 1;
    }

    // loop through all the results and bind to the first we can
    for(serv_info_ptr = serv_info; serv_info_ptr != NULL; serv_info_ptr = serv_info_ptr->ai_next) 
    {
        // try to socket
        if ((sockfd = socket(serv_info_ptr->ai_family, serv_info_ptr->ai_socktype, serv_info_ptr->ai_protocol)) == -1) 
        {
            perror("server: socket");
            continue;
        }

        // Keep port open if error
        if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) 
        {
            perror("setsockopt");
            exit(EXIT_FAILURE);
        }

        // try to bind
        if (bind(sockfd, serv_info_ptr->ai_addr, serv_info_ptr->ai_addrlen) == -1) 
        {
            close(sockfd);
            perror("server: bind");
            continue;
        }
        break;
    }

    freeaddrinfo(serv_info);

    if (serv_info_ptr == NULL)  
    {
        perror("server failed to bind\n");
        exit(EXIT_FAILURE);
    }

    sig_act.sa_handler = sigchld_handler;
    sigemptyset(&sig_act.sa_mask);
    sig_act.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sig_act, NULL) == -1) 
    {
        perror("sigaction");
        exit(EXIT_FAILURE);
    }
    printf("Hostname: %s\n", get_hostname_str().c_str());
    printf("server: waiting for connections...\n");

    while(true)
    {
        if((numbytes = recvfrom(sockfd, in_buf, MAXBUFLEN+1, 0, (struct sockaddr*)&client_addr, &client_addr_len)) < 1)
        {
            perror("recvfrom");
            exit(EXIT_FAILURE);
        }

        in_buf[numbytes]= '\0';
        
        //getting the clients ip address
        inet_ntop(client_addr.ss_family, &(((struct sockaddr_in*)(struct sockaddr*)&client_addr)->sin_addr),client_ip, sizeof client_ip);
        client_ip_str = string(client_ip);  
        printf("\nrecvfrom: %s\n", client_ip_str.c_str());
        
        //getting the connection type 
        // TODO
        
        // read clients message and generate response to store in msg_out
        string msg_out = process_client_msg(in_buf, active_connectors, client_ip_str, "UDP");    
        
        char out_buf[msg_out.size()];                 
        strcpy(out_buf, msg_out.c_str());              
        
        // Send message to client
        if((numbytes = sendto(sockfd, &out_buf, sizeof(out_buf)+1, 0, (struct sockaddr*)&client_addr, sizeof client_addr))==-1)
        {
            perror("sendto");
            exit(EXIT_FAILURE);
        }
        printf("sendto: %s\n", client_ip);
    }
    return 0;
}