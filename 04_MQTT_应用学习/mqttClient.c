#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "/home/wangchuan/mqtt_client/paho_mqtt/paho.mqtt.c-1.3.8/src/MQTTClient.h"		//包含MQTT客户端库头文件

/* ########################宏定义##################### */
/* 
 * 因为本地服务器是搭建在您的ubuntu上，所以此处的服务器地址
 * 要填写您ubuntu的ip地址，笔者ubuntu的ip地址为192.168.2.150，
 * 你们填写自己的ip地址即可
 */
#define BROKER_ADDRESS	"192.168.2.150"	//服务器地址

/* 
 * 客户端id、用户名、密码 *
 * 客户端id可以随意填写，只需要保证跟MQTT.fx客户端使用的不同，
 * 用户名要使用MQTT服务端注册的用户，即 mqtt1
 * 密码也是之前设置的 123456 
 */
#define CLIENTID		"test3"		//客户端id
#define USERNAME		"mqtt1"		//用户名
#define PASSWORD		"123456"	//密码

/*
 * 以下 dt_mqtt/ 便是笔者的个人主题级别
 */
#define WILL_TOPIC		"dt_mqtt/will"		//遗嘱主题
#define LED_TOPIC		"dt_mqtt/led"		//LED主题
#define TEMP_TOPIC		"dt_mqtt/temperature"	//温度主题
/* ################################################# */

static int msgarrvd(void *context, char *topicName, int topicLen,
			MQTTClient_message *message)
{
	if (!strcmp(topicName, LED_TOPIC)) {//校验消息的主题
		// sys/class/leds/user-led0/brightness
		if (!strcmp("1", message->payload)) {//如果是"1"则LED常量
			
			system("echo 1 > /sys/class/leds/user-led0/brightness");
		}
		else if (!strcmp("0", message->payload)) {//如果是"0"则LED熄灭
			
			system("echo 0 > /sys/class/leds/user-led0/brightness");
		}

		// 接收到其它数据 不做处理
	}

	/* 释放占用的内存空间 */
	MQTTClient_freeMessage(&message);
	MQTTClient_free(topicName);

	/* 退出 */
	return 1;
}

static void connlost(void *context, char *cause)
{
	printf("\nConnection lost\n");
	printf("    cause: %s\n", cause);
}

int main(int argc, char *argv[])
{
	MQTTClient client;
	MQTTClient_connectOptions conn_opts = MQTTClient_connectOptions_initializer;
	MQTTClient_willOptions will_opts = MQTTClient_willOptions_initializer;
	MQTTClient_message pubmsg = MQTTClient_message_initializer;
	int rc;

	/* 创建mqtt客户端对象 */
	if (MQTTCLIENT_SUCCESS !=
			(rc = MQTTClient_create(&client, BROKER_ADDRESS, CLIENTID,
			MQTTCLIENT_PERSISTENCE_NONE, NULL))) {
		printf("Failed to create client, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto exit;
	}

	/* 设置回调 */
	if (MQTTCLIENT_SUCCESS !=
			(rc = MQTTClient_setCallbacks(client, NULL, connlost,
			msgarrvd, NULL))) {
		printf("Failed to set callbacks, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	/* 连接MQTT服务器 */
	will_opts.topicName = WILL_TOPIC;	//遗嘱主题
	will_opts.message = "Unexpected disconnection";//遗嘱消息
	will_opts.retained = 1;	//保留消息
	will_opts.qos = 0;		//QoS0

	conn_opts.will = &will_opts;
	conn_opts.keepAliveInterval = 30;	//心跳包间隔时间
	conn_opts.cleansession = 0;			//cleanSession标志
	conn_opts.username = USERNAME;		//用户名
	conn_opts.password = PASSWORD;		//密码
	if (MQTTCLIENT_SUCCESS !=
			(rc = MQTTClient_connect(client, &conn_opts))) {
		printf("Failed to connect, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto destroy_exit;
	}

	printf("MQTT服务器连接成功!\n");

	/* 发布上线消息 */
	pubmsg.payload = "Online";	//消息的内容
	pubmsg.payloadlen = 6;		//内容的长度
	pubmsg.qos = 0;				//QoS等级
	pubmsg.retained = 1;		//保留消息
	if (MQTTCLIENT_SUCCESS !=
		(rc = MQTTClient_publishMessage(client, WILL_TOPIC, &pubmsg, NULL))) {
		printf("Failed to publish message, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto disconnect_exit;
	}

	/* 订阅主题 dt_mqtt/led */
	if (MQTTCLIENT_SUCCESS !=
			(rc = MQTTClient_subscribe(client, LED_TOPIC, 0))) {
		printf("Failed to subscribe, return code %d\n", rc);
		rc = EXIT_FAILURE;
		goto disconnect_exit;
	}

	/* 向服务端发布芯片温度信息 */
	for ( ; ; ) {

		MQTTClient_message tempmsg = MQTTClient_message_initializer;
		char temp_str[10] = {0};
		int fd;

		/* 读取温度值 */
		fd = open("/sys/class/thermal/thermal_zone0/temp", O_RDONLY);
		read(fd, temp_str, sizeof(temp_str));//读取temp属性文件即可获取温度
		close(fd);

		/* 发布温度信息 */
		tempmsg.payload = temp_str;	//消息的内容
		tempmsg.payloadlen = strlen(temp_str);		//内容的长度
		tempmsg.qos = 0;				//QoS等级
		tempmsg.retained = 1;		//保留消息
		if (MQTTCLIENT_SUCCESS !=
			(rc = MQTTClient_publishMessage(client, TEMP_TOPIC, &tempmsg, NULL))) {
			printf("Failed to publish message, return code %d\n", rc);
			rc = EXIT_FAILURE;
			goto unsubscribe_exit;
		}

		sleep(10);		//每隔30秒 更新一次数据
	}

unsubscribe_exit:
	if (MQTTCLIENT_SUCCESS !=
		(rc = MQTTClient_unsubscribe(client, LED_TOPIC))) {
		printf("Failed to unsubscribe, return code %d\n", rc);
		rc = EXIT_FAILURE;
	}
disconnect_exit:
	if (MQTTCLIENT_SUCCESS !=
		(rc = MQTTClient_disconnect(client, 10000))) {
		printf("Failed to disconnect, return code %d\n", rc);
		rc = EXIT_FAILURE;
	}
destroy_exit:
	MQTTClient_destroy(&client);
exit:
	return rc;
}

