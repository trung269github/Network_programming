#include "FTP_Server.h"

char current_username[MAX_USERNAME_LENGTH];
// result of search file
typedef struct
{
	int count;
	char **files;
} SearchResult;
/*
 * Trim whiteshpace and line ending
 * characters from a string
 */
void trimstr(char *str, int n)
{
	int i;
	for (i = 0; i < n; i++)
	{
		if (isspace(str[i]))
			str[i] = 0;
		if (str[i] == '\n')
			str[i] = 0;
	}
}

void log_activity(const char *username, const char *cmd, const char *arg)
{
	FILE *file = fopen("../auth/log.txt", "a");
	if (file == NULL)
	{
		printf("Error opening log file!\n");
		return;
	}

	time_t rawtime;
	struct tm *timeinfo;
	time(&rawtime);
	timeinfo = localtime(&rawtime);

	char activity[256];
	snprintf(activity, sizeof(activity), "User: %s, Command: %s %s", username, cmd, arg);

	fprintf(file, "----------------------------------------\n");
	fprintf(file, "Time: %s", asctime(timeinfo)); // asctime() adds a newline character
	fprintf(file, "%s\n", activity);
	fprintf(file, "----------------------------------------\n\n");

	fclose(file);
}

int check_private_key(char *input_username, char *user_key)
{
	const char *filename = "../auth/privatekey.txt";
	char *pch;
	char buf[MAX_SIZE];
	char private_key[MAX_SIZE];
	char username[MAX_SIZE];
	char *line = NULL;
	FILE *file = fopen(filename, "r");
	if (file == NULL)
	{
		perror("Error opening file");
		return 0;
	}
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

		if ((strcmp(username, input_username) == 0) && (strcmp(private_key, user_key) == 0))
		{
			return 1;
		}
	}
	fclose(file);
	return 0;
}

int isDirectoryExists(const char *path)
{
	struct stat stats;

	stat(path, &stats);

	// Check for file existence
	if (S_ISDIR(stats.st_mode))
		return 1;

	return 0;
}

void trimSpaces(char *str)
{
	int i, j = 0;
	int n = strlen(str);

	// Leading spaces
	while (isspace((unsigned char)str[j]) && j < n)
	{
		j++;
	}
	for (i = 0; j < n; j++)
	{
		// Copy non-space characters
		if (!isspace((unsigned char)str[j]) || (j < n - 1 && !isspace((unsigned char)str[j + 1])))
		{
			str[i++] = str[j];
		}
	}
	// Remove trailing space
	if (i > 0 && isspace((unsigned char)str[i - 1]))
	{
		i--;
	}
	str[i] = '\0'; // Null terminate the modified string
}

char current_username[MAX_USERNAME_LENGTH];

void make_folder(char username[MAX_SIZE])
{

	strcpy(current_username, username);
	char command[MAX_SIZE];
	char folder_name[MAX_SIZE];
	strcpy(folder_name, "./data/private/");
	strcat(folder_name, current_username);
	strcpy(command, "mkdir ");
	strcat(command, folder_name);
	if (isDirectoryExists(folder_name))
	{
		printf("%s's folder exists\n", current_username);
	}
	else
	{
		// create new directory for user
		system(command);
	}
}
/**
 * Function to split arg
 * eg input="name1 name2" into str1="name1",str2="name2"
 * return 0 when success, return -1 on error
 */
int splitString(char *input, char **str1, char **str2)
{
	if (input == NULL || strlen(input) == 0)
	{
		return -1;
	}

	char *spacePos = strchr(input, ' ');

	if (spacePos == NULL || *(spacePos + 1) == '\0')
	{
		return -1;
	}

	size_t len1 = spacePos - input;

	// Allocate memory for the first substring and copy it
	*str1 = (char *)malloc(len1 + 1);
	if (*str1 == NULL)
	{
		// Return error if memory allocation fails
		return -1;
	}
	strncpy(*str1, input, len1);
	(*str1)[len1] = '\0'; // Null-terminate the string

	// Find the first non-space character after the space
	char *nonSpacePos = spacePos + 1;
	while (*nonSpacePos != '\0' && *nonSpacePos == ' ')
	{
		nonSpacePos++;
	}

	if (*nonSpacePos == '\0')
	{
		// Return error if there are no characters after the space
		free(*str1);
		return -1;
	}

	// Calculate the length of the second substring
	size_t len2 = strlen(nonSpacePos);

	// Allocate memory for the second substring and copy it
	*str2 = (char *)malloc(len2 + 1);
	if (*str2 == NULL)
	{
		// Return error if memory allocation fails
		free(*str1);
		return -1;
	}
	strcpy(*str2, nonSpacePos);

	return 0; // Success
}

