#include "FTP_Client.h"

char current_username[MAX_SIZE];
/*Validating IP Address*/
int validate_ip(const char *ip)
{
	int value_1 = -1;
	int value_2 = -1;
	int value_3 = -1;
	int value_4 = -1;
	int count = 0;
	int i = 0;

	while (ip[i] != '\0')
	{
		if (ip[i] == '.')
			count++;
		i++;
	}

	if (count != 3)
		return INVALID_IP;
	else
	{
		sscanf(ip, "%d.%d.%d.%d", &value_1, &value_2, &value_3, &value_4);

		if (value_1 < 0 || value_2 < 0 || value_3 < 0 || value_4 < 0 || value_1 > 255 || value_2 > 255 || value_3 > 255 || value_4 > 255) /* IP Addresses from 0.0.0.0 to 255.255.255.255*/
			return INVALID_IP;
		else
			return 1;
	}
}

void trimSpaces(char *str)
{
	int i, j = 0;
	int n = strlen(str);

	while (isspace((unsigned char)str[j]) && j < n)
	{
		j++;
	}

	for (i = 0; j < n; j++)
	{
		if (!isspace((unsigned char)str[j]) || (j < n - 1 && !isspace((unsigned char)str[j + 1])))
		{
			str[i++] = str[j];
		}
	}

	if (i > 0 && isspace((unsigned char)str[i - 1]))
	{
		i--;
	}

	str[i] = '\0';
}

/**
 * Print response message
 */
void print_reply(int rc)
{
	switch (rc)
	{
	case 220:
		printf("220 Welcome, FTP server ready.\n\n");
		break;
	case 221:
		printf("221 Goodbye!\n");
		break;
	case 212:
		printf("221 Directory status!\n");
		break;
	case 226:
		printf("226 Closing data connection. Requested file action successful.\n");
		break;
	case 250:
		printf("250 Directory successfully changed.\n");
		break;
	case 550:
		printf("550 Requested action not taken. File unavailable.\n");
		break;
	case 502:
		printf("502 Private key incorrect. Command not implemented.\n");
		break;
	}
}

/**
 * Receive a response from server
 * Returns -1 on error, return code on success
 */
int read_reply(int sock_control)
{
	int retcode = 0;
	if (recv(sock_control, &retcode, sizeof(retcode), 0) < 0)
	{
		perror("client: error reading message from server\n");
		return -1;
	}
	return retcode;
}

/**
 * Trim whiteshpace and get array
 * of files names from a string
 */
void separate_filenames(const char *input, char output[][MAX_FILENAME_LEN], int *count)
{
	char *token;
	char strCopy[MAX_FILENAME_LEN * MAX_FILES];

	// Copy the input string as strtok modifies the original string
	strcpy(strCopy, input);
	// Initialize count
	*count = 0;

	// Get the first token
	token = strtok(strCopy, " ");

	// Walk through other tokens
	while (token != NULL)
	{
		strcpy(output[*count], token);
		(*count)++;
		token = strtok(NULL, " ");
	}
}

/**
 * Input: cmd struct with an a code and an arg
 * Concats code + arg into a string and sends to server
 */
int ftclient_send_cmd(struct command *cmd, int sock_control)
{
	char buffer[MAX_SIZE + 5];
	int rc;

	sprintf(buffer, "%s %s", cmd->code, cmd->arg);

	// Send command string to server
	rc = send(sock_control, buffer, (int)strlen(buffer), 0);
	if (rc < 0)
	{
		perror("Error sending command to server");
		return -1;
	}
	return 0;
}

/**
 * Read input from command line
 */
void read_input(char *user_input, int size)
{
	memset(user_input, 0, size);
	int n = read(STDIN_FILENO, user_input, size);
	user_input[n] = '\0';

	/* Remove trailing return and newline characters */
	if (user_input[n - 1] == '\n')
		user_input[n - 1] = '\0';
	if (user_input[n - 1] == '\r')
		user_input[n - 1] = '\0';
}

