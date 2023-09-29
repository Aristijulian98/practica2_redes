/*
 * Copyright (c) 2016, Freescale Semiconductor, Inc.
 * Copyright 2016-2020 NXP
 * All rights reserved.
 *
 *
 * SPDX-License-Identifier: BSD-3-Clause
 */

/*******************************************************************************
 * Includes
 ******************************************************************************/
#include "lwip/opt.h"

#if LWIP_IPV4 && LWIP_RAW && LWIP_NETCONN && LWIP_DHCP && LWIP_DNS

#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_phy.h"

#include "lwip/api.h"
#include "lwip/apps/mqtt.h"
#include "lwip/dhcp.h"
#include "lwip/netdb.h"
#include "lwip/netifapi.h"
#include "lwip/prot/dhcp.h"
#include "lwip/tcpip.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "enet_ethernetif.h"
#include "lwip_mqtt_id.h"

#include "ctype.h"
#include "stdio.h"

#include "fsl_phyksz8081.h"
#include "fsl_enet_mdio.h"
#include "fsl_device_registers.h"

#include<stdio.h>
#include <stdlib.h>
#include <time.h>
//#include <cstdlib>

/*******************************************************************************
 * Definitions
 ******************************************************************************/

/* @TEST_ANCHOR */

/* MAC address configuration. */
#ifndef configMAC_ADDR
#define configMAC_ADDR                     \
    {                                      \
        0x02, 0x12, 0x13, 0x10, 0x15, 0x12 \
    }
#endif

/* Address of PHY interface. */
#define EXAMPLE_PHY_ADDRESS BOARD_ENET0_PHY_ADDRESS

/* MDIO operations. */
#define EXAMPLE_MDIO_OPS enet_ops

/* PHY operations. */
#define EXAMPLE_PHY_OPS phyksz8081_ops

/* ENET clock frequency. */
#define EXAMPLE_CLOCK_FREQ CLOCK_GetFreq(kCLOCK_CoreSysClk)

/* GPIO pin configuration. */
#define BOARD_LED1_GPIO       BOARD_LED_RED_GPIO
#define BOARD_LED1_GPIO_PIN   BOARD_LED_RED_GPIO_PIN
#define BOARD_LED2_GPIO       BOARD_LED_GREEN_GPIO
#define BOARD_LED2_GPIO_PIN   BOARD_LED_GREEN_GPIO_PIN
#define BOARD_LED3_GPIO       BOARD_LED_BLUE_GPIO
#define BOARD_LED3_GPIO_PIN   BOARD_LED_BLUE_GPIO_PIN
#define BOARD_SW_GPIO         BOARD_SW3_GPIO
#define BOARD_SW_GPIO_PIN     BOARD_SW3_GPIO_PIN
#define BOARD_SW_PORT         BOARD_SW3_PORT
#define BOARD_SW_IRQ          BOARD_SW3_IRQ
#define BOARD_SW_IRQ_HANDLER  BOARD_SW3_IRQ_HANDLER


#ifndef EXAMPLE_NETIF_INIT_FN
/*! @brief Network interface initialization function. */
#define EXAMPLE_NETIF_INIT_FN ethernetif0_init
#endif /* EXAMPLE_NETIF_INIT_FN */

/*! @brief MQTT server host name or IP address. */
#define EXAMPLE_MQTT_SERVER_HOST "driver.cloudmqtt.com"

/*! @brief MQTT server port number. */
#define EXAMPLE_MQTT_SERVER_PORT 18591

/*! @brief Stack size of the temporary lwIP initialization thread. */
#define INIT_THREAD_STACKSIZE 1024

/*! @brief Priority of the temporary lwIP initialization thread. */
#define INIT_THREAD_PRIO DEFAULT_THREAD_PRIO

/*! @brief Stack size of the temporary initialization thread. */
#define APP_THREAD_STACKSIZE 1024

/*! @brief Priority of the temporary initialization thread. */
#define APP_THREAD_PRIO DEFAULT_THREAD_PRIO

/*******************************************************************************
 * Prototypes
 ******************************************************************************/

static void connect_to_mqtt(void *ctx);

/*******************************************************************************
 * Variables
 ******************************************************************************/

