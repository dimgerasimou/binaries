#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "colorscheme.h"

int main() {
    /* Get the proper icon */
    FILE* fp;
    char input[128];
    char icon[6];

    fp = fopen("/sys/class/net/enp4s0/operstate", "r");
    if (fp == NULL) {
        puts("Network error.");
        return 0;
    }
    fscanf(fp, "%s", input);
    fclose(fp);
    if(!strcmp(input, "up")) {
        strcpy(icon, ""); 
    } else {
    	fp = fopen("/sys/class/net/wlo1/operstate", "r");
    	if (fp == NULL) {
        	puts("Network error.");
        	return 0;
    	}
    	fscanf(fp, "%s", input);
    	fclose(fp);
    	if(!strcmp(input, "up")){
        	strcpy(icon, "");
    	}else
        	strcpy(icon, "睊");
	}
    
    /* Get ip adress */
    char temp[32];
    unsigned short ip[4];
    char ipadress[32];
    char found = 0;

    fp = popen("/bin/nmcli", "r");
    if (fp == NULL){
        fprintf(stderr, "Failed to run nmcli command.\n");
        return 1;
    }

    while (fgets(input, sizeof(input), fp) != NULL) {
        if (strstr(input, "inet4")){
            found = 1;
            break;
        }
    }
    pclose(fp);
    if (found) {
        sscanf(input, "%s %hd.%hd.%hd.%hd", temp, &ip[0], &ip[1], &ip[2], &ip[3]);
        sprintf(ipadress, "%d.%d.%d.%d", ip[0], ip[1], ip[2], ip[3]);
    }

    /* Print the final string */

    if (found)
        printf(SFG SBG" %s "NBG NFG" %s ^d^\n", icon, ipadress);
	else
		printf(SFG SBG" %s ^d^\n", icon);
    return 0;
}