int isFile(const char *path)
{
	struct stat pathStat;
	if (stat(path, &pathStat) == 0)
	{
		return S_ISREG(pathStat.st_mode);
	}
	return 0; // Return 0 for error or if the path is not a regular file
}

// Function to check if a path corresponds to a directory (folder)
int isFolder(const char *path)
{
	struct stat pathStat;
	if (stat(path, &pathStat) == 0)
	{
		return S_ISDIR(pathStat.st_mode);
	}
	return 0; // Return 0 for error or if the path is not a directory
}

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

int socket_create()
{
	int sockfd;
	int yes = 1;
	SOCKADDR_IN sock_addr;

	// create new socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("socket() error");
		return -1;
	}

	// set local address info
	sock_addr.sin_family = AF_INET;
	sock_addr.sin_port = htons(PORT);
	sock_addr.sin_addr.s_addr = INADDR_ANY;

	// bind
	int flag = 1;
	setsockopt(sockfd, SOL_SOCKET, SO_REUSEPORT, &flag, sizeof(flag));

	if (bind(sockfd, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) < 0)
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

void handleError(const char *message)
{
	perror(message);
	exit(EXIT_FAILURE);
}

void recursiveDelete(const char *path)
{
	DIR *dir = opendir(path);
	if (!dir)
	{
		handleError("Error opening folder");
	}

	struct dirent *entry;
	struct stat info;

	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
		{
			char filePath[PATH_MAX];
			snprintf(filePath, sizeof(filePath), "%s/%s", path, entry->d_name);

			if (stat(filePath, &info) != 0)
			{
				handleError("Error getting file information");
			}

			if (S_ISDIR(info.st_mode))
			{
				recursiveDelete(filePath);
				rmdir(filePath);
			}
			else
			{
				if (remove(filePath) != 0)
				{
					handleError("Error deleting file");
				}
			}
		}
	}

	closedir(dir);
}

int deleteFolder(const char *path)
{
	recursiveDelete(path);
	if (rmdir(path) != 0)
	{
		perror("Error deleting folder");
		return -1;
	}

	printf("Folder deleted: %s\n", path);
	return 0;
}
int deleteFile(const char *filename)
{
	if (isFile(filename))
	{
		printf("file\n");
		if (remove(filename) == 0)
		{
			printf("File %s deleted successfully.\n", filename);
			return 0; // Success
		}
		else
		{
			perror("Error deleting file");
			return -1; // Error
		}
	}
	else if (isFolder(filename))
	{
		printf("folder\n");
		if (deleteFolder(filename) == 0)
		{
			printf("Folder %s deleted successfully.\n", filename);
			return 0; // Success
		}
		else
		{
			perror("Error deleting folder");
			return -1; // Error
		}
	}
}

/**
 * Delete file
 * over data connection
 */
void ftserve_delete(int sock_control, int sock_data, char *arg)
{
	if (deleteFile(arg) == 0)
		send_response(sock_control, 252);
	else
		send_response(sock_control, 453);
}

int send_response(int sockfd, int rc)
{
	int conv = rc;
	if (send(sockfd, &conv, sizeof(conv), 0) < 0)
	{
		perror("error sending...\n");
		return -1;
	}
	return 0;
}

/**
 * Receive data on sockfd
 * Returns -1 on error, number of bytes received
 * on success
 */
int recv_data(int sockfd, char *buf, int bufsize)
{
	memset(buf, 0, bufsize);
	int num_bytes = recv(sockfd, buf, bufsize, 0);
	if (num_bytes < 0)
	{
		return -1;
	}
	return num_bytes;
}

/**
 * Authenticate a user's credentials
 * Return 1 if authenticated, 0 if not
 */
int ftserve_check_user(char *user, char *pass)
{
	char username[MAX_SIZE];
	char password[MAX_SIZE];
	char *pch;
	char buf[MAX_SIZE];
	char *line = NULL;
	size_t num_read;
	size_t len = 0;
	FILE *fd;
	int auth = 0;

	fd = fopen("./auth/.auth", "r");
	if (fd == NULL)
	{
		perror("file not found");
		exit(1);
	}

	while ((num_read = getline(&line, &len, fd)) != -1)
	{
		memset(buf, 0, MAX_SIZE);
		strcpy(buf, line);

		pch = strtok(buf, " ");
		strcpy(username, pch);

		if (pch != NULL)
		{
			pch = strtok(NULL, " ");
			strcpy(password, pch);
		}

		// remove end of line and whitespace
		trimstr(password, (int)strlen(password));

		if ((strcmp(user, username) == 0) && (strcmp(pass, password) == 0))
		{
			auth = 1;
			make_folder(username);
			break;
		}
	}
	free(line);
	fclose(fd);
	return auth;
}

