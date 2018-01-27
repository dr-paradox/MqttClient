/**************************************************************************//**
*  @par Language: C
*******************************************************************************
*  @par 	Project:
*  @brief   MqttClient internal API implementation
*  @author  dr-paradox
*  @version 1.0.0
*******************************************************************************
*  @ref 	MqttClient "API Reference"
*  @file 	MqttClientFunctions.c
*******************************************************************************/

/* -------------------------------- Includes -------------------------------- */

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/param.h>
#include <sys/time.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "MqttClientFunctions.h"

/* -------------------------------- Defines --------------------------------- */

/* Timer running intervals */
#define MODEM_REQ_TIME_INTERVAL		((unsigned char)10)
#define KEEP_ALIVE_TIME_INTERVAL	((unsigned char)20)
#define PING_REQ_TIME_INTERVAL		((unsigned char)10)

/* Invalid socket descriptor */
#define INVALID_SOCKET				((int)-1)

/* "Last Will and Testament" (LWT) options initializer */
#define WILL_OPTIONS_INIT 			{ {'M', 'Q', 'T', 'W'}, 0, {NULL, {0, NULL}}, {NULL, {0, NULL}}, 0, 0 }

/* Mqtt connect packet initializer */
#define CONNECT_OPTIONS_INIT 		{ {'M', 'Q', 'T', 'C'}, 0, 4, {NULL, {0, NULL}}, 60, 1, 0, \
									WILL_OPTIONS_INIT, {NULL, {0, NULL}}, {NULL, {0, NULL}} }

/* Mqtt publish packet initializer */
#define PUBLISH_OPTIONS_INIT		{{(unsigned char)0}, {NULL, {0, NULL}}, (unsigned char)0, NULL, (unsigned char)0}

/* Max connect packet size */
#define MAX_CONN_PACK_SIZE			((unsigned short)512)

/* Max connect packet size */
#define MAX_PING_PACK_SIZE			((unsigned short)2)

/* Max connect packet size */
#define MAX_DISCONN_PACK_SIZE		((unsigned short)2)

/* Max connect packet size */
#define MAX_DISCONN_PACK_SIZE		((unsigned short)2)

/* Max connect packet size */
#define INT_PUBLISH_PACK_SIZE		((unsigned short)5005)

/* Max connect packet size */
#define MAX_PUBLISH_PACK_SIZE		((unsigned short)(INT_PUBLISH_PACK_SIZE + TOPIC_LENGTH))

/* Max connect packet size */
#define MAX_CONNACK_PACK_SIZE		((unsigned char)4)

/* Max connect packet size */
#define MAX_PUBACK_PACK_SIZE		((unsigned char)4)

/* Connect packet header byte : bit 4 set to 1*/
#define CONNECT_HEADER_BYTE			((unsigned char)0x10)

/* Connect packet creation error */
#define BUFFER_TOO_SHORT			((unsigned char)0)

/* ConnectAck packet return code success */
#define CONNECTION_ACCEPTED			((unsigned char)0x00)

/* Size of a single byte */
#define ONE_BYTE					((int)1)
#define TWO_BYTES					((int)2)
#define FOUR_BYTES					((int)4)

/* Ping packet contents */
#define PING_HEADER_BYTE			((unsigned char)0xC0);
#define PING_LENGTH_BYTE			((unsigned char)0x00);

/* Disconnect packet contents */
#define DISCONN_HEADER_BYTE			((unsigned char)0xE0);
#define DISCONN_LENGTH_BYTE			((unsigned char)0x00);

/* ------------------------------- Data Types ------------------------------- */

/* structure to store length delimited data */
typedef struct
{
	int len;
	char* data;
} t_mqtt_string_len;

/* Mqtt formatted string data */
typedef struct
{
	char* cstring;
	t_mqtt_string_len lenstring;
} t_mqtt_string;

/* defines the MQTT "Last Will and Testament" (LWT) settings for the connect packet */
typedef struct
{
	char struct_id[4];				/* struct id currently unused */
	int struct_version;				/* The version number of this structure.  Must be 0 */
	t_mqtt_string topicName;		/* The LWT topic to which the LWT message will be published. */
	t_mqtt_string message;			/* The LWT payload. */
	unsigned char retained;			/* The retained flag for the LWT message */
	char qos;						/* The quality of service setting for the LWT message */
} t_mqtt_will_options;

/* Mqtt connect packet options */
typedef struct
{
	char struct_id[4];					/* struct id currently unused */
	int struct_version;					/* The version number of this structure */
	unsigned char mqtt_version;			/* Version of MQTT to be used.  3 = 3.1 4 = 3.1.1 */
	t_mqtt_string client_id;			/* mqtt formatted client id string */
	unsigned short keep_alive_interval;	/* keep alive interval */
	unsigned char clean_session;		/* clean session flag */
	unsigned char will_flag;			/* will flag */
	t_mqtt_will_options will;			/* will options */
	t_mqtt_string username;				/* mqtt formatted username string */
	t_mqtt_string password;				/* mqtt formatted password string */
} t_mqtt_connect_packet_options;

typedef union
{
	unsigned char all;					/* mqtt formatted password string */

	struct
	{
		unsigned int : 1;				/* unused */
		unsigned int clean_session : 1;	/* cleansession flag */
		unsigned int will : 1;			/* will flag */
		unsigned int willQoS : 2;		/* will QoS value */
		unsigned int willRetain : 1;	/* will retain setting */
		unsigned int password : 1; 		/* 3.1 password */
		unsigned int username : 1;		/* 3.1 user name */
	} bits;
} t_mqtt_connect_flags;	/**< connect flags byte */

/* Structure to retrieve message type from header byte received */
typedef union
{
	unsigned char byte;	                /* the whole byte */

	struct
	{
		unsigned int retain : 1;		/*retained flag bit */
		unsigned int qos : 2;			/* QoS value, 0, 1 or 2 */
		unsigned int dup : 1;			/* DUP flag bit */
		unsigned int type : 4;			/* message type nibble */
	} bits;
}t_mqtt_header_byte;

typedef struct
{
	t_mqtt_header_byte header_options;
	t_mqtt_string topic_name;
	unsigned short packet_id;
	unsigned char* payload;
	unsigned short payload_len;

}t_mqtt_publish_packet_options;

