/*
    C socket server example
*/
 
#include "server_stub.h"
 
MinMaxAverage event_callback(FiveNumbers nums)
{
	MinMaxAverage reply;

	int max = nums.num1;
	int min = nums.num1;
	
	double sum = 0;
	int *pnum = &(nums.num1);
	for (int i = 0; i < 5; i++)
	{
		if (*pnum > max) max = *pnum;
		if (*pnum < min) min = *pnum;
		sum += *pnum;
		pnum++;
	}

printf("average = %f\n", sum / 5.0);
	reply.min = min;
	reply.max = max;
	reply.average = 3;

	return reply;
}

int main(int argc , char *argv[])
{
    rpc_server_handler_t server = create_rpc_server(8888);
    start_rpc_server(server, event_callback);    
     
    return 0;
}