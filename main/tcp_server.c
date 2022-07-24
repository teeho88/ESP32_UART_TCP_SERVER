#include "tcp_server.h"

static EventGroupHandle_t wifi_event_group;
const int CONNECTED_BIT = BIT0;
static const char *TAG="sta_mode_tcp_server";

static QueueHandle_t uart0_queue;
static QueueHandle_t queue_data;
uint8_t* data_to_trans;


void wifi_connect(){
    wifi_config_t cfg = {
        .sta = {
            .ssid = SSID,
            .password = PASSPHARSE,
        },
    };
    ESP_ERROR_CHECK( esp_wifi_disconnect() );
    ESP_ERROR_CHECK( esp_wifi_set_config(ESP_IF_WIFI_STA, &cfg) );
    ESP_ERROR_CHECK( esp_wifi_connect() );
}

static void event_handler(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    if(base ==  WIFI_EVENT && id == WIFI_EVENT_STA_START) 
    {
        wifi_connect();
        printf("WIFI_EVENT_STA_START\n");
    }
    else if(base ==  WIFI_EVENT && id == WIFI_EVENT_STA_DISCONNECTED)
    {
        esp_wifi_connect();
        xEventGroupClearBits(wifi_event_group, CONNECTED_BIT);
        printf("WIFI_EVENT_STA_DISCONNECTED\n");
    }
    else if(base == IP_EVENT && id == IP_EVENT_STA_GOT_IP)
    {
        xEventGroupSetBits(wifi_event_group, CONNECTED_BIT);
        printf("IP_EVENT_STA_GOT_IP\n");
    }
}

static void initialise_wifi(void)
{
    esp_log_level_set("wifi", ESP_LOG_NONE); // disable wifi driver logging
    tcpip_adapter_init();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK( esp_wifi_init(&cfg) );
    ESP_ERROR_CHECK( esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK( esp_wifi_start() );
}

void printWiFiIP(void *pvParam){
    printf("print_WiFiIP task started \n");
    xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,false,true,portMAX_DELAY);
    tcpip_adapter_ip_info_t ip_info;
	ESP_ERROR_CHECK(tcpip_adapter_get_ip_info(TCPIP_ADAPTER_IF_STA, &ip_info));
    printf("IP :  %s\n", ip4addr_ntoa(&ip_info.ip));
    vTaskDelete( NULL );
}