/* Mqtt control packet types */
typedef enum
{
	CONNECT 	= 1,
	CONNACK 	= 2,
	PUBLISH 	= 3,
	PUBACK 		= 4,
	PUBREC		= 5,
	PUBREL		= 6,
	PUBCOMP		= 7,
	SUBSCRIBE 	= 8,
	SUBACK		= 9,
	UNSUBSCRIBE	= 10,
	UNSUBACK	= 11,
	PINGREQ		= 12,
	PINGRESP	= 13,
	DISCONNECT	= 14
}t_msg_types;

/* ---------------------------- Global Variables ---------------------------- */

/* List to hold data related to each associated service such as retry count, json, size, cbk func */
t_PendingRequest ServiceRequestList[SERVICE_LAST] = {{0, {0}, 0, NULL}};

/* Request to cancel current ongoing transmission */
bool cancel_request = false;

/* profile currently in use */
unsigned char profile = ((unsigned char)0);

/* flag to store modem connection status */
static bool modem_connected = false;
/* flag to store profile start status */
static bool profile_started = false;
/* flag to store ip status */
static bool ip_obtained = false;
/* variable to store timer run duration */
static unsigned char timer_interval = ZERO;
/* flag to store timer request status */
static bool timer_start = false;
/* flag to store timer elapsed status */
static bool timer_elapsed = false;
/* flag to store client connection status */
static bool client_connected = false;
/* index of current service id under process */
static unsigned char service_idx = ((unsigned char)SERVICE_LAST);
/* index of current service id under process */
static unsigned char pub_req_status = FAILURE;
/**
 *  socket descriptor : This simple low-level implementation assumes a single connection for a single thread.
 *   Thus, a static variable is used for that connection.
*/
static int socket_desc = INVALID_SOCKET;

/* --------------------------- Routine prototypes --------------------------- */
/**
 * @brief	Session result handler function.
 *
 * @param[in]	: profile_id		: index of profile in use
 * @param[in]	: session_result	: session result returned from ModemDataMng
 * @param[in]	: contextPtr		: NA
 * @return 		void
 *
*/
//[TODO: modem func disabled]static void SessionResultHandlerFunction(unsigned char profile_id, ModemDataMng_res_t session_result, void* contextPtr);

/**
 * @brief	Session state handler function.
 *
 * @param[in]	: profile_id	: index of profile in use
 * @param[in]	: session_state	: session state returned from ModemDataMng
 * @param[in]	: contextPtr	: NA
 * @return 		void
 *
*/
//[TODO: modem func disabled]static void SessionStateHandlerFunction(unsigned char profile_id, ModemDataMng_conn_state_t session_state, void* contextPtr);

/**
 * @brief	Set timer interval value.
 *
 * @param[in]	: req_timer_interval	: timer interval value to be set
 * @return 		void
 *
*/
static void MqttClientSetTimerInterval(unsigned char req_timer_interval);

/**
 * @brief	Set timer interval value.
 *
 * @param	void
 * @return 	void
 *
*/
static void MqttClientSetStartTimer(void);

/**
 * @brief	Create a socket and connect to desired host on specified port.
 *
 * @param[in]	: addr	: host address
 * @param[in]	: port	: port
 * @return 		int
 * @retval 		>=0: socket descriptor since socket connection successful
 * @retval 		<0: failure
 *
*/
static int MqttClientTransportOpen(char *addr, int port);

/**
* @brief	send a packet over socket
*
* @param[in]	: socket	: socket descriptor
* @param[in]	: buf		: packet buffer to be sent over network
* @param[in]	: buflen	: length of packet buffer to be sent
* @return 		int
* @reval		0 : Success
* @retval		!0: failure
*
*/
static int MqttClientTransportSendPacketBuffer(int sock, unsigned char* buf, int buflen);

/**
* @brief	receive a packet over socket
*
* @param[in]	: buf	: packet buffer to be sent over network
* @param[in]	: count	: bytes to read
* @return 		int
* @reval		0 : Success
* @retval		!0: failure
*
*/
static int MqttClientTransportReceivePacketBuffer(unsigned char* buf, int byte_count);

/**
 * @brief	sets connect packet options structure with defined options in configuration file.
 *
 * @param[in]	: options	:  pointer to connect packet options structure to set
 * @return 		void
 *
*/
static void MqttClientSetConnectPacketOptions(t_mqtt_connect_packet_options *options);

/**
 * @brief	sets publish packet options structure with defined options in configuration file.
 *
 * @param[in]	: options	:  pointer to connect packet options structure to set
 * @return 		void
 *
*/
static void MqttClientSetPublishPacketOptions(t_mqtt_publish_packet_options *options);

/**
* @brief	Serializes ping packet into supplied buffer.
*
* @param[out]	: buf	: the buffer into which the ping packet will be serialized
* @return		int
* @retval 		serialized length
* @retval 		0 as error
 */
static int MqttClientCreatePingPacket(unsigned char* buf);

/**
* @brief	Serializes disconnect packet into supplied buffer.
*
* @param[out]	: buf	: the buffer into which the disconnect packet will be serialized
* @return		int
* @retval 		serialized length
* @retval 		0 as error
 */
static int MqttClientCreateDisconnectPacket(unsigned char* buf);

/**
* @brief	Serializes the connect options into the buffer.
*
* @param[out]	: buf 			: the buffer into which the packet will be serialized
* @param[in]	: len 			: the length in bytes of the supplied buffer
* @param[in]	: def_options	: the options to be used to build the connect packet
* @return		int
* @retval 		serialized length
* @retval 		0 as error
 */
static int MqttClientCreateConnectPacket(unsigned char* buf, int buflen, t_mqtt_connect_packet_options* def_options);

/**
* @brief	Serializes the publish options into the buffer.
*
* @param[out]	: buf			: the buffer into which the packet will be serialized
* @param[in]	: buflen		: the length in bytes of the supplied buffer
* @param[in]	: def_options	: the options to be used to build the publish packet
* @return		int
* @retval 		serialized length
* @retval 		0 as error
 */
static int MqttClientCreatePublishPacket(unsigned char* buf, int buflen, t_mqtt_publish_packet_options* def_options);

/**
* brief		Determines the length of the MQTT connect packet that would be produced using the supplied connect options.
*
* @param[in]	: options	: the options to be used to build the connect packet
* @return		int
* @retval		length of buffer needed to contain the serialized version of the packet
*/
static int MqttClientCalConnectPacketLength(t_mqtt_connect_packet_options* conn_options);

