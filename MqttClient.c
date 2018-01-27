/**************************************************************************//**
*  @par Language  : C
*******************************************************************************
*  @brief       MqttClient public API implementation.
*  @author      dr-paradox
*  @version     1.0.0
*******************************************************************************
*  @page 	MqttClient
*  @ref 	MqttClient "API Reference"
*  @file 	MqttClient.c
*
*******************************************************************************/

/* -------------------------------- Includes -------------------------------- */

#include "MqttClient.h"
#include "MqttClientCfg.h"
#include "MqttClientFunctions.h"
#include "MqttClientMng.h"
#include "MqttClientTimerMng.h"

/* -------------------------------- Defines --------------------------------- */

/* ------------------------------- Data Types ------------------------------- */

/* ---------------------------- Global Variables ---------------------------- */

/* --------------------------- Routine prototypes --------------------------- */

/* -------------------------------- Routines -------------------------------- */
/**
 * @brief	MqttClientH2 initialization
 *
 * @param[in]	: void
 * @return		void
 *
*/
 void MqttClient_Init(void)
{
    MqttClientH2Mng_Init(MQTTCLIENTH2_HANDLER);
    MqttClientH2TimerMng_Init(MQTTCLIENTH2_HANDLER);

    printf("MqttClient initialized");
}

/**
 * @brief	MqttClientH2 periodical task
 *
 * @param[in]	: void
 * @return		void
 *
*/
 void MqttClient_Task(void)
{
	MqttClientH2Mng_Task(MQTTCLIENTH2_HANDLER);
	MqttClientH2TimerMng_Task(MQTTCLIENTH2_HANDLER);
}

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
 void MqttClient_SendData(unsigned char *json, unsigned short size, unsigned char service_id, RxCbk cbk)
{
	printf("MqttClient: Service %d want to send a %d bytes message", service_id, size);

	if (( service_id >= SERVICE_LAST ) || ( json == NULL) ||
		( cbk == NULL ) || ( size > SERVER_COM_JSON_MAX_SIZE ))
	{
		printf("MqttClient: Bad request received, service id: %d, json size:%d ", service_id, size);

		/*Notify the service that is send a bad request*/
		if ( cbk != NULL )
		{
			cbk((unsigned char)SERVERCOM_BAD_REQUEST);
		}
	}
	else
	{

		printf("MqttClient: Request received from service %d, size %d", service_id, size);

		/*Store the json*/
		memcpy( &ServiceRequestList[service_id].json, json, size);

		/*Store the service notification cbk*/
		ServiceRequestList[service_id].cbk = cbk;

		ServiceRequestList[service_id].json_size = size;

		/*Load the retry counter. This must be the last instruction because setting the retry_count
		 * marks the request as being active. So if this function is interrupted by ServerCom thread
		 * it shouldn't be the situation in which the retry_counter value !=0 and the rest of the request data not set yet*/
		ServiceRequestList[service_id].retry_count = MAX_REQ_RETRY_COUNT;
	}
}

/**
 * @brief		External API called to send data to server by other services
 *
 * @param[in]	: void
 * @return		void
 *
*/
 void MqttClient_CancelRequest(void)
{
	printf("MqttClient: Cancel request received!");
	cancel_request = true;
}
