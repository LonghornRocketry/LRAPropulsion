#ifndef __UIP_CONF_H__
#define __UIP_CONF_H__
	
	#include <stdint.h>
	#include "src/networking.h"
	
	typedef uint8_t u8_t;
	typedef uint16_t u16_t;
	typedef uint16_t uip_stats_t;
	
	//lol need to figure out what these do
	typedef uint8_t uip_tcp_appstate_t;
	typedef uint8_t uip_udp_appstate_t;
	
	//max concurrent TCP connections
	#define UIP_CONF_MAX_CONNECTIONS 10 
	
	//max listening TCP ports
	#define UIP_CONF_MAX_LISTENPORTS 1
	
	//uIP buffer size
	#define UIP_CONF_BUFFER_SIZE 2048
	
	//CPU byte order (ARM). If this isn't set right weird stuff happens...
	#define UIP_CONF_BYTE_ORDER UIP_LITTLE_ENDIAN
	
	//logging - on or off?
	#define UIP_CONF_LOGGING 0
	
	//UDP support - on or off?
	#define UIP_CONF_UDP 1
	
	//uIP statistics - on or off?
	#define UIP_CONF_STATISTICS 1
	
	//uIP's hook into our program
	#define UIP_APPCALL uip_appcall
	#define UIP_UDP_APPCALL uip_udp_appcall
	
	
	//#define UIP_OFFLOAD_ICMP_CHKSUM 1
	
#endif /*__UIP_CONF_H__*/
