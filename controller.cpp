/*
AMAN ANAND
CMPUT-379 Assignment#3
*/

#include<stdlib.h>
#include<string.h>
#include<errno.h>
#include<stdio.h> // for remove
#include<iostream> // std
#include<unistd.h>
#include<fcntl.h>
#include<fstream> // for ifstream
#include<sys/stat.h>
#include <signal.h>
#include<string>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;


string SwitchInfo[MAX_Switches][4]; // [i] -> nswitch|port1|port2|port3
int PacketStats[4]={0}; // [i] -> open|query|ack|add

/*
Triggered when user inputs list/exit
print the switch info and packet info
*/
void C_printList(){
  cout<<"Switch information:"<<"\n";
  string port1,port2;
  for(int i=0;i<MAX_Switches;i++){

    if(SwitchInfo[i][0]=="") continue;

    if(SwitchInfo[i][1]=="null") port1="-1";
    else port1=SwitchInfo[i][1][2];

    if(SwitchInfo[i][2]=="null") port2="-1";
    else port2=SwitchInfo[i][2][2];

    //Switch Info
    cout<<"[sw"<<SwitchInfo[i][0]<<"] port1= "<<port1<<", ";
    cout<<"port2= "<<port2<<", port3= "<<SwitchInfo[i][3]<<"\n";
  }
  cout<<"\n";
  //Packet stats
  cout<<"Packet Stats:"<<"\n";
  cout<<"    Recieved:    OPEN:"<< PacketStats[0] <<", QUERY:"<< PacketStats[1] <<"\n";
  cout<<"    Transmitted: ACK:"<< PacketStats[2] <<", ADD:"<< PacketStats[3] <<"\n\n";
}

/*
poll keyboard
When Triggered calls C_printList to print PacketStats on screen
*/
int C_pollKeyboard(struct pollfd pollter[1]){

  string command="";
  char buf[MAXBUF];
  int pollResult;

  pollter[0].fd=0;
  pollter[0].events = POLLIN;
  pollter[0].revents= 0;

  pollResult = poll(pollter, 1, 0);

  if(pollter[0].revents & POLLIN) {
    read(0, buf, MAXBUF);
    command = string(buf);

    if(command.substr(0,4)=="list"){
      C_printList();
    } else if (command.substr(0,4) == "exit"){
        C_printList();
        return -1;
    }

  } else if(pollResult==-1) {
      cout<<"In Controller : Erorr on polling fifo\n";
  } else { }// timeout
  fflush(stdout);
  return 1;
}

/*
Checks if IP exists in the Switches Connected
returns switch number if true else -1
*/
int CheckIPExists(int d_IP){
  int dest_sw=-1;

  for(int i=0;i<MAX_Switches;i++){
    if(SwitchInfo[i][0]=="") break; // finished infos

    int IPlow= stoi(SwitchInfo[i][3].substr(0,SwitchInfo[i][3].find("-")));
    int IPhigh=stoi(SwitchInfo[i][3].substr(SwitchInfo[i][3].find("-")+1,\
                  SwitchInfo[i][3].length()));

    if(IPlow<=d_IP  && IPhigh>=d_IP){ // check dest IP
        dest_sw=stoi(SwitchInfo[i][0]);
        break;
    }
  }
  return dest_sw;
}

/*
Recurse through the Switches connected and checks
if Switch is reachable from left neighbour

**
Ended up not using but may be better for a generic program which has
random names to Clients rather than switch number
As for this assignment according to the requirements spec that
sw1 is always connected to sw2 from left

*/
bool CheckLeftReachable(int src_sw,int dest_sw){
  bool isReachable=false;

  for(int i=0;i<MAX_Switches;i++){
    if(SwitchInfo[i][0]=="") break; // finished infos

    if(stoi(SwitchInfo[i][0])!=src_sw) continue;

    int port1;

    if(SwitchInfo[i][1]=="null") port1=-1;
    else port1=SwitchInfo[i][1][2] - '0';

    // as dest null so not reachable
    if( port1==-1 ){
      return false;
    }
    else if( port1==dest_sw ) {
      return true;
    }
    else { // recursion
      int source_sw=port1; // basically the dstination
      int destination_sw=dest_sw;
      return CheckLeftReachable(source_sw,destination_sw);
    }
  }
  return isReachable;
}