/**
* brief		Determines the length of the MQTT publish packet that would be produced using the supplied options.
*
* @param[in]	: def_options	: options to be used to build the publish packet
* @return		int
* @retval		length of buffer needed to contain the serialized version of the packet
*/
static int MqttClientCalPublishPacketLength(t_mqtt_publish_packet_options* def_options);

/**
* @brief	Return the length of the MQTTstring - C string if there is one, otherwise the length delimited string
*
* @param[in]	: mqttstring	: string to return the length of
* @return		int
* @retval		length of the string
*/
static int MqttClientStrLen(t_mqtt_string mqttstring);

/**
* @brief	Check whether available length is enough to store serialized packet
*
* @param[in]	: req_len		: length required to store serialized buffer except fixed header
* @param[in]	: available_len	: length available to store serialized packet
* @return		bool
* @retval		true			: available length enough to store serialized packet
* @retval		false			: available length not enough to store serialized packet
*/
static bool MqttClientCheckBufferSize(int req_len, int available_len);

/**
* @brief	Reads one character from an input buffer.
* @param[in]	: buff_index_ptr	: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]	: byte				: byte read
* @return		void
*/
static void MqttClientByteRead(unsigned char** buff_index_ptr, unsigned char* byte);

/**
* @brief	Writes one character to an output buffer.
* @param[in-out]	: buff_index_ptr	: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]		: byte		: character to the write
* @return			void
*/
static void MqttClientByteWrite(unsigned char** buff_index_ptr, char byte);

/**
* @brief	Encodes the message length according to the MQTT algorithm
* @param[out]	: buf		: buffer into which the encoded data is written
* @param[in]	: length	: length to be encoded
* @return		int
* @retval		number of bytes written to buffer
*/
static int MqttClientEncodePacketLen(unsigned char* buf, int length);

/**
* @brief	Writes a "UTF" string to an output buffer.  Converts C string to length-delimited.
* @param[out]	: buff_index_ptr		: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]	: string	: C string to write
* @return 		: void
*/
static void MqttClientStringWrite(unsigned char** buff_index_ptr, const char* string);

/**
* @brief	Writes an integer as 2 bytes to an output buffer.
* @param[out]	: buff_index_ptr	: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]	: len	: integer to write
* @retrun		: void
*/
static void MqttClientLenWrite(unsigned char** buff_index_ptr, int len);

/**
* @brief	Calculates an integer from two bytes read from the input buffer
* @param[in]	: buff_index_ptr	: pointer to the output buffer - incremented by the number of bytes used & returned
* @retrun		: int
* @retval		: integer value calculated
*/
static int MqttClientLenRead(unsigned char* buff_index);

/**
* @brief	Writes length delimitted UTF string into output buffer from t_mqtt_string formatted string
* @param[out]	: buff_index_ptr 	: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]	: mqttstring		: t_mqtt_string formatted string
* @return		void
*/
static void MqttClientMqttStringWrite(unsigned char** buff_index_ptr, t_mqtt_string mqttstring);

/* -------------------------------- Routines -------------------------------- */

/**
 * @brief	Set timer interval value.
 *
 * @param[in]	: req_timer_interval	: timer interval value to be set
 * @return 		void
 *
*/
static void MqttClientSetTimerInterval(unsigned char req_timer_interval)
{
	timer_interval = req_timer_interval;
}

/**
 * @brief	Set timer interval value.
 *
 * @param	void
 * @return 	void
 *
*/
static void MqttClientSetStartTimer(void)
{
	timer_start = true;
}

/**
 * @brief	Create a socket and connect to desired host on specified port.
 *
 * @param[in]	: addr	: host address
 * @param[in]	: port	: port
 * @return 		int
 * @retval 		>=0: socket descriptor since socket connection successful
 * @retval 		<0: failure
 *
*/
static int MqttClientTransportOpen(char *addr, int port)
{
		struct sockaddr_in address;
		int ret_code = -1;
		sa_family_t family = AF_INET;

		struct addrinfo *result = NULL;
		struct addrinfo hints = {0, AF_UNSPEC, SOCK_STREAM, IPPROTO_TCP, 0, NULL, NULL, NULL};
		static struct timeval tv;

		socket_desc = INVALID_SOCKET;

		if ((ret_code = getaddrinfo(addr, NULL, &hints, &result)) == SYS_SUCCESS)
		{
			struct addrinfo* res = result;

			/* prefer ip4 addresses */
			while (res)
			{
				if (res->ai_family == AF_INET)
				{
					result = res;
					break;
				}
				else
				{
					/* Check for next structure till null */
					res = res->ai_next;
				}
			}

			if (result->ai_family == AF_INET)
			{
				address.sin_port = htons(port);
				address.sin_family = family;
				address.sin_addr = ((struct sockaddr_in*)(result->ai_addr))->sin_addr;

				/* Create socket for ipv4 addr over tcp connection */
				socket_desc = socket(AF_INET, SOCK_STREAM, 0);

				if (socket_desc != INVALID_SOCKET)
				{
					ret_code = connect(socket_desc, (struct sockaddr*)&address, sizeof(address));

					if(ret_code != INVALID_SOCKET)
					{
						tv.tv_sec = 1;  /* 1 second Timeout */
						tv.tv_usec = 0;
						setsockopt(socket_desc, SOL_SOCKET, SO_RCVTIMEO, (char *)&tv,sizeof(struct timeval));
						printf("MqttClient: TCP connection over socket: %d successful", socket_desc);
					}
					else
					{
						socket_desc = INVALID_SOCKET;
						printf("MqttClient: Error in connecting host with ret code: %d on socket %d",ret_code, socket_desc);
					}
				}
				else
				{
					socket_desc = INVALID_SOCKET;
					printf("MqttClient: Error in creating socket returned socket: %d", socket_desc);
				}
			}
			else
			{
				socket_desc = INVALID_SOCKET;
				printf("MqttClient: Error in getting IPV4 address info of host");
			}

			freeaddrinfo(result);
		}
		else
		{
			socket_desc = INVALID_SOCKET;
			printf("MqttClient: Error in getting address info of host");
		}

		return socket_desc;
}

