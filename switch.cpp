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
#include<sys/times.h>
#include <poll.h>
#include <sys/socket.h>
#include <netdb.h>
#include <arpa/inet.h>

using namespace std;

const int MAXBUF=1200;
const int MAX_Switches = 7 ;

// [i] -> ADMIT|ACK|ADDRULE|RELAYIN|OPEN|QUERY|RELAYOUT
int S_PacketStats[7]={0};

/* Format of FlowTable
[srcIP lo, srcIP hi, destIP lo, destIP hi, actionType, actionVal, pri, pktCount]
*/
string FlowTable[110][8];

// counter to add to correct index of flow table
int counter_FT=0;

// Write to Fifo
void WriteToFifo(int fd, char buf[MAXBUF]){
  write (fd, buf, MAXBUF);
}

/*
Forwards the packet to the switch number provided by the Flow table
*/
void forwardPacket(int ruleExistAt,string curr_switch,int srcIP,int destIP){

  int fdwrite;
  string command="";
  char buff[MAXBUF];

  string path = "fifo-";
  // string msg_relay="R|"+FlowTable[ruleExistAt][2]+"|"+\
  //                   FlowTable[ruleExistAt][3]+"|";
  string msg_relay="R|"+to_string(srcIP)+"|"+\
                    to_string(destIP)+"|";

  int curr_sw = curr_switch[2]-'0';

  int destination_sw;
  if( (stoi(FlowTable[ruleExistAt][5])== curr_sw - 1 ) || \
      (stoi(FlowTable[ruleExistAt][5])== curr_sw + 1 ) )
  {
    destination_sw = stoi(FlowTable[ruleExistAt][5]);
  }
  else if( stoi(FlowTable[ruleExistAt][5]) > curr_sw ){
    destination_sw = curr_sw + 1;
  }
  else{
    destination_sw = curr_sw - 1 ;
  }

  path = path + curr_switch[2] + "-" + to_string(destination_sw);
  if ((fdwrite = open(path.c_str(), O_RDWR)) < 0)
      cout<<"In Switch : open fifo error for writing";

  strcpy(buff, msg_relay.c_str());

  // write fifo
  WriteToFifo(fdwrite,buff);

  // sleep(1);
  // fflush(stdout);

  // update the RELAOUT
  S_PacketStats[6]=S_PacketStats[6]+1;

  cout<<"Transmitted (src= sw"<<curr_switch[2]<<", dest= sw"<<\
        destination_sw<<") [RELAY]:  header= (srcIP= "<<\
        srcIP<<", destIP= "<<\
        destIP<<")"<<endl;

  // close fifo
  // close(fdwrite);
}