static mdio_handle_t mdioHandle = {.ops = &EXAMPLE_MDIO_OPS};
static phy_handle_t phyHandle   = {.phyAddr = EXAMPLE_PHY_ADDRESS, .mdioHandle = &mdioHandle, .ops = &EXAMPLE_PHY_OPS};

/*! @brief MQTT client data. */
static mqtt_client_t *mqtt_client;

/*! @brief MQTT client ID string. */
static char client_id[40];

/*! @brief MQTT client information. */
static const struct mqtt_connect_client_info_t mqtt_client_info = {
    .client_id   = (const char *)&client_id[0],
    .client_user = "julian_o2023",
    .client_pass = "o2023",
    .keep_alive  = 100,
    .will_topic  = NULL,
    .will_msg    = NULL,
    .will_qos    = 0,
    .will_retain = 0,
#if LWIP_ALTCP && LWIP_ALTCP_TLS
    .tls_config = NULL,
#endif
};

/*! @brief MQTT broker IP address. */
static ip_addr_t mqtt_addr;

/*! @brief Indicates connection to MQTT broker. */
static volatile bool connected = false;

int temp = 0;
int temp_alarm = 0;
int liquido = 0;
int energia = 0;
char dato[40] = "b";
char a[40] = "a";
char b[40] = "b";
char estado[40] = "b";
char t[40] = "0";
char zero[40] = "0";
int res1 = 0;
int res2 = 0;
int res3 = 0;
int tiempo = 0;

volatile bool g_ButtonPress = false;

/*******************************************************************************
 * Code
 ******************************************************************************/

void BOARD_SW_IRQ_HANDLER(void)
{
    /* Clear external interrupt flag. */
    GPIO_PortClearInterruptFlags(BOARD_SW_GPIO, 1U << BOARD_SW_GPIO_PIN);

    /* Change state of button. */
    if(g_ButtonPress == false){
    	g_ButtonPress = true;
    	SDK_ISR_EXIT_BARRIER;
    }else{
    	g_ButtonPress = false;
    	SDK_ISR_EXIT_BARRIER;
    }
}

static void init_button_and_led(void)
{
    /* Define the init structure for the input switch pin */
    gpio_pin_config_t sw_config = {
        kGPIO_DigitalInput,
        0,
    };

    /* Define the init structure for the output LED pin */
    gpio_pin_config_t led_config = {
        kGPIO_DigitalOutput,
        0,
    };

    /* Print a note to terminal. */
    PRINTF("Init SW3 and LED pins\r\n");


    PORT_SetPinInterruptConfig(BOARD_SW_PORT, BOARD_SW_GPIO_PIN, kPORT_InterruptFallingEdge);
    EnableIRQ(BOARD_SW_IRQ);
    GPIO_PinInit(BOARD_SW_GPIO, BOARD_SW_GPIO_PIN, &sw_config);

    /* Init output LED GPIO. */
    LED_RED_INIT(0U);
    LED_GREEN_INIT(0U);
    LED_BLUE_INIT(0U);
    LED_RED_OFF();
    LED_GREEN_OFF();
    LED_BLUE_OFF();
}

/*!
 * @brief Called when subscription request finishes.
 */
static void mqtt_topic_subscribed_cb(void *arg, err_t err)
{
    const char *topic = (const char *)arg;

    if (err == ERR_OK)
    {
        PRINTF("Subscribed to the topic \"%s\".\r\n", topic);
    }
    else
    {
        PRINTF("Failed to subscribe to the topic \"%s\": %d.\r\n", topic, err);
    }
}

/*!
 * @brief Called when there is a message on a subscribed topic.
 */
static void mqtt_incoming_publish_cb(void *arg, const char *topic, u32_t tot_len)
{
    LWIP_UNUSED_ARG(arg);

    PRINTF("Received %u bytes from the topic \"%s\": \"", tot_len, topic);
}

/*!
 * @brief Called when recieved incoming published message fragment.
 */
static void mqtt_incoming_data_cb(void *arg, const u8_t *data, u16_t len, u8_t flags)
{
    int i;

    LWIP_UNUSED_ARG(arg);

    for (i = 0; i < len; i++)
    {
        if (isprint(data[i]))
        {
            PRINTF("%c", (char)data[i]);
            dato[0] = data[0];
        }
        else
        {
            PRINTF("\\x%02x", data[i]);
            dato[0] = data[0];
        }
    }

    if (flags & MQTT_DATA_FLAG_LAST)
    {
        PRINTF("\"\r\n");
    }

}