/**
* @brief	send a packet over socket
*
* @param[in]	: socket	: socket descriptor
* @param[in]	: buf		: packet buffer to be sent over network
* @param[in]	: buflen	: length of packet buffer to be sent
* @return 		int
* @reval		0 : Success
* @retval		!0: failure
*
*/
static int MqttClientTransportSendPacketBuffer(int sock, unsigned char* buf, int buflen)
{
	int rc = 0;
	rc = write(sock, buf, buflen);

	if(SUCCESS == rc)
	{
		printf("MqttClient: Packet sent");
	}
	else
	{
		printf("MqttClient: Error in sending packet over socket:%d", sock);
	}
	return rc;
}

/**
* @brief	receive a packet over socket
*
* @param[in]	: buf		: packet buffer to be sent over network
* @param[in]	: count		: bytes to read
* @return 		int
* @reval		0 : Success
* @retval		!0: failure
*
*/
static int MqttClientTransportReceivePacketBuffer(unsigned char* buf, int byte_count)
{
	int bytes_received = 0;

	bytes_received = recv(socket_desc, buf, byte_count, 0);
	printf("MqttClient: received %d bytes count %d\n", bytes_received, byte_count);
	return bytes_received;
}

/**
 * @brief	sets connect packet options structure with defined options in config file.
 *
 * @param[in]	: options		:  pointer to connect packet options structure to set
 * @return 		void
 *
*/
static void MqttClientSetConnectPacketOptions(t_mqtt_connect_packet_options *options)
{
	/* Set connect packet options from config file */
	options->mqtt_version			= MQTT_V_3_1_1;
	options->client_id.cstring 		= CLIENT_ID;
	options->keep_alive_interval 	= KEEP_ALIVE_INTERVAL;
	options->clean_session 			= CLEAN_SESSION_FLAG;
	options->username.cstring		= USERNAME;
	options->password.cstring		= PASSWORD;
}

/**
 * @brief		sets publish packet options structure with defined options in config file.
 *
 * @param[in]	: options		:  pointer to connect packet options structure to set
 * @return 		void
 *
*/
static void MqttClientSetPublishPacketOptions(t_mqtt_publish_packet_options *options)
{
	/* Set connect packet options from config file */
	options->header_options.bits.dup	= ZERO;
	options->header_options.bits.qos	= QOS_1;
	options->header_options.bits.retain	= RETAIN;
	options->header_options.bits.type	= PUBLISH;
	options->packet_id					= 1;	/* TODO revisit */
	options->topic_name.cstring			= TOPIC;
	options->payload					= ServiceRequestList[service_idx].json;
	options->payload_len				= ServiceRequestList[service_idx].json_size;
}

/**
* @brief	Serializes ping packet into supplied buffer.
*
* @param[out]	: buf	: the buffer into which the ping packet will be serialized
* @return		int
* @retval 		serialized length
* @retval 		0 as error
 */
static int MqttClientCreatePingPacket(unsigned char* buf)
{
	/* Ping request packet is Byte 1: 0xC0 Byte 2: 0x00 */
	buf[0] = PING_HEADER_BYTE;
	buf[1] = PING_LENGTH_BYTE;

	return MAX_PING_PACK_SIZE;
}

/**
* @brief	Serializes disconnect packet into supplied buffer.
*
* @param[out]	: buf	: the buffer into which the disconnect packet will be serialized
* @return		int
* @retval 		serialized length
* @retval 		0 as error
 */
static int MqttClientCreateDisconnectPacket(unsigned char* buf)
{
	/* Disconnect request packet is Byte 1: 0xE0 Byte 2: 0x00 */
	buf[0] = DISCONN_HEADER_BYTE;
	buf[1] = DISCONN_LENGTH_BYTE;

	return MAX_DISCONN_PACK_SIZE;
}

/**
* @brief	Serializes the connect options into the buffer.
*
* @param[out]	: buf			: the buffer into which the packet will be serialized
* @param[in]	: len			: the length in bytes of the supplied buffer
* @param[in]	: def_options	: the options to be used to build the connect packet
* @return		int
* @retval 		serialized length
* @retval 		0 as error
 */
static int MqttClientCreateConnectPacket(unsigned char* buf, int buflen, t_mqtt_connect_packet_options* def_options)
{
	unsigned char *buff_index = buf;
	t_mqtt_connect_flags flags = {0};
	bool buffer_available = false;
	int len = 0;
	int serialized_len = BUFFER_TOO_SHORT;

	len = MqttClientCalConnectPacketLength(def_options);
	buffer_available = MqttClientCheckBufferSize(len, buflen);

	if (true == buffer_available)
	{
		/* write header CONNECT_HEADER_BYTE */
		MqttClientByteWrite(&buff_index, CONNECT_HEADER_BYTE);

		/* encode and write remaining length byte */
		buff_index += MqttClientEncodePacketLen(buff_index, len);

		if (def_options->mqtt_version == 4)
		{
			/* write protocol string in length delimited form */
			MqttClientStringWrite(&buff_index, "MQTT");
			/* write protocol level byte */
			MqttClientByteWrite(&buff_index, (char) 4);
		}
		else
		{
			/* write protocol string in length delimited form */
			MqttClientStringWrite(&buff_index, "MQIsdp");
			/* write protocol level byte */
			MqttClientByteWrite(&buff_index, (char) 3);
		}

		/* Generate flags byte based on connect options */
		flags.all = 0;
		flags.bits.clean_session = def_options->clean_session;
		flags.bits.will = (def_options->will_flag) ? 1 : 0;
		if (flags.bits.will)
		{
			flags.bits.willQoS = def_options->will.qos;
			flags.bits.willRetain = def_options->will.retained;
		}

		if (def_options->username.cstring || def_options->username.lenstring.data)
			flags.bits.username = 1;
		if (def_options->password.cstring || def_options->password.lenstring.data)
			flags.bits.password = 1;

		MqttClientByteWrite(&buff_index, flags.all);
		MqttClientLenWrite(&buff_index, def_options->keep_alive_interval);
		MqttClientMqttStringWrite(&buff_index, def_options->client_id);
		if (def_options->will_flag)
		{
			MqttClientMqttStringWrite(&buff_index, def_options->will.topicName);
			MqttClientMqttStringWrite(&buff_index, def_options->will.message);
		}
		if (flags.bits.username)
			MqttClientMqttStringWrite(&buff_index, def_options->username);
		if (flags.bits.password)
			MqttClientMqttStringWrite(&buff_index, def_options->password);

		serialized_len = buff_index - buf;
	}
	else
	{
		serialized_len = BUFFER_TOO_SHORT;
		printf("MqttClient: Error in creating connect packet buffer length: %d not sufficient", buflen);
	}

	printf("MqttClient: connect packet created in buffer: %s\twith length:%d",buf,serialized_len);
	return serialized_len;
}

