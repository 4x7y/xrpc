## XRPC

In this project, I implemented a simple rpc generator, which allows user to define messages and rpc function in a `json` style format and generate `server-stub.h`  and `client-stub.h` to include. The objective platform is Linux and OS X and do not depend on other libraries.

The xrpc generator consists of a simple parser which is used to read rpc definition files , a `C` socket program skeleton and some data strutures for saving message and rpc function data. The following is a sample of my rpc definition file:

```json
message FiveNumbers
{
	int num1
	int num2
	int num3
	int num4
	int num5
}

message MinMaxAverage
{
	int max
	int min
	double average
}

rpc Statistics
{
	param  FiveNumbers
	reply  MinMaxAverage
}
```

`message` , `rpc`, `param` and `reply` are keywords. `param` and `reply` indicate the input and output of a rpc function.

### Compile

Use the following command

```
gcc rpc_gen.c -o rpc-gen
```

and you will have the generator program.

### Usage

Write your rpc definition file and pass the filename to the `rpc-gen` as a parameter.

```
./rpc-gen ./stat.proto
```

and it will automatically generate two stub files `client_stub.h` and `server_stub.h`.

Include these header files in your client and server program respectively will allow you to use rpc service.

```C
*
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
```

```C
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
```



