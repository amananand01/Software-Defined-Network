# Software Defined Network

Client Server program using TCP Sockets and FIFOs

Note:  
Controller is the Server & Switches are the Clients.  

Run the app from command line:  
 1) Running Server on one terminal  
  => Usage: ./a3sdn cont nSwitch portNumber"  
  where   
   cont = controller,   
   nswitch = number of clients/switches (ex: 2),   
   portNumber = port Number (ex: 997)  
  
 2) Running Client on another terminal   
  => Usage: ./a3sdn swi trafficFile [null|swj] [null|swj] IPlow-IPhigh serverAddress portNumber   
  where   
  swi = client (ex: sw1, sw2, sw3, etc),   
  trafficFile = test File (ex: t2.dat),   
  [null|swj] = left connected switch (ex: sw1) (null stands for nothing connected),  
  [null|swj] = right connected switch (ex: sw3) (null stands for nothing connected),  
  IPlow-IPhigh = integer-integer (ex: 100-120),   
  serverAddress = Server Address (ex: localhost, 0.0.0.0 127.0.0.1, etc.),   
  portNumber = port Number (ex: 997)  
  