/*!
 * @brief Subscribe to MQTT topics.
 */
static void mqtt_subscribe_topics(mqtt_client_t *client)
{
    char topic[40] = {0};
    err_t err;

    mqtt_set_inpub_callback(client, mqtt_incoming_publish_cb, mqtt_incoming_data_cb,
                            LWIP_CONST_CAST(void *, &mqtt_client_info));

    sprintf(topic, "julian_o2023/bloqueo");
    err = mqtt_subscribe(client, topic, 0, mqtt_topic_subscribed_cb, LWIP_CONST_CAST(void *, "bloqueo"));
    if (err == ERR_OK) PRINTF("Subscribing to the topic \"%s\" with QoS %d...\r\n", topic, 0);
    else PRINTF("Failed to subscribe to the topic \"%s\" with QoS %d: %d.\r\n", topic, 0, err);

    sprintf(topic, "julian_o2023/tiempo");
    err = mqtt_subscribe(client, topic, 0, mqtt_topic_subscribed_cb, LWIP_CONST_CAST(void *, "tiempo"));
    if (err == ERR_OK) PRINTF("Subscribing to the topic \"%s\" with QoS %d...\r\n", topic, 0);
    else PRINTF("Failed to subscribe to the topic \"%s\" with QoS %d: %d.\r\n", topic, 0, err);
}

/*!
 * @brief Called when connection state changes.
 */
static void mqtt_connection_cb(mqtt_client_t *client, void *arg, mqtt_connection_status_t status)
{
    const struct mqtt_connect_client_info_t *client_info = (const struct mqtt_connect_client_info_t *)arg;

    connected = (status == MQTT_CONNECT_ACCEPTED);

    switch (status)
    {
        case MQTT_CONNECT_ACCEPTED:
            PRINTF("MQTT client \"%s\" connected.\r\n", client_info->client_id);
            mqtt_subscribe_topics(client);
            break;

        case MQTT_CONNECT_DISCONNECTED:
            PRINTF("MQTT client \"%s\" not connected.\r\n", client_info->client_id);
            /* Try to reconnect 1 second later */
            sys_timeout(1000, connect_to_mqtt, NULL);
            break;

        case MQTT_CONNECT_TIMEOUT:
            PRINTF("MQTT client \"%s\" connection timeout.\r\n", client_info->client_id);
            /* Try again 1 second later */
            sys_timeout(1000, connect_to_mqtt, NULL);
            break;

        case MQTT_CONNECT_REFUSED_PROTOCOL_VERSION:
        case MQTT_CONNECT_REFUSED_IDENTIFIER:
        case MQTT_CONNECT_REFUSED_SERVER:
        case MQTT_CONNECT_REFUSED_USERNAME_PASS:
        case MQTT_CONNECT_REFUSED_NOT_AUTHORIZED_:
            PRINTF("MQTT client \"%s\" connection refused: %d.\r\n", client_info->client_id, (int)status);
            /* Try again 10 seconds later */
            sys_timeout(10000, connect_to_mqtt, NULL);
            break;

        default:
            PRINTF("MQTT client \"%s\" connection status: %d.\r\n", client_info->client_id, (int)status);
            /* Try again 10 seconds later */
            sys_timeout(10000, connect_to_mqtt, NULL);
            break;
    }
}

/*!
 * @brief Starts connecting to MQTT broker. To be called on tcpip_thread.
 */
static void connect_to_mqtt(void *ctx)
{
    LWIP_UNUSED_ARG(ctx);

    PRINTF("Connecting to MQTT broker at %s...\r\n", ipaddr_ntoa(&mqtt_addr));

    mqtt_client_connect(mqtt_client, &mqtt_addr, EXAMPLE_MQTT_SERVER_PORT, mqtt_connection_cb,
                        LWIP_CONST_CAST(void *, &mqtt_client_info), &mqtt_client_info);
}

/*!
 * @brief Called when publish request finishes.
 */
static void mqtt_message_published_cb(void *arg, err_t err)
{
    const char *topic = (const char *)arg;

    if (err == ERR_OK)
    {
        //PRINTF("Published to the topic \"%s\".\r\n", topic);
        //PRINTF("Published to topic \r\n");
    }
    else
    {
        PRINTF("Failed to publish to the topic \"%s\": %d.\r\n", topic, err);
    }
}

