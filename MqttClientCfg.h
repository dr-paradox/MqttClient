/**************************************************************************//**
*  @par Language: C
*******************************************************************************
*  @par 	Project:
*  @brief   MqttClientCfg static configuration header file
*  @author  dr-paradox
*  @version 1.0.0
*******************************************************************************
*  @page 	MqttClientCfg
*  @ref 	MqttClient
*  @file 	MqttClientCfg.h
*
*******************************************************************************/

#ifndef MQTTCLIENT_CFG_H_
#define MQTTCLIENT_CFG_H_

/* -------------------------------- Includes -------------------------------- */

/* -------------------------------- Defines --------------------------------- */

/* Service cycle time set as One second */
#define SERVICE_CYCLE_TIME						((unsigned char)1)

#define SERVER_ADDRESS					"iot.eclipse.org"
#define SERVER_PORT						((int)1883)

#define CLIENT_ID						((char *)"insert_client_id_here")
#define KEEP_ALIVE_INTERVAL				((unsigned short)20)
#define CLEAN_SESSION_FLAG				((unsigned char)1)
#define MQTT_V_3_1_1					((unsigned char)4)
#define USERNAME						((char *)"insert_uname_here")
#define PASSWORD						((char *)"insert_passwd_here")

/* topic string length */
#define TOPIC_LENGTH					((unsigned short)21)

/* topic string  enter VIN below */
#define TOPIC							((char *)"$SYS/broker/connection/evt/test_platform")

/* Quality of Service 1 */
#define QOS_1							((unsigned char)1)

/* Message retain flags */
#define RETAIN							((unsigned char)1)
#define NO_RETAIN						((unsigned char)0)

/* The maximum size of Json provided by services */
#define SERVER_COM_JSON_MAX_SIZE		((unsigned short)4096)

/* How many times a request will be resend */
#define MAX_REQ_RETRY_COUNT				((unsigned char)3)

/* ------------------------------- Data Types ------------------------------- */

/* ---------------------------- Global Variables ---------------------------- */

/* --------------------------- Routine prototypes --------------------------- */

/* -------------------------------- Routines -------------------------------- */

#endif
