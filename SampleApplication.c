#include "unistd.h"
#include "stdbool.h"
#include "MqttClient.h"

/* Callback API to receive response from MqttClient */
void ServerResp(unsigned char server_response);

unsigned char json_msg[40] = "\"name\":\"John\",\"age\":31,\"city\":\"New York\"";

int main()
{
	int count = 0;
	/*1. Initialize the MqttClient */
	MqttClient_Init();

	/*2. Create a software timer to run the task of MqttClient every 1 Second */
	while (count < 100000) {
		sleep(1);
		MqttClient_Task();
		/*3. Send a dummy string to server in the created task */
		MqttClient_SendData(json_msg, (unsigned short int)40, SERVICE_REQ_1, ServerResp);

		count++;
	}

	return 0;
}

/* Callback API to receive response from MqttClientH2 */
void ServerResp(unsigned char server_response)
{
	/* A dummy definition */
}