/*
Recurse through the Switches connected and checks
if Switch is reachable from right neighbour

**
Ended up not using but may be better for a generic program which has
random names to Clients rather than switch number
As for this assignment according to the requirements spec that
sw1 is always connected to sw2 from left

*/
bool CheckRightReachable(int src_sw,int dest_sw){
  bool isReachable=false;

  for(int i=0;i<MAX_Switches;i++){
    if(SwitchInfo[i][0]=="") break; // finished infos

    if(stoi(SwitchInfo[i][0])!=src_sw) continue;

    int port2;

    if(SwitchInfo[i][2]=="null") port2=-1;
    else port2=SwitchInfo[i][2][2] - '0';

    // as dest null so not reachable
    if( port2==-1 ){
      return false;
    }
    else if( port2==dest_sw ) {
      return true;
    }
    else { // recursion
      int source_sw=port2; // basically the dstination
      int destination_sw=dest_sw;
      return CheckRightReachable(source_sw,destination_sw);
    }
  }
  return isReachable;
}

/*
Returns the left neighbour of the client
**
Ended up not using but may be better for a generic program which has
random names to Clients rather than switch number
As for this assignment according to the requirements spec that
sw1 is always connected to sw2 from left

*/
string LeftNeighbour(int sw){
  string port1="0";

  for(int i=0;i<MAX_Switches;i++){
    if(stoi(SwitchInfo[i][0])==sw) return string(1,SwitchInfo[i][1][2]);
  }
  return port1;
}

/*
Returns the right neighbour of the client
**
Ended up not using but may be better for a generic program which has
random names to Clients rather than switch number
As for this assignment according to the requirements spec that
sw1 is always connected to sw2 from left

*/
string RightNeighbour(int sw){
  string port2="0";
  for(int i=0;i<MAX_Switches;i++){
    if(stoi(SwitchInfo[i][0])==sw) return string(1,SwitchInfo[i][2][2]);
  }
  return port2;
}

/*
Queries the packet and checks if IP exists
returns Drop or Forward packet
*/
string QueryPacket (string srcIP,string destIP,int src_sw){

  string Q_result="0";

  int dest_sw = CheckIPExists(stoi(destIP));

  if(dest_sw==-1){ // drop the packet as no dest IP
    return "D";
  }

  return "A" + to_string(dest_sw);

  return Q_result;
}

/*
Signal Handler for SIGUSER1
Prints the Switch Info and PacketStats
*/
void controllerHandler(int signum)
{
  cout<<"\nCaught signal SIGUSER \n";
	C_printList();
}

