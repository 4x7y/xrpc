/*
    C ECHO client example using sockets
*/
#include "client_stub.h"
 
int main(int argc , char *argv[])
{
	rpc_client_handler_t client 
		= create_rpc_client("127.0.0.1", 8888);
	if (!client) return FAILURE;

    FiveNumbers message;
    message.num1 = 5;
    message.num2 = 3;
    message.num3 = 1;
    message.num4 = 4;
    message.num5 = 2;
    MinMaxAverage response;
    Statistics(client, message, &response);
    printf("Min: %d\tMax: %d\tAverage: %f\n", response.min, response.max, response.average);

    close_rpc_client(client);
    return SUCCESS;
}