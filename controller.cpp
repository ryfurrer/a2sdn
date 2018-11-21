#include "controller.h"




#include <stdio.h> /* printf */
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>

#define BUF_SIZE 1024

int monitorSwitchSocket(const char* id, int socket) {
  struct pollfd pollSwitch[1];
  pollSwitch[0].fd = socket; //fd for incoming socket
	pollSwitch[0].events = POLLIN;

  while(true){
	   poll(pollSwitch, 1, 0);
	   if ((pollSwitch[0].revents&POLLIN) == POLLIN) {
       char buffer[32];
       if (recv(socket, buffer, sizeof(buffer), MSG_DONTWAIT) == 0) { //MSG_PEEK
         //http://www.stefan.buettcher.org/cs/conn_closed.html
         // if recv returns zero, that means the connection has been closed:
         // kill the child process
         printf("\n Note: Switchy %s be closed\n", id);
         close(socket);
         exit(EXIT_SUCCESS);
       }
     }
  }
}

int pollControllerSocket(int sfd) {
  int new_socket;
  struct sockaddr_in address;
  int addrlen = sizeof(address);
    struct pollfd pollSocket[1];
		pollSocket[0].fd = sfd;
		pollSocket[0].events = POLLIN;
		poll(pollSocket, 1, 0);
		if ((pollSocket[0].revents&POLLIN) == POLLIN) {
      if ((new_socket = accept(sfd, (struct sockaddr *)&address,
                       (socklen_t*)&addrlen))<0) {
        perror("accept");
        exit(EXIT_FAILURE);
      }

      printf("\nSocket connection accepted\n");
      pid_t pid = fork();
      if (pid == 0) {
        char buffer[32] = {0};
        int val = read( new_socket , buffer, 32);
        // child process
        //I don't want to change my assiment 2 code so the sockets will
        //be handled in a seperate process
        return monitorSwitchSocket(buffer, new_socket);
      } else if (pid > 0) {
          //parent just needs to continue polling
      } else{
        exit(EXIT_FAILURE);
      }
    }
  return EXIT_SUCCESS;
}


Controller::Controller(int maxConns, int socket): nSwitches(maxConns),
                                                        sfd(socket) {
  openCount = 0;
  queryCount = 0;
  ackCount = 0;
  addCount = 0;
}

int Controller::getNumSwitches() {
    /* Returns the number of switches */
    return nSwitches;
}

int Controller::findOpenSwitch(int id) {
  /* checks if this switch id has sent an open packet to the Controller
  before. Returns its index or -1 if not found */
  for (unsigned int i = 0; i < openSwitches.size(); i++) {
    if (openSwitches[i].myID == id) {
      return i;
    }
  }
  return -1;//not found
}


void Controller::addToOpenSwitches(MSG_OPEN openMSG) {
  /*adds new switchs' info to list when an open packet is sent*/
  if (findOpenSwitch(openMSG.myID) < 0) {
    printf("Adding switch %i\n", openMSG.myID);
    openSwitches.push_back(openMSG);
  }
}


void Controller::print(){
  /* Print out controller info */
  printf("Switch information: \n");
  //print out switch info
  for (unsigned int i = 0; i < openSwitches.size(); i++) {
    printf("[sw%i] port1= %i, port2= %i, port3= %i-%i\n",
            openSwitches[i].myID, openSwitches[i].port1,
            openSwitches[i].port2, openSwitches[i].lowIP,
            openSwitches[i].highIP);
  }

  //print packet counts
  printf("\n");
  printf("Packet Stats: \n");
  printf("\tReceived:\t OPEN:%i, QUERY:%i\n", openCount, queryCount);
  printf("\tTransmitted:\t ACK:%i, ADD:%i\n", ackCount, addCount);
}

void Controller::doIfValidCommand(string cmd) {
  /*Check string for a valid command and if exists, execute it*/

  if (cmd == "list") { /* print Controller info */
    print();
    printf("\nPlease enter 'list' or 'exit': ");

  } else if (cmd == "exit") { /* print Controller info and exit. */
    print();
    close(sfd);
    exit(0);

  } else { /* Not a valid command */
    printf("%s not valid.\n", cmd.c_str());
    printf("Please enter only 'list' or 'exit':");
  }

  fflush(stdout);
  fflush(stdin);
}


void Controller::respondToOPENPacket(MSG_OPEN openMSG){
  int fd = openWriteFIFO(openMSG.myID, 0);
  conns[openMSG.myID].wfd = fd;
  sendACK(fd, 0, openMSG.myID);
  openCount++;
  ackCount++;
  addToOpenSwitches(openMSG);
}


/*checks if a certian switch in openSwitches contains IPs between
 lowIP and highIP*/
bool Controller::inSwitchRange(int swID, int lowIP, int highIP) {
  if (swID >= 0 && (unsigned int) swID < openSwitches.size() &&
      openSwitches[swID].lowIP <= lowIP &&
      openSwitches[swID].highIP >= highIP) {
    return true;
  }
  return false;
}

flow_entry Controller::makeForwardRule(unsigned int actionVal, unsigned int swID){
  flow_entry new_rule = {
    .srcIP_lo = 0,
    .srcIP_hi = MAXIP,
    .destIP_lo = (unsigned int) openSwitches[swID].lowIP,
    .destIP_hi = (unsigned int) openSwitches[swID].highIP,
    .actionType = FORWARD,
    .actionVal = actionVal,
    .pri = MINPRI,
    .pktCount = 1
  };
  return new_rule;
}