/**
* @brief	Serializes the publish options into the buffer.
*
* @param[out]	: buf			: the buffer into which the packet will be serialized
* @param[in]	: buflen		: the length in bytes of the supplied buffer
* @param[in]	: def_options	: the options to be used to build the publish packet
* @return		int
* @retval 		serialized length
* @retval 		0 as error
 */
static int MqttClientCreatePublishPacket(unsigned char* buf, int buflen, t_mqtt_publish_packet_options* def_options)
{
	unsigned char *buff_index = buf;
	bool buffer_available = false;
	int len = 0;
	int serialized_len = BUFFER_TOO_SHORT;

	len = MqttClientCalPublishPacketLength(def_options);
	buffer_available = MqttClientCheckBufferSize(len, buflen);

	if (true == buffer_available)
	{
		/* write header */
		MqttClientByteWrite(&buff_index, def_options->header_options.byte);

		/* encode and write remaining length */
		buff_index += MqttClientEncodePacketLen(buff_index, len);

		/* Write length delimited mqtt string */
		MqttClientMqttStringWrite(&buff_index, def_options->topic_name);

		/* Write packet identifier */
		if (def_options->header_options.bits.qos > ZERO)
			MqttClientLenWrite(&buff_index, def_options->packet_id);

		/* Write payload */
		memcpy(buff_index, def_options->payload, def_options->payload_len);
		buff_index += def_options->payload_len;

		serialized_len = buff_index - buf;
	}
	else
	{
		serialized_len = BUFFER_TOO_SHORT;
		printf("MqttClient: Error in creating publish packet buffer length: %d not sufficient", buflen);
	}

	printf("MqttClient: publish packet created in buffer: %s\twith length:%d",buf,serialized_len);
	return serialized_len;
}


/**
* brief		Determines the length of the MQTT connect packet that would be produced using the supplied connect options.
*
* @param[in]	: options	: the options to be used to build the connect packet
* @return		int
* @retval		length of buffer needed to contain the serialized version of the packet
*/
static int MqttClientCalConnectPacketLength(t_mqtt_connect_packet_options* conn_options)
{
	int len = 0;

	if (conn_options->mqtt_version == 3)
		len = 12; /* variable depending on MQTT or MQIsdp */
	else if (conn_options->mqtt_version == 4)
		len = 10;

	len += MqttClientStrLen(conn_options->client_id)+2;
	if (conn_options->will_flag)
		len += MqttClientStrLen(conn_options->will.topicName)+2 + MqttClientStrLen(conn_options->will.message)+2;
	if (conn_options->username.cstring || conn_options->username.lenstring.data)
		len += MqttClientStrLen(conn_options->username)+2;
	if (conn_options->password.cstring || conn_options->password.lenstring.data)
		len += MqttClientStrLen(conn_options->password)+2;

	return len;
}

/**
* brief		Determines the length of the MQTT publish packet that would be produced using the supplied options.
*
* @param[in]	: def_options	: options to be used to build the publish packet
* @return		int
* @retval		length of buffer needed to contain the serialized version of the packet
*/
static int MqttClientCalPublishPacketLength(t_mqtt_publish_packet_options* def_options)
{
	int len = 0;

	len += 2 + MqttClientStrLen(def_options->topic_name) + def_options->payload_len;

	if (def_options->header_options.bits.qos > 0)
	{
		len += 2; /* packetid */
	}
	else
	{
		/* No packet id requirred */
	}

	return len;
}

/**
* @brief	Return the length of the MQTTstring - C string if there is one, otherwise the length delimited string
*
* @param[in]	: mqttstring	: string to return the length of
* @return		int
* @retval		length of the string
*/
static int MqttClientStrLen(t_mqtt_string mqttstring)
{
	int len = 0;

	if (mqttstring.cstring)
		len = strlen(mqttstring.cstring);
	else
		len = mqttstring.lenstring.len;
	return len;
}

/**
* @brief	Check whether available length is enough to store serialized packet
*
* @param[in]	: req_len		: length required to store serialized buffer except fixed header
* @param[in]	: available_len	: length available to store serialized packet
* @return		bool
* @retval		true			: available length enough to store serialized packet
* @retval		false			: available length not enough to store serialized packet
*/
static bool MqttClientCheckBufferSize(int req_len, int available_len)
{
	bool buffer_available = false;

	req_len += 1; /* fixed header byte 1 */

	/* now remaining_length field */
	if (req_len < 128)
		req_len += 1;
	else if (req_len < 16384)
		req_len += 2;
	else if (req_len < 2097151)
		req_len += 3;
	else
		req_len += 4;

	if(available_len >= req_len)
	{
		buffer_available = true;
	}
	else
	{
		buffer_available = false;
		printf("MqttClient:: Error in creating serialized buffer buffer length requirred:%d\t available length:%d", req_len, available_len);
	}

	return buffer_available;
}

/**
* @brief	Reads one character from an input buffer.
* @param[in]	: buff_index_ptr	: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]	: byte				: byte read
* @return		void
*/
static void MqttClientByteRead(unsigned char** buff_index_ptr, unsigned char* byte)
{
	/* Read specified byte in buffer */
	*byte = **buff_index_ptr;
	/* increment buff_index to next byte */
	(*buff_index_ptr)++;
}

/**
* @brief	Writes one character to an output buffer.
* @param[in-out]	: buff_index_ptr	: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]		: byte		: character to the write
* @return			void
*/
static void MqttClientByteWrite(unsigned char** buff_index_ptr, char byte)
{
	/* Write specified byte in buffer */
	**buff_index_ptr = byte;
	/* increment buff_index to next byte */
	(*buff_index_ptr)++;
}

/**
* @brief	Writes a "UTF" string to an output buffer.  Converts C string to length-delimited.
* @param[out]	: buff_index_ptr		: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]	: string	: C string to write
*/
static void MqttClientStringWrite(unsigned char** buff_index_ptr, const char* string)
{
	int len = 0;

	len = strlen(string);
	MqttClientLenWrite(buff_index_ptr, len);
	memcpy(*buff_index_ptr, string, len);
	*buff_index_ptr += len;
}

