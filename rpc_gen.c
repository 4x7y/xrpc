#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define BUFSIZE 1024
#define SUCCESS 0
#define FAILURE 1
#define TRUE    1
#define FALSE   0

// #define hash #
// #define f(x) x
// #define label(a) f(hash)a

/* assumes no word exceeds length of 1023 */
static char word[BUFSIZE];

int generate_client_stub(FILE *fp);
int generate_server_stub(FILE *fp);


int next_word(FILE* fp)
{
	return fscanf(fp, " %1023s", word);
}

int next_word_check(FILE *fp, char* str)
{
	int ret = fscanf(fp, " %1023s", word);
	if (ret != 1) return ret;
	if (strcmp(word, str) == 0) return TRUE;
	return FALSE;
}

int word_check(char* left, char *right)
{
	if (strcmp(left, right) == 0) return TRUE;
	return FALSE;
}

typedef struct MsgElement
{
	enum {INT, DOUBLE} type;
	char name[BUFSIZE];

	struct MsgElement* next;
} msg_element_t, *msg_element_handler_t;

typedef struct Message
{
	char name[BUFSIZE];
	msg_element_handler_t var;

	struct Message *next;
} msg_t, *msg_handler_t;

typedef struct RPC
{
	char name[BUFSIZE];
	msg_handler_t reply_type;
	msg_handler_t param_type;

	struct RPC* next;
} rpc_t, *rpc_handler_t;

static rpc_handler_t rpcs_head = NULL;
static rpc_handler_t rpcs_tail = NULL;
static msg_handler_t msgs_head = NULL;
static msg_handler_t msgs_tail = NULL;

int message_parser(FILE* fp)
{
	msg_handler_t msg = (msg_handler_t)malloc(sizeof(msg_t));
	msg_element_handler_t* pvar = &msg->var;
	msg->var  = NULL;
	msg->next = NULL;

	if (!next_word(fp)) goto MSG_PARSER_FAIL;
	strcpy(msg->name, word);
	if (!next_word_check(fp, "{")) goto MSG_PARSER_FAIL;

	int var_count = 0;
	// parse variables in message structure
	while (next_word(fp))
	{
		if (word_check(word, "}"))
		{
			// message structure finish
			if (var_count != 0) break;
			printf("Empty message structure.\n");
			goto MSG_PARSER_FAIL;
		}
		
		*pvar = (msg_element_handler_t)malloc(sizeof(msg_element_t));
		if (word_check(word, "int"))
		{
			(*pvar)->type = INT;
			
		}
		else if (word_check(word, "double"))
		{
			(*pvar)->type = DOUBLE;
		}
		else
		{
			// no match message type
			printf("Invalid type name.\n");
			goto MSG_PARSER_FAIL;
		}

		if (!next_word(fp)) goto MSG_PARSER_FAIL;
		strcpy((*pvar)->name, word);
		pvar = &(*pvar)->next;
		*pvar = NULL;
		var_count++;
	}

	// add msg to the end of msgs list
	if (msgs_head == NULL)
	{
		// no message in the msgs list
		msgs_head = msg;
		msgs_tail = msg;
	}
	else
	{
		// message is not empty
		// add new message to the end of list
		msgs_tail->next = msg;
		msgs_tail = msg;
	}
	return SUCCESS;

MSG_PARSER_FAIL:
	free(msg);
	return FAILURE;
}

int rpc_add_param(rpc_handler_t rpc, char *msg_name)
{
	// check if msg_name is a valid rpc message
	// iterate nodes in message list
	msg_handler_t ptr = msgs_head;
	while (ptr != NULL)
	{
		if (word_check(ptr->name, msg_name))
		{
			// find desired rpc message
			rpc->param_type = ptr;
			return SUCCESS;
		}

		ptr = ptr->next;
	}

	return FAILURE;
}

int rpc_add_reply(rpc_handler_t rpc, char *msg_name)
{
	// check if msg_name is a valid rpc message
	// iterate nodes in message list
	msg_handler_t ptr = msgs_head;
	while (ptr != NULL)
	{
		if (word_check(ptr->name, msg_name))
		{
			// find desired rpc message
			rpc->reply_type = ptr;
			return SUCCESS;
		}

		ptr = ptr->next;
	}

	return FAILURE;
}