/**
 * Log in connected client
 */
int ftserve_login(int sock_control)
{
	char buf[MAX_SIZE];
	char user[MAX_SIZE];
	char pass[MAX_SIZE];
	memset(user, 0, MAX_SIZE);
	memset(pass, 0, MAX_SIZE);
	memset(buf, 0, MAX_SIZE);

	// Wait to recieve username
	if ((recv_data(sock_control, buf, sizeof(buf))) == -1)
	{
		perror("recv error\n");
		exit(1);
	}

	strcpy(user, buf + 5); // 'USER ' has 5 char

	// tell client we're ready for password
	send_response(sock_control, 331);

	// Wait to recieve password
	memset(buf, 0, MAX_SIZE);
	if ((recv_data(sock_control, buf, sizeof(buf))) == -1)
	{
		perror("recv error\n");
		exit(1);
	}

	strcpy(pass, buf + 5); // 'PASS ' has 5 char

	return (ftserve_check_user(user, pass));
}

/**
 * Wait for command from client and
 * send response
 * Returns response code
 */
int ftserve_recv_cmd(int sock_control, char *cmd, char *arg)
{
	int rc = 200;
	char user_input[MAX_SIZE];

	memset(user_input, 0, MAX_SIZE);
	memset(cmd, 0, 5);
	memset(arg, 0, MAX_SIZE);

	// Wait to recieve command
	if ((recv_data(sock_control, user_input, sizeof(user_input))) == -1)
	{
		perror("recv error\n");
		return -1;
	}

	strncpy(cmd, user_input, 4);
	strcpy(arg, user_input + 5);
	if (strcmp(cmd, "QUIT") == 0)
	{
		rc = 221;
	}
	else if ((strcmp(cmd, "USER") == 0) || (strcmp(cmd, "PASS") == 0) ||
			 (strcmp(cmd, "LIST") == 0) || (strcmp(cmd, "RETR") == 0) ||
			 (strcmp(cmd, "CWD ") == 0) || (strcmp(cmd, "PWD") == 0) ||
			 (strcmp(cmd, "STOR") == 0) || (strcmp(cmd, "SORT") == 0) ||
			 (strcmp(cmd, "FOLD") == 0) || (strcmp(cmd, "STOU") == 0) ||
			 (strcmp(cmd, "MRET") == 0) || (strcmp(cmd, "FIND") == 0) ||
			 (strcmp(cmd, "MKDR") == 0) || (strcmp(cmd, "RENM") == 0) ||
			 (strcmp(cmd, "DEL ") == 0) || (strcmp(cmd, "CPY ") == 0) ||
			 (strcmp(cmd, "PPUT") == 0) || (strcmp(cmd, "PGET") == 0) ||
			 (strcmp(cmd, "HELP") == 0))
	{
		rc = 200;
	}
	else
	{ // invalid command
		rc = 502;
	}

	send_response(sock_control, rc);
	return rc;
}

/**
 * Connect to remote host at given port
 * Returns:	socket fd on success, -1 on error
 */
int socket_connect(int port, char *host)
{
	int sockfd;
	SOCKADDR_IN dest_addr;

	// create socket
	if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0)
	{
		perror("error creating socket");
		return -1;
	}

	// create server address
	memset(&dest_addr, 0, sizeof(dest_addr));
	dest_addr.sin_family = AF_INET;
	dest_addr.sin_port = htons(port);
	dest_addr.sin_addr.s_addr = inet_addr(host);

	// Connect on socket
	if (connect(sockfd, (struct sockaddr *)&dest_addr, sizeof(dest_addr)) < 0)
	{
		perror("error connecting to server");
		return -1;
	}
	return sockfd;
}

/**
 * Open data connection to client
 * Returns: socket for data connection
 * or -1 on error
 */
int ftserve_start_data_conn(int sock_control)
{
	char buf[1024];
	int wait, sock_data;

	// Wait for go-ahead on control conn
	if (recv(sock_control, &wait, sizeof wait, 0) < 0)
	{
		perror("Error while waiting");
		return -1;
	}

	// Get client address
	SOCKADDR_IN client_addr;
	int len = sizeof(client_addr);
	getpeername(sock_control, (struct sockaddr *)&client_addr, &len);
	inet_ntop(AF_INET, &client_addr.sin_addr, buf, sizeof(buf));

	// Initiate data connection with client
	if ((sock_data = socket_connect(DEFAULT_PORT, buf)) < 0)
		return -1;

	return sock_data;
}