flow_entry Controller::makeDropRule(unsigned int dst_lo, unsigned int dst_hi){
  flow_entry new_rule = {
    .srcIP_lo = 0,
    .srcIP_hi = MAXIP,
    .destIP_lo = dst_lo,
    .destIP_hi = dst_hi,
    .actionType = DROP,
    .actionVal = DROP,
    .pri = MINPRI,
    .pktCount = 1
  };
  return new_rule;
}

flow_entry Controller::makeFlowEntry(MSG_QUERY queryMSG) {
  /* makes a flow entry for a add packet */
  int port1 = findOpenSwitch(queryMSG.port1);
  int port2 = findOpenSwitch(queryMSG.port2);
  //port 1 is the correct destination
  if (inSwitchRange(port1, queryMSG.dstIP, queryMSG.dstIP))
    return makeForwardRule(queryMSG.port1, port1);
  //port 2 is the correct destination
  if (inSwitchRange(port2, queryMSG.dstIP, queryMSG.dstIP))
    return makeForwardRule(queryMSG.port2, port2);

  //no correct port
  return makeDropRule(queryMSG.dstIP, queryMSG.dstIP);
}

void Controller::respondToQUERYPacket(MSG_QUERY queryMSG){
  /*Responds to open packets within its range of switches*/
  if (queryMSG.myID <= getNumSwitches()) {
    int fd = openWriteFIFO(queryMSG.myID, 0);
    conns[queryMSG.myID].wfd = fd;

    MSG msg;
    msg.add = makeFlowEntry(queryMSG);
    sendADD(fd, 0, queryMSG.myID, msg);
    queryCount++;
    addCount++;
  }
}

void Controller::doIfValidPacket(FRAME packet) {
  if (packet.type == OPEN) {
    respondToOPENPacket(packet.msg.open);
  } else if (packet.type == QUERY) {
    respondToQUERYPacket(packet.msg.query);
  } else {
    //invalid types counters?
    printf("Unexpected packet type received\n");
  }
}


void Controller::checkKeyboardPoll(struct pollfd* pfd) {
  /* 1. Poll the keyboard for a user command. */
  char buf[BUF_SIZE];
  memset((char *)&buf, ' ', sizeof(buf));
  if (pfd->revents & POLLIN) {
    read(pfd->fd, buf, BUF_SIZE);
    string cmd = string(buf);
    trimWhitespace(cmd);
    doIfValidCommand(cmd);
  }
}


void Controller::checkFIFOPoll(struct pollfd* pfds) {
  /* 2.  Poll the incoming switch fifos
    note: pfds[0] is not used as it is the keyboard*/
  for (int i = 1; i < getNumSwitches()+1; i++) {
    if (pfds[i].revents & POLLIN) {
      FRAME packet = rcvFrame(pfds[i].fd);
      doIfValidPacket(packet);

      //reprint prompt as packet type is printed
      printf("Please enter 'list' or 'exit': ");
    }
  }
}


void Controller::doPolling(struct pollfd* pfds) {
  int ret = poll(pfds, getNumSwitches()+1, 0);
  if (errno == 4 && ret < 0) { // system call interupted: ie SIGUSR1 sent
    errno = 0;
  } else if (errno && ret < 0) {
    perror("POLL ERROR: ");
    exit(errno);
  } else {
    /* 1. Poll the keyboard for a user command. */
    checkKeyboardPoll(&pfds[0]);
    pollControllerSocket(sfd);

    /* 2.  Poll the incoming switch fifos
    and the attached switches.*/
    checkFIFOPoll(pfds);
  }
}


void Controller::setupPollingFileDescriptors(struct pollfd* pfds) {
  // setup pfd for stdin
  pfds[0].fd = STDIN_FILENO;
  pfds[0].events = POLLIN;

  //setup pfds for read fds
  for (int i = 1; i <= getNumSwitches(); i++) {
    pfds[i].fd = conns[i-1].rfd;
    pfds[i].events = POLLIN;
  }
}

int Controller::run() {
  makeAllReadFifos();

  struct pollfd pfds[getNumSwitches()+1];

  setupPollingFileDescriptors(pfds);

  printf("Please enter 'list' or 'exit': ");
  for (;;) {
  	fflush(stdout);// flush to display output
    doPolling(pfds); // poll keyboard and FIFO polling
  }

  return -1; //never reached
}

MSG Controller::makeAddMSG(unsigned int srcIP_lo,
                            unsigned int srcIP_hi,
                            unsigned int destIP_lo,
                            unsigned int destIP_hi,
                            unsigned int actionType,
                            unsigned int actionVal,
                            unsigned int pri,
                            unsigned int pktCount){
  MSG msg;
  flow_entry rule = {.srcIP_lo= srcIP_lo,
                          .srcIP_hi= srcIP_hi,
                          .destIP_lo= destIP_lo,
                          .destIP_hi= destIP_hi,
                          .actionType= actionType,
                          .actionVal= actionVal,
                          .pri= pri,
                          .pktCount= pktCount};
  msg.add = rule;
  return msg;
}

void Controller::makeAllReadFifos(){
  for (int port = 1; port <= nSwitches; port++) {
    // port and swID are the same
    conns[port-1].rfd = openReadFIFO(0, port);
  }
  printf("Controller read fifos opened. \n");
}

//int main(int argc, char *argv[]) { return 0;}