int rpc_parser(FILE* fp)
{
	rpc_handler_t rpc = (rpc_handler_t)malloc(sizeof(rpc_t));
	rpc->next = NULL;
	if (!next_word(fp)) goto RPC_PARSER_FAIL;
	strcpy(rpc->name, word);

	if (!next_word_check(fp, "{"))                goto RPC_PARSER_FAIL;
	if (!next_word_check(fp, "param"))            goto RPC_PARSER_FAIL;
	if (!next_word(fp))                           goto RPC_PARSER_FAIL;
	if ( rpc_add_param(rpc, word) == FAILURE)     goto RPC_PARSER_FAIL;
	if (!next_word_check(fp, "reply"))            goto RPC_PARSER_FAIL;
	if (!next_word(fp))                           goto RPC_PARSER_FAIL;
	if ( rpc_add_reply(rpc, word) == FAILURE)     goto RPC_PARSER_FAIL;
	if (!next_word_check(fp, "}"))                goto RPC_PARSER_FAIL;

	// add msg to the end of msgs list
	if (rpcs_head == NULL)
	{
		// no message in the msgs list
		rpcs_head = rpc;
		rpcs_tail = rpc;
	}
	else
	{
		// message is not empty
		// add new message to the end of list
		rpcs_tail->next = rpc;
		rpcs_tail = rpc;
	}
	return SUCCESS;

RPC_PARSER_FAIL:
	free(rpc);
	return FAILURE;
}

int main(int argc, char* argv[])
{
	if (argc != 2)
	{
		printf("Pass RPC definition file location as a parameter.");
		return FAILURE;
	}

	FILE *fp = fopen(argv[1], "r");
	if (fp == NULL)
	{
		printf("RPC definition file not exists.");
		return FAILURE;
	}

	while (next_word(fp) == TRUE) 
	{
		if (word_check(word, "rpc"))
		{
			if (rpc_parser(fp) == FAILURE) goto MAIN_FAIL;
		}
		else if (word_check(word, "message"))
		{
			if (message_parser(fp) == FAILURE) goto MAIN_FAIL;
		}
	}

	// Finish parsing proto file successfully
	fclose(fp);

	fp = fopen("client_stub.h", "w");
	generate_client_stub(fp);
	fclose(fp);

	fp = fopen("server_stub.h", "w");
	generate_server_stub(fp);
	fclose(fp);

	return SUCCESS;

MAIN_FAIL:
	return FAILURE;
}

int generate_client_stub(FILE *fp)
{
	fprintf(fp, "\
#ifndef __XRPC_CLIENT_STUB__\n\
#define __XRPC_CLIENT_STUB__\n\
\n\
#include <stdio.h>\n\
#include <string.h>\n\
#include <sys/socket.h>\n\
#include <arpa/inet.h>\n\
#include <stdlib.h>\n\
\n\
#define SUCCESS 0\n\
#define FAILURE 1\n\
\n\
");

	msg_element_handler_t pvar;
	fprintf(fp, "typedef struct %s \n{\n", rpcs_head->reply_type->name);
	pvar = rpcs_head->reply_type->var;
	while(pvar)
	{
		fprintf(fp, "\t%s\t%s;\n", pvar->type == INT ? "int" : "double", pvar->name);
		pvar = pvar->next;
	}
	fprintf(fp, "} __attribute__((packed)) %s;\n\n", rpcs_head->reply_type->name);

	fprintf(fp, "typedef struct %s \n{\n", rpcs_head->param_type->name);
	pvar = rpcs_head->param_type->var;
	while(pvar)
	{
		fprintf(fp, "\t%s\t%s;\n", pvar->type == INT ? "int" : "double", pvar->name);
		pvar = pvar->next;
	}
	fprintf(fp, "} __attribute__((packed)) %s;\n\n", rpcs_head->param_type->name);

	fprintf(fp, "\
typedef struct RpcClient\n\
{\n\
	int sock;\n\
} rpc_client_t, *rpc_client_handler_t;\n\
\n\
rpc_client_handler_t\n\
create_rpc_client(char *ip_addr, int port)\n\
{\n\
	rpc_client_handler_t client;\n\
	client = (rpc_client_handler_t) malloc (sizeof(rpc_client_t));\n\
    struct sockaddr_in server;\n\
\n\
    //Create socket\n\
    client->sock = socket(AF_INET , SOCK_STREAM , 0);\n\
    if (client->sock == -1)\n\
    {\n\
        perror(\"Could not create socket\");\n\
        return NULL;\n\
    }\n\
    printf(\"Socket created\");\n\
\n\
    server.sin_addr.s_addr = inet_addr(ip_addr);\n\
    server.sin_family = AF_INET;\n\
    server.sin_port = htons( port );\n\
\n\
    //Connect to remote server\n\
    if (connect(client->sock , (struct sockaddr *)&server , sizeof(server)) < 0)\n\
    {\n\
        perror(\"connect failed. Error\");\n\
        return NULL;\n\
    }\n\
\n\
    printf(\"Connected\\n\");\n\
\n\
    return client;\n\
}\n\
\n\
void\n\
close_rpc_client(rpc_client_handler_t client)\n\
{\n\
	shutdown(client->sock, 0);\n\
	free(client);\n\
}\n\
\n\
int\n\
Statistics(rpc_client_handler_t client, %s message, %s* reply)\n\
{\n\
	 //Send some data\n\
    if( send(client->sock , &message , sizeof(message) , 0) < 0)\n\
    {\n\
        perror(\"Send failed\");\n\
        return FAILURE;\n\
    }\n\
\n\
    //Receive a reply from the server\n\
    if( recv(client->sock , reply , sizeof(*reply) , 0) < 0)\n\
    {\n\
        puts(\"recv failed\");\n\
        return FAILURE;\n\
    }\n\
\n\
    return SUCCESS;\n\
}\n\
\n\
#endif /* __XRPC_CLIENT_STUB__ */\n", rpcs_head->param_type->name, rpcs_head->reply_type->name);

	return SUCCESS;
}

