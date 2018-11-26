/*
AMAN ANAND
CMPUT-379 Assignment#3
*/

#include<string.h>
#include<iostream>
#include"switch.cpp"
#include"controller.cpp"
using namespace std;

// returns bool : if string is an integer
bool is_integer(string str){
    for(string::iterator it = str.begin(); it != str.end(); ++it)
      if(!isdigit(*it)) return false;
    return true;
}

// exit code bad input
void C_exit_code(){
  cout<<"Bad Arguments\n"<<"Usage: ";
  cout<<"cont nSwitch portNumber\n";
  cout<<"\n";
}

void S_exit_code(){
  cout<<"Bad Arguments\n"<<"Usage: ";
  cout<<"swi trafficFile [null|swj] [null|swj] IPlow-IPhigh ";
  cout<<"serverAddress portNumber\n"<<endl;
}

int main (int argc, char *argv[]) {
  cout<<"\n";

  // check for input
  if(argc<4){
    C_exit_code(); return 0;
  }

  string arg1 = argv[1];

  // Controller
  if(arg1=="cont"){

    if(!is_integer(argv[2])){
      C_exit_code(); return 0;
    }
    // check igf port number is an integer
    else if(!is_integer(argv[3])){
      C_exit_code(); return 0;
    }

    int switches_count = stoi(argv[2]);
    if(switches_count>7 || switches_count<1 ){
      cout<<"Switches count should be between 1 and 7.\n\n";
      return 0;
    }

    Controller(switches_count,stoi(argv[3]));
    return 0;
  }

  // Switch
  else if(arg1.substr(0,2)=="sw"){

    // check if i is an integer in => "swi" and if user enterd 7 arguments
    if(!(arg1.length()==3 && isdigit(arg1[2]) && argc==8)){
      S_exit_code(); return 0;
    }
    else if(argc==8){
      //check if port number is an integer
      if(!is_integer(argv[7])){
        cout<<"Port Number should be an integer"<<endl;
        S_exit_code(); return 0;
      }
    }
    /* Start Switch */
    Switch(argv[1],argv[2],argv[3],argv[4],argv[5],argv[6],stoi(argv[7]));
  }

  cout<<"\n";
  return 0;
}