/*!
 * @brief Publishes a message. To be called on tcpip_thread.
 */
static void publish_temp(void *ctx)
{
	LWIP_UNUSED_ARG(ctx);

	char topic[40] = {0};
	char message[40] = {0};

	sprintf(topic, "julian_o2023/temp");
	sprintf(message, "%d", temp);

	PRINTF("Going to publish to the topic: \"%s\", message:  \"%s\"...\r\n", topic, message);

	mqtt_publish(mqtt_client, topic, message, strlen(message), 1, 0, mqtt_message_published_cb, (void *)topic);

}

static void publish_alarma_temp(void *ctx)
{
	LWIP_UNUSED_ARG(ctx);

	char topic[40] = {0};
	char message[40] = {0};

	sprintf(topic, "julian_o2023/alarma_temp");
	sprintf(message, "%d", temp_alarm);

	PRINTF("Going to publish to the topic: \"%s\", message:  \"%s\"...\r\n", topic, message);

	mqtt_publish(mqtt_client, topic, message, strlen(message), 1, 0, mqtt_message_published_cb, (void *)topic);

}

/*!
 * @brief Publishes a message. To be called on tcpip_thread.
 */
static void publish_liquido(void *ctx)
{
	LWIP_UNUSED_ARG(ctx);

	char topic[40] = {0};
	char message[40] = {0};

	sprintf(topic, "julian_o2023/liquido");
	sprintf(message, "%d", liquido);

   	PRINTF("Going to publish to the topic: \"%s\", message:  \"%s\"...\r\n", topic, message);

	mqtt_publish(mqtt_client, topic, message, strlen(message), 1, 0, mqtt_message_published_cb, (void *)topic);

}

/*!
 * @brief Publishes a message. To be called on tcpip_thread.
 */
static void publish_energia(void *ctx)
{
	LWIP_UNUSED_ARG(ctx);

	char topic[40] = {0};
	char message[40] = {0};

	sprintf(topic, "julian_o2023/energia");
	sprintf(message, "%d", energia);

	PRINTF("Going to publish to the topic: \"%s\", message:  \"%s\"...\r\n", topic, message);

	mqtt_publish(mqtt_client, topic, message, strlen(message), 1, 0, mqtt_message_published_cb, (void *)topic);

}

/*!
 * @brief Application thread.
 */
