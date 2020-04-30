#include "contiki.h"
#include "net/routing/routing.h"
#include "random.h"
#include "net/netstack.h"
#include "net/ipv6/simple-udp.h"

#include "dev/button-sensor.h"
#include "sys/node-id.h"

#include "sys/log.h"
#include "dev/sht11/sht11-sensor.h"

#define LOG_MODULE "CLIENT"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL (60 * CLOCK_SECOND)

static struct simple_udp_connection udp_conn;

PROCESS(udp_client_process, "UDP client");

AUTOSTART_PROCESSES(&udp_client_process);

static void
udp_rx_callback(struct simple_udp_connection *c,
                const uip_ipaddr_t *sender_addr,
                uint16_t sender_port,
                const uip_ipaddr_t *receiver_addr,
                uint16_t receiver_port,
                const uint8_t *data,
                uint16_t datalen)
{
  // Code for handling response from server - not neccessary
  // or maybe just log osme kind of ackn.
  LOG_INFO("Response from server received!\n");
}



PROCESS_THREAD(udp_client_process, ev, data)
{
  static struct etimer periodic_timer;
  static char str[1000];
  uip_ipaddr_t dest_ipaddr;

  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);
  SENSORS_ACTIVATE(sht11_sensor);

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback);

  etimer_set(&periodic_timer, random_rand() % SEND_INTERVAL);
  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event && data == &button_sensor);
        if (NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr))
    {
      /* Send to DAG root */
      unsigned temp = sht11_sensor.value(SHT11_SENSOR_TEMP);
      unsigned hum = sht11_sensor.value(SHT11_SENSOR_HUMIDITY);
      
      LOG_INFO("Sending data to ");
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      snprintf(str, sizeof(str), "%d %d %d", node_id, temp, hum);
      simple_udp_sendto(&udp_conn, str, strlen(str), &dest_ipaddr);
    }
    else
    {
      LOG_INFO("Routing not estabilished yet or server not reachable\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/
