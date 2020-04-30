#include "contiki.h"
#include "dev/button-sensor.h"
#include "net/ipv6/simple-udp.h"

#include "net/routing/routing.h"
#include "net/netstack.h"

#include <stdio.h> /* For printf() */

#define LOG_MODULE "App"
#define LOG_LEVEL LOG_LEVEL_INFO

#define WITH_SERVER_REPLY 1
#define UDP_CLIENT_PORT 8765
#define UDP_SERVER_PORT 5678

#define SEND_INTERVAL (60 * CLOCK_SECOND)

static void
udp_rx_callback_server(struct simple_udp_connection *c,
                       const uip_ipaddr_t *sender_addr,
                       uint16_t sender_port,
                       const uip_ipaddr_t *receiver_addr,
                       uint16_t receiver_port,
                       const uint8_t *data,
                       uint16_t datalen);

static void
udp_rx_callback_server(struct simple_udp_connection *c,
                       const uip_ipaddr_t *sender_addr,
                       uint16_t sender_port,
                       const uip_ipaddr_t *receiver_addr,
                       uint16_t receiver_port,
                       const uint8_t *data,
                       uint16_t datalen);

/*---------------------------------------------------------------------------*/
PROCESS(server_p, "server process");
PROCESS(client_p, "client process");
AUTOSTART_PROCESSES(&server_p, &client_p);
//AUTOSTART_PROCESSES(&button);
/*---------------------------------------------------------------------------*/

static struct simple_udp_connection udp_conn_server;
static struct simple_udp_connection udp_conn_client;

PROCESS_THREAD(server_p, ev, data)
{
  PROCESS_BEGIN();
  simple_udp_register(
      &udp_conn_server,
      LOCAL_PORT,
      REMOTE ADRESS, // CAN BE NULL (see next slide)
      REMOTE_PORT,
      udp_rx_callback // MESSAGE HANDLING
  );
  NETSTACK_ROUTING.root_start();

  simple_udp_register(&udp_conn_server, UDP_SERVER_PORT, NULL,
                      UDP_CLIENT_PORT, udp_rx_callback_server);
  PROCESS_END();
}

PROCESS_THREAD(client_p, ev, data)
{
  PROCESS_BEGIN();
  SENSORS_ACTIVATE(button_sensor);

  NETSTACK_ROUTING.node_is_reachable();
  NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr);

  /* Initialize UDP connection */
  simple_udp_register(&udp_conn_client, UDP_CLIENT_PORT, NULL,
                      UDP_SERVER_PORT, udp_rx_callback_client);

  while (1)
  {
    PROCESS_WAIT_EVENT_UNTIL(ev == sensors_event);

    if (NETSTACK_ROUTING.node_is_reachable() && NETSTACK_ROUTING.get_root_ipaddr(&dest_ipaddr))
    {
      /* Send to DAG root */
      LOG_INFO("Sending request %u to ", count);
      LOG_INFO_6ADDR(&dest_ipaddr);
      LOG_INFO_("\n");
      snprintf(str, sizeof(str), "hello %d", count);
      simple_udp_sendto(&udp_conn_client, str, strlen(str), &dest_ipaddr);
      count++;
    }
    else
    {
      LOG_INFO("Not reachable yet\n");
    }

    /* Add some jitter */
    etimer_set(&periodic_timer, SEND_INTERVAL - CLOCK_SECOND + (random_rand() % (2 * CLOCK_SECOND)));
  }

  PROCESS_END();
}
/*---------------------------------------------------------------------------*/

static void
udp_rx_callback_client(struct simple_udp_connection *c,
                       const uip_ipaddr_t *sender_addr,
                       uint16_t sender_port,
                       const uip_ipaddr_t *receiver_addr,
                       uint16_t receiver_port,
                       const uint8_t *data,
                       uint16_t datalen)
{
  LOG_INFO("Received response '%.*s' from ", datalen, (char *)data);
  LOG_INFO_6ADDR(sender_addr);
#if LLSEC802154_CONF_ENABLED
  LOG_INFO_(" LLSEC LV:%d", uipbuf_get_attr(UIPBUF_ATTR_LLSEC_LEVEL));
#endif
  LOG_INFO_("\n");
}

static void
udp_rx_callback_server(struct simple_udp_connection *c,
                       const uip_ipaddr_t *sender_addr,
                       uint16_t sender_port,
                       const uip_ipaddr_t *receiver_addr,
                       uint16_t receiver_port,
                       const uint8_t *data,
                       uint16_t datalen)
{
  LOG_INFO("Received request '%.*s' from ", datalen, (char *)data);
  LOG_INFO_6ADDR(sender_addr);
  LOG_INFO_("\n");
#if WITH_SERVER_REPLY
  /* send back the same string to the client as an echo reply */
  LOG_INFO("Sending response.\n");
  simple_udp_sendto(&udp_conn_server, data, datalen, sender_addr);
#endif /* WITH_SERVER_REPLY */
}