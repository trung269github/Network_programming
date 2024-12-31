#include "FTP_Client.h"

// Compile command: gcc client.c FTP_Client.c -o ftpclient

int main(int argc, char const *argv[])
{
	int sock_control;
	int data_sock, retcode;
	char user_input[MAX_SIZE];
	struct command cmd;

	if (argc != 2)
	{
		printf("usage: ./ftclient ip-address\n");
		exit(0);
	}

	int ip_valid = validate_ip(argv[1]);
	if (ip_valid == INVALID_IP)
	{
		printf("Error: Invalid ip-address\n");
		exit(1);
	}

	sock_control = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);

	if (sock_control == INVALID_SOCKET)
	{
		perror("Error");
		exit(1);
	}

	SOCKADDR_IN servAddr;

	servAddr.sin_family = AF_INET;
	servAddr.sin_port = htons(PORT); // use some unused port number
	servAddr.sin_addr.s_addr = inet_addr(argv[1]);

	int connectStatus = connect(sock_control, (SOCKADDR *)&servAddr, sizeof(servAddr));

	if (connectStatus == -1)
	{
		printf("Error...\n");
		exit(1);
	}
	// Get connection, welcome messages
	printf("Connected to %s\n", argv[1]);
	print_reply(read_reply(sock_control));
	int choice = login_menu();
	if (choice == 2) // sign up
	{
		signup();
	}
	else if (choice != 1) // exit
	{
		exit(1);
	}

	ftclient_login(sock_control);

	while (1)
	{ // loop until user types quit

		// Get a command from user
		int cmd_stt = ftclient_read_command(user_input, sizeof(user_input), &cmd);
		if (cmd_stt == -1)
		{
			printf("Invalid command\n");
			continue; // loop back for another command
		}
		else if (cmd_stt == 0)
		{

			// Send command to server
			if (send(sock_control, user_input, strlen(user_input), 0) < 0)
			{
				close(sock_control);
				exit(1);
			}

			retcode = read_reply(sock_control);
			if (retcode == 221)
			{
				/* If command was quit, just exit */
				print_reply(221);
				break;
			}

			if (retcode == 502)
			{
				// If invalid command, show error message
				printf("%d Invalid command.\n", retcode);
			}
			else
			{

				// Command is valid (RC = 200), process command

				// open data connection
				if ((data_sock = ftclient_open_conn(sock_control)) < 0)
				{
					perror("Error opening socket for data connection");
					exit(1);
				}

				// execute command
				if (strcmp(cmd.code, "LIST") == 0)
				{
					// runProgressBar(1);
					ftclient_list(data_sock, sock_control);
				}
				else if (strcmp(cmd.code, "HELP") == 0)
				{
					ftclient_help(data_sock, sock_control);
				}
				else if (strcmp(cmd.code, "SORT") == 0)
				{
					runProgressBar(1);
					ftclient_list(data_sock, sock_control);
				}
				else if (strcmp(cmd.code, "FOLD") == 0)
				{
					runProgressBar(1);
					ftclient_zip(data_sock, sock_control);
				}
				else if (strcmp(cmd.code, "STOU") == 0)
				{
					printf("Uploading...");
					runProgressBar(1);
					ftclient_send_multiple(data_sock, cmd.arg, sock_control);
					printf("Done!\n");
				}
				else if (strcmp(cmd.code, "COPY") == 0)
				{
					int repl = read_reply(sock_control);
					if (repl == 253)
					{
						runProgressBar(1);
						printf("253 Copied successfully\n");
					}
					else if (repl == 454)
					{
						runProgressBar(0);
						printf("454 Copy failure\n");
					}
					else if (repl == 455)
					{
						runProgressBar(0);
						printf("455 Syntax error (COPY<filepath> <newfilepath>)\n");
					}
				}
				else if (strcmp(cmd.code, "FIND") == 0)
				{
					int repl = read_reply(sock_control);
					if (repl == 241)
					{
						runProgressBar(1);
						int nums = read_reply(sock_control);
						for (int i = 0; i < nums; ++i)
							ftclient_list(data_sock, sock_control); // ham nay in mess tu server
					}
					else if (repl == 441)
					{
						runProgressBar(0);
						printf("441 File not found!\n");
					}
				}
				else if (strcmp(cmd.code, "MKDR") == 0)
				{
					int repl = read_reply(sock_control);
					if (repl == 254)
					{
						runProgressBar(1);
						printf("254 Mkdir successfully\n");
					}

					else if (repl == 456)
					{
						runProgressBar(0);
						printf("451 Mkdir failure\n");
					}
				}
				else if (strcmp(cmd.code, "CPY ") == 0)
				{
					int repl = read_reply(sock_control);
					if (repl == 253)
					{
						runProgressBar(1);
						printf("253 Copied successfully\n");
					}
					else if (repl == 454)
					{
						runProgressBar(0);
						printf("454 Copy failure\n");
					}

					else if (repl == 455)
					{
						runProgressBar(0);
						printf("455 Syntax error (cpy <filepath> <newfilepath>)\n");
					}
				}
				else if (strcmp(cmd.code, "DEL ") == 0)
				{
					int repl = read_reply(sock_control);
					if (repl == 252)
					{
						runProgressBar(1);
						printf("252 Delete successfully\n");
					}
					else if (repl == 453)
					{
						runProgressBar(0);
						printf("451 Delete failure\n");
					}
				}

				else if (strcmp(cmd.code, "CWD ") == 0)
				{
					if (read_reply(sock_control) == 250)
					{
						print_reply(250);
					}
					else
					{
						printf("%s is not a directory\n", cmd.arg);
					}
				}
				else if (strcmp(cmd.code, "PWD ") == 0)
				{
					if (read_reply(sock_control) == 212)
					{
						runProgressBar(1);
						ftclient_list(data_sock, sock_control); // ham nay in mess tu server
					}
				}
				else if (strcmp(cmd.code, "RETR") == 0)
				{
					// wait for reply (is file valid)
					if (read_reply(sock_control) == 550)
					{
						runProgressBar(0);
						print_reply(550);
						close(data_sock);
						continue;
					}
					runProgressBar(1);
					clock_t start = clock();
					ftclient_get(data_sock, sock_control, cmd.arg);
					clock_t end = clock();
					double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
					print_reply(read_reply(sock_control));
					printf("Time taken %lf\n", cpu_time);
				}
				else if (strcmp(cmd.code, "MRET") == 0)
				{
					runProgressBar(1);
					int count, i;
					char filenames[MAX_FILES][MAX_FILENAME_LEN];
					separate_filenames(cmd.arg, filenames, &count);
					for (int i = 0; i < count; i++)
					{
						if (read_reply(sock_control) == 550)
						{
							print_reply(550);
							close(data_sock);
							continue;
						}
						ftclient_get(data_sock, sock_control, filenames[i]);
						print_reply(read_reply(sock_control));
					}
				}
				else if (strcmp(cmd.code, "PGET") == 0)
				{
					if (read_reply(sock_control) == 550)
					{
						runProgressBar(0);
						print_reply(550);
						close(data_sock);
						continue;
					}
					clock_t start = clock();
					ftclient_private_get(data_sock, sock_control, cmd.arg);
					clock_t end = clock();
					double cpu_time = ((double)(end - start)) / CLOCKS_PER_SEC;
					print_reply(read_reply(sock_control));
					printf("Time taken %lf\n", cpu_time);
				}
				else if (strcmp(cmd.code, "PPUT") == 0)
				{
					runProgressBar(1);
					printf("Uploading ...\n");
					private_upload(data_sock, cmd.arg, sock_control);
					printf("Done!\n");
				}
				else if (strcmp(cmd.code, "STOR") == 0)
				{
					runProgressBar(1);
					printf("Uploading ...\n");
					upload(data_sock, cmd.arg, sock_control);
					printf("Done!\n");
				}
				close(data_sock);
			}
		}

	} // loop back to get more user input

	// Close the socket (control connection)
	close(sock_control);
	return 0;
}
