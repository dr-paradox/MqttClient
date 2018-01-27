/**************************************************************************//**
*  @par Language  : C
*******************************************************************************
*  @par         Project:
*  @brief       MqttClient public API header file.
*  @author      dr-paradox
*  @version     1.0.0
*******************************************************************************
*  @page 	MqttClient
*  @ref 	MqttClient "API Reference" header file
*  @file 	MqttClient.h
*
*******************************************************************************/

#ifndef MQTTCLIENT_H
#define MQTTCLIENT_H

/* -------------------------------- Includes -------------------------------- */

/* -------------------------------- Defines --------------------------------- */
/**
* @def MQTTCLIENTH2_HANDLER
* Defines MqttClientH2 component handler for FSM.
*/
#define MQTTCLIENTH2_HANDLER              ((unsigned char)0)
/* ------------------------------- Data Types ------------------------------- */

/*Callback function for service notification*/
typedef void (*RxCbk)(unsigned char server_response);

/*Services ID used by MqttClientH2_SendData() */
typedef enum {
	SERVICE_REQ_1,
	SERVICE_LAST/* <--- Do not remove this!!!*/
} t_ServiceID;

/*These are all possible response codes for service notification callback*/
typedef enum {
	SERVERCOM_OK = 0,
	SERVERCOM_ERROR,
	SERVERCOM_TIMEOUT,
	SERVERCOM_CANCELED,
	SERVERCOM_BAD_REQUEST,/*Properly formatted response was received from server but status code is not 200 or 480*/
	SERVERCOM_BAD_FORMAT_RESPONSE,/*Response with invalid format was received from server*/
} t_ServerReplyCodes;

/* ---------------------------- Global Variables ---------------------------- */

/* --------------------------- Routine prototypes --------------------------- */

/**
 * @brief	MqttClientH2 initialization
 *
 * @param[in]	: void
 * @return 		void
 *
*/
 void MqttClient_Init(void);

/**
 * @brief	MqttClientH2 periodical task
 *
 * @param[in]	: void
 * @return 		void
 *
*/
 void MqttClient_Task(void);

/**
 * @brief		External API called to send data to server by other services
 *
 * @param[in]	: json			: json message to be sent
 * @param[in]	: size			: size of json message to be sent
 * @param[in]	: service_id	: service id of calling service
 * @param[out]	: cbk			: callback to notify status
 * @return 		void
 *
*/
 void MqttClient_SendData(unsigned char *json, unsigned short size, unsigned char service_id, RxCbk cbk);

/**
 * @brief		External API called to send data to server by other services
 *
 * @param[in]	: void
 * @return 		void
 *
*/
 void MqttClient_CancelRequest(void);

/* -------------------------------- Routines -------------------------------- */

#endif /* MQTTCLIENTH2_H */