/*
Switch recieves new packet
Checks if rule exsits in FlowTable else Query the Controller for the
rule and takes the required action
*/
void newPacket(int srcIP, int destIP,int ruleExistAt,string curr_switch,int sock){

  char buff[MAXBUF];
  int fdwrite,counter,pollResult,fdread;
  size_t pos;
  string command,path;

  fflush(stdout);
  if(ruleExistAt>=0){ // perform the action based on the rule

      if(FlowTable[ruleExistAt][4]=="FORWARD"){ // action type
        // if action val=3 then do nothing
        // else forward the packet to the action value port
        if(FlowTable[ruleExistAt][5]!="3"){ // action value
          forwardPacket(ruleExistAt,curr_switch,srcIP,destIP);
        }
      }
      // update the pktCount
      FlowTable[ruleExistAt][7]=to_string(stoi(FlowTable[ruleExistAt][7])+1);
  }else{
     // ask the controller for the rule
     // 1 for QUERY Packet
     string msg_query = "1|" + to_string(srcIP) + "|" + to_string(destIP) + "|";

     memset(buff, 0, MAXBUF);
     strcpy(buff, msg_query.c_str());

     // write to controller
     write(sock, buff, MAXBUF);

     // update the query count for the packet
     S_PacketStats[5]=S_PacketStats[5]+1;

     cout<<"Transmitted (src= sw"<<curr_switch[2]<<", dest= cont) [QUERY]:";
     cout<<"  header= (srcIP= "<<srcIP<<", destIP= "<<destIP<<")"<<endl;
     fflush(stdout);

     // read from controller
     //empty variables
     command="";
     memset(buff, 0, MAXBUF);

     struct pollfd poll_ADD[1];
     poll_ADD[0].fd=sock;
     poll_ADD[0].events = POLLIN;
     poll_ADD[0].revents= 0;

     // poll the controller fd
     pollResult=poll(poll_ADD, 1, -1);

     if(poll_ADD[0].revents && POLLIN) {
       int bytesRecv = read(sock, buff, MAXBUF);

       command = string(buff);

       if(bytesRecv>0){

         // cout<<"Messge recieved: " << command <<endl;

         // If packet is ADD
         if(command.substr(0,1)=="A"){

           // update the ADDRULE count for the packet
           S_PacketStats[2]=S_PacketStats[2]+1;

           cout<<"Received (src= cont, dest= sw"<<curr_switch[2]<<") [ADD]:"<<endl;

           if(command.substr(4,1)=="D"){ // Drop Type Pakcet
               // parse the line
               string dropIP;
               counter=0;pos = 0;
               while ((pos = command.find("|")) != string::npos){
                   if(counter==2) dropIP=command.substr(0, pos);
                   counter++;
                   command.erase(0,pos+1);
               }

               // uppate the FLOW TABLE only
               FlowTable[counter_FT][0]="0";FlowTable[counter_FT][1]="1000";
               FlowTable[counter_FT][2]=dropIP;FlowTable[counter_FT][3]=dropIP;
               FlowTable[counter_FT][4]="DROP";FlowTable[counter_FT][5]="0";
               FlowTable[counter_FT][6]="4";FlowTable[counter_FT][7]="1";
               // update the Flow Table counter
               counter_FT++;

               cout<<"(srcIP= 0-1000, destIP= "<<dropIP<<"-"<<dropIP<<", ";
               cout<<"action= DROP:0, ";
               cout<<"pri= 4, pktCount= 0)"<<endl;
           }
           else if(command.substr(4,1)=="F"){ // Forward Type Packet
             string destIP_low,destIP_high,forward_switch;

             // parse the packet
             counter=0;pos = 0;
             while ((pos = command.find("|")) != string::npos){
                 if(counter==2) forward_switch=command.substr(0, pos);
                 if(counter==3) destIP_low=command.substr(0, pos);
                 if(counter==4) destIP_high=command.substr(0, pos);
                 counter++;
                 command.erase(0,pos+1);
             }

             // uppate the FLOW TABLE
             FlowTable[counter_FT][0]="0";FlowTable[counter_FT][1]="1000";
             FlowTable[counter_FT][2]=destIP_low;
             FlowTable[counter_FT][3]=destIP_high;
             FlowTable[counter_FT][4]="FORWARD";
             FlowTable[counter_FT][5]=forward_switch;
             FlowTable[counter_FT][6]="4";FlowTable[counter_FT][7]="1";
             // update the Flow Table counter
             counter_FT++;

             // print message on screen
             cout<<"(srcIP= 0-1000, destIP= "<<destIP_low<<"-"<<destIP_high<<", ";
             cout<<"action= FORWARD:"<<forward_switch<<", ";
             cout<<"pri= 4, pktCount= 0)"<<endl;

             ruleExistAt=counter_FT-1;
             forwardPacket(ruleExistAt,curr_switch,srcIP,destIP);
           }
         }
       }
     }
     else if(pollResult==-1) {
         cout<<"In Switch : Erorr on polling\n";
     } else { }// timeout
   }
}

/*
Checks if rule Exist in FlowTable
return index of the rule in FLowTable else -1
*/
int GetRuleIfExists(int sourceIP,int destIP){

  int ruleExistAt=-1; // means rule not found

  for(int i=0;i<counter_FT;i++){

    if( (stoi(FlowTable[i][0]) <= sourceIP) && \
          (stoi(FlowTable[i][1]) >= sourceIP) &&
          (stoi(FlowTable[i][2]) <= destIP) &&
          (stoi(FlowTable[i][3]) >= destIP) ){
            ruleExistAt=i;
            break;
    }
  }
  return ruleExistAt;
}