/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
char prefix[MAX_SIZE] = ""; // Buffer to hold the prefix string
void list_files(const char *path, int depth, int sock_data, char prefix[MAX_SIZE]);

int ftserve_list(int sock_data, int sock_control)
{
	char curr_dir[MAX_SIZE];
	memset(curr_dir, 0, MAX_SIZE);

	getcwd(curr_dir, sizeof(curr_dir));
	// strcat(curr_dir, "/data");

	list_files(curr_dir, 0, sock_data, prefix); // Start with depth 0 and is_last = 1 (true) for the root directory

	return 0;
}


int count_entries(const char *path)
{
	int count = 0;
	DIR *d = opendir(path);
	struct dirent *entry;
	if (!d)
		return -1; // Return -1 in case of an error

	while ((entry = readdir(d)) != NULL)
	{
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
		{
			count++;
		}
	}
	closedir(d);
	return count;
}

void list_files(const char *path, int depth, int sock_data, char prefix[MAX_SIZE])
{
	DIR *dir;
	struct dirent *entry;

	if (!(dir = opendir(path)))
		return;

	// Count total entries in the current directory for tracking the last entry
	int total_entries = count_entries(path);
	int entries_count = 0;

	while ((entry = readdir(dir)) != NULL)
	{

		if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0 || strcmp(entry->d_name, "private") == 0)
			continue;

		entries_count++; // Increment count of processed entries

		// Determine if it's the last entry in the current directory
		int is_last_entry = (entries_count == total_entries);

		// Prepare entry line with appropriate indentation
		char entry_line[MAX_SIZE];

		if (depth > 0)
		{
			strcat(prefix, "|   "); // Standard indentation for non-root items
		}
		else
		{
			strcpy(prefix, ""); // No indentation for root items
		}

		char lineSymbol[MAX_SIZE];	// Buffer to hold the line symbol string
		strcpy(lineSymbol, "|-- "); // Last entry uses |--

		snprintf(entry_line, sizeof(entry_line), "%s%s%s\n", prefix, lineSymbol, entry->d_name);

		// Send the entry line in a single send call
		send(sock_data, entry_line, strlen(entry_line), 0);

		// If it's a directory, recursively list files in the subdirectory
		if (entry->d_type == DT_DIR)
		{
			// Prepare next path for recursion
			char next_path[MAX_SIZE];
			snprintf(next_path, sizeof(next_path), "%s/%s", path, entry->d_name);
			// Recursively list files in the subdirectory
			list_files(next_path, depth + 1, sock_data, prefix);
		}
		strcpy(prefix, "");
	}

	closedir(dir);
}
int compare(const struct dirent **a, const struct dirent **b)
{
	return strcasecmp((*a)->d_name, (*b)->d_name);
}
int ftserve_list_sorted(int sock_data, int sock_control)
{
	struct dirent **output = NULL;
	char msgToClient[MAX_SIZE];
	memset(msgToClient, 0, MAX_SIZE);

	char curr_dir[MAX_SIZE];
	getcwd(curr_dir, sizeof(curr_dir));
	// strcat(curr_dir, "/data");
	int n = scandir(curr_dir, &output, NULL, compare);

	if (n > 0)
	{
		for (int i = 0; i < n; i++)
		{
			if (strcmp(output[i]->d_name, ".") != 0 && strcmp(output[i]->d_name, "..") != 0 && strcmp(output[i]->d_name, "private") != 0)
			{
				strcat(msgToClient, output[i]->d_name);
				strcat(msgToClient, "  ");
			}
			free(output[i]);
		}
		free(output);
	}

	strcat(msgToClient, "\n");
	if (send(sock_data, msgToClient, strlen(msgToClient), 0) < 0)
	{
		perror("error");
	}

	return 0;
}

/*
 * Receive a folder from client input
 * Store the folder into the data folder in server side
 */
int ftserve_zip(int sock_data, int sock_control)
{
	char data[MAX_SIZE];
	int size, stt = 0;
	const char *filename = "client_folder.zip";
	char dest[256];
	char curr_dir[MAX_SIZE];
	memset(curr_dir, 0, MAX_SIZE);
	getcwd(curr_dir, sizeof(curr_dir));
	strcpy(dest, curr_dir);
	strcat(dest, "/");
	strcat(dest, filename);
	recv(sock_control, &stt, sizeof(stt), 0);
	if (stt == 550)
	{
		printf("can't not open file!\n");
		return -1;
	}
	else
	{
		FILE *fd = fopen(dest, "w");

		while ((size = recv(sock_data, data, MAX_SIZE, 0)) > 0)
		{
			fwrite(data, 1, size, fd);
		}

		if (size < 0)
		{
			perror("error\n");
		}

		// char command[100] = "cd data\nunzip -l client_folder.zip";
		// strcat(command, filename);
		// system(command);
		fclose(fd);
		return 0;
	}
	return 0;
}

