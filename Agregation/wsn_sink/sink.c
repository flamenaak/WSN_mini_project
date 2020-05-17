#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "sys/log.h"
#define LOG_MODULE "SINK"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY  1
#define UDP_CLIENT_PORT	8765
#define UDP_SERVER_PORT	5678

#define SEND_INTERVAL		  (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;
bool responseReceived = false;

/*---------------------------------------------------------------------------*/
PROCESS(udp_client_process, "SINK");
AUTOSTART_PROCESSES(&udp_client_process);
/*---------------------------------------------------------------------------*/
static void
udp_rx_callback(struct simple_udp_connection *c,
         const uip_ipaddr_t *sender_addr,
         uint16_t sender_port,
         const uip_ipaddr_t *receiver_addr,
         uint16_t receiver_port,
         const uint8_t *data,
         uint16_t datalen)
{
  if (strcmp((char *) data, "sink") == 0){
    //uip_ipaddr_copy(sink_ipaddr,sender_addr);
    LOG_INFO("Sink registered confirmation!\n");
    responseReceived = true;
    return;
  }else {
    LOG_INFO("Data received: %s \n",data);
  }
  
}
/*---------------------------------------------------------------------------*/
PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[32];
  uip_ipaddr_t dest_ipaddr;
  PROCESS_BEGIN();
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL, UDP_SERVER_PORT, udp_rx_callback);
  etimer_set(&periodic_timer, 2 % SEND_INTERVAL);
  while(!responseReceived) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&periodic_timer));
    if(NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr)) {
      snprintf(str, sizeof(str), "sink");
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
      LOG_INFO("Sent information about sink!\n");
    } else {
      LOG_INFO("Not reachable yet\n");
    }
    etimer_set(&periodic_timer, (5 * CLOCK_SECOND));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