// Main Controller Loop
void Controller(int switches,int portNumber){
  cout<<"Controller Output\n==================\n";

  struct pollfd pollfd[7]; // for switches
  struct pollfd pollter[1]; // terminal
  struct pollfd pollfd_listen[1]; // for listening to incoming connections
  int fdread,fdwrite;
  int pollResult;

  string command;
  char buf[MAXBUF];


  string ControllerWriteFifo[MAX_Switches] = {"fifo-0-1","fifo-0-2","fifo-0-3",\
                                "fifo-0-4","fifo-0-5","fifo-0-6","fifo-0-7"};
  string ControllerReadFifo[MAX_Switches] = {"fifo-1-0","fifo-2-0","fifo-3-0",\
                                "fifo-4-0","fifo-5-0","fifo-6-0","fifo-7-0"};

  // Catch the SIGUSER1 signal
  signal(SIGUSR1, controllerHandler);

  // used for parsing strings
  int counter=0;
  size_t pos = 0;
  string temp;


  // For init sockets *******************************************************

  //create a socket
  int listening = socket(AF_INET, SOCK_STREAM, 0);
  if(listening==-1){
    cout<<"Cannot create a socket!";
    return;
  }

  // bind the socket
  sockaddr_in address;

  string IPaddress = "0.0.0.0";

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(portNumber); // port number
  // inet_pton(AF_INET, IPaddress.c_str(), &address.sin_addr);

  if( ::bind(listening, (sockaddr*) &address, sizeof(address)) == -1 ){
    cout << "Cannot bind to IP/port";
    return;
  }

  // Mark the socket for listening in

  if(listen(listening, SOMAXCONN ) == -1 ){
    cout << "Can't listen!";
    return;
  }

  pollfd_listen[0].fd=listening;;
  pollfd_listen[0].events = POLLIN;
  pollfd_listen[0].revents= 0;

  // For init sockets *******************************************************

  int sw_connected = 0; // number of switches connected
  int sw_closed[7]={0,0,0,0,0,0,0};
  while(true){

    // /* *************** Poll the keyboard ****************** */
    if(C_pollKeyboard(pollter)<0) break;
    // /* **************************************************** */

    /* *************** Poll the switches ****************** */

    int clientSocket=-1; // initialize the client socket

    // check for incoming connections from switches
    pollResult=poll(pollfd_listen, 1, 0);

    if(pollfd_listen[0].revents && POLLIN) {
      // Accept a call
      sockaddr_in client;
      socklen_t clientSize = sizeof(client);
      char host[NI_MAXHOST]; // Client's remote name
      char service[NI_MAXSERV]; // Service (i.e. port) the client is connect on

      clientSocket = accept(listening, (sockaddr*) &client, &clientSize);
      if(clientSocket == -1){
        cout << "Problem with client connecting!";
        return;
      }

      // Clean things up
      memset(host, 0, NI_MAXHOST);
      memset(service, 0, NI_MAXSERV);

      int result = getnameinfo( (sockaddr*) &client, sizeof(client),\
                                host, NI_MAXHOST, service, NI_MAXSERV, 0);

      if(result){
        // cout << "host" << host << ", connected on " << service << endl;
      }
      else{
        inet_ntop (AF_INET, &client.sin_addr, host, NI_MAXHOST);
        // cout << "host" << " connected on " << ntohs(client.sin_port) << endl;
      }

      // cout<<"Sw connected: "<<sw_connected<<endl;

      pollfd[sw_connected].fd=clientSocket;
      pollfd[sw_connected].events = POLLIN;
      pollfd[sw_connected].revents= 0;

      sw_connected++;
      sw_closed[sw_connected]=0;
    }

    //empty variables
    command="";
    memset(buf, 0, MAXBUF);

    // poll and read
    pollResult=poll(pollfd, sw_connected, 0);

    for(int i=0;i<sw_connected;i++){

      // means its closed and no need to poll
      if(sw_closed[i]==1){
        continue;
      }

      if(pollfd[i].revents && POLLIN) {

        int bytesRecv = read(pollfd[i].fd, buf, MAXBUF);
        command = string(buf);

        if(bytesRecv == -1){
          cerr << "There was a connection issue" << endl;
          break;
        }
        else if(bytesRecv == 0 && sw_closed[i]==0){
          cout<< "Lost connection to sw" << i+1 << endl;
          sw_closed[i]=1;
          break;
        }

        // If packet is OPEN
        if(command.substr(0,1)=="0"){

          //empty the variables
          counter=0;
          pos = 0;
          temp="";
          // parse the string from switch AND update the switch info
          while ((pos = command.find("|")) != string::npos) {
              if(counter==4){
                  temp=command.substr(0, pos);
              }else if(counter==5){
                  SwitchInfo[i][3]=temp+"-"+command.substr(0, pos);
              }
              else if(counter!=0){
                SwitchInfo[i][counter-1]=command.substr(0, pos);
              }
              counter++;
              command.erase(0,pos+1);
          }

          // print Open message on screen
          cout<<"Received (src= sw"<<i+1;
          cout<<", dest= cont) [OPEN]:"<<endl;
          cout<<"         (port0= cont, port1= "<<SwitchInfo[i][1];
          cout<<", port2= "<<SwitchInfo[i][2]<<", port3= ";
          cout<<SwitchInfo[i][3]<<")"<<endl;

          // send ACK
          char msg_ack[MAXBUF];
          string msg_ack_temp = "ACK";
          strcpy(msg_ack, msg_ack_temp.c_str());

          int sendResult = write(pollfd[i].fd, msg_ack, MAXBUF);
          if(sendResult==-1)
          {
              cout << "Unable to send ACK packet";
              close(clientSocket);
              return;
          }

          // Write the ACK message to screen
          cout<<"Transmitted (src= cont, dest= sw"<<i+1;
          cout<<")[ACK]"<<"\n";

          // update the packet stats
          PacketStats[0]=PacketStats[0]+1;
          PacketStats[2]=PacketStats[2]+1;
        }

        // If packet is QUERY
        if(command.substr(0,1)=="1"){

          string str=command;
          string srcIP,destIP;

          //empty the variables
          counter=0;
          pos = 0;
          temp="";
          while ((pos = str.find("|")) != string::npos) {
              if(counter==1) srcIP=str.substr(0, pos);
              else if(counter==2) destIP=str.substr(0, pos);
              str.erase(0,pos+1);
              counter++;
          }

          // update the QUERY in packet stats
          PacketStats[1]=PacketStats[1]+1;

          // print QUERY on message screen
          cout<<"Received (src= sw"<<i+1<<", dest= cont) [QUERY]:  ";
          cout<<"header= (srcIP= "<<srcIP<<", destIP= "<<destIP<<")"<<endl;

          string Q_result = QueryPacket(srcIP,destIP,i+1);

          // empty the variables
          memset(buf, 0, MAXBUF);
          string query_msg;
          string destIP_low,destIP_high;

          if (Q_result=="D") {
            query_msg = "ADD|D|" + destIP + "|"; // Drop the Packet
          }
          else if(Q_result.substr(0,1)=="A") { // Forward the packet
            int sw_no=stoi(Q_result.substr(1,sizeof(Q_result)))-1;
            destIP_low=SwitchInfo[sw_no][3].substr(0,SwitchInfo[i][3].find("-"));
            destIP_high=SwitchInfo[sw_no][3].substr(SwitchInfo[i][3].find("-")+1,\
                          SwitchInfo[i][3].length());
            query_msg = "ADD|F|" + Q_result.substr(1,sizeof(Q_result)) + "|" \
                          + destIP_low + "|" + destIP_high +"|";
          }
          else if(Q_result=="0"){
            cout<<"In Controller, Error while Quering Packet";
            return; // should never happen
          }

          strcpy(buf, query_msg.c_str());

          int sendResult = write(pollfd[i].fd, buf, MAXBUF);
          if(sendResult==-1)
          {
              cout << "Unable to send ADD packet";
              close(pollfd[i].fd);
              return;
          }

         // print the QUERY message on the screen
         cout<<"Transmitted (src= cont, dest= sw"<<i+1<<")[ADD]:"<<endl;

         if(Q_result=="D")  {
           cout<<"            (srcIP= 0-1000, destIP= "<<destIP<<"-"<<destIP<<",";
           cout<<" action= DROP:0, ";
         }
         else {
           cout<<"            (srcIP= 0-1000, destIP= "<<destIP_low<<"-"<<destIP_high<<",";
           cout<<" action= FORWARD:"<<Q_result.substr(1,sizeof(Q_result))<<", ";
         }
         cout<<"pri= 4, pktCount= 0)"<<endl;

          // update the ADD in packet stats
          PacketStats[3]=PacketStats[3]+1;
        }

      } else if(pollResult==-1) {
          cout<<"In Controller : Erorr on polling \n";
          return;
      } else { }// timeout
        // close(pollfd[i].fd);
      // }
    }

    /* **************************************************** */
  }
  // close all file descripters
  for(int i=0; i<sw_connected;i++){
    close(pollfd[i].fd);
  }
  // Close the listening socket
  close (listening);
}