static void app_thread(void *arg)
{
    struct netif *netif = (struct netif *)arg;
    struct dhcp *dhcp;
    err_t err;
    int i;

    /* Wait for address from DHCP */

    PRINTF("Getting IP address from DHCP...\r\n");

    do
    {
        if (netif_is_up(netif))
        {
            dhcp = netif_dhcp_data(netif);
        }
        else
        {
            dhcp = NULL;
        }

        sys_msleep(20U);

    } while ((dhcp == NULL) || (dhcp->state != DHCP_STATE_BOUND));

    PRINTF("\r\nIPv4 Address     : %s\r\n", ipaddr_ntoa(&netif->ip_addr));
    PRINTF("IPv4 Subnet mask : %s\r\n", ipaddr_ntoa(&netif->netmask));
    PRINTF("IPv4 Gateway     : %s\r\n\r\n", ipaddr_ntoa(&netif->gw));

    /*
     * Check if we have an IP address or host name string configured.
     * Could just call netconn_gethostbyname() on both IP address or host name,
     * but we want to print some info if goint to resolve it.
     */
    if (ipaddr_aton(EXAMPLE_MQTT_SERVER_HOST, &mqtt_addr) && IP_IS_V4(&mqtt_addr))
    {
        /* Already an IP address */
        err = ERR_OK;
    }
    else
    {
        /* Resolve MQTT broker's host name to an IP address */
        PRINTF("Resolving \"%s\"...\r\n", EXAMPLE_MQTT_SERVER_HOST);
        err = netconn_gethostbyname(EXAMPLE_MQTT_SERVER_HOST, &mqtt_addr);
    }

    if (err == ERR_OK)
    {
        /* Start connecting to MQTT broker from tcpip_thread */
        err = tcpip_callback(connect_to_mqtt, NULL);
        if (err != ERR_OK)
        {
            PRINTF("Failed to invoke broker connection on the tcpip_thread: %d.\r\n", err);
        }
    }
    else
    {
        PRINTF("Failed to obtain IP address: %d.\r\n", err);
    }

    init_button_and_led();
    srand(time(NULL));

    /* Publish some messages */
    for (;;)
    {
        if (connected)
        {
        	/* publish about temperature */
        	//SCANF("%d", temp);
        	temp = rand()%21;
        	PRINTF("Temperatura: %d \r\n", temp);
            err = tcpip_callback(publish_temp, NULL);
            if (err != ERR_OK)
            {
                PRINTF("Failed to invoke publishing of a temperature on the tcpip_thread: %d.\r\n", err);
            }

        	if (temp > 10)
        	{
        		temp_alarm = 1;
                err = tcpip_callback(publish_alarma_temp, NULL);
                if (err != ERR_OK)
                {
                    PRINTF("Failed to invoke publishing of a temperature alarm on the tcpip_thread: %d.\r\n", err);
                }
        		PRINTF("La temperatura se encuentra fuera del rango normal!! \r\n");
        		LED_BLUE_ON();
                sys_msleep(3000U);
                LED_BLUE_OFF();
                temp_alarm = 0;
                err = tcpip_callback(publish_alarma_temp, NULL);
                if (err != ERR_OK)
                {
                    PRINTF("Failed to invoke publishing of a temperature alarm on the tcpip_thread: %d.\r\n", err);
                }
        	}

            /* publish about liquid */
        	//SCANF("%d", liquido);
        	liquido = rand()%2;
        	PRINTF("Derrame de liquido? %d \r\n", liquido);
        	PRINTF("Si:1 - No:0 \r\n");
        	if (liquido == 1)
			{
                err = tcpip_callback(publish_liquido, NULL);
                if (err != ERR_OK)
                {
                    PRINTF("Failed to invoke publishing of a liquid change on the tcpip_thread: %d.\r\n", err);
                }
        		PRINTF("Se detecto derrame de liquido!! \r\n");
        		LED_RED_ON();
        		LED_BLUE_ON();
				LED_GREEN_ON();
				sys_msleep(3000U);
				LED_RED_OFF();
				LED_BLUE_OFF();
				LED_GREEN_OFF();
				liquido = 0;
	            err = tcpip_callback(publish_liquido, NULL);
	            if (err != ERR_OK)
	            {
	                PRINTF("Failed to invoke publishing of a liquid change on the tcpip_thread: %d.\r\n", err);
	            }

			}
            err = tcpip_callback(publish_liquido, NULL);
            if (err != ERR_OK)
            {
                PRINTF("Failed to invoke publishing of a liquid change on the tcpip_thread: %d.\r\n", err);
            }

            /* publish about energy */
            PRINTF("Energia? %d \r\n", energia);
    		err = tcpip_callback(publish_energia, NULL);
    		if (err != ERR_OK)
    		{
            PRINTF("Failed to invoke publishing of a message about energy on the tcpip_thread: %d.\r\n", err);
    		}

        	if(g_ButtonPress == true)
        	{
        		energia = 1;
        		err = tcpip_callback(publish_energia, NULL);
        		if (err != ERR_OK)
        		{
                PRINTF("Failed to invoke publishing of a message about energy on the tcpip_thread: %d.\r\n", err);
        		}
        		PRINTF("Se detecto falla en el suministro de energia!! \r\n");
				LED_RED_ON();
				LED_BLUE_ON();
				sys_msleep(3000U);
				LED_RED_OFF();
				LED_BLUE_OFF();
        		energia = 0;
        		err = tcpip_callback(publish_energia, NULL);
        		if (err != ERR_OK)
        		{
                PRINTF("Failed to invoke publishing of a message about energy on the tcpip_thread: %d.\r\n", err);
        		}

            	/* Operate received topics */
            	res1 = strcmp(dato, a);
            	res2 = strcmp(dato, b);
            	if((res1 == 0 ) || (res2 == 0)){
            		memcpy(estado, dato, 1);
            	}else{
            		memcpy(t, dato, 1);
            	}
            	res3 = strcmp(estado, b);

            	if(res3 != 0){
            		PRINTF("La puerta se ha bloqueado por peticion del usuario \r\n");
    				tiempo = atoi(t);
    				PRINTF("El tiempo bloqueado es: %d \r\n", tiempo);
    				int i;
    				LED_RED_ON();
    				for(i=0; i< tiempo; i++){
    					sys_msleep(1000U);
    					PRINTF("Segundo bloqueado: %d \r\n", i+1);
    				}
    				LED_RED_OFF();

    				LED_GREEN_ON();
    				for(i=0; i< 2; i++){
    					sys_msleep(1000U);
    					PRINTF("Segundo libre: %d \r\n", i+1);
    				}
    				LED_GREEN_OFF();
    				LED_RED_ON();
    				for(i=0; i< tiempo; i++){
    					sys_msleep(1000U);
    					PRINTF("Segundo bloqueado: %d \r\n", i+1);
    				}
    				LED_RED_OFF();
            	}
            	else{
            		PRINTF("La puerta esta desbloqueada \r\n");
            	}

        	}else{
        		energia = 0;
        	}
            PRINTF("\r\n");
            PRINTF("\r\n");
            PRINTF("\r\n");
            sys_msleep(5000U);
        }
    }
    vTaskDelete(NULL);
}