void tcp_server(void *pvParam){
    ESP_LOGI(TAG,"tcp_server task started\n");
    struct sockaddr_in tcpServerAddr;
    tcpServerAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    tcpServerAddr.sin_family = AF_INET;
    tcpServerAddr.sin_port = htons(3000);
    int s;//TCP socket
    // char recv_buf[64];
    static struct sockaddr_in remote_addr;
    static unsigned int socklen;
    socklen = sizeof(remote_addr);
    int cs;//client socket
    xEventGroupWaitBits(wifi_event_group,CONNECTED_BIT,false,true,portMAX_DELAY);
    while(1){
        s = socket(AF_INET, SOCK_STREAM, 0);
        if(s < 0) {
            ESP_LOGE(TAG, "... Failed to allocate socket.\n");
            vTaskDelay(1000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... allocated socket\n");
        if(bind(s, (struct sockaddr *)&tcpServerAddr, sizeof(tcpServerAddr)) != 0) {
            ESP_LOGE(TAG, "... socket bind failed errno=%d \n", errno);
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        ESP_LOGI(TAG, "... socket bind done \n");
        if(listen(s, LISTENQ) != 0) {
            ESP_LOGE(TAG, "... socket listen failed errno=%d \n", errno);
            close(s);
            vTaskDelay(4000 / portTICK_PERIOD_MS);
            continue;
        }
        while(1){
            cs=accept(s,(struct sockaddr *)&remote_addr, &socklen);
            ESP_LOGI(TAG,"New connection request, Request data:");
            //set O_NONBLOCK so that recv will return, otherwise we need to impliment message end 
            //detection logic. If know the client message format you should instead impliment logic
            //detect the end of message
            
            // fcntl(cs,F_SETFL,O_NONBLOCK);           
            // bzero(recv_buf, sizeof(recv_buf));
            // int r = recv(cs, recv_buf, sizeof(recv_buf)-1,0);
            while (1)
            {
                char rxbuff[128];
                if (xQueueReceive(queue_data, rxbuff, (TickType_t)10))
                {
                    if (write(cs, rxbuff, strlen(rxbuff)) <= 0)
                    {
                        ESP_LOGE(TAG, "... Send failed\n");
                        close(cs);
                        vTaskDelay(3000/portTICK_PERIOD_MS);
                        break;
                    }
                    ESP_LOGI(TAG, "Write done: %s", rxbuff);
                }
            }
        }
        ESP_LOGI(TAG, "... server will be opened in 5 seconds");
        vTaskDelay(5000 / portTICK_PERIOD_MS);
    }
    ESP_LOGI(TAG, "...tcp_client task closed\n");
}

static void uart_event_task(void *pvParam)
{
    uart_event_t event;
    for(;;) {
        //Waiting for UART event.
        if(xQueueReceive(uart0_queue, (void * )&event, (portTickType)portMAX_DELAY)) {
            ESP_LOGI(TAG, "uart[%d] event:", EX_UART_NUM);
            bzero(data_to_trans, RD_BUF_SIZE);
            switch(event.type) {
                    //Event of UART receving data
                    /*We'd better handler data event fast, there would be much more data events than
                    other types of events. If we take too much time on data event, the queue might
                    be full.*/
                case UART_DATA:
                    ESP_LOGI(TAG, "[UART DATA]: %d", event.size);
                    uart_read_bytes(EX_UART_NUM, data_to_trans, event.size, portMAX_DELAY);
                    xQueueSend(queue_data, (void*)data_to_trans , (TickType_t)0);
                    break;
                // Event of HW FIFO overflow detected
                case UART_FIFO_OVF:
                    ESP_LOGI(TAG, "hw fifo overflow");
                    // If fifo overflow happened, you should consider adding flow control for your application.
                    // The ISR has already reset the rx FIFO,
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                // Event of UART ring buffer full
                case UART_BUFFER_FULL:
                    ESP_LOGI(TAG, "ring buffer full");
                    // If buffer full happened, you should consider encreasing your buffer size
                    // As an example, we directly flush the rx buffer here in order to read more data.
                    uart_flush_input(EX_UART_NUM);
                    xQueueReset(uart0_queue);
                    break;
                // Others
                default:
                    ESP_LOGI(TAG, "uart event type: %d", event.type);
                    break;
            }
        }
    }
    free(data_to_trans);
    data_to_trans = NULL;
    vTaskDelete(NULL);
}

static void uart_send_cmd_task(void *pvParam)
{
    // while(1) {
    //     float pitch;
    //     float roll;
    //     float yaw;
    //     bw_imu_get_ang(&pitch, &roll, &yaw);
    //     sprintf((char*)data_to_trans, "b%.2f!%.2f!%.2fe", pitch, roll, yaw);
    //     xQueueSend(queue_data, (void*)data_to_trans , (TickType_t)0);
    // }
}

void app_main()
{	
    // Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = 115200,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_APB,
    };
    //Install UART driver, and get the queue.
    uart_param_config(EX_UART_NUM, &uart_config);
    uart_driver_install(EX_UART_NUM, BUF_SIZE * 2, BUF_SIZE * 2, 20, &uart0_queue, 0);

    wifi_event_group = xEventGroupCreate();
    // Create WIFI event loop
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    ESP_ERROR_CHECK(esp_event_handler_instance_register(ESP_EVENT_ANY_BASE, ESP_EVENT_ANY_ID, event_handler, NULL, NULL));

    //Set UART log level
    esp_log_level_set(TAG, ESP_LOG_INFO);
    //Set UART pins (using UART0 default pins ie no changes.)
    uart_set_pin(EX_UART_NUM, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE);
    data_to_trans = (uint8_t*)malloc(RD_BUF_SIZE);

    queue_data = xQueueCreate(10, 128);

    bw_imu_init(EX_UART_NUM);
    initialise_wifi();

    xTaskCreate(&printWiFiIP, "printWiFiIP", 2048, NULL, 5, NULL);
    xTaskCreate(&tcp_server, "tcp_server", 4096, NULL, 5, NULL);
    //Create a task to handler UART event from ISR
    xTaskCreate(uart_event_task, "uart_event_task", 4096, NULL, 12, NULL);
    // xTaskCreate(uart_send_cmd_task, "uart_send_cmd_task", 4096, NULL, 10, NULL);
}