/*
Prints the FlowTable and PacketStats on STDIN
*/
void S_printList(){
  cout<<"Flow Table:"<<"\n";

  for(int i=0;i<counter_FT;i++){

    //Switch Info
    cout<<"["<<i<<"] (srcIP= "<<FlowTable[i][0]<<"-"<<FlowTable[i][1]<<", ";
    cout<<"destIP= "<<FlowTable[i][2]<<"-"<<FlowTable[i][3];
    cout<<", action= "<<FlowTable[i][4]<<":"<<FlowTable[i][5];
    cout<<", pri= "<<FlowTable[i][6]<<", pktCount= "<<FlowTable[i][7]<<")\n";
  }
  cout<<"\n";

  //Packet stats
  cout<<"Packet Stats:"<<"\n";
  cout<<"    Recieved:    ADMIT:"<< S_PacketStats[0] <<", ACK:"<< S_PacketStats[1];
  cout<< ", ADDRULE:"<< S_PacketStats[2] <<", RELAYIN:"<<S_PacketStats[3]<<"\n";
  cout<<"    Transmitted: OPEN:"<< S_PacketStats[4] <<", QUERY:"<< S_PacketStats[5];
  cout<<", RELAYOUT:"<< S_PacketStats[6]<<"\n\n";
}

/*
POLLS the keyboard
When Triggered prints the FlowTable and PacketStats on STDIN
*/
int S_pollKeyboard(struct pollfd pollter[1],int sock){
  string command="";
  char buff[MAXBUF];
  int pollResult;

  pollter[0].fd=0;
  pollter[0].events = POLLIN;
  pollter[0].revents= 0;

  pollResult = poll(pollter, 1, 0);

  if(pollter[0].revents & POLLIN) {
    read(0, buff, MAXBUF);
    command = string(buff);

    if(command.substr(0,4)=="list"){
      S_printList();
    } else if(command.substr(0,4)=="exit"){
        S_printList();
        close(sock); // closing the socket with server
        return -1;
    }

  } else if(pollResult==-1) {
      cout<<"In Switch : Erorr on polling fifo\n";
  } else { }// timeout
  return 0;
}

/*
Signal Handler for SIGUSER1
When Triggered Prints FLowTable and PacketStats
*/
void switchHandler(int signum)
{
  cout<<"\nCaught signal SIGUSER \n";
	S_printList();
}