void create_sharing_key(char *username)
{
	const char *filename = "../ServerSide/auth/.privatekey";
	char private_key[MAX_SIZE];
	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		perror("Error opening file");
		return;
	}
	char *pch;
	char buf[MAX_SIZE];
	char *line = NULL;
	size_t num_read;
	size_t len = 0;
	while ((num_read = getline(&line, &len, file)) != -1)
	{
		memset(buf, 0, MAX_SIZE);
		strcpy(buf, line);

		pch = strtok(buf, " ");
		strcpy(username, pch);

		if (pch != NULL)
		{
			pch = strtok(NULL, " ");
			strcpy(private_key, pch);
		}

		// remove end of line and whitespace
		for (int i = 0; i < (int)strlen(private_key); i++)
		{
			if (isspace(private_key[i]))
				private_key[i] = 0;
			if (private_key[i] == '\n')
				private_key[i] = 0;
		}

		if ((strcmp(current_username, username) == 0))
		{
			// private key is already existed
			return;
		}
	}
	fclose(file);
	printf("But first, create a private sharing key!\n");
	printf("Your private key: ");
	scanf("%s", private_key);

	file = fopen(filename, "a");
	if (file == NULL)
	{
		perror("Error opening file");
		return;
	}

	// Write the account and password to the file
	fprintf(file, "%s %s\n", current_username, private_key);
	printf("Your private key has been registered successfully!\n\n");
	fclose(file);
}

void change_password(char *username, char *old_password)
{
	int c;
while ((c = getchar()) != '\n' && c != EOF);
	printf("Do you want to change your password? (y/n): ");
	char choice;

	scanf("%c", &choice);
	if (choice == 'n')
	{
		printf("--------------------------------");
		printf("\nWelcome to the application!\n");
		printf("Enter your command to use the app.\nFor example: %s>help\n\n", current_username);

		return;
	}
	Account creds[10];
	char new_password[MAX_SIZE];
	const char *filename = "../ServerSide/auth/.auth";
	printf("Enter new password: ");
	scanf("%s", new_password);
	fflush(stdin);
	FILE *fp = fopen(filename, "r");
	if (fp == NULL)
	{
		printf("Cannot open .auth file\n");
		return;
	}

	int count = 0;
	char *lines[100];

	while (fscanf(fp, "%s %s", creds[count].username, creds[count].password) != EOF)
	{
		count++;
	}
	fclose(fp);

	// Tìm dòng chứa username
	for (int i = 0; i < count; i++)
	{
		if (strcmp(creds[i].username, username) == 0)
		{
			strcpy(creds[i].password, new_password);
			fp = fopen(filename, "w");
			if (fp == NULL)
			{
				printf("Cannot open .auth file\n");
				return;
			}
			for (int i = 0; i < count; i++)
			{
				fprintf(fp, "%s %s\n", creds[i].username, creds[i].password);
			}
			fclose(fp);
			printf("Changed password successfully!\n");
			printf("Welcome to the application!\n");
			printf("Enter your command to use the app.\nFor example: %s>help\n\n", current_username);
			return;
		}
	}

	// Nếu không tìm thấy dòng chứa username
	printf("Cannot find username\n");
}

/**
 * Get login details from user and
 * send to server for authentication
 */
void ftclient_login(int sock_control)
{
	struct command cmd;
	char user[MAX_SIZE];
	memset(user, 0, MAX_SIZE);
	char pass[MAX_SIZE];
	memset(pass, 0, MAX_SIZE);
	// Get username from user
	printf("Name: ");
	fflush(stdout);
	read_input(user, MAX_SIZE);

	// Send USER command to server
	strcpy(cmd.code, "USER");
	strcpy(cmd.arg, user);
	ftclient_send_cmd(&cmd, sock_control);

	// Wait for go-ahead to send password
	int wait;
	recv(sock_control, &wait, sizeof(wait), 0);

	// Get password from user
	printf("Password: ");
	fflush(stdout);
	read_input(pass, MAX_SIZE);

	// Send PASS command to server
	strcpy(cmd.code, "PASS");
	strcpy(cmd.arg, pass);
	ftclient_send_cmd(&cmd, sock_control);

	// wait for response
	int retcode = read_reply(sock_control);
	switch (retcode)
	{
	case 430:
		printf("430 Invalid username/password.\n");
		exit(0);
	case 230:
		printf("230 Successful login.\n\n");
		change_password(user, pass);
		break;
	default:
		perror("error reading message from server");
		exit(1);
		break;
	}
}