/**
* @brief	Writes an integer as 2 bytes to an output buffer.
* @param[out]	: buff_index_ptr	: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]	: len	: integer to write
* @retrun		: void
*/
static void MqttClientLenWrite(unsigned char** buff_index_ptr, int len)
{
	**buff_index_ptr = (unsigned char)(len / 256);
	(*buff_index_ptr)++;
	**buff_index_ptr = (unsigned char)(len % 256);
	(*buff_index_ptr)++;
}

/**
* @brief	Calculates an integer from two bytes read from the input buffer
* @param[in]	: buff_index	: pointer to the output buffer - incremented by the number of bytes used
* @retrun		: int
* @retval		: integer value calculated
*/
static int MqttClientLenRead(unsigned char* buff_index)
{
	int value = 0;

	value = (*buff_index) * 256 + *(buff_index+1) ;

	return value;
}

/**
* @brief	Encodes the message length according to the MQTT algorithm
* @param[out]	: buf		: buffer into which the encoded data is written
* @param[in]	: length	: length to be encoded
* @return		int
* @retval		number of bytes written to buffer
*/
static int MqttClientEncodePacketLen(unsigned char* buf, int length)
{
	int written_bytes = 0;

	do
	{
		char enc_len = length % 128;
		length /= 128;
		/* if there are more digits to encode, set the top bit of this digit */
		if (length > 0)
			enc_len |= 0x80;
		buf[written_bytes++] = enc_len;
	} while (length > 0);

	return written_bytes;
}

/**
* @brief	Decodes the message length according to the MQTT algorithm
* @param[in]	: buff	: buffer containing encoded length
* @param[out]	: value	: decoded length
* @return void
*/
static void MqttClientDecodePacketLen(unsigned char *buff, int* value)
{
	unsigned char c;
	int multiplier = 1;
	unsigned char *buff_index = buff;

	*value = 0;
	do
	{
		MqttClientByteRead(&buff_index, &c);

		*value += (c & 127) * multiplier;
		multiplier *= 128;
	} while ((c & 128) != 0);

	printf("MqttClient: Packet length decoded: %d",*value);
}

/**
* @brief	Writes length delimitted UTF string into output buffer from t_mqtt_string formatted string
* @param[out]	: buff_index_ptr	: pointer to the output buffer - incremented by the number of bytes used & returned
* @param[in]	: mqttstring		: t_mqtt_string formatted string
* @return		void
*/
static void MqttClientMqttStringWrite(unsigned char** buff_index_ptr, t_mqtt_string mqttstring)
{
	if (mqttstring.lenstring.len > 0)
	{
		MqttClientLenWrite(buff_index_ptr, mqttstring.lenstring.len);
		memcpy(*buff_index_ptr, mqttstring.lenstring.data, mqttstring.lenstring.len);
		*buff_index_ptr += mqttstring.lenstring.len;
	}
	else if (mqttstring.cstring)
		MqttClientStringWrite(buff_index_ptr, mqttstring.cstring);
	else
		MqttClientLenWrite(buff_index_ptr, 0);
}

/**
 * @brief	Initializes modem_connected, profile_started and ip_obtained flags on power on reset.
 *
 * @param 		void
 * @return 		void
 *
*/
void MqttClientResetFlags(void)
{
	modem_connected = false;
	profile_started = false;
	ip_obtained = false;

	printf("MqttClient: Flags reseted modem_connected, profile_started, ip_obtained");
}

/**
 * @brief	Register session result handler, session state handler with ModemDataMng and start default profile.
 *
 * @param 		void
 * @return 		void
 *
*/
void MqttClientModemInit(void)
{
    printf("MqttClient: MqttClientModemInit()");

    /* currently hard coded since in linux no need to explicitly activate modem */
    modem_connected = true;
}

/**
 * @brief	Initializes modem_connected, profile_started and ip_obtained flags on power on reset.
 *
 * @param[in]	: timer_req		:  timer req type to set timer interval
 * @return 		void
 *
*/
void MqttClientClearStartTimer(t_timer_req timer_req)
{
	switch(timer_req)
	{
		case MODEM_REQ:
			MqttClientSetTimerInterval(MODEM_REQ_TIME_INTERVAL);
			MqttClientSetStartTimer();
			printf("MqttClient: Timer started for MODEM_REQ");
			break;

		case KEEP_ALIVE:
			MqttClientSetTimerInterval(KEEP_ALIVE_TIME_INTERVAL);
			MqttClientSetStartTimer();
			printf("MqttClient: Timer started for KEEP_ALIVE");
			break;

		case PING_REQ:
			MqttClientSetTimerInterval(PING_REQ_TIME_INTERVAL);
			MqttClientSetStartTimer();
			printf("MqttClient: Timer started for PING_REQ");
			break;

		default:
			/*Added for defensive programming: Called from MqttClientH2Mng FSM only with above mentioned parameters */
			/* Do nothing */
			break;
	}

}

/**
 * @brief	check modem connection status by returning modem_connected flag status.
 *
 * @param	: void
 * @return 	bool
 * @retval 	false: modem not connected
 * @retval 	true: modem connected
 *
*/
bool MqttClientCheckModemConnection(void)
{
	return modem_connected;
}


/**
 * @brief	check timer elapsed flag status.
 *
 * @param	: void
 * @return 	bool
 * @retval 	false: timer running
 * @retval 	true: timer elapsed
 *
*/
bool MqttClientCheckTimeToRetry(void)
{
	return timer_elapsed;
}

/**
* @brief	create and send a Mqtt connect control packet over socket
*
* @param	: void
* @return 	void
*
*/
void MqttClientSendConnectRequest(void)
{
	int mysock = 0;
	bool packet_sent = false;
	unsigned char connect_packet_buffer[MAX_CONN_PACK_SIZE] = {0};
	t_mqtt_connect_packet_options mqtt_connect_packet_options = CONNECT_OPTIONS_INIT;
	int len = 0;

	mysock = MqttClientTransportOpen(SERVER_ADDRESS, SERVER_PORT);


	if(mysock >= ZERO)
	{
		MqttClientSetConnectPacketOptions(&mqtt_connect_packet_options);
		/* Create connect packet here ready to be sent since socket connection is successful */
		len = MqttClientCreateConnectPacket(&connect_packet_buffer[0], MAX_CONN_PACK_SIZE, &mqtt_connect_packet_options);

		/*rc = since packet is read then retry is attempted */
		packet_sent = MqttClientTransportSendPacketBuffer(mysock, connect_packet_buffer, len);

		if(true == packet_sent)
		{
			/* Wait for CONNACK to receive */
		}
		else
		{
			/* error in sending packet over socket */
			client_connected = false;
			printf("MqttClient: Error in sending connect packet");
		}

	}
	else
	{
		/* error in socket creation and connection */
		client_connected = false;
		printf("MqttClient: Error in sending connect packet socket id:%d", mysock);
	}

}

