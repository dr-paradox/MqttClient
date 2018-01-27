/**************************************************************************//**
*  @par Language  : C
*******************************************************************************
*  @brief       MqttClient internal API header file.
*  @author      dr-paradox
*  @version     1.0.0
*******************************************************************************
*  @page 	MqttClientFunctions
*  @ref 	MqttClient "API Reference" header file
*  @file 	MqttClientFunctions.h
*
*******************************************************************************/

#ifndef MQTTCLIENT_FUNCTIONS_H
#define MQTTCLIENT_FUNCTIONS_H

/* -------------------------------- Includes -------------------------------- */

#include <stdint.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "MqttClient.h"
#include "MqttClientCfg.h"

#ifdef EXT_MODEM
/* Modem connections */
#include "ModemDataMng_interface.h"
#endif

/* -------------------------------- Defines --------------------------------- */

#define ZERO 									((unsigned char)0)
#define ONE 									((unsigned char)1)

/* returns for linux system calls */
#define SYS_SUCCESS								((int)0)
#define SYS_FAILURE								((int)-1)

/* returns for publish request sent status */
#define SUCCESS									((unsigned char)1)
#define FAILURE									((unsigned char)0)

/* ------------------------------- Data Types ------------------------------- */

/* different types of power on reset actions */
typedef enum {
	MODEM_REQ = 0,
	KEEP_ALIVE = 1,
	PING_REQ = 2
}t_timer_req;

typedef struct {

	unsigned char 	retry_count;					/*Counter for retry request*/
	unsigned char	json[SERVER_COM_JSON_MAX_SIZE];	/*buffer for json received from services*/
	uint16_t 		json_size;						/*the size of Json*/
	RxCbk cbk;		/*Store the pointer to a function provided by the services. This function is called by ServerCom
	 	 	 	 	 * after a request in order to notify the service about the result */
} t_PendingRequest;

/* ---------------------------- Global Variables ---------------------------- */

/* List to hold data related to each associated service such as retry count, json, size, cbk func */
extern t_PendingRequest ServiceRequestList[SERVICE_LAST];

/* Request to cancel current ongoing transmission */
extern bool cancel_request;

/* profile currently in use */
extern unsigned char profile;

/* --------------------------- Routine prototypes --------------------------- */

/**
 * @brief	Initializes modem_connected, profile_started and ip_obtained flags on power on reset.
 *
 * @param 	void
 * @return 	void
 *
*/
void MqttClientResetFlags(void);

/**
 * @brief	Register session result handler, session state handler with ModemDataMng and start default profile.
 *
 * @param 	void
 * @return 	void
 *
*/
void MqttClientModemInit(void);

/**
 * @brief	Initializes modem_connected, profile_started and ip_obtained flags on power on reset.
 *
 * @param[in]	: timer_req		: timer req type to set timer interval
 * @return 		void
 *
*/
void MqttClientClearStartTimer(t_timer_req timer_req);

/**
 * @brief	Check whether request to start timer is received or not.
 *
 * @param	: void
 * @return 	bool
 * @retval	: TRUE	: Request to start timer received
 * @retval	: false	: Request to start timer not received
 *
*/
bool MqttClientCheckTimerReq(void);

/**
 * @brief	Clear timer request, set timer start flag to false
 *
 * @param	: void
 * @return 	void
 *
*/
void MqttClientClearTimerReq(void);

/**
 * @brief	Get timer interval to check against count
 *
 * @param	: void
 * @return 	unsigned char
 * @retval	: TRUE	: Request to start timer received
 * @retval	: false	: Request to start timer not received
 *
*/
unsigned char MqttClientGetTimerInterval(void);

/**
 * @brief	Set timer elapsed flag to TRUE
 *
 * @param	: void
 * @return 	void
 *
*/
void MqttClientNotifyTimerElapsed(void);

/**
 * @brief	check modem connection status by returning modem_connected flag status.
 *
 * @param	: void
 * @return 	bool
 * @retval 	false: modem not connected
 * @retval 	TRUE: modem connected
 *
*/
bool MqttClientCheckModemConnection(void);