void runProgressBar(int status)
{
	const int total = 100;
	int progress;
	for (progress = 0; progress <= total; progress++)
	{
		printf("\r\033[0;36mProcessing: ");
		printf("[");
		int pos = 50 * progress / total;
		for (int i = 0; i < 50; i++)
		{
			if (i < pos)
				printf("\033[0;36m=");
			else if (i == pos)
				printf("\033[0;33m>");
			else
				printf("\033[0;31m ");
		}
		printf("\033[0;36m] \033[0;36m%d%%\033[0m", progress);
		fflush(stdout);
		usleep(20000);
	}

	printf("\n");

	if (status == 0)
		printf("\033[0;31mError occurred.\033[0m\n");
	else if (status == 1)
		printf("\033[0;32mTask completed successfull.\033[0m\n");

	printf("\n");
}

/**
 * Parse command in cstruct
 */
int ftclient_read_command(char *user_input, int size, struct command *cstruct)
{

	memset(cstruct->code, 0, sizeof(cstruct->code));
	memset(cstruct->arg, 0, sizeof(cstruct->arg));

	printf("%s> ", current_username); // prompt for input
	fflush(stdout);

	// wait for user to enter a command
	read_input(user_input, size);

	// user_input:
	// chang directory on client side
	if (strcmp(user_input, "!ls") == 0 || strcmp(user_input, "!ls ") == 0)
	{
		system("tree"); // client side
		return 1;
	}
	else if (strcmp(user_input, "!pwd") == 0 || strcmp(user_input, "!pwd ") == 0)
	{
		system("pwd"); // client side
		return 1;
	}
	else if (strncmp(user_input, "!cd ", 4) == 0)
	{
		if (chdir(user_input + 4) == 0)
		{
			printf("Directory successfully changed\n");
		}
		else
		{
			perror("Error change directory");
		}
		return 1;
	}
	// change directory on server side
	else if (strcmp(user_input, "ls ") == 0 || strcmp(user_input, "ls") == 0)
	{
		strcpy(cstruct->code, "LIST");
		memset(user_input, 0, MAX_SIZE);
		strcpy(user_input, cstruct->code);
	}
	else if (strcmp(user_input, "sort ") == 0 || strcmp(user_input, "sort") == 0)
	{
		strcpy(cstruct->code, "SORT");
		memset(user_input, 0, MAX_SIZE);
		strcpy(user_input, cstruct->code);
	}
	else if (strcmp(user_input, "fold ") == 0 || strcmp(user_input, "fold") == 0)
	{
		strcpy(cstruct->code, "FOLD");
		memset(user_input, 0, MAX_SIZE);
		strcpy(user_input, cstruct->code);
	}
	else if (strcmp(user_input, "help ") == 0 || strcmp(user_input, "help") == 0)
	{
		strcpy(cstruct->code, "HELP");
		memset(user_input, 0, MAX_SIZE);
		strcpy(user_input, cstruct->code);
	}
	else if (strncmp(user_input, "renm ", 5) == 0)
	{
		strcpy(cstruct->code, "RENM");
		strcpy(cstruct->arg, user_input + 5);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strncmp(user_input, "cd ", 3) == 0)
	{
		strcpy(cstruct->code, "CWD ");
		strcpy(cstruct->arg, user_input + 3);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strncmp(user_input, "find ", 5) == 0)
	{
		strcpy(cstruct->code, "FIND");
		strcpy(cstruct->arg, user_input + 5);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strncmp(user_input, "mkdir ", 6) == 0)
	{
		strcpy(cstruct->code, "MKDR");
		strcpy(cstruct->arg, user_input + 6);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strcmp(user_input, "pwd") == 0 || strcmp(user_input, "pwd ") == 0)
	{
		strcpy(cstruct->code, "PWD ");
		memset(user_input, 0, MAX_SIZE);
		strcpy(user_input, cstruct->code);
	}
	// upload and download file
	else if (strncmp(user_input, "get ", 4) == 0)
	{ // RETRIEVE
		strcpy(cstruct->code, "RETR");
		strcpy(cstruct->arg, user_input + 4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strncmp(user_input, "mget ", 4) == 0)
	{
		strcpy(cstruct->code, "MRET");
		strcpy(cstruct->arg, user_input + 4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strncmp(user_input, "pget", 4) == 0)
	{
		strcpy(cstruct->code, "PGET");
		strcpy(cstruct->arg, user_input + 4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strncmp(user_input, "put ", 4) == 0)
	{
		strcpy(cstruct->code, "STOR"); // STORE
		strcpy(cstruct->arg, user_input + 4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strncmp(user_input, "mput ", 4) == 0)
	{
		strcpy(cstruct->code, "STOU"); // STORE multiple files
		strcpy(cstruct->arg, user_input + 4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strncmp(user_input, "pput ", 4) == 0)
	{
		strcpy(cstruct->code, "PPUT");
		strcpy(cstruct->arg, user_input + 4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	else if (strncmp(user_input, "del ", 4) == 0)
	{
		strcpy(cstruct->code, "DEL ");
		strcpy(cstruct->arg, user_input + 4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}

	else if (strncmp(user_input, "cpy ", 4) == 0)
	{
		strcpy(cstruct->code, "CPY ");
		strcpy(cstruct->arg, user_input + 4);

		memset(user_input, 0, MAX_SIZE);
		sprintf(user_input, "%s %s", cstruct->code, cstruct->arg);
	}
	// quit
	else if (strcmp(user_input, "quit") == 0)
	{
		strcpy(cstruct->code, "QUIT");
		memset(user_input, 0, MAX_SIZE);
		strcpy(user_input, cstruct->code);
	}

	else
	{ // invalid
		return -1;
	}

	return 0;
}

/**
 * Create listening socket on remote host
 * Returns -1 on error, socket fd on success
 */
int socket_create(int port)
{
	int sockfd;
	SOCKADDR_IN sock_addr;

	// create new socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket() error");
		return -1;
	}

	// set local address info
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(port);
	sock_addr.sin_addr.s_addr = htonl(INADDR_ANY);

	// bind
	int flag = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));
	if (bind(sockfd, (SOCKADDR *)&sock_addr, sizeof(sock_addr)) < 0)
	{
		close(sockfd);
		perror("bind() error");
		return -1;
	}

	// begin listening for incoming TCP requests
	if (listen(sockfd, 5) < 0)
	{
		close(sockfd);
		perror("listen() error");
		return -1;
	}
	return sockfd;
}

/**
 * Create new socket for incoming client connection request
 * Returns -1 on error, or fd of newly created socket
 */
int socket_accept(int sock_listen)
{
	int sockfd;
	SOCKADDR_IN client_addr;
	int len = sizeof(client_addr);

	// Wait for incoming request, store client info in client_addr
	sockfd = accept(sock_listen, (SOCKADDR *)&client_addr, &len);

	if (sockfd < 0)
	{
		perror("accept() error");
		return -1;
	}
	return sockfd;
}

/**
 * Open data connection
 */
int ftclient_open_conn(int sock_con)
{
	int sock_listen = socket_create(DEFAULT_PORT);

	// send an ACK on control conn
	int ack = 1;
	if ((send(sock_con, &ack, sizeof(ack), 0)) < 0)
	{
		printf("client: ack write error :%d\n", errno);
		exit(1);
	}

	int sock_conn = socket_accept(sock_listen);
	close(sock_listen);
	return sock_conn;
}

/**
 * Do list commmand
 */
void printTree(const char *path)
{
	const char *token = path;
	int level = 0;

	while (*token != '\0')
	{
		// Find the next occurrence of '/'
		const char *nextSlash = strchr(token, '/');
		if (nextSlash == NULL)
		{
			// No more '/' found, print the rest of the path
			for (int i = 0; i < level; i++)
			{
				printf("|   ");
			}
			printf("|-- %s\n", token);
			break;
		}
		else
		{
			// Print the current part of the path up to the next '/'
			for (int i = 0; i < level; i++)
			{
				printf("|   ");
			}
			printf("|-- %.*s\n", (int)(nextSlash - token), token);

			// Move to the next part of the path
			token = nextSlash + 1;
			level++;
		}
	}
}

int ftclient_list(int sock_data, int sock_ctrl)
{
	size_t num_recvd;	// number of bytes received with recv()
	char buf[MAX_SIZE]; // hold a filename received from server
	int tmp = 0;

	memset(buf, 0, sizeof(buf));
	while ((num_recvd = recv(sock_data, buf, MAX_SIZE, 0)) > 0)
	{
		printf("%s", buf);
		memset(buf, 0, sizeof(buf));
	}

	if (num_recvd < 0)
	{
		perror("error");
	}
	return 0;
}

int ftclient_zip(int sock_data, int sock_ctrl)
{
	FILE *fd = NULL;
	char data[MAX_SIZE];
	memset(data, 0, MAX_SIZE);
	size_t num_read;
	int stt;
	const char *filename = "archive.zip";
	int status;
	char *command = "zip -r -j archive.zip *";

	status = system(command);
	fd = fopen(filename, "r");

	if (!fd)
	{
		// send error code (550 Requested action not taken)
		printf("ko the mo file\n");
		stt = 550;
		send(sock_ctrl, &stt, sizeof(stt), 0);
	}
	else
	{
		// send okay (150 File status okay)
		stt = 150;
		send(sock_ctrl, &stt, sizeof(stt), 0);

		do
		{
			num_read = fread(data, 1, MAX_SIZE, fd);

			if (num_read < 0)
			{
				printf("error in fread()\n");
			}

			// send block
			send(sock_data, data, num_read, 0);

		} while (num_read > 0);

		char *clean = "rm archive.zip";
		system(clean);
		fclose(fd);
	}
}

/**
 * Do get <filename> command
 */
int ftclient_get(int data_sock, int sock_control, char *arg)
{
	char data[MAX_SIZE];
	int size;
	char dest[MAX_SIZE];
	strcpy(dest, "./download/");
	strcat(dest, arg);
	printf("%s\n", dest);
	FILE *fd = fopen(dest, "w");

	while ((size = recv(data_sock, data, MAX_SIZE, 0)) > 0)
	{
		fwrite(data, 1, size, fd);
	}

	if (size < 0)
	{
		perror("error\n");
	}

	fclose(fd);
	return 0;
}

int ftclient_private_get(int data_sock, int sock_control, char *arg)
{
	char private_key[MAX_SIZE];
	printf("Enter private key to access this folder!\n");
	printf("Private key: ");
	scanf("%s", private_key);

	// send private key for server to check
	int rc = send(sock_control, private_key, (int)strlen(private_key), 0);
	if (rc < 0)
	{
		perror("Error sending command to server");
		return -1;
	}

	rc = read_reply(sock_control);
	if (rc == 200)
	{
		runProgressBar(1);
		ftclient_get(data_sock, sock_control, arg);
	}
	else
	{
		runProgressBar(0);
	}
	return 0;
}

void upload(int data_sock, char *filename, int sock_control)
{

	FILE *fd = NULL;
	char data[MAX_SIZE];
	memset(data, 0, MAX_SIZE);
	size_t num_read;
	int stt;

	fd = fopen(filename, "r");

	if (!fd)
	{
		// send error code (550 Requested action not taken)
		printf("Cannot open file\n");
		stt = 550;
		send(sock_control, &stt, sizeof(stt), 0);
	}
	else
	{
		// send okay (150 File status okay)
		stt = 150;
		send(sock_control, &stt, sizeof(stt), 0);

		do
		{
			num_read = fread(data, 1, MAX_SIZE, fd);

			if (num_read < 0)
			{
				printf("error in fread()\n");
			}

			// send block
			send(data_sock, data, num_read, 0);

		} while (num_read > 0);

		fclose(fd);
	}
}

void private_upload(int data_sock, char *filename, int sock_control)
{
	trimSpaces(filename);
	FILE *fd = NULL;
	char data[MAX_SIZE];
	memset(data, 0, MAX_SIZE);
	size_t num_read;
	int stt;

	fd = fopen(filename, "r");

	if (!fd)
	{
		// send error code (550 Requested action not taken)
		printf("Cannot open file\n");
		stt = 550;
		send(sock_control, &stt, sizeof(stt), 0);
	}
	else
	{
		// send okay (150 File status okay)
		stt = 150;
		send(sock_control, &stt, sizeof(stt), 0);

		do
		{
			num_read = fread(data, 1, MAX_SIZE, fd);

			if (num_read < 0)
			{
				printf("error in fread()\n");
			}

			// send block
			send(data_sock, data, num_read, 0);

		} while (num_read > 0);

		fclose(fd);
	}
}

int ftclient_send_multiple(int data_sock, char *filename, int sock_control)
{
	int count, i;
	char filenames[MAX_FILES][MAX_FILENAME_LEN];
	separate_filenames(filename, filenames, &count);
	for (i = 0; i < count; i++)
	{
		upload(data_sock, filenames[i], sock_control);
	}
	return 0;
}

void ftclient_help(int data_sock, int sock_control)
{
	printf("Client side commands:\n");
	printf("\t1.  !ls\t\t\t\tList directory\n");
	printf("\t2.  !pwd\t\t\tShow current working directory\n");
	printf("\t3.  !cd <path>\t\t\tChange directory <path>\n\n");
	printf("Application commands:\n");
	printf("\t1.  ls\t\t\t\tList directory\n");
	printf("\t2.  pwd\t\t\t\tShow current working directory\n");
	printf("\t3.  sort\t\t\tUpload client's current directory\n");
	printf("\t4.  renm  <path1> <path2>\tRename <path1> to <path2>\n");
	printf("\t5.  find  <filename>\t\tFind <filename> in directory\n");
	printf("\t6.  mkdir <name>\t\tMake new directory\n");
	printf("\t7.  get   <filename>\t\tDownload from server\n");
	printf("\t8.  mget  <filenames..>\t\tDownload multiple files from server\n");
	printf("\t9.  pget  <filename>\t\tDownload from private folder\n");
	printf("\t10. put   <filename>\t\tUpload file to server\n");
	printf("\t11. mput  <filenames..>\t\tUpload multiple files to server\n");
	printf("\t12. pput  <filename>\t\tUpload file to private folder\n");
	printf("\t13. del   <filename>\t\tDelete file from server\n");
	printf("\t14. cpy   <filename>\t\tCopy files/directories\n");
	printf("\t15. quit\t\t\tQuit program\n");
	return;
}

int login_menu()
{
	printf("Choose action:\n");
	int choice = 0;
	while (choice > 3 || choice <= 0)
	{
		printf("1. Login\n");
		printf("2. Signup\n");
		printf("3. Exit\n");
		printf("Your choice: ");
		scanf("%d", &choice);
		fflush(stdin);
	}
	return choice;
}

int signup()
{
	printf("Register new account\n");
	char username[MAX_USERNAME_LENGTH];
	char password[MAX_PASSWORD_LENGTH];
	fflush(stdin);
	// scanf("\n");
	printf("Enter username: ");
	scanf("%s", username);
	if (strlen(username) == 0)
	{
		return -1;
	}

	printf("Enter password: ");
	scanf("%s", password);
	if (strlen(password) == 0)
	{
		return -1;
	}

	FILE *file = fopen("../ServerSide/auth/.auth", "a");
	if (file == NULL)
	{
		perror("Error opening file");
		return -1;
	}

	// Write the account and password to the file
	fprintf(file, "%s %s\n", username, password);
	printf("Account has been registered successfully!\n\n");
	printf("Please login to use the program\n");
	fclose(file);
}