/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
int ftpServer_cwd(int sock_control, char *folderName)
{
	if (chdir(folderName) == 0) // change directory
	{
		send_response(sock_control, 250); // 250 Directory successfully changed.
	}
	else
	{
		send_response(sock_control, 550); // 550 Requested action not taken
	}
	return 0;
}

/**
 * Send list of files in current directory
 * over data connection
 * Return -1 on error, 0 on success
 */
void ftpServer_pwd(int sock_control, int sock_data)
{
	char curr_dir[MAX_SIZE - 2], msgToClient[MAX_SIZE];
	memset(curr_dir, 0, MAX_SIZE);
	memset(msgToClient, 0, MAX_SIZE);

	getcwd(curr_dir, sizeof(curr_dir));
	sprintf(msgToClient, "%s\n", curr_dir);
	if (send(sock_data, msgToClient, strlen(msgToClient), 0) < 0)
	{
		perror("error");
		send_response(sock_control, 550);
	}
	send_response(sock_control, 212);
}

/**
 * Send file specified in filename over data connection, sending
 * control message over control connection
 * Handles case of null or invalid filename
 */
void ftserve_retr(int sock_control, int sock_data, char *filename)
{
	FILE *fd = NULL;
	char data[MAX_SIZE];
	memset(data, 0, MAX_SIZE);
	size_t num_read;
	char dest[256];
	char curr_dir[MAX_SIZE];
	memset(curr_dir, 0, MAX_SIZE);

	getcwd(curr_dir, sizeof(curr_dir));
	strcpy(dest, curr_dir);
	strcat(dest, "/");
	strcat(dest, filename);
	fd = fopen(dest, "r");
	if (!fd)
	{
		// send error code (550 Requested action not taken)
		send_response(sock_control, 550);
	}
	else
	{
		// send okay (150 File status okay)
		send_response(sock_control, 150);

		do
		{
			num_read = fread(data, 1, MAX_SIZE, fd);

			if (num_read < 0)
			{
				printf("error in fread()\n");
			}

			// send block
			if (send(sock_data, data, num_read, 0) < 0)
				perror("error sending file\n");

		} while (num_read > 0);

		// send message: 226: closing conn, file transfer successful
		send_response(sock_control, 226);

		fclose(fd);
	}
}

void private_retr(int sock_control, int sock_data, char *arg)
{
	FILE *fd = NULL;
	char data[MAX_SIZE];
	char username[MAX_SIZE];
	char filename[MAX_SIZE];
	char buffer[MAX_SIZE];
	char dest[MAX_SIZE];
	char private_key[MAX_SIZE];
	memset(data, 0, MAX_SIZE);
	size_t num_read;

	int count, i;
	char filenames[MAX_FILES][MAX_FILENAME_LEN];
	separate_filenames(arg, filenames, &count);
	printf("user: %s\nfile: %s\n", filenames[0], filenames[1]);
	strcpy(username, filenames[0]);
	strcpy(filename, filenames[1]);

	char curr_dir[MAX_SIZE];
	memset(curr_dir, 0, MAX_SIZE);

	getcwd(curr_dir, sizeof(curr_dir));
	strcpy(dest, curr_dir);
	// strcpy(dest, "./data/private/");
	strcat(dest, "/private/");
	strcat(dest, username);
	strcat(dest, "/");
	strcat(dest, filename);
	fd = fopen(dest, "r");
	printf(" file: %s\n", dest);
	if (!fd)
	{
		// send error code (550 Requested action not taken)
		send_response(sock_control, 550);
	}
	else
	{
		// send okay (150 File status okay)
		send_response(sock_control, 150);
		// receive key from client
		if ((recv_data(sock_control, buffer, sizeof(buffer))) == -1)
		{
			perror("recv error\n");
			exit(1);
		}
		strcpy(private_key, buffer);
		printf("Private key received: %s\n", private_key);
		if (check_private_key(username, private_key))
		{
			printf("Private key correct\n");
			send_response(sock_control, 200);
		}
		else
		{
			printf("Private key incorrect\n");
			send_response(sock_control, 500); // key incorrect
			send_response(sock_control, 502); // get file failed
			return;
		}
		do
		{
			num_read = fread(data, 1, MAX_SIZE, fd);

			if (num_read < 0)
			{
				printf("error in fread()\n");
			}

			// send block
			if (send(sock_data, data, num_read, 0) < 0)
				perror("error sending file\n");

		} while (num_read > 0);

		// send message: 226: closing conn, file transfer successful
		send_response(sock_control, 226);

		fclose(fd);
	}
}

