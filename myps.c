/*
 *
 * This code replicates a process monitor (like ps in linux). It requires the
 * system have a /proc/ style FS for processes, traverses said file system, and
 * outputs all the currently running processes by name, id, parent id, and owner,
 * as well as lists their running command line parameters
 *
 * By: Tyler Sullivan
 *
 */

#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <pwd.h>
#include <sys/types.h>

#define MAX_LINE_SIZE 1024

/* I got this trimming code from http://stackoverflow.com/questions/122616/how-do-i-trim-leading-trailing-whitespace-in-a-standard-way */
char *trimwhitespace(char *str)
{
  char *end;
  while(isspace(*str)) str++;
  if(*str == 0)
    return str;
  end = str + strlen(str) - 1;
  while(end > str && isspace(*end)) end--;
  *(end+1) = 0;
  return str;
}

int main (int argc, char* args[])
{
	DIR *dir, *new_dir;
	struct dirent *dent;
	struct passwd *pws;
	int uname, charcount, inchar;
	char *name;
	char *proc = "/proc/";
	char *new_dir_name, *old_dir_name, *file_name, *powner, *token;
	char pname[MAX_LINE_SIZE], pid[MAX_LINE_SIZE], ppid[MAX_LINE_SIZE], state[MAX_LINE_SIZE], cmds[MAX_LINE_SIZE];
	char line[MAX_LINE_SIZE], line_bak[MAX_LINE_SIZE], line_bak_bak[MAX_LINE_SIZE];
	FILE *statusfile, *cmdlinefile;

	if(argc != 1){
		printf("No arguments, Usage: %s\n", args[0]);
		return EXIT_FAILURE;
	}

	printf("%-6s %-6s %-5s %-12s %-36s %-s\n", "Pid", "PPid", "State", "Owner", "Name", "Args");

	/* starts scanning directories */
	if ((dir = opendir (proc)) != NULL){
		while ((dent = readdir(dir)) != NULL){

			name = dent->d_name;
			cmds[0] = '\0';
			pname[0] = '\0';
			line[0] = '\0';

			/* opens directory if it's a process */
			if (isdigit(*name)){
				new_dir_name = (char*) malloc(strlen(proc) + strlen(name) + 1);
				old_dir_name = (char*) malloc(strlen(proc) + strlen(name) + 1);
				if (new_dir_name == NULL){
					perror("new_dir_name");
					return EXIT_FAILURE;
				}
				if (new_dir_name == NULL){
					perror("new_dir_name");
					return EXIT_FAILURE;
				}

				/* creates strings with adaptive directory names */
				strcat(new_dir_name,proc);
				strcat(new_dir_name,name);
				strcpy(old_dir_name,new_dir_name);

				if ((new_dir = opendir(new_dir_name)) != NULL){
					/* designs and opens directory and file for status */
					file_name = realloc (new_dir_name, sizeof(new_dir_name) + sizeof("/status") + 1);
					new_dir_name = file_name;
					if (new_dir_name == NULL){
						perror("file_name");
						free(new_dir_name);
						free(old_dir_name);
						return EXIT_FAILURE;
					}
					strcat(file_name, "/status");
					if (NULL == (statusfile = fopen(new_dir_name, "r"))){
						perror("status file");
						free(new_dir_name);
						free(old_dir_name);
						return EXIT_FAILURE;
					}

					/* designs and opens directory and file for cmdline */
					file_name = realloc (old_dir_name, sizeof(old_dir_name) + sizeof("/cmdline") + 1);
					old_dir_name = file_name;
					if (old_dir_name == NULL){
						perror("file_name");
						free(new_dir_name);
						free(old_dir_name);
						return EXIT_FAILURE;
					}
					strcat(file_name, "/cmdline");
					if (NULL == (cmdlinefile = fopen(old_dir_name, "r"))){
						perror("cmdline file");
						free(new_dir_name);
						free(old_dir_name);
						return EXIT_FAILURE;
					}

					/* gets args from /proc/[pid]/cmdline file and compensates for null char delimiter */
					charcount = 0;
					while (EOF != (inchar = fgetc(cmdlinefile))){
						if (inchar == '\0') inchar = ' ';
						line[charcount] = inchar;
						charcount++;
					}

					line[charcount] = '\0';
					strncpy(line_bak, line, strlen(line));
					strncpy(line_bak_bak, line, strlen(line));
					line_bak[strlen(line)] = '\0';
					line_bak_bak[strlen(line)] = '\0';
					token = strtok(line_bak, " ");
					token = strtok(token, "/");
					while (token != NULL){
						strncpy(pname, trimwhitespace(token), strlen(token));
						pname[strlen(token)] = '\0';
						token = strtok(NULL, "/");
					}
					token = strtok(line, " ");
					token = line_bak_bak + strlen(line);
					if (token != NULL) {
						strcpy(cmds, trimwhitespace(token));
						cmds[strlen(token)] = '\0';
					}

					free(new_dir_name);
					free(old_dir_name);

					/* gets most info from /proc/[pid]/status file */
					while (NULL != fgets(line, MAX_LINE_SIZE, statusfile)){
						token = strtok(line, ":");
						/* checks for name for cases where cmdline file was empty */
						if (0 == strncmp(line, "Name", strlen("Name"))){
							token = trimwhitespace(strtok(NULL, " \n"));
							if (0 != strncmp(token, pname, strlen(token))){
								strncpy(pname, token, strlen(token));
								pname[strlen(token)] = '\0';
							}
						}
						if (0 == strncmp(line, "Pid", strlen("Pid"))){
							token = trimwhitespace(strtok(NULL, " \n"));
							strncpy(pid, token, strlen(token));
						}
						if (0 == strncmp(line, "PPid", strlen("PPid"))){
							token = trimwhitespace(strtok(NULL, " \n"));
							strncpy(ppid, token, strlen(token));
						}
						if (0 == strncmp(line, "State", strlen("State"))){
							token = trimwhitespace(strtok(NULL, " \n"));
							strncpy(state, token, strlen(token));
						}
						if (0 == strncmp(line, "Uid", strlen("Uid"))){
							token = strtok(NULL, " \n");
							uname = atoi(strtok(token, " "));
							pws = getpwuid(uname);
							powner = pws->pw_name;

						}

					}
					/* prints formatted output */
					printf("%-6s %-6s %-5s %-12s %36s %-s\n", pid, ppid, state, powner, pname, cmds);
				}
			}
		}
	}
	return EXIT_SUCCESS;
}
