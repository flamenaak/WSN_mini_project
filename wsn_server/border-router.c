#include "contiki.h"
#include <stdio.h>
#include <stdlib.h>
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "sys/log.h"
#include "sys/node-id.h"
#include <string.h>


//#include "os/storage/cfs/cfs.h"

#define LOG_MODULE "SERVER"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define SEND_INTERVAL (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;



/* Declare and auto-start this file's process */
PROCESS(udp_server_process, "UDP server");
PROCESS(contiki_ng_br, "Contiki-NG Border Router");
AUTOSTART_PROCESSES(&contiki_ng_br,&udp_server_process);
const int maxStoredMsgs = 10;
char msgs[10][15];
int counter = 0;


static void DoAgregatinAndSend(){
  LOG_INFO("Agregation executed\n");

  //char *results[10];
  char nodes[10][10];
  bool setFlag = false;
  int nodeCounter = 0;
  int subCounter = 0;
  char *nodeID;
  char tmpMsg[15] = "";

  int tmpMax = 0;
  int tmpEntries = 0;
  int tmpMin = 0;
  int tmpSum = 0;
  int tmpValue = 0;
  char *firstValue = "";
   

   counter = 0;
   for(counter = 0; counter < 5; counter++){
    //LOG_INFO("%s\n", msgs[counter]);
    // Get ID from data msg
    strcpy(tmpMsg, msgs[counter]);
    nodeID = strtok (tmpMsg," ,.-");
    //Check IF ID already in set of IDS if not add it
    subCounter = 0;
    for (subCounter = 0; subCounter < maxStoredMsgs; subCounter++){
      if(strcmp(nodeID, nodes[subCounter]) == 0) {
            setFlag = true;
            break;
      }
    }
    if (setFlag == false){
      strcpy(nodes[nodeCounter], nodeID);
      nodeCounter++;
    }
    else{
      setFlag = false;
    }
   }
  counter = 0;
  

  //Agregation calculation itself:
   
  for(counter = 0; counter < nodeCounter; counter++){
    tmpMax = 0;
    tmpEntries = 0;
    tmpMin = -90;
    tmpSum = 0;
    tmpValue = 0;
    firstValue = "";

    for (subCounter = 0; subCounter < maxStoredMsgs; subCounter++){
      //nodeID = strtok (msgs[subCounter]," ");
      //firstValue = strtok (NULL, " ");
      strcpy(tmpMsg, msgs[subCounter]);
      nodeID = strtok (tmpMsg," ,.-");
      if(strcmp(nodeID, nodes[counter]) == 0) {
        tmpEntries++;
        firstValue = strtok (NULL, " ,.-");
        tmpValue = atoi(firstValue);
        tmpSum += tmpValue;
        if (tmpMax < tmpValue) tmpMax = tmpValue;
        if (tmpMin > tmpValue || tmpMin == -90) tmpMin = tmpValue;
      }
    }
    printf ("ID:%s Sum:%d Max:%d Min:%d Entries:%d\n",nodes[counter], tmpSum, tmpMax, tmpMin, tmpEntries);
  }
  counter = 0;
}

/*---------------------------------------------------------------------------*/
PROCESS_THREAD(contiki_ng_br, ev, data)
{
  PROCESS_BEGIN();
  PROCESS_END();
}

static void udp_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen){  
/* Send same data back to client as ack */
  simple_udp_sendto(&udp_conn, data, datalen, sender_addr);
  LOG_INFO("Storing received data %d - '%s' \n", counter, data);
  strcpy(msgs[counter], (char *) data);
  counter = counter+1;

  if (counter == maxStoredMsgs){
    DoAgregatinAndSend();
  }
  
  // Not enough ROM memory for use of CSF co  de
  //int fd;
  //fd = cfs_open("receivedMsgs", CFS_WRITE + CFS_APPEND);
  //cfs_write(fd,(char *) data, datalen);
  //cfs_close(fd);


}


PROCESS_THREAD(udp_server_process, ev, data){
  static struct etimer periodic_timer;
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);

  //NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_callback);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    LOG_INFO("Button Clicked!\n");
     DoAgregatinAndSend();
    etimer_set(&periodic_timer, SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}