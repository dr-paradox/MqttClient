/**
 * @file MqttClientTimerMng.c
 * @author dr-paradox
 *
 */

/*------------------------------------ INCLUDES ------------------------------------*/

#include "MqttClientTimerMng.h"

/*------------------------------------ DEFINES -------------------------------------*/
#define THIS(index) (instance_params[(index)])

/*------------------------------------ VARIABLES -----------------------------------*/

static t_mqttclienth2timermng_instance_struct instance_params[MQTTCLIENTH2TIMERMNG_NUMBER_OF_INSTANCES];

/* --------------------------- BEGIN EDITABLE CODE AREA  -------------------------- */

/* ---------------------------- END EDITABLE CODE AREA  --------------------------- */

/*--------------------------------- FUNCTION HEADERS -------------------------------*/

static void MqttClientH2TimerMng0(unsigned char handler);
static void TimerRunning(unsigned char handler);
static void WaitTimerStart(unsigned char handler);

/*---------------------------------- INIT ROUTINES ---------------------------------*/

/**
 * @name MqttClientH2TimerMng_Init
 * @author dr-paradox
 * @brief MqttClientH2TimerMng task initialization routine
 *
 */
void MqttClientH2TimerMng_Init(unsigned char handler)
{
	if (handler <  (unsigned char)MQTTCLIENTH2TIMERMNG_NUMBER_OF_INSTANCES) {
/* ------ BEGIN MqttClientH2TimerMng user instance variables initialization  ------ */

/* ------- END MqttClientH2TimerMng user instance variables initialization  ------- */

		/*-- State variable initialization --*/

		THIS(handler).state_mqttclienth2timermng = STATE_0_MQTTCLIENTH2TIMERMNG;

		/*- TODO: print UART debug traces notifying state change -*/

		/*-- State machine initial cycle execution --*/
		MqttClientH2TimerMng_Task(handler);

	}
}

/*---------------------------------- TASK ROUTINES ---------------------------------*/

/**
 * @name MqttClientH2TimerMng
 * @author dr-paradox
 * @brief MqttClientH2TimerMng FSM
 *
 */
void MqttClientH2TimerMng_Task(unsigned char handler)
{

	if (handler <  (unsigned char)MQTTCLIENTH2TIMERMNG_NUMBER_OF_INSTANCES) {

		/*-- Check FSM current state --*/
		switch (THIS(handler).state_mqttclienth2timermng){
		case STATE_0_MQTTCLIENTH2TIMERMNG:
			MqttClientH2TimerMng0(handler);
			break;
		case STATE_MQTTCLIENTH2TIMERMNG_TIMERRUNNING:
			TimerRunning(handler);
			break;
		case STATE_MQTTCLIENTH2TIMERMNG_WAITTIMERSTART:
			WaitTimerStart(handler);
			break;
		case STATE_END_MQTTCLIENTH2TIMERMNG:
			/*State machine is stopped*/
			THIS(handler).state_mqttclienth2timermng = STATE_END_MQTTCLIENTH2TIMERMNG;
			break;
		default:
			THIS(handler).state_mqttclienth2timermng = STATE_END_MQTTCLIENTH2TIMERMNG;
			break;
		}
	}
}


/*------------------------------- GET STATES ROUTINES ------------------------------*/

/**
 * @name MqttClientH2TimerMng_GetState
 * @author dr-paradox
 * @brief Used to get state of a process
 *
 */
t_state_mqttclienth2timermng MqttClientH2TimerMng_GetState(unsigned char handler)
{
	t_state_mqttclienth2timermng result;

	if (handler >=  (unsigned char)MQTTCLIENTH2TIMERMNG_NUMBER_OF_INSTANCES) {
		/*Instance handler higher than existing instances*/
		result = STATE_END_MQTTCLIENTH2TIMERMNG;
	} else {
		result = THIS(handler).state_mqttclienth2timermng;
	}

	return (result);
}


/*---------------------------------- STATE ROUTINE  --------------------------------*/

/**
 * @name MqttClientH2TimerMng0
 * @author dr-paradox
 * @brief MqttClientH2TimerMng first state routine
 *
 */
static void MqttClientH2TimerMng0(unsigned char handler)
{
    /*-- Case where transition T1_MqttClientTimerMngActivated is executed --*/
	/*-- Changing to next state --*/

	THIS(handler).state_mqttclienth2timermng = STATE_MQTTCLIENTH2TIMERMNG_WAITTIMERSTART;

}

/**
 * @name TimerRunning
 * @author dr-paradox
 * @brief Wait for Timer to elapse
 *
 */
static void TimerRunning(unsigned char handler)
{
	/*-- Code of the current state --*/

	uint8_t count = ZERO;;
    uint8_t timer_interval = ZERO;;
    timer_interval = MqttClientGetTimerInterval();;

	if ( count < timer_interval ) {

		/*-- Case where transition T1_IncreamentTimerCount is executed --*/

		/*-- Action of the transition --*/

		count = count + SERVICE_CYCLE_TIME;

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2timermng = STATE_MQTTCLIENTH2TIMERMNG_TIMERRUNNING;

	}

	else if ( count >= timer_interval  ) {

		/*-- Case where transition T2_TimerElapsed is executed --*/

		/*-- Action of the transition --*/

		MqttClientClearTimerReq();
        MqttClientNotifyTimerElapsed();

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2timermng = STATE_MQTTCLIENTH2TIMERMNG_WAITTIMERSTART;

	}

	else {

		/*-- No transition condition accomplished. Stay in same state --*/

		THIS(handler).state_mqttclienth2timermng = STATE_MQTTCLIENTH2TIMERMNG_TIMERRUNNING;
	}
}

/**
 * @name WaitTimerStart
 * @author dr-paradox
 * @brief Wait for Timer to start
 *
 */
static void WaitTimerStart(unsigned char handler)
{
	/*-- Code of the current state --*/

	bool start_timer = false;;
    start_timer = MqttClientCheckTimerReq();;

	if ( true == start_timer ) {

		/*-- Case where transition T1_TimerStartReqReceived is executed --*/

		/*-- Changing to next state --*/

		THIS(handler).state_mqttclienth2timermng = STATE_MQTTCLIENTH2TIMERMNG_TIMERRUNNING;

	}

	else {

		/*-- No transition condition accomplished. Stay in same state --*/

		THIS(handler).state_mqttclienth2timermng = STATE_MQTTCLIENTH2TIMERMNG_WAITTIMERSTART;
	}
}

/*------------------------------- TERMINATE ROUTINES -------------------------------*/

/**
 * @name MqttClientH2TimerMng_Terminate
 * @author dr-paradox
 * @brief Routine used to stop main process
 *
 */
void MqttClientH2TimerMng_Terminate(unsigned char handler)
{
	/*MqttClientH2TimerMng Terminate function code*/

	if (handler <  (unsigned char)MQTTCLIENTH2TIMERMNG_NUMBER_OF_INSTANCES) {

		THIS(handler).state_mqttclienth2timermng = STATE_END_MQTTCLIENTH2TIMERMNG;
/* --------------------------- BEGIN TERMINATE ACTIONS  --------------------------- */
		(void)MqttClientTransportClose();
/* ---------------------------- END TERMINATE ACTIONS  ---------------------------- */
	}
}

