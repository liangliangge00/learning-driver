#include "MQTTClient.h"
#include "ql_data_call.h"
#include "ql_application.h"
#include "sockets.h"
#include "ql_ssl_hal.h"

#define mqtt_exam_log(fmt, ...) \ 
	printf("[MQTT_EXAM][%s, %d] "fmt"\r\n", __FUNCTION__, __LINE__, ##__VA_ARGS__)

#define PROFILE_IDX 1
//#define SERVER_DOMAIN "220.180.239.212"
#define SERVER_DOMAIN 	"112.17.28.15"
#define SERVER_PORT		1883
#define SERVER_ADDR		"mqtt.ctwing.cn"

//#define TP_DEV_B150001

static void messageArrived(MessageData* data)
{
	mqtt_exam_log("Message arrived on topic %.*s: %.*s",
		data->topicName->lenstring.len, (char*)data->topicName->lenstring.data,
		data->message->payloadlen, (char*)data->message->payload);
}

static void MQTTEchoTask(void *argv)
{
	MQTTClient client = {0};
	Network network = {0};
	unsigned char sendbuf[500] = {0}, readbuf[500] = {0};
	int rc = 0, count = 0;
	MQTTPacket_connectData connectData = MQTTPacket_connectData_initializer;
	
	argv = 0;
	NetworkInit(&network, NULL, PROFILE_IDX);
	MQTTClientInit(&client, &network, 30000, sendbuf, sizeof(sendbuf), readbuf, sizeof(readbuf));

	char *address = SERVER_ADDR;
	if ((rc = NetworkConnect(&network, address, SERVER_PORT)) != 0)
	{
		mqtt_exam_log("Return code from network connect is %d", rc);
		MQTTClientDeinit(&client);
		goto exit;
	}
	else 
	{
		mqtt_exam_log("NetworkConnect OK");
	}

#if defined(TP_DEV_B150001)
	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "15015098B150001";
	connectData.username.cstring = "B150001";
	connectData.password.cstring = "70JgFV5MIRRU0J_fMgWP65Pz3ObPC8zDN9zpoQNrsjE";
    connectData.keepAliveInterval = 60;
    connectData.cleansession = 0;
#else
	connectData.MQTTVersion = 3;
	connectData.clientID.cstring = "15015098B150002";
	connectData.username.cstring = "B150002";
	connectData.password.cstring = "70JgFV5MIRRU0J_fMgWP65Pz3ObPC8zDN9zpoQNrsjE";
    connectData.keepAliveInterval = 60;
    connectData.cleansession = 0;
#endif

	if ((rc = MQTTConnect(&client, &connectData)) != 0)
	{
		mqtt_exam_log("Return code from MQTT connect is %d", rc);
		NetworkDisconnect(&network);
		MQTTClientDeinit(&client);
		goto exit;
	}
	else
		mqtt_exam_log("MQTT Connected");

#if defined(MQTT_TASK)
	if ((rc = MQTTStartTask(&client)) != 0)
	{
		mqtt_exam_log("Return code from start tasks is %d", rc);
		NetworkDisconnect(&network);
		MQTTClientDeinit(&client);
		goto exit;
	}
#endif

	if ((rc = MQTTSubscribe(&client, "remote_cmd", 2, messageArrived)) != 0)
	{
		mqtt_exam_log("Return code from MQTT subscribe is %d", rc);
		
		rc = MQTTDisconnect(&client);
		if(rc == SUCCESS)
			mqtt_exam_log("MQTT Disconnected by client");
		else
			mqtt_exam_log("MQTT Disconnected failed by client");
		
		NetworkDisconnect(&network);
		
		MQTTClientDeinit(&client);

		goto exit;
	}

	if ((rc = MQTTSubscribe(&client, "key_add_cmd", 2, messageArrived)) != 0)
	{
		mqtt_exam_log("Return code from MQTT subscribe is %d", rc);
		
		rc = MQTTDisconnect(&client);
		if(rc == SUCCESS)
			mqtt_exam_log("MQTT Disconnected by client");
		else
			mqtt_exam_log("MQTT Disconnected failed by client");
		
		NetworkDisconnect(&network);
		
		MQTTClientDeinit(&client);

		goto exit;
	}

	if ((rc = MQTTSubscribe(&client, "set_access_time", 2, messageArrived)) != 0)
	{
		mqtt_exam_log("Return code from MQTT subscribe is %d", rc);
		
		rc = MQTTDisconnect(&client);
		if(rc == SUCCESS)
			mqtt_exam_log("MQTT Disconnected by client");
		else
			mqtt_exam_log("MQTT Disconnected failed by client");
		
		NetworkDisconnect(&network);
		
		MQTTClientDeinit(&client);

		goto exit;
	}

	if ((rc = MQTTSubscribe(&client, "auth_delete_cmd", 2, messageArrived)) != 0)
	{
		mqtt_exam_log("Return code from MQTT subscribe is %d", rc);
		
		rc = MQTTDisconnect(&client);
		if(rc == SUCCESS)
			mqtt_exam_log("MQTT Disconnected by client");
		else
			mqtt_exam_log("MQTT Disconnected failed by client");
		
		NetworkDisconnect(&network);
		
		MQTTClientDeinit(&client);

		goto exit;
	}

	if ((rc = MQTTSubscribe(&client, "set_access_qrcode", 2, messageArrived)) != 0)
	{
		mqtt_exam_log("Return code from MQTT subscribe is %d", rc);
		
		rc = MQTTDisconnect(&client);
		if(rc == SUCCESS)
			mqtt_exam_log("MQTT Disconnected by client");
		else
			mqtt_exam_log("MQTT Disconnected failed by client");
		
		NetworkDisconnect(&network);
		
		MQTTClientDeinit(&client);

		goto exit;
	}

	if ((rc = MQTTSubscribe(&client, "set_community_code", 2, messageArrived)) != 0)
	{
		mqtt_exam_log("Return code from MQTT subscribe is %d", rc);
		
		rc = MQTTDisconnect(&client);
		if(rc == SUCCESS)
			mqtt_exam_log("MQTT Disconnected by client");
		else
			mqtt_exam_log("MQTT Disconnected failed by client");
		
		NetworkDisconnect(&network);
		
		MQTTClientDeinit(&client);

		goto exit;
	}

	while (++count <= 0xFFFFFFFF)
	{
		#if 0
		MQTTMessage message;
		char payload[256] = {0};

		message.qos = 1;
		message.retained = 0;
		message.payload = payload;
		sprintf(payload, "{\"pci\":%d,\"rsrp\":%d,\"cell_id\":%d,\"sinr\":%d,\"ecl\":%d}",
			123, 456, 789, 666, 555);
		message.payloadlen = strlen(payload);
		mqtt_exam_log("payload = %s", (char *)message.payload);

		if ((rc = MQTTPublish(&client, "signal_report", &message)) != 0)
			mqtt_exam_log("Return code from MQTT publish is %d", rc);
		
#if !defined(MQTT_TASK)
		if ((rc = MQTTYield(&client, 1000)) != 0)
			mqtt_exam_log("Return code from yield is %d", rc);
#endif
		#endif
		
		mqtt_exam_log("mqtt echo task running now");
		ql_rtos_task_sleep_ms(1000);
	}

	rc = MQTTDisconnect(&client);
	if(rc == SUCCESS)
		mqtt_exam_log("MQTT Disconnected by client");
	else
		mqtt_exam_log("MQTT Disconnected failed by client");

	NetworkDisconnect(&network);
	
	MQTTClientDeinit(&client);

exit:
	
	mqtt_exam_log("========== mqtt test end ==========");
	ql_rtos_task_delete(NULL);
}

static void StartMQTTTask(void)
{
	ql_task_t task = NULL;
	ql_rtos_task_create(&task, 8 * 1024, 100, "mqtt_test", MQTTEchoTask, NULL);
}

static void ql_nw_status_callback(int profile_idx, int nw_status)
{
	mqtt_exam_log("profile(%d) status: %d", profile_idx, nw_status);
}

static int datacall_start(void)
{
	struct ql_data_call_info info = {0};
	char ip4_addr_str[16] = {0};
	
	mqtt_exam_log("wait for network register done");

	if(ql_network_register_wait(120) != 0)
	{
		mqtt_exam_log("*** network register fail ***");
		return -1;
	}
	else
	{
		mqtt_exam_log("doing network activating ...");
		
		ql_wan_start(ql_nw_status_callback);
		ql_set_auto_connect(PROFILE_IDX, TRUE);
		if(ql_start_data_call(PROFILE_IDX, 0, "3gnet.mnc001.mcc460.gprs", NULL, NULL, 0) == 0)
		{
			ql_get_data_call_info(PROFILE_IDX, 0, &info);
			
			mqtt_exam_log("info.profile_idx: %d", info.profile_idx);
			mqtt_exam_log("info.ip_version: %d", info.ip_version);
			mqtt_exam_log("info.v4.state: %d", info.v4.state);
			mqtt_exam_log("info.v4.reconnect: %d", info.v4.reconnect);
			
			inet_ntop(AF_INET, &info.v4.addr.ip, ip4_addr_str, sizeof(ip4_addr_str));
			mqtt_exam_log("info.v4.addr.ip: %s", ip4_addr_str);
			
			inet_ntop(AF_INET, &info.v4.addr.pri_dns, ip4_addr_str, sizeof(ip4_addr_str));
			mqtt_exam_log("info.v4.addr.pri_dns: %s", ip4_addr_str);
			
			inet_ntop(AF_INET, &info.v4.addr.sec_dns, ip4_addr_str, sizeof(ip4_addr_str));
			mqtt_exam_log("info.v4.addr.sec_dns: %s", ip4_addr_str);

			return 0;
		}

		mqtt_exam_log("*** network activated fail ***");
		
		return -1;
	}
}

static void MQTTTest(void *argv)
{
	(void)argv;
	
	mqtt_exam_log("========== mqtt test satrt ==========");
	
	if(datacall_start() == 0)
		StartMQTTTask();
	else
		mqtt_exam_log("========== mqtt test end ==========");
}

application_init(MQTTTest, "mqtttest", 2, 14);