// Main Switch Loop
void Switch(string curr_switch,string trafficFileName,string left_switch,\
            string right_switch,string IP,string serverAddress,int portNumber){
  cout<<"SW"<<curr_switch[2]<<" Output\n==========\n";

  const string MAXIP  = "1000";
  const string MINPRI = "4";

  // split high and low IP
  string IPlow  = IP.substr(0, IP.find("-"));
  string IPhigh = IP.substr( IP.find("-")+1,IP.length());

  // Initial rule
  FlowTable[counter_FT][0]="0";FlowTable[counter_FT][1]=MAXIP;
  FlowTable[counter_FT][2]=IPlow;FlowTable[counter_FT][3]=IPhigh;
  FlowTable[counter_FT][4]="FORWARD";FlowTable[counter_FT][5]="3";
  FlowTable[counter_FT][6]=MINPRI;FlowTable[counter_FT][7]="0";
  counter_FT++;


  // Catch the SIGUSER1 signal
  signal(SIGUSR1, switchHandler);

  // struct pollfd pollfd[7]; // for switches
  struct pollfd pollcr[1]; // for contorller
  struct pollfd pollter[1]; // terminal
  struct pollfd pollfd[8]; // for switches and a controller
  int fdread,fdwrite;
  int pollResult;

  // used to send and recieve msges
  string command;
  char buff[MAXBUF];

  bool msg_recieved=false;

  //empty variables
  command="";

  /* ************* OPEN CONNECTION AND RECIEVE ACK ***************/

  hostent * getHost = gethostbyname(serverAddress.c_str());
	if(getHost == NULL){
		cout<< "host is unavailable: " << serverAddress << endl;;
		return;
	}

	in_addr * host_addr = (in_addr * ) getHost->h_addr;
	string host_IP = inet_ntoa(* host_addr);
  serverAddress=host_IP;

  // for listening the controller
  // create a socket
  int sock = socket(AF_INET, SOCK_STREAM, 0);
  if(sock == -1){
    cout << "Can't create socket" << endl;
    return;
  }

  // the address structure
  sockaddr_in address;
  address.sin_family = AF_INET;
  address.sin_port = htons(portNumber);


  if(inet_pton(AF_INET, serverAddress.c_str(), &address.sin_addr) <= 0){
    cout<< "Invalid Address" << endl ;
    return;
  }

  // connect to server
  int connResult = connect(sock, (sockaddr*) &address, sizeof(address));
  if(connResult == -1){
    cout<< "Can't connect to server" << endl;
    close(sock);
    return;
  }

  string msg_open = "0|"; //0 for open packet
  msg_open = msg_open + curr_switch[2] + "|";msg_open = msg_open + left_switch + "|";
  msg_open = msg_open + right_switch + "|"; msg_open = msg_open + IPlow + "|";
  msg_open = msg_open + IPhigh + "|.";

  memset(buff, 0, MAXBUF);
  strcpy(buff, msg_open.c_str());

  int sendResult = write(sock, buff, MAXBUF);
  if(sendResult==-1)
  {
      cout << "Unable to send OPEN packet";
      close(sock);
      return;
  }

  // update packet stats
  S_PacketStats[4]=S_PacketStats[4]+1;
  // print message OPEN packet
  cout<<"Transmitted (src= sw"<<curr_switch[2]<<", dest= cont) [OPEN]:"<<endl;
	cout<<"            (port0= cont, port1= "<<left_switch<<", port2= "<<right_switch;
  cout<<", port3= "<<IPlow<<"-"<<IPhigh<<")"<<endl;
  fflush(stdout);


  pollcr[0].fd=sock;
  pollcr[0].events = POLLIN;
  pollcr[0].revents= 0;

  while(!msg_recieved){

    pollResult = poll(pollcr, 1, 0);

    if(pollcr[0].revents && POLLIN) {
      msg_recieved=true;

      int bytesRecv = read(sock, buff, MAXBUF);
      if(bytesRecv>0){
        command = string(buff);
        if(command=="ACK"){
          msg_recieved=true;
          S_PacketStats[1]=S_PacketStats[1]+1;
          cout<<"Received (src= cont, dest= sw"<<curr_switch[2]<<") [ACK]"<<endl;
        }
        else if(pollResult==-1) {
          cout<<"In Switch : Erorr on polling\n";
          return;
        }
        else {}// timeout
      }
    }
  }

  // close(pollcr[0].fd);
  /* ************************************************************/

  // Read the file in the traffileFIle ARRAY
  string trafficFile[100][2];
  string line;
  string str="";
  int fileLength,counter,file_counter;
  size_t pos = 0;

  ifstream myfile (trafficFileName);
  if (myfile.is_open())
  {
    while (getline(myfile,line)){

      // ignore the foll lines
      if(line.substr(0,1)=="") continue;
      else if(line.substr(0,1)=="#") continue;

      counter=0;
      str="";
      str+=line[0];
      //remove extra white spaces from line
      for (int i = 1; i < line.size(); i++)
      {
          if (line.substr(i,1) != " "){
              str+=line[i];
              counter=0;
          }else if(counter==0){
            str+=" ";
            counter++;
          }
      }

      if(str[2]!=curr_switch[2]) continue;
      // parse the line
      counter=0;pos = 0;
      while ((pos = str.find(" ")) != string::npos){
          str.erase(0,pos+1);
          if(counter==0) trafficFile[fileLength][0]=str.substr(0, pos);
          else if(counter==1) trafficFile[fileLength][1]=str.substr(0, pos);
          counter++;
      }
      fileLength++;
    }
  }
  myfile.close();

  int srcIP,destIP;

  // times command
  struct tms  tms_start, tms_end;
  clock_t start, end;

  // start the time
  start = times(&tms_start);

  bool TimerStarted = true;

  static long clktck = 0;
  clktck = sysconf(_SC_CLK_TCK);// fetch clock ticks per second first time

  // polls the keyboard / switches / controller
  while(true){

    /* ******* Process line of the trafficFile ************ */
    if(fileLength>file_counter){

      if(trafficFile[file_counter][0]=="del"){ // delay
        end = times(&tms_end);

        if(TimerStarted){
          cout<<endl<<"** Entering a delay period of "<<\
            trafficFile[file_counter][1]<< " msec\n"<<endl;
            TimerStarted=false;
        }

        if(float((end-start)/(double)clktck)==\
            float(stoi(trafficFile[file_counter][1])/1000))
            {
              cout<<"\n** Delay period ended\n\n";
              file_counter++;
              TimerStarted=true;
            }
      }
      else{
        srcIP=stoi(trafficFile[file_counter][0]);
        destIP=stoi(trafficFile[file_counter][1]);

        // update the admit count for packet
        S_PacketStats[0]=S_PacketStats[0]+1;

        //check if rule exists in the Flow Table
        int ruleExistAt=GetRuleIfExists(srcIP,destIP);

        // Process the new packet
        newPacket(srcIP,destIP,ruleExistAt,curr_switch,sock);

        file_counter++;
      }
    }
    // /* ***************************************************** */


    // /* *************** Poll the keyboard ****************** */
    if(S_pollKeyboard(pollter,sock)<0) return;
    // /* **************************************************** */

    /* ******* Poll The Controller and Switches *********** */

    //empty variables
    command="";
    memset(buff, 0, MAXBUF);

    // open all file descripters
    for(int i=0; i<MAX_Switches;i++){
      // i=1 to 7 switches

      // dont listen to myself(switch)
      if(i+1==curr_switch[2]) continue;

      string IncomingFifo,OutgoingFifo;
      IncomingFifo="fifo-" + to_string(i+1);
      IncomingFifo=IncomingFifo + "-" + curr_switch[2];

      if ((fdread = open(IncomingFifo.c_str(), O_RDWR)) < 0){ // bad fifo
          //cout<<"In Controller: failed to open fifo:"<<IncomingFifo<<"\n";
      }
      else{ }// good fifo

      pollfd[i].fd=fdread;
      pollfd[i].events = POLLIN;
      pollfd[i].revents= 0;
    }

    // poll and read
    pollResult=poll(pollfd, 7, 0);

    for(int i=0; i<MAX_Switches;i++){

      // dont listen to myself(switch)
      if(i+1==curr_switch[2]) continue;

      if(pollfd[i].revents && POLLIN) {
        read(pollfd[i].fd, buff, MAXBUF);
        command = string(buff);

        // Relay Packet
        if(command.substr(0,1)=="R"){
            // empty variables
            counter=0;pos = 0;
            srcIP=0;destIP=0;

            while ((pos = command.find("|")) != string::npos){
                if(counter==1) srcIP=stoi(command.substr(0, pos));
                else if(counter==2) destIP=stoi(command.substr(0, pos));
                command.erase(0,pos+1);
                counter++;
            }

            cout<<"Received (src= sw"<<i+1<<", dest= sw"<<curr_switch[2]<<\
                  ") [RELAY]:  header= (srcIP= "<<srcIP<<", destIP= "<<\
                  destIP<<")"<<endl;

            //check if rule exists in the Flow Table
            int ruleExistAt=GetRuleIfExists(srcIP,destIP);

            //Process the new packet
            newPacket(srcIP,destIP,ruleExistAt,curr_switch,sock);

            // update the RELAYIN for the current switch
            S_PacketStats[3]=S_PacketStats[3]+1;
        }

        fflush(stdout);

      } else if(pollResult==-1) {
          cout<<"In Switch : Erorr on polling fifo\n";
          return;
      } else { }// timeout
      close(pollfd[i].fd);
    }

    /* **************************************************** */
    // End of Loop
  }

}

