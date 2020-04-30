#include "contiki.h"
#include "net/routing/routing.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"
#include "os/storage/cfs/cfs.h"
#include "random.h"
#include "dev/button-sensor.h"
#include "sys/log.h"
#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678
#define SEND_INTERVAL (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

PROCESS(udp_server_process, "UDP server");
AUTOSTART_PROCESSES(&udp_server_process);

static void udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen){  
/* Send same data back to client as ack */
simple_udp_sendto(&udp_conn, data, datalen, sender_addr);

LOG_INFO("[SERVER]: Storing received data '%.*s' \n", datalen, (char *) data);
int fd;
fd = cfs_open("receivedMsgs", CFS_WRITE + CFS_APPEND);
cfs_write(fd, (char *) data, 32);
cfs_close(fd);
  
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_server_process, ev, data){
  static struct etimer periodic_timer;
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);


  /* Initialize DAG root */
  NETSTACK_ROUTING.root_start();

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_SERVER_PORT, NULL, UDP_CLIENT_PORT, udp_rx_callback);

  while (1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
    // On button click event here!////////   
    LOG_INFO("[SERVER]: Button Clicked!\n");
    //////////////////////////////////////
    etimer_set(&periodic_timer, SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