int generate_server_stub(FILE *fp)
{
	fprintf(fp, "\
#ifndef __XRPC_SERVER_STUB__\n\
#define __XRPC_SERVER_STUB__\n\
\n\
#include <stdio.h>\n\
#include <string.h>\n\
#include <sys/socket.h>\n\
#include <arpa/inet.h>\n\
#include <unistd.h>\n\
#include <stdlib.h>\n\
\n\
#define SUCCESS 0\n\
#define FAILURE 1\n\
\n");

	msg_element_handler_t pvar;
	fprintf(fp, "typedef struct %s \n{\n", rpcs_head->reply_type->name);
	pvar = rpcs_head->reply_type->var;
	while(pvar)
	{
		fprintf(fp, "\t%s\t%s;\n", pvar->type == INT ? "int" : "double", pvar->name);
		pvar = pvar->next;
	}
	fprintf(fp, "} __attribute__((packed)) %s;\n\n", rpcs_head->reply_type->name);

	fprintf(fp, "typedef struct %s \n{\n", rpcs_head->param_type->name);
	pvar = rpcs_head->param_type->var;
	while(pvar)
	{
		fprintf(fp, "\t%s\t%s;\n", pvar->type == INT ? "int" : "double", pvar->name);
		pvar = pvar->next;
	}
	fprintf(fp, "} __attribute__((packed)) %s;\n\n", rpcs_head->param_type->name);

	fprintf(fp, "\
typedef struct RpcServer\n\
{\n\
	int socket_desc;\n\
	int client_sock;\n\
} rpc_server_t, *rpc_server_handler_t;\n\
\n\
rpc_server_handler_t\n\
create_rpc_server(int port)\n\
{\n\
	rpc_server_handler_t server = (rpc_server_handler_t) malloc(sizeof(rpc_server_t));\n\
    struct sockaddr_in server_addr;\n\
\n\
    //Create socket\n\
    server->socket_desc = socket(AF_INET , SOCK_STREAM , 0);\n\
    if (server->socket_desc == -1)\n\
    {\n\
        perror(\"Could not create socket\");\n\
        free(server);\n\
        return NULL;\n\
    }\n\
    puts(\"Socket created\");\n\
\n\
    //Prepare the sockaddr_in structure\n\
    server_addr.sin_family = AF_INET;\n\
    server_addr.sin_addr.s_addr = INADDR_ANY;\n\
    server_addr.sin_port = htons( port );\n\
\n\
    //Bind\n\
    if( bind(server->socket_desc,(struct sockaddr *)&server_addr , sizeof(server_addr)) < 0)\n\
    {\n\
        //print the error message\n\
        perror(\"bind failed. Error\");\n\
        free(server);\n\
        return NULL;\n\
    }\n\
    puts(\"bind done\");\n\
\n\
    return server;\n\
}\n\
\n\
int\n\
start_rpc_server(rpc_server_handler_t server, %s(*f)(%s ))\n\
{\n\
	struct sockaddr_in client;\n\
	int read_size;\n\
	 //Listen\n\
    listen(server->socket_desc , 3);\n\
	while (1)\n\
	{\n\
		//Accept and incoming connection\n\
	    puts(\"Waiting for incoming connections...\");\n\
	    int c = sizeof(struct sockaddr_in);\n\
\n\
	    //accept connection from an incoming client\n\
	    server->client_sock = accept(server->socket_desc, (struct sockaddr *)&client, (socklen_t*)&c);\n\
	    if (server->client_sock < 0)\n\
	    {\n\
	        perror(\"accept failed\");\n\
	        return 1;\n\
	    }\n\
	    puts(\"Connection accepted\");\n\
\n\
	    %s client_message;\n\
	    //Receive a message from client\n\
	    while( (read_size = recv(server->client_sock , &client_message , sizeof(client_message) , 0)) > 0 )\n\
	    {\n\
	    	// do calculation here\n\
	    	%s response = f(client_message);\n\
	        //Send the message back to client\n\
	        write(server->client_sock , &response , sizeof(response));\n\
	    }\n\
\n\
	    if(read_size == 0)\n\
	    {\n\
	        puts(\"Client disconnected\");\n\
	        fflush(stdout);\n\
	    }\n\
	    else if(read_size == -1)\n\
	    {\n\
	        perror(\"recv failed\");\n\
	    }\n\
	}\n\
}\n\
\n\
\n\
#endif /* __XRPC_SERVER_STUB__ */\n\
\n", 
rpcs_head->reply_type->name,
rpcs_head->param_type->name,
rpcs_head->param_type->name,
rpcs_head->reply_type->name);

	return SUCCESS;
}
