/**
 * @file MqttClientTimerMng.h
 * @author dr-paradox
 *
 */

#ifndef _MQTTCLIENTTIMERMNG_H
#define _MQTTCLIENTTIMERMNG_H

/* ------------------- BEGIN PERSISTENT NUMBER OF FSM INSTANCES  ------------------ */
#define MQTTCLIENTH2TIMERMNG_NUMBER_OF_INSTANCES 1u
/* -------------------- END PERSISTENT NUMBER OF FSM INSTANCES  ------------------- */

/*------------------------------------ INCLUDES ------------------------------------*/

/* -------------------------- BEGIN USER MANUAL INCLUDES  ------------------------- */

#include "MqttClientFunctions.h"

/* --------------------------- END USER MANUAL INCLUDES  -------------------------- */

/*----------------------------------- DATA TYPES -----------------------------------*/

typedef enum {
	STATE_0_MQTTCLIENTH2TIMERMNG = 0, /* 1 */
	STATE_MQTTCLIENTH2TIMERMNG_TIMERRUNNING = 1, /* 3 */
	STATE_MQTTCLIENTH2TIMERMNG_WAITTIMERSTART = 2, /* 9 */
	STATE_END_MQTTCLIENTH2TIMERMNG = 3 /* 10 */
} t_state_mqttclienth2timermng;

typedef struct {
	t_state_mqttclienth2timermng state_mqttclienth2timermng;
/* --------------------------- BEGIN INSTANCE VARIABLES  -------------------------- */

/* ---------------------------- END INSTANCE VARIABLES  --------------------------- */
} t_mqttclienth2timermng_instance_struct;

/* ------------------------------- BEGIN USER CODE  ------------------------------- */

/* -------------------------------- END USER CODE  -------------------------------- */

/*---------------------------------- INIT ROUTINES ---------------------------------*/

/**
 * @name MqttClientH2TimerMng_Init
 * @author dr-paradox
 * @brief Initialization routines
 *
 */
void MqttClientH2TimerMng_Init(unsigned char handler);

/*---------------------------------- TASK ROUTINES ---------------------------------*/

/**
 * @name MqttClientH2TimerMng_Task
 * @author dr-paradox
 * @brief Task routines
 *
 */
void MqttClientH2TimerMng_Task(unsigned char handler);

/*------------------------------- TERMINATE ROUTINES -------------------------------*/

/**
 * @name MqttClientH2TimerMng_Terminate
 * @author dr-paradox
 * @brief Terminate routine
 *
 */
void MqttClientH2TimerMng_Terminate(unsigned char handler);

/*------------------------------- GET STATES ROUTINES ------------------------------*/

/**
 * @name MqttClientH2TimerMng_GetState
 * @author dr-paradox
 * @brief Get state routine
 *
 */
t_state_mqttclienth2timermng MqttClientH2TimerMng_GetState(unsigned char handler);

/*-------------------------------------- END ---------------------------------------*/

#endif