/**
* @brief	check for CONNACK packet received
*
* @param	: void
* @return	bool
* @retval	true	: CONNACK packet received client successfully connected
* 			false	: CONNACK packet not received client not connected
*/
bool MqttClientCheckMqttConnection(void)
{
	bool client_connected = false;
	unsigned char resp_buf[MAX_CONNACK_PACK_SIZE] = {0};
	t_mqtt_header_byte header = {0};
	int len = 0;
	int rem_len = 0;

	len = MqttClientTransportReceivePacketBuffer(resp_buf, ONE_BYTE);
	header.byte = resp_buf[0];


	if((len == ONE_BYTE) && (CONNACK == header.bits.type))
	{
		len += MqttClientTransportReceivePacketBuffer(&resp_buf[1], ONE_BYTE);
		MqttClientDecodePacketLen(&resp_buf[1], &rem_len);

		if( 2 == rem_len )
		{
			len += MqttClientTransportReceivePacketBuffer(&resp_buf[2], TWO_BYTES);
			if(FOUR_BYTES == len && CONNECTION_ACCEPTED == resp_buf[3])
			{
				client_connected = true;
			}
			else
			{
				printf("MqttClient: Mqtt connection refused by host with return: %d", resp_buf[3]);
			}
		}
		else
		{
			client_connected = false;
			printf("MqttClient: Incorrect decoded len received: %d", rem_len);
		}
	}
	else
	{
		client_connected = false;
		printf("MqttClient: Incorrect data received with len: %d and packet type: %d",len ,header.bits.type);
	}

	return client_connected;
}

/**
* @brief	check for data available to be sent
*
* @param	: void
* @return	bool
* @retval	true	: data to be sent
* 			false	: no data to send
*/
bool MqttClientCheckDataToSend(void)
{
	bool data_present = false;

	/* Check if there is an active request to be send*/
	for (service_idx = ((unsigned char)0); service_idx < ((unsigned char)SERVICE_LAST); service_idx++)
	{
		/* An inactive request will have it's retry counter set to 0*/
		if ( ServiceRequestList[service_idx].retry_count > 0U )
		{
			/*found one*/
			printf("MqttClient: Found an active request by service %d with %d retry counter", service_idx, ServiceRequestList[service_idx].retry_count);
			data_present = true;
			break;
		}
		else
		{
			/* no data */
			data_present = false;
		}
	}

	return data_present;
}

/**
* @brief	check whether timer for ping request is elapsed
*
* @param	: void
* @return	bool
* @retval	true	: timer elapsed ping request should be sent
* 			false	: timer not elapsed ping request should not be sent
*/
bool MqttClientCheckTimeToPing(void)
{
	return timer_elapsed;
}

/**
* @brief	Send ping mqtt control packet over socket
*
* @param	: void
* @return	void
*/
void MqttClientSendPingRequest(void)
{
	bool packet_sent = false;
	unsigned char ping_packet_buffer[MAX_PING_PACK_SIZE] = {0};
	int len = 0;

	/* Create ping packet */
	len = MqttClientCreatePingPacket(&ping_packet_buffer[0]);

	packet_sent = MqttClientTransportSendPacketBuffer(socket_desc, ping_packet_buffer, len);

	if(true == packet_sent)
	{
		/* Do nothing since PingAck is not monitored */
		printf("MqttClient: Ping request sent");
	}
	else
	{
		/* can't help but dont worry socket is created and modem connectivity is checked */
		printf("MqttClient: Error in sending ping request ");
	}

}

/**
* @brief	Send publish mqtt control packet over socket
*
* @param	: void
* @return	void
*/
void MqttClientSendPubRequest(void)
{
	bool packet_sent = false;
	unsigned char publish_packet_buffer[MAX_PUBLISH_PACK_SIZE] = {0};
	t_mqtt_publish_packet_options mqtt_publish_packet_options = PUBLISH_OPTIONS_INIT;
	int len = 0;

	MqttClientSetPublishPacketOptions(&mqtt_publish_packet_options);

	/* Create publish packet to be sent over socket */
	len = MqttClientCreatePublishPacket(&publish_packet_buffer[0], MAX_PUBLISH_PACK_SIZE, &mqtt_publish_packet_options);

	/* will be sent since socket is created */
	packet_sent = MqttClientTransportSendPacketBuffer(socket_desc, publish_packet_buffer, len);

	if(true == packet_sent)
	{
		/* Wait for PUBACK to receive */
		pub_req_status = SUCCESS;
		printf("MqttClient: Publish request sent");
	}
	else
	{
		/* error in sending packet over socket */
		pub_req_status = FAILURE;
		printf("MqttClient: Error in publish request ");
	}

}

/**
* @brief	Decrement retry count associated with currently active service after sending publish request
*
* @param	: void
* @return	void
*/
void MqttClientDecrementRetryCount(void)
{
	if ( ServiceRequestList[service_idx].retry_count > 0U)
	{
		ServiceRequestList[service_idx].retry_count--;
	}
	else
	{
		/* shoudn't have been called here possibly unreachable code */
	}
}

/**
* @brief	Increment retry count associated with currently active service after sending publish request
*
* @param	: void
* @return	void
*/
void MqttClientIncrementRetryCount(void)
{
	if ( ServiceRequestList[service_idx].retry_count > 0U)
	{
		ServiceRequestList[service_idx].retry_count++;
	}
	else
	{
		/* shoudn't have been called here possibly unreachable code */
	}
}

/**
* @brief	Check whether to cancel current ongoing process of transmission due to cancellation request
*
* @param	: void
* @return	bool
* @retval	true	: Cancellation request received
* @retval	false	: Cancellation request not received
*/
bool MqttClientCheckRequestToCancel(void)
{
	return cancel_request;
}

/**
* @brief	Check retry count associated with current ongoing service transmission
*
* @param	: void
* @return	unsigned char
* @retval	<=3	: retry count associated with current ongoing transmission
*/
unsigned char MqttClientCheckRetryCount(void)
{
	return ServiceRequestList[service_idx].retry_count;
}