/**
 * @brief	check timer elapsed flag status.
 *
 * @param	: 	void
 * @return 	bool
 * @retval 	false: timer running
 * @retval 	TRUE: timer elapsed
 *
*/
bool MqttClientCheckTimeToRetry(void);

/**
 * @brief	create and send a Mqtt connect control packet over socket
 *
 * @param	: void
 * @return 	void
 *
*/
void MqttClientSendConnectRequest(void);

/**
* @brief	check for CONNACK packet received
*
* @param	: void
* @return	bool
* @retval	TRUE	: CONNACK packet received client successfully connected
* 			false	: CONNACK packet not received client not connected
*/
bool MqttClientCheckMqttConnection(void);

/**
* @brief	check for data available to be sent
*
* @param	: void
* @return	bool
* @retval	TRUE	: data to be sent
* 			false	: no data to send
*/
bool MqttClientCheckDataToSend(void);

/**
* @brief	check whether timer for ping request is elapsed
*
* @param	: void
* @return	bool
* @retval	TRUE	: timer elapsed ping request should be sent
* 			false	: timer not elapsed ping request should not be sent
*/
bool MqttClientCheckTimeToPing(void);

/**
* @brief	Send ping mqtt control packet over socket
*
* @param	: void
* @return	void
*/
void MqttClientSendPingRequest(void);

/**
* @brief	Send publish mqtt control packet over socket
*
* @param	: void
* @return	void
*/
void MqttClientSendPubRequest(void);

/**
* @brief	Decrement retry count associated with currently active service after sending publish request
*
* @param	: void
* @return	void
*/
void MqttClientDecrementRetryCount(void);

/**
* @brief	Increment retry count associated with currently active service after sending publish request
*
* @param	: void
* @return	void
*/
void MqttClientIncrementRetryCount(void);

/**
* @brief	Check wether to cancel current ongoing process of transmission due to cancellation request
*
* @param	: void
* @return	bool
* @retval	TRUE	: Cancellation request received
* @retval	false	: Cancellation request not received
*/
bool MqttClientCheckRequestToCancel(void);

/**
* @brief	Check retry count associated with current ongoing service transmission
*
* @param	: void
* @return	unsigned char
* @retval	retry_count	: retry count associated with current ongoing transmission
*/
unsigned char MqttClientCheckRetryCount(void);

/**
* @brief	Check retry count associated with current ongoing service transmission
*
* @param	: void
* @return	unsigned char
* @retval	SUCCESS	: publish request sent successfully
* @retval	FAILURE	: publish request sending failed
*/
unsigned char MqttClientCheckPubReqStatus(void);

/**
* @brief	Clear retry count associated with currently active service transmission
*
* @param	: void
* @return	void
*/
void MqttClientClearRetryCount(void);

/**
* @brief	Notify active service for response
*
* @param[in]	: resp	: response to be sent to active service
* @return	void
*/
void MqttClientServiceNotify(t_ServerReplyCodes resp);

/**
* @brief	Check for PubAck control packet for previously sent publish request
*
* @param	: void
* @return	unsigned char
* @retval	SUCCESS	: pubAck received successfully
* @retval	FAILURE	: pubAck not received
*/
unsigned char MqttClientCheckPubAckRspStatus(void);

/**
 * @brief	check timer elapsed flag status.
 *
 * @param	: 	void
 * @return 	bool
 * @retval 	false: timer running
 * @retval 	TRUE: timer elapsed
 *
*/
bool MqttClientCheckRspTimerStatus(void);

/**
 * @brief	Send disconnect control packet
 *
 * @param	: void
 * @return 	void
 *
*/
void MqttClientDisconnect(void);

/**
* @brief	Close TCP socket
*
* @param	: void
* @return 	void
*
*/
void MqttClientTransportClose(void);
/* -------------------------------- Routines -------------------------------- */

#endif /* MQTTCLIENTH2_FUNCTIONS_H */
