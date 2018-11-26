# Software Defined Network

Client Server program using TCP Sockets and FIFOs

Note:__
Controller is the Server & Switches are the Clients.__

Run the app from command line:__
 1) Running Server on one terminal__
  => Usage: ./a3sdn cont nSwitch portNumber"__
  where __
   cont = controller, __
   nswitch = number of clients/switches (ex: 2), __
   portNumber = port Number (ex: 997)__
  
 2) Running Client on another terminal__
  => Usage: ./a3sdn swi trafficFile [null|swj] [null|swj] IPlow-IPhigh serverAddress portNumber__
  where __
  swi = client (ex: sw1, sw2, sw3, etc), __
  trafficFile = test File (ex: t2.dat),__
  [null|swj] = left connected switch (ex: sw1) (null stands for nothing connected)__
  [null|swj] = right connected switch (ex: sw3) (null stands for nothing connected)__
  IPlow-IPhigh = integer-integer (ex: 100-120)______
  serverAddress = Server Address (ex: localhost, 0.0.0.0 127.0.0.1, etc.)____
  portNumber = port Number (ex: 997)__
  