static void generate_client_id(void)
{
    uint32_t mqtt_id[MQTT_ID_SIZE];
    int res;

    get_mqtt_id(&mqtt_id[0]);

    res = snprintf(client_id, sizeof(client_id), "nxp_%08lx%08lx%08lx%08lx", mqtt_id[3], mqtt_id[2], mqtt_id[1],
                   mqtt_id[0]);
    if ((res < 0) || (res >= sizeof(client_id)))
    {
        PRINTF("snprintf failed: %d\r\n", res);
        while (1)
        {
        }
    }
}

/*!
 * @brief Initializes lwIP stack.
 *
 * @param arg unused
 */
static void stack_init(void *arg)
{
    static struct netif netif;
    ip4_addr_t netif_ipaddr, netif_netmask, netif_gw;
    ethernetif_config_t enet_config = {
        .phyHandle  = &phyHandle,
        .macAddress = configMAC_ADDR,
    };

    LWIP_UNUSED_ARG(arg);
    generate_client_id();

    mdioHandle.resource.csrClock_Hz = EXAMPLE_CLOCK_FREQ;

    IP4_ADDR(&netif_ipaddr, 0U, 0U, 0U, 0U);
    IP4_ADDR(&netif_netmask, 0U, 0U, 0U, 0U);
    IP4_ADDR(&netif_gw, 0U, 0U, 0U, 0U);

    tcpip_init(NULL, NULL);

    LOCK_TCPIP_CORE();
    mqtt_client = mqtt_client_new();
    UNLOCK_TCPIP_CORE();
    if (mqtt_client == NULL)
    {
        PRINTF("mqtt_client_new() failed.\r\n");
        while (1)
        {
        }
    }

    netifapi_netif_add(&netif, &netif_ipaddr, &netif_netmask, &netif_gw, &enet_config, EXAMPLE_NETIF_INIT_FN,
                       tcpip_input);
    netifapi_netif_set_default(&netif);
    netifapi_netif_set_up(&netif);

    netifapi_dhcp_start(&netif);

    PRINTF("\r\n************************************************\r\n");
    PRINTF(" MQTT client example\r\n");
    PRINTF("************************************************\r\n");

    if (sys_thread_new("app_task", app_thread, &netif, APP_THREAD_STACKSIZE, APP_THREAD_PRIO) == NULL)
    {
        LWIP_ASSERT("stack_init(): Task creation failed.", 0);
    }

    vTaskDelete(NULL);
}

/*!
 * @brief Main function
 */
int main(void)
{
    SYSMPU_Type *base = SYSMPU;
    BOARD_InitBootPins();
    BOARD_InitBootClocks();
    BOARD_InitDebugConsole();
    /* Disable SYSMPU. */
    base->CESR &= ~SYSMPU_CESR_VLD_MASK;

    /* Initialize lwIP from thread */
    if (sys_thread_new("main", stack_init, NULL, INIT_THREAD_STACKSIZE, INIT_THREAD_PRIO) == NULL)
    {
        LWIP_ASSERT("main(): Task creation failed.", 0);
    }

    vTaskStartScheduler();

    /* Will not get here unless a task calls vTaskEndScheduler ()*/
    return 0;
}
#endif
