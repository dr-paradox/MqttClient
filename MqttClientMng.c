/**
 * @file MqttClientMng.c
 * @author dr-paradox
 *
 */

/*------------------------------------ INCLUDES ------------------------------------*/

#include "MqttClientMng.h"

/*------------------------------------ DEFINES -------------------------------------*/

#define THIS(index) (instance_params[(index)])

/*------------------------------------ VARIABLES -----------------------------------*/

static t_mqttclienth2mng_instance_struct instance_params[MQTTCLIENTH2MNG_NUMBER_OF_INSTANCES];

/* --------------------------- BEGIN EDITABLE CODE AREA  -------------------------- */

/* ---------------------------- END EDITABLE CODE AREA  --------------------------- */

/*--------------------------------- FUNCTION HEADERS -------------------------------*/

static void MqttClientH2Mng0(unsigned char handler);
static void SendPublishRequest(unsigned char handler);
static void WaitForData(unsigned char handler);
static void WaitMqttClientConnect(unsigned char handler);
static void WaitForPubAck(unsigned char handler);
static void WaitModemInit(unsigned char handler);

/*---------------------------------- INIT ROUTINES ---------------------------------*/

/**
 * @name MqttClientH2Mng_Init
 * @author dr-paradox
 * @brief MqttClientH2Mng task initialization routine
 *
 */
void MqttClientH2Mng_Init(unsigned char handler)
{
	if (handler <  (unsigned char)MQTTCLIENTH2MNG_NUMBER_OF_INSTANCES) {
/* --------- BEGIN MqttClientH2Mng user instance variables initialization  -------- */

/* ---------- END MqttClientH2Mng user instance variables initialization  --------- */

		/*-- State variable initialization --*/

		THIS(handler).state_mqttclienth2mng = STATE_0_MQTTCLIENTH2MNG;

		/*- TODO: print UART debug traces notifying state change -*/

		/*-- State machine initial cycle execution --*/
		MqttClientH2Mng_Task(handler);

	}
}

/*---------------------------------- TASK ROUTINES ---------------------------------*/

/**
 * @name MqttClientH2Mng
 * @author dr-paradox
 * @brief MqttClientH2Mng FSM
 *
 */
void MqttClientH2Mng_Task(unsigned char handler)
{

	if (handler <  (unsigned char)MQTTCLIENTH2MNG_NUMBER_OF_INSTANCES) {

		/*-- Check FSM current state --*/
		switch (THIS(handler).state_mqttclienth2mng){
		case STATE_0_MQTTCLIENTH2MNG:
			MqttClientH2Mng0(handler);
			break;
		case STATE_MQTTCLIENTH2MNG_SENDPUBLISHREQUEST:
			SendPublishRequest(handler);
			break;
		case STATE_MQTTCLIENTH2MNG_WAITFORDATA:
			WaitForData(handler);
			break;
		case STATE_MQTTCLIENTH2MNG_WAITMQTTCLIENTCONNECT:
			WaitMqttClientConnect(handler);
			break;
		case STATE_MQTTCLIENTH2MNG_WAITFORPUBACK:
			WaitForPubAck(handler);
			break;
		case STATE_MQTTCLIENTH2MNG_WAITMODEMINIT:
			WaitModemInit(handler);
			break;
		case STATE_END_MQTTCLIENTH2MNG:
			/*State machine is stopped*/
			THIS(handler).state_mqttclienth2mng = STATE_END_MQTTCLIENTH2MNG;
			break;
		default:
			THIS(handler).state_mqttclienth2mng = STATE_END_MQTTCLIENTH2MNG;
			break;
		}
	}
}


/*------------------------------- GET STATES ROUTINES ------------------------------*/

/**
 * @name MqttClientH2Mng_GetState
 * @author dr-paradox
 * @brief Used to get state of a process
 *
 */
t_state_mqttclienth2mng MqttClientH2Mng_GetState(unsigned char handler)
{
	t_state_mqttclienth2mng result;

	if (handler >=  (unsigned char)MQTTCLIENTH2MNG_NUMBER_OF_INSTANCES) {
		/*Instance handler higher than existing instances*/
		result = STATE_END_MQTTCLIENTH2MNG;
	} else {
		result = THIS(handler).state_mqttclienth2mng;
	}

	return (result);
}


/*---------------------------------- STATE ROUTINE  --------------------------------*/

/**
 * @name MqttClientH2Mng0
 * @author dr-paradox
 * @brief MqttClientH2Mng first state routine
 *
 */
static void MqttClientH2Mng0(unsigned char handler)
{
	/*flags to reset:
    1. modem_connected
    2.profile_started
    3.ip_obtained*/

    /*-- Case where transition T1_PowerOnModemInit is executed --*/

	/*-- Action of the transition --*/

	MqttClientResetFlags();
    MqttClientModemInit();
    MqttClientClearStartTimer(MODEM_REQ);

	/*-- Changing to next state --*/

	THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITMODEMINIT;

}

/**
 * @name SendPublishRequest
 * @author dr-paradox
 * @brief Json message to be published to Mqtt broker
 *
 */
static void SendPublishRequest(unsigned char handler)
{
	/*-- Code of the current state --*/

	bool request_to_cancel = false;;
    uint8_t retry_count = ZERO;;
    uint8_t publish_req_status = FAILURE;;
    request_to_cancel = MqttClientCheckRequestToCancel();;
    retry_count = MqttClientCheckRetryCount();;
    publish_req_status = MqttClientCheckPubReqStatus();;

	if ( (true == request_to_cancel) && (FAILURE == publish_req_status) ) {

		/*-- Case where transition T1_CancelPublishRequest is executed --*/

		/*-- Action of the transition --*/

		MqttClientClearRetryCount();
        MqttClientServiceNotify(SERVERCOM_CANCELED);

		/*-- Entry of destination state --*/

		MqttClientClearStartTimer(PING_REQ);;

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITFORDATA;

	}

	/*MAX_REQ_RETRY_COUNT = ((uint8_t)3)*/
	else if ( (retry_count < MAX_REQ_RETRY_COUNT) && (FAILURE == publish_req_status) && (false == request_to_cancel) ) {

		/*-- Case where transition T2_RetryPublishRequest is executed --*/

		/*-- Action of the transition --*/

		MqttClientSendPubRequest();
        MqttClientIncrementRetryCount();

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_SENDPUBLISHREQUEST;

	}

	else if ( SUCCESS== publish_req_status  ) {

		/*-- Case where transition T3_WaitForPubAck is executed --*/

		/*-- Action of the transition --*/

		MqttClientClearRetryCount();
        MqttClientClearStartTimer(KEEP_ALIVE);

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITFORPUBACK;

	}

	else if ( (retry_count >= MAX_REQ_RETRY_COUNT) && (FAILURE == publish_req_status) ) {

		/*-- Case where transition T4_PublishRequestFailure is executed --*/

		/*-- Action of the transition --*/

		MqttClientClearRetryCount();
        MqttClientServiceNotify(SERVERCOM_ERROR);

		/*-- Entry of destination state --*/

		MqttClientClearStartTimer(PING_REQ);;

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITFORDATA;

	}

	else {

		/*-- No transition condition accomplished. Stay in same state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_SENDPUBLISHREQUEST;
	}
}

/**
 * @name WaitForData
 * @author dr-paradox
 * @brief Write brief explanation here
 *
 */
static void WaitForData(unsigned char handler)
{
	/*-- Code of the current state --*/

	bool modem_is_connected = false;;
    bool data_is_available = false;;
    bool time_to_ping = false;;
    modem_is_connected = MqttClientCheckModemConnection();;
    data_is_available = MqttClientCheckDataToSend();;
    time_to_ping = MqttClientCheckTimeToPing();;

    if ( false == modem_is_connected ) {

		/*-- Case where transition T1_RetryModemConnection is executed --*/

		/*-- Action of the transition --*/

    	MqttClientModemInit();

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITMODEMINIT;

	}

    else if ( (true == time_to_ping) && (false == data_is_available) ) {

		/*-- Case where transition T2_SendPingRequest is executed --*/

		/*-- Action of the transition --*/

		MqttClientSendPingRequest();

		/*-- Entry of destination state --*/

		MqttClientClearStartTimer(PING_REQ);;

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITFORDATA;

	}

	else if ( true == data_is_available ) {

		/*-- Case where transition T3_SendPublishRequest is executed --*/

		/*-- Action of the transition --*/

		MqttClientSendPubRequest();
        MqttClientDecrementRetryCount();

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_SENDPUBLISHREQUEST;

	}

	else {

		/*-- No transition condition accomplished. Stay in same state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITFORDATA;
	}
}

/**
 * @name WaitMqttClientConnect
 * @author dr-paradox
 * @brief Wait till Mqtt client connects to Mqtt broker
 *
 */
static void WaitMqttClientConnect(unsigned char handler)
{
	/*-- Code of the current state --*/

	bool modem_is_connected = false;;
    bool mqtt_client_is_connected = false;;
    bool time_to_retry = false;;
    modem_is_connected = MqttClientCheckModemConnection();;
    mqtt_client_is_connected = MqttClientCheckMqttConnection();;
    time_to_retry  = MqttClientCheckTimeToRetry();;

    if ( false == modem_is_connected ) {

		/*-- Case where transition T1_RetryModemInit is executed --*/

		/*-- Action of the transition --*/

    	MqttClientModemInit();
    	MqttClientClearStartTimer(MODEM_REQ);

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITMODEMINIT;

	}

    else if ( (false == mqtt_client_is_connected) && (true == time_to_retry)  ) {

		/*-- Case where transition T2_RetryMqttConnectRequest is executed --*/

		/*-- Action of the transition --*/

		MqttClientClearRetryCount();
        MqttClientServiceNotify(SERVERCOM_TIMEOUT);
        MqttClientSendConnectRequest();
        MqttClientClearStartTimer(KEEP_ALIVE);

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITMQTTCLIENTCONNECT;

	}

	else if ( true == mqtt_client_is_connected  ) {

		/*-- Case where transition T3_MqttClientConnectionEstablished is executed --*/

		/*-- Entry of destination state --*/

		MqttClientClearStartTimer(PING_REQ);;

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITFORDATA;

	}

	else {

		/*-- No transition condition accomplished. Stay in same state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITMQTTCLIENTCONNECT;
	}
}

/**
 * @name WaitForPubAck
 * @author dr-paradox
 * @brief Write brief explanation here
 *
 */
static void WaitForPubAck(unsigned char handler)
{
	/*-- Code of the current state --*/

	uint8_t puback_rsp_status = FAILURE;;
    bool timer_expired = false;;
    puback_rsp_status = MqttClientCheckPubAckRspStatus();;
    timer_expired = MqttClientCheckRspTimerStatus();;

	if ( SUCCESS == puback_rsp_status ) {

		/*-- Case where transition T1_DataSentNotifyService is executed --*/

		/*-- Action of the transition --*/

		MqttClientServiceNotify(SERVERCOM_OK);

		/*-- Entry of destination state --*/

		MqttClientClearStartTimer(PING_REQ);;

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITFORDATA;

	}

	else if ( (puback_rsp_status != SUCCESS) && (true == timer_expired) ) {

		/*-- Case where transition T2_MqttClientReconnect is executed --*/

		/*-- Action of the transition --*/

		MqttClientServiceNotify(SERVERCOM_ERROR);
        MqttClientDisconnect();
        MqttClientClearStartTimer(KEEP_ALIVE);

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITMQTTCLIENTCONNECT;

	}

	else {

		/*-- No transition condition accomplished. Stay in same state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITFORPUBACK;
	}
}

/**
 * @name WaitModemInit
 * @author dr-paradox
 * @brief Wait for ModemDataMng to start data connectivity.
 *
 */
static void WaitModemInit(unsigned char handler)
{
	/*-- Code of the current state --*/

	bool modem_is_connected = false;;
    bool time_to_retry = false;;
    modem_is_connected = MqttClientCheckModemConnection();;
    time_to_retry  = MqttClientCheckTimeToRetry();;

	if ( (false == modem_is_connected) && (true == time_to_retry)  ) {

		/*-- Case where transition T1_RetryModemInit is executed --*/

		/*-- Action of the transition --*/

		MqttClientClearRetryCount();
        MqttClientServiceNotify(SERVERCOM_TIMEOUT);
        MqttClientModemInit();
        MqttClientClearStartTimer(MODEM_REQ);

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITMODEMINIT;

	}

	else if ( true == modem_is_connected ) {

		/*-- Case where transition T2_ModemConnected is executed --*/

		/*-- Action of the transition --*/

		MqttClientSendConnectRequest();
        MqttClientClearStartTimer(KEEP_ALIVE);

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITMQTTCLIENTCONNECT;

	}

	else {

		/*-- No transition condition accomplished. Stay in same state --*/

		THIS(handler).state_mqttclienth2mng = STATE_MQTTCLIENTH2MNG_WAITMODEMINIT;
	}
}

/*------------------------------- TERMINATE ROUTINES -------------------------------*/

/**
 * @name MqttClientH2Mng_Terminate
 * @author dr-paradox
 * @brief Routine used to stop main process
 *
 */
void MqttClientH2Mng_Terminate(unsigned char handler)
{
	/*MqttClientH2Mng Terminate function code*/

	if (handler <  (unsigned char)MQTTCLIENTH2MNG_NUMBER_OF_INSTANCES) {

		THIS(handler).state_mqttclienth2mng = STATE_END_MQTTCLIENTH2MNG;
/* --------------------------- BEGIN TERMINATE ACTIONS  --------------------------- */
		(void)MqttClientTransportClose();
/* ---------------------------- END TERMINATE ACTIONS  ---------------------------- */
	}
}

