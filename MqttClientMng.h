/**
 * @file MqttClientMng.h
 * @author dr-paradox
 *
 */

#ifndef _MQTTCLIENTMNG_H
#define _MQTTCLIENTMNG_H

/* ------------------- BEGIN PERSISTENT NUMBER OF FSM INSTANCES  ------------------ */
#define MQTTCLIENTH2MNG_NUMBER_OF_INSTANCES 1u
/* -------------------- END PERSISTENT NUMBER OF FSM INSTANCES  ------------------- */

/*------------------------------------ INCLUDES ------------------------------------*/

/* -------------------------- BEGIN USER MANUAL INCLUDES  ------------------------- */

#include "MqttClientFunctions.h"

/* --------------------------- END USER MANUAL INCLUDES  -------------------------- */

/*----------------------------------- DATA TYPES -----------------------------------*/

typedef enum {
	STATE_0_MQTTCLIENTH2MNG = 0, /* 2 */
	STATE_MQTTCLIENTH2MNG_SENDPUBLISHREQUEST = 1, /* 4 */
	STATE_MQTTCLIENTH2MNG_WAITFORDATA = 2, /* 5 */
	STATE_MQTTCLIENTH2MNG_WAITMQTTCLIENTCONNECT = 3, /* 6 */
	STATE_MQTTCLIENTH2MNG_WAITFORPUBACK = 4, /* 7 */
	STATE_MQTTCLIENTH2MNG_WAITMODEMINIT = 5, /* 8 */
	STATE_END_MQTTCLIENTH2MNG = 6 /* 9 */
} t_state_mqttclienth2mng;

typedef struct {
	t_state_mqttclienth2mng state_mqttclienth2mng;
/* --------------------------- BEGIN INSTANCE VARIABLES  -------------------------- */

/* ---------------------------- END INSTANCE VARIABLES  --------------------------- */
} t_mqttclienth2mng_instance_struct;

/* ------------------------------- BEGIN USER CODE  ------------------------------- */

/* -------------------------------- END USER CODE  -------------------------------- */

/*---------------------------------- INIT ROUTINES ---------------------------------*/

/**
 * @name MqttClientH2Mng_Init
 * @author dr-paradox
 * @brief Initialization routines
 *
 */
void MqttClientH2Mng_Init(unsigned char handler);

/*---------------------------------- TASK ROUTINES ---------------------------------*/

/**
 * @name MqttClientH2Mng_Task
 * @author dr-paradox
 * @brief Task routines
 *
 */
void MqttClientH2Mng_Task(unsigned char handler);

/*------------------------------- TERMINATE ROUTINES -------------------------------*/

/**
 * @name MqttClientH2Mng_Terminate
 * @author dr-paradox
 * @brief Terminate routine
 *
 */
void MqttClientH2Mng_Terminate(unsigned char handler);

/*------------------------------- GET STATES ROUTINES ------------------------------*/

/**
 * @name MqttClientH2Mng_GetState
 * @author dr-paradox
 * @brief Get state routine
 *
 */
t_state_mqttclienth2mng MqttClientH2Mng_GetState(unsigned char handler);

/*-------------------------------------- END ---------------------------------------*/

#endif