/**
* @brief	Check retry count associated with current ongoing service transmission
*
* @param	: void
* @return	unsigned char
* @retval	SUCCESS	: publish request sent successfully
* @retval	FAILURE	: publish request sending failed
*/
unsigned char MqttClientCheckPubReqStatus(void)
{
	return pub_req_status;
}

/**
* @brief	Clear retry count associated with currently active service transmission
*
* @param	: void
* @return	void
*/
void MqttClientClearRetryCount(void)
{
	unsigned char service_idx;

	for ( service_idx = 0U; service_idx < (unsigned char)SERVICE_LAST; service_idx++ )
		{
			/*Notify all demanding services that a canceling request was received*/
			if ( ServiceRequestList[service_idx].retry_count > 0U )
			{
				/*Cancel the service request*/
				ServiceRequestList[service_idx].retry_count = 0U;
				printf("MqttClient: Retry count cleared for service: %d", service_idx);
			}
			else
			{
				/* Do nothing since retry count is already zero */
			}
		}
}

/**
* @brief	Notify active service for response
*
* @param[in]	: resp	: response to be sent to active service
* @return		void
*/
void MqttClientServiceNotify(t_ServerReplyCodes resp)
{
	unsigned char service_idx;

	for ( service_idx = 0U; service_idx < (unsigned char)SERVICE_LAST; service_idx++ )
		{
		/*Notify all demanding services that a canceling request was received*/
			if ( ServiceRequestList[service_idx].retry_count > 0U )
			{
				/*Notify the service about cancellation*/
				if ( ServiceRequestList[service_idx].cbk != (RxCbk)NULL )
				{
					ServiceRequestList[service_idx].cbk(resp);
					printf("MqttClient: Notified service: %d with resp: %d", service_idx, resp);
				}
			}
		}
}

/**
* @brief	Check for PubAck control packet for previously sent publish request
*
* @param	: void
* @return	unsigned char
* @retval	SUCCESS	: pubAck received successfully
* @retval	FAILURE	: pubAck not received
*/
unsigned char MqttClientCheckPubAckRspStatus(void)
{
	unsigned char puback_resp_status = FAILURE;
	unsigned char resp_buf[MAX_PUBACK_PACK_SIZE] = {0};
	t_mqtt_header_byte header = {0};
	int len = 0;
	int rem_len = 0;
	int decoded_len = 0;

	len = MqttClientTransportReceivePacketBuffer(resp_buf, ONE_BYTE);
	header.byte = resp_buf[0];


	if((len == ONE_BYTE) && (PUBACK == header.bits.type))
	{
		len += MqttClientTransportReceivePacketBuffer(&resp_buf[1], ONE_BYTE);
		MqttClientDecodePacketLen(&resp_buf[1], &rem_len);

		if( 2 == rem_len )
		{
			len += MqttClientTransportReceivePacketBuffer(&resp_buf[2], TWO_BYTES);

			decoded_len = MqttClientLenRead(&resp_buf[2]);

			if(FOUR_BYTES == len && 0x01 == decoded_len) /* packet id has to be the one sent in publish */
			{
				puback_resp_status = SUCCESS;
				printf("MqttClient: Mqtt publish ack received successfully");
			}
			else
			{
				/* LEDBUG(decoded_len as packet id ) */
				puback_resp_status = SUCCESS;
				printf("MqttClient: Mqtt publish ack received for packet id:%d",decoded_len);
			}
		}
		else
		{
			puback_resp_status = FAILURE;
			printf("MqttClient: Incorrect decoded len received: %d", rem_len);
		}
	}
	else
	{
		puback_resp_status = FAILURE;
		printf("MqttClient: Incorrect data received with len: %d and packet type: %d",len ,header.bits.type);
	}

	return puback_resp_status;
}

/**
 * @brief	check timer elapsed flag status.
 *
 * @param	: 	void
 * @return 	bool
 * @retval 	false: timer running
 * @retval 	true: timer elapsed
 *
*/
bool MqttClientCheckRspTimerStatus(void)
{
	return timer_elapsed;
}

/**
 * @brief	Send disconnect control packet
 *
 * @param	: void
 * @return 	void
 *
*/
void MqttClientDisconnect(void)
{
	bool packet_sent = false;
	unsigned char disconnect_packet_buffer[MAX_DISCONN_PACK_SIZE] = {0};
	int len = 0;

	/* Create ping packet */
	len = MqttClientCreateDisconnectPacket(&disconnect_packet_buffer[0]);

	packet_sent = MqttClientTransportSendPacketBuffer(socket_desc, disconnect_packet_buffer, len);

	if(true == packet_sent)
	{
		/* Do nothing since no ack for disconnect */
		printf("MqttClient: Mqtt disconnect packet sent");
	}
	else
	{
		/* can't help but dont worry socket is created and modem connectivity is checked */
		printf("MqttClient: Error in sending disconnect packet");
	}

}

/**
 * @brief	Check whether request to start timer is received or not.
 *
 * @param	: 	void
 * @return 	bool
 * @retval	: true	: Request to start timer received
 * @retval	: false	: Request to start timer not received
 *
*/
bool MqttClientCheckTimerReq(void)
{
	return timer_start;
}

/**
 * @brief	Get timer interval to check against count
 *
 * @param	: 	void
 * @return 	unsigned char
 * @retval	: true	: Request to start timer received
 * @retval	: false	: Request to start timer not received
 *
*/
unsigned char MqttClientGetTimerInterval(void)
{
	return timer_interval;
}

/**
 * @brief	Set timer elapsed flag to true
 *
 * @param	: void
 * @return 	void
 *
*/
void MqttClientNotifyTimerElapsed(void)
{
	timer_elapsed = true;
}

/**
 * @brief	Clear timer request, set timer start flag to false
 *
 * @param	: void
 * @return 	void
 *
*/
void MqttClientClearTimerReq(void)
{
	timer_start = false;
}

/**
* @brief	Close TCP socket
*
* @param	: void
* @return 	void
*
*/
void MqttClientTransportClose(void)
{
	/* Sends FIN packet over TCP to indicate shutting down further sends */
	(void)shutdown(socket_desc, SHUT_WR);

	/* receive any pending data in TCP buffer */
	(void)recv(socket_desc, NULL, (size_t)0, 0);

	/* Destroy socket */
	(void)close(socket_desc);

	/* socket destroyed */
	socket_desc = INVALID_SOCKET;
	printf("MqttClient: Socket connection closed");
}