/*
 * Receive the files from client input
 * Store the files into data folder in server side
 */
int recvFile(int sock_control, int sock_data, char *filename)
{
	char data[MAX_SIZE];
	int size, stt = 0;
	recv(sock_control, &stt, sizeof(stt), 0);
	char curr_dir[MAX_SIZE];
	memset(curr_dir, 0, MAX_SIZE);
	getcwd(curr_dir, sizeof(curr_dir));
	strcat(curr_dir, "/");
	strcat(curr_dir, filename);
	// printf("%s\n", curr_dir);

	if (stt == 550)
	{
		printf("can't not open file!\n");
		return -1;
	}
	else
	{
		FILE *fd = fopen(curr_dir, "w");

		while ((size = recv(sock_data, data, MAX_SIZE, 0)) > 0)
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
	return 0;
}
int recvMulti(int sock_control, int sock_data, char *arg)
{
	int count, i;
	char filenames[MAX_FILES][MAX_FILENAME_LEN];
	separate_filenames(arg, filenames, &count);
	for (i = 0; i < count; i++)
	{
		recvFile(sock_control, sock_data, filenames[i]);
	}
	return 0;
}

/*
 * Receive private file
 */

int private_recv(int sock_control, int sock_data, char *filename)
{
	trimSpaces(filename);
	char data[MAX_SIZE];
	int size, stt = 0;
	recv(sock_control, &stt, sizeof(stt), 0);
	char dest[255] = "./private/";
	strcat(dest, current_username);
	strcat(dest, "/");
	strcat(dest, filename);
	printf("%s\n", dest);
	if (stt == 550)
	{
		printf("can't not open file!\n");
		return -1;
	}
	else
	{
		FILE *fd = fopen(dest, "w");

		while ((size = recv(sock_data, data, MAX_SIZE, 0)) > 0)
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
	return 0;
}

int renameFile(const char *oldName, const char *newName)
{
	if (oldName == NULL || newName == NULL)
	{
		fprintf(stderr, "Invalid input: oldName and newName cannot be NULL\n");
		return -1;
	}

	if (rename(oldName, newName) != 0)
	{
		perror("Error renaming file");
		return -1;
	}

	printf("File successfully renamed from '%s' to '%s'\n", oldName, newName);
	return 0;
}

/**
 * Rename file and folder
 * over data connection
 */
void ftserve_rename(int sock_control, int sock_data, char *arg)
{
	char *from, *to;
	if (splitString(arg, &from, &to) == 0)
	{
		if (renameFile(from, to) == 0)
			send_response(sock_control, 251);
		else
			send_response(sock_control, 451);
		free(from);
		free(to);
	}
	else
		send_response(sock_control, 452);
}

SearchResult searchInDirectory(char *dirPath, char *fileName)
{
	SearchResult result = {0, NULL};

	DIR *dir;
	struct dirent *entry;
	if ((dir = opendir(dirPath)) == NULL)
	{
		perror("opendir");
		exit(EXIT_FAILURE);
	}

	while ((entry = readdir(dir)) != NULL)
	{
		if (entry->d_type == DT_DIR)
		{
			// Ignore "." and ".." directories
			if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
			{
				char path[PATH_MAX];
				snprintf(path, PATH_MAX, "%s/%s", dirPath, entry->d_name);
				SearchResult subdirResult = searchInDirectory(path, fileName);

				// Merge the results
				result.count += subdirResult.count;
				result.files = realloc(result.files, result.count * sizeof(char *));
				for (int i = 0; i < subdirResult.count; ++i)
				{
					result.files[result.count - subdirResult.count + i] = subdirResult.files[i];
				}

				free(subdirResult.files);
			}
		}
		else
		{
			if (strcmp(entry->d_name, fileName) == 0)
			{
				result.count++;
				result.files = realloc(result.files, result.count * sizeof(char *));
				result.files[result.count - 1] = (char *)malloc(PATH_MAX);
				snprintf(result.files[result.count - 1], PATH_MAX, "%s/%s", dirPath, entry->d_name);
			}
		}
	}

	closedir(dir);
	return result;
}

/**
 * Send path of the file in the dirPath
 * over data connection
 * Return -1 on error, 0 on success
 */

void ftserve_find(int sock_control, int sock_data, char *filename)
{
	char curr_dir[MAX_SIZE - 2];
	memset(curr_dir, 0, MAX_SIZE);
	getcwd(curr_dir, sizeof(curr_dir));
	SearchResult result = searchInDirectory(curr_dir, filename);

	// File found
	if (result.count > 0)
	{
		send_response(sock_control, 241);
		send_response(sock_control, result.count);
		for (int i = 0; i < result.count; ++i)
		{
			strcat(result.files[i], "\n");
			if (send(sock_data, result.files[i], strlen(result.files[i]), 0) < 0)
			{
				perror("error");
				send_response(sock_control, 550);
			}
			free(result.files[i]); // Free each file path
		}
	}
	// File not found
	else
		send_response(sock_control, 441);
	free(result.files); // Free the array of file paths
}
// Function to create a directory
int createDirectory(const char *path)
{
	int status = 0;
	status = mkdir(path, 0755);

	if (status == 0)
	{
		printf("Directory %s created successfully.\n", path);
		return 0; // Success
	}
	else
	{
		perror("Error creating directory");
		return -1; // Error
	}
}
/**
 * Make new directiory
 * over data connection
 */
void ftserve_mkdir(int sock_control, int sock_data, char *arg)
{
	char dest[MAX_SIZE];
	char curr_dir[MAX_SIZE];
	memset(curr_dir, 0, MAX_SIZE);

	getcwd(curr_dir, sizeof(curr_dir));
	strcpy(dest, curr_dir);
	strcat(dest, "/");
	strcat(dest, arg);
	if (createDirectory(dest) == 0)
		send_response(sock_control, 254);
	else
		send_response(sock_control, 456);
}

int copyDirectory(char *sourcePath, char *destinationPath)
{
	DIR *dir;
	struct dirent *entry;

	// Open the source directory
	dir = opendir(sourcePath);
	if (dir == NULL)
	{
		perror("Error opening source directory");
		return -1; // Error
	}

	// Create the destination directory
	createDirectory(destinationPath);

	// Iterate through all entries in the source directory
	while ((entry = readdir(dir)) != NULL)
	{
		if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0)
		{
			// Create full paths for source and destination
			char sourceFile[PATH_MAX];
			char destinationFile[PATH_MAX];
			sprintf(sourceFile, "%s/%s", sourcePath, entry->d_name);
			sprintf(destinationFile, "%s/%s", destinationPath, entry->d_name);

			if (entry->d_type == DT_DIR)
			{
				// Recursively copy subdirectories
				copyDirectory(sourceFile, destinationFile);
			}
			else
			{
				// Copy files
				FILE *sourceFilePtr, *destinationFilePtr;
				char ch;

				sourceFilePtr = fopen(sourceFile, "rb");
				if (sourceFilePtr == NULL)
				{
					perror("Error opening source file");
					closedir(dir);
					return -1; // Error
				}

				destinationFilePtr = fopen(destinationFile, "wb");
				if (destinationFilePtr == NULL)
				{
					fclose(sourceFilePtr);
					perror("Error opening destination file");
					closedir(dir);
					return -1; // Error
				}

				while ((ch = fgetc(sourceFilePtr)) != EOF)
				{
					fputc(ch, destinationFilePtr);
				}

				fclose(sourceFilePtr);
				fclose(destinationFilePtr);
			}
		}
	}

	closedir(dir);
	return 0; // Success
}

int copyOrMoveFile(char *sourceFilename, char *destinationFilename, int mode)
{
	FILE *sourceFile, *destinationFile;
	char ch;

	sourceFile = fopen(sourceFilename, "rb");
	if (sourceFile == NULL)
	{
		perror("Error opening source file");
		return -1;
	}

	destinationFile = fopen(destinationFilename, "wb");
	if (destinationFile == NULL)
	{
		fclose(sourceFile);
		perror("Error opening destination file");
		return -1;
	}

	while ((ch = fgetc(sourceFile)) != EOF)
	{
		fputc(ch, destinationFile);
	}

	fclose(sourceFile);
	fclose(destinationFile);

	if (mode == 0)
	{
		printf("File %s copied to %s successfully.\n", sourceFilename, destinationFilename);
	}
	else if (mode == 1)
	{
		printf("File %s moved to %s successfully.\n", sourceFilename, destinationFilename);
	}
	else
	{
		printf("Invalid mode. Use 0 for copy or 1 for move.\n");
		return -1;
	}
	return 0;
}

char *appendCopyFileString(const char *source)
{
	char *ofCopy = "OfCopy";

	size_t sourceLen = strlen(source);
	size_t ofCopyLen = strlen(ofCopy);
	char *result;

	// Check if the source has an extension
	const char *dot = strrchr(source, '.');
	if (dot != NULL && source[0] != '.')
	{
		size_t extensionLen = sourceLen - (size_t)(dot - source);
		size_t resultLen = sourceLen + ofCopyLen + 1;

		result = (char *)malloc(resultLen);
		strncpy(result, source, sourceLen - extensionLen);
		result[sourceLen - extensionLen] = '\0';
		strcat(result, ofCopy);
		strcat(result, dot);
	}

	else
	{
		size_t resultLen = sourceLen + ofCopyLen + 1;

		result = (char *)malloc(resultLen);
		strcpy(result, source);
		strcat(result, ofCopy);
	}

	return result;
}

/**
 * Copy file and folder
 * over data connection
 */
void ftserve_copy(int sock_control, int sock_data, char *arg)
{
	char *result = appendCopyFileString(arg);

	if (isFile(arg))
	{
		printf("%s\n", arg);
		printf("%s\n", result);
		if (copyOrMoveFile(arg, result, 1) == 0)
			send_response(sock_control, 253);
		else
			send_response(sock_control, 454);
	}

	if (isFolder(arg))
	{
		createDirectory(result);
		if (copyDirectory(arg, result) == 0)
			send_response(sock_control, 253);
		else
			send_response(sock_control, 454);
	}

	else
		send_response(sock_control, 455);

	free(result);
}
/**
 * Child process handles connection to client
 */
void ftserve_process(int sock_control)
{
	int sock_data;
	char cmd[5];
	char arg[MAX_SIZE];
	printf("Client connected succesfully\n");
	// Send welcome message
	send_response(sock_control, 220);

	// Authenticate user
	if (ftserve_login(sock_control) == 1)
	{
		send_response(sock_control, 230);
	}
	else
	{
		send_response(sock_control, 430);
		exit(0);
	}
	chdir("./data");

	while (1)
	{
		// Wait for command
		int rc = ftserve_recv_cmd(sock_control, cmd, arg);

		log_activity(current_username, cmd, arg);

		if ((rc < 0) || (rc == 221))
		{
			break;
		}

		if (rc == 200)
		{
			// Open data connection with client
			if ((sock_data = ftserve_start_data_conn(sock_control)) < 0)
			{
				close(sock_control);
				exit(1);
			}

			// Execute command
			if (strcmp(cmd, "LIST") == 0)
			{ // Do list
				ftserve_list(sock_data, sock_control);
			}
			else if (strcmp(cmd, "SORT") == 0)
			{ // do list sort by name
				ftserve_list_sorted(sock_data, sock_control);
			}
			else if (strcmp(cmd, "FOLD") == 0)
			{ // get received folder
				printf("Receving ...\n");
				ftserve_zip(sock_data, sock_control);
			}
			else if (strcmp(cmd, "CWD ") == 0)
			{ // change directory
				ftpServer_cwd(sock_control, arg);
			}
			else if (strcmp(cmd, "PWD ") == 0)
			{ // print working directory
				ftpServer_pwd(sock_control, sock_data);
			}
			else if (strcmp(cmd, "RETR") == 0)
			{ // RETRIEVE: get file
				ftserve_retr(sock_control, sock_data, arg);
			}
			else if (strcmp(cmd, "PGET") == 0)
			{ // RETRIEVE: get file
				private_retr(sock_control, sock_data, arg);
			}
			else if (strcmp(cmd, "FIND") == 0)
			{ // find file
				ftserve_find(sock_control, sock_data, arg);
			}
			else if (strcmp(cmd, "MRET") == 0)
			{
				int count, i;
				char filenames[MAX_FILES][MAX_FILENAME_LEN];
				separate_filenames(arg, filenames, &count);
				for (int i = 0; i < count; i++)
				{
					ftserve_retr(sock_control, sock_data, filenames[i]);
				}
			}
			else if (strcmp(cmd, "STOR") == 0)
			{ // RETRIEVE: get file
				printf("Receving...\n");
				recvFile(sock_control, sock_data, arg);
			}
			else if (strcmp(cmd, "STOU") == 0)
			{
				printf("Receiving...\n");
				recvMulti(sock_control, sock_data, arg);
			}
			else if (strcmp(cmd, "PPUT") == 0)
			{
				printf("Receiving...\n");
				private_recv(sock_control, sock_data, arg);
			}
			else if (strcmp(cmd, "RENM") == 0)
			{ // rename file and folder
				ftserve_rename(sock_control, sock_data, arg);
			}
			else if (strcmp(cmd, "DEL ") == 0)
			{ // rename file and folder
				ftserve_delete(sock_control, sock_data, arg);
			}
			else if (strcmp(cmd, "MKDR") == 0)
			{
				ftserve_mkdir(sock_control, sock_data, arg);
			}
			else if (strcmp(cmd, "CPY ") == 0)
			{ // rename file and folder
				ftserve_copy(sock_control, sock_data, arg);
			}
			// Close data connection
			close(sock_data);
		}
	}
}
