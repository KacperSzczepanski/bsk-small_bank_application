#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <stdint.h>
#include <security/pam_appl.h>
#include <security/pam_misc.h>
#include <unistd.h>
#include <fcntl.h> 
#include <sys/time.h>
#include <pwd.h>

static struct pam_conv login_conv =
{
  misc_conv,               /* przykładowa funkcja konwersacji z libpam_misc */
  NULL                        /* ewentualne dane aplikacji (,,domknięcie'') */
};

struct user {
    char* username;
    char* fullname;
    uint32_t deposits, credits;
};

typedef struct node {
    uint32_t val;
    struct node *next;
} node_t;

void add_node(node_t **queue, uint32_t val) {
    node_t *new_node = malloc(sizeof(node_t));
    if (!new_node) {
        return;
    }

    new_node->val = val;
    new_node->next = *queue;

    *queue = new_node;
}

void clear_console() {
    system("clear");
}

bool is_number(char* num) {
    for (int i = 0; i < strlen(num); ++i) {
        if (num[i] < '0' || '9' < num[i]) {
            return false;
        }
    }

    return true;
}

bool is_bigger(char* num1, char* num2) { //>, assuming both char* express the number
    if (strlen(num1) > strlen(num2)) {
        return true;
    } else if (strlen(num2) > strlen(num1)) {
        return false;
    }

    for (int i = 0; i < strlen(num1); ++i) {
        if (num1[i] > num2[i]) {
            return true;
        } else if (num2[i] > num1[i]) {
            return false;
        }
    }

    return false;
}

bool is_date_later(char* d1, char* m1, char* y1, char* d2, char* m2, char* y2) { //>
    if (is_bigger(y1, y2)) {
        return true;
    }
    if (is_bigger(y2, y1)) {
        return false;
    }

    if (is_bigger(m1, m2)) {
        return true;
    }
    if (is_bigger(m2, m1)) {
        return false;
    }

    if (is_bigger(d1, d2)) {
        return true;
    }

    return false;
}

bool is_correct_date(char* date) {
    int no_of_dots = 0;
    int len = 0;

    for (int i = 0; i < strlen(date); ++i) {
        if ('0' <= date[i] && date[i] <= '9') {
            if (len >= 2 && no_of_dots < 2) {
                return false;
            }

            ++len;
        } else if (date[i] == '.') {
            if (no_of_dots == 2) {
                return false;
            }
            if (len == 0 || len > 2) {
                return false;
            }

            ++no_of_dots;
            len = 0;
        }
    }

    return true;
}

void print_file_content(char *filename) {
    printf("\n%s\n", filename);

    FILE * fp;
    char* line = NULL;
    size_t len = 0;
    ssize_t read = 0;

    fp = fopen(filename, "r");

    fseek(fp, 0, SEEK_END);
    long fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *content = malloc(fsize + 1);
    fread(content, fsize, 1, fp);
    
    fclose(fp);

    uint32_t first_s = -1, no_of_endl = 0;
    for (int i = 0; i < fsize; ++i) {
        if (content[i] == '\n') {
            ++no_of_endl;
        }
        if (content[i] == 'S' && no_of_endl == 2) {
            first_s = i;
            break;
        }

        printf("%c", content[i]);
    }

    uint32_t last = fsize;

    for (int i = fsize - 1; i >= first_s; --i) {
        if (content[i] == 'P' || content[i] == 'D' || content[i] == 'S') {
            for (int j = i; j < last; ++j) {
                printf("%c", content[j]);
            }
            last = i;
        }
    }

    free(content);
}

void login() { //pam causes memory leaks, I don't know if it is possible to repair

    pam_handle_t* pamh = NULL;
    int retval;
    char *username = NULL;

    retval = pam_start("login", username, &login_conv, &pamh);
    if (pamh == NULL || retval != PAM_SUCCESS) 
    {
        fprintf(stderr, "Error when starting: %d\n", retval);
        exit(1);
    }

    retval = pam_authenticate(pamh, 0);  /* próba autoryzacji */
    if (retval != PAM_SUCCESS) 
    {
        fprintf(stderr, "Access denied\n");
        exit(2);
    }

    pam_end(pamh, PAM_SUCCESS);

    printf("Please provide system time: ");
    unsigned long long t;
    scanf("%llu", &t);

    struct timeval tv;
    gettimeofday(&tv, NULL);
    unsigned long long res = (unsigned long long)(tv.tv_sec) * 1000 + (unsigned long long)(tv.tv_usec) / 1000;

    if (abs((long long)t - res) >= 15000) {
        fprintf(stderr, "Provided time is not correct\n");
        fprintf(stderr, "Provided time: %llu\n", t);
        fprintf(stderr, "System time: %llu\n", res);
        exit(2);
    }
}

int main(int argc, char** argv) {

    login();

    uint32_t unique_id = 0;

    FILE * fp;
    char * line = NULL;
    size_t len = 0;
    ssize_t read;

    fp = fopen("../uzytkownicy.txt", "r");
    if (fp == NULL) {
        exit(EXIT_FAILURE);
    }

    char* input = malloc(1);
    input[0] = 0;
    size_t ind = 0, size = 1;
    uint32_t clients_no = 0, officers_no = 0;

    while ((read = getline(&line, &len, fp)) != -1) { //reading uzytkownicy.txt, counting number of clients and officers
        char *username, *role, *name, *lastname;      //and saving input for later compute

        username = strtok(line, " ");
        role = strtok(NULL, " ");
        name = strtok(NULL, " ");
        lastname = strtok(NULL, "\n");

        char* fullname = malloc(sizeof(char) * (strlen(name) + strlen(lastname) + 2));
        for (int i = 0; i < strlen(name); ++i) {
            fullname[i] = name[i];
        }
        fullname[strlen(name)] = ' ';
        for (int i = 0; i < strlen(lastname); ++i) {
            fullname[strlen(name) + 1 + i] = lastname[i];
        }
        fullname[strlen(name) + strlen(lastname) + 1] = 0;

        if (!strncmp(role, "client", 6)) {
            ++clients_no;
        } else if (!strncmp(role, "officer", 7)) {
            ++officers_no;
        } else {
            continue;
        }

        size_t len = strlen(username) + strlen(role) + strlen(fullname) + 3;

        while (size - 1 < ind + len) {
            size *= 2;
            input = realloc(input, sizeof(char) * size);
        }

        for (int i = 0; i < strlen(username); ++i) {
            input[ind + i] = username[i];
        }
        ind += strlen(username);
        input[ind] = ' ';
        ++ind;
        for (int i = 0; i < strlen(role); ++i) {
            input[ind + i] = role[i];
        }
        ind += strlen(role);
        input[ind] = ' ';
        ++ind;
        for (int i = 0; i < strlen(fullname); ++i) {
            input[ind + i] = fullname[i];
        }
        ind += strlen(fullname);
        input[ind] = '\n';
        ++ind;

        free(fullname);
    }
    fclose(fp);

    struct user clients[clients_no], officers[officers_no];
    node_t *closed_deposits[clients_no], *closed_credits[clients_no];

    for (int i = 0; i < clients_no; ++i) {
        closed_deposits[i] = NULL;
        closed_credits[i] = NULL;
    }

    size_t c_cnt = 0, o_cnt = 0;

    for (int i = 0; i < clients_no + officers_no; ++i) {
        char *username, *role, *name;

        if (i == 0) { //here at first I had temporary variable to read strtoks, but it was conditional jump
            username = strtok(input, " ");
        } else {
            username = strtok(NULL, " ");
        }
        role = strtok(NULL, " ");
        name = strtok(NULL, "\n");
        
        if (role[0] == 'c') {
            clients[c_cnt].username = malloc(sizeof(char) * (strlen(username) + 1));
            clients[c_cnt].fullname = malloc(sizeof(char) * (strlen(name) + 1));

            strcpy(clients[c_cnt].username, username); //after mallocing we cannot assign with =, had to use strcpy (valgrind)
            strcpy(clients[c_cnt].fullname, name);
            clients[c_cnt].username[strlen(username)] = 0;
            clients[c_cnt].fullname[strlen(name)] = 0;

            clients[c_cnt].deposits = 0;
            clients[c_cnt].credits = 0;

            ++c_cnt;
        } else {
            officers[o_cnt].username = malloc(sizeof(char) * (strlen(username) + 1));
            officers[o_cnt].fullname = malloc(sizeof(char) * (strlen(name) + 1));

            strcpy(officers[o_cnt].username,  username);
            strcpy(officers[o_cnt].fullname, name);
            officers[o_cnt].username[strlen(username)] = 0;
            officers[o_cnt].fullname[strlen(name)] = 0;

            officers[o_cnt].deposits = 0;
            officers[o_cnt].credits = 0;

            ++o_cnt;
        }
    }
    free(input);

    bool exit = false;

    int mode = 0;
    //0 - menu
    //1 - clients
    //2 - show
    //3 - add
    //4 - modify
    //5 - exit
    int no_of_selected_client = 0;
    //0 - not seleted

    while (!exit) {
        clear_console();

        if (mode == 1) {
            printf("0. Deselect\n");

            for (int i = 0; i < clients_no; ++i) {
                printf("%d. %s (%s)\n", i + 1, clients[i].fullname, clients[i].username);
            }

            int a;
            scanf("%d", &a);

            if (0 <= a && a <= clients_no) {
                no_of_selected_client = a;
            }

            mode = 0;
        } else if (mode == 2) {
            if (no_of_selected_client == 0) {
                mode = 0;
                continue;
            }

            printf("====================DEPOSITS OF %s (%s)====================\n", clients[no_of_selected_client - 1].fullname, clients[no_of_selected_client - 1].username);
            for (int file_no = 0; file_no < clients[no_of_selected_client - 1].deposits; ++file_no) {
                char filename[strlen(clients[no_of_selected_client - 1].username) + 20];
                sprintf(filename, "deposits/%s_%d.txt", clients[no_of_selected_client - 1].username, file_no + 1);
                print_file_content(filename);
            }
            printf("====================CREDITS OF %s (%s)=====================\n", clients[no_of_selected_client - 1].fullname, clients[no_of_selected_client - 1].username);
            for (int file_no = 0; file_no < clients[no_of_selected_client - 1].credits; ++file_no) {
                char filename[strlen(clients[no_of_selected_client - 1].username) + 20];
                sprintf(filename, "credits/%s_%d.txt", clients[no_of_selected_client - 1].username, file_no + 1);
                print_file_content(filename);
            }

            int a = 0;
            scanf("%d", &a);

            mode = 0;
        } else if (mode == 3) {
            if (no_of_selected_client == 0) {
                mode = 0;
                continue;
            }

            printf("1. Add deposit\n");
            printf("2. Add credit\n\n");
            
            bool dep = false;
            
            int a;
            scanf("%d", &a);
            
            if (a == 1) {
                dep = true;
            } else if (a == 2) {
                dep = false;
            } else {
                mode = 0;
                continue;
            }

            
            printf("Adding new %s\n\n", dep == true ? "deposit" : "credit");
            printf("Type clients id\n");
            printf("Leave blank to choose default client (%s)\n\n", clients[no_of_selected_client - 1].username);
            printf("id: ");

            char *username;

            if (line != NULL) free(line);
            line = NULL;
            len = 0;
            read = getline(&line, &len, stdin); //apparently (if not in while?) we have to free memory before every next getline (line)
            if (line != NULL) free(line);
            line = NULL;
            read = getline(&line, &len, stdin);

            uint32_t ind = -1;

            if (read < 1) {
                mode = 0;
                continue;
            } else if (read == 1) {
                username = clients[no_of_selected_client - 1].username;
                ind = no_of_selected_client - 1;
            } else {
                line[read - 1] = 0;
                bool exists = false;

                for (int i = 0; i < clients_no && !exists; ++i) {
                    if (!strcmp(clients[i].username, line)) {
                        username = clients[i].username;
                        exists = true;
                        ind = i;
                    }
                }

                if (!exists) {
                    mode = 0;
                    continue;
                }
            }

            printf("Selected user: %s\n\n", username);
            printf("Type sum: ");
            
            int32_t sum;
            scanf("%d", &sum);

            printf("\nType date of first interval (DD.MM.YYYY): ");

            if (line != NULL) free(line);
            line = NULL;
            len = 0;
            getline(&line, &len, stdin);
            if (line != NULL) free(line);
            line = NULL;
            read = getline(&line, &len, stdin);

            char* date = line;
            if (!is_correct_date(date)) {
                mode = 0;
                continue;
            }

            char *daytok, *monthtok, *yeartok;
            daytok = strtok(date, ".");
            monthtok = strtok(NULL, ".");
            yeartok = strtok(NULL, "\n");

            char day[3], month[3], year[5];
            char zeros[4];
            for (int i = 0; i < 4; ++i) {
                zeros[i] = 0;
            }

            if (strlen(daytok) == 1) {
                sprintf(day, "0%s", daytok);
            } else {
                sprintf(day, "%s", daytok);
            }
            if (strlen(monthtok) == 1) {
                sprintf(month, "0%s", monthtok);
            } else {
                sprintf(month, "%s", monthtok);
            }

            for (int i = 0; i < 4 - strlen(yeartok); ++i) {
                zeros[i] = '0';
            }
            sprintf(year, "%s%s", zeros, yeartok);
            
            if (line != NULL) free(line);
            line = NULL;

            int procent;
            printf("\n\nType percent: ");
            scanf("%d", &procent);
            printf("\n");

            char *directory;
            int no = 0;

            if (dep == true) { //new deposit
                ++clients[ind].deposits;
                no = clients[ind].deposits;
                directory = "deposits";
            } else { //new credit
                ++clients[ind].credits;
                no = clients[ind].credits;
                directory = "credits";
            }
            
            char filename[strlen(username) + 20];
            sprintf(filename, "%s/%s_%d.txt", directory, username, no);
            
            FILE *fp;
            fp = fopen(filename, "w");
            if (fp == NULL) {
                puts("WTF");
            }
            
            fprintf(fp, "Name: %s\n", clients[ind].fullname);
            fprintf(fp, "Number: %d\n", ++unique_id);
            fprintf(fp, "Sum: %d\n", sum);
            fprintf(fp, "Date: %s.%s.%s\n", day, month, year);
            fprintf(fp, "Procent: %d\n", procent);

            struct passwd *pwd = getpwnam(username);
            chown(filename, pwd->pw_uid, -1);

            fclose(fp);
            
            printf("Operation finished successfully\n");

            a = 0;
            scanf("%d", &a);

            mode = 0;
        } else if (mode == 4) {
            if (no_of_selected_client == 0) {
                mode = 0;
                continue;
            }

            bool closed_c[clients[no_of_selected_client - 1].credits + 1];
            bool closed_d[clients[no_of_selected_client - 1].deposits + 1];
            node_t *iter = NULL;

            printf("Client %s (%s)\n\n", clients[no_of_selected_client - 1].fullname, clients[no_of_selected_client - 1].username);
            printf("Opened depostits:\n");
            for (int i = 1; i <= clients[no_of_selected_client - 1].deposits; ++i) {
                closed_d[i] = false;
            }
            iter = closed_deposits[no_of_selected_client - 1];
            while (iter != NULL) {
                closed_d[iter->val] = true;
                iter = iter->next;
            }
            for (int i = 1; i <= clients[no_of_selected_client - 1].deposits; ++i) {
                if (closed_d[i]) {
                    continue;
                }

                printf("%dD\n", i);
            }

            printf("Opened credits:\n");
            for (int i = 1; i <= clients[no_of_selected_client - 1].credits; ++i) {
                closed_c[i] = false;
            }
            iter = closed_credits[no_of_selected_client - 1];
            while (iter != NULL) {
                closed_c[iter->val] = true;
                iter = iter->next;
            }
            for (int i = 1; i <= clients[no_of_selected_client - 1].credits; ++i) {
                if (closed_c[i]) {
                    continue;
                }

                printf("%dC\n", i);
            }

            printf("\nIf you want to check details use option 2 from main menu\n");
            printf("Type code of deposit or credit that you want to modify: ");

            char code[64];
            scanf("%s", code);
            char c = code[strlen(code) - 1];
            code[strlen(code) - 1] = 0;
            int no = atoi(code);

            char* username = clients[no_of_selected_client - 1].username;
            char* directory;
            char filename[strlen(username) + 20];

            if (c == 'D') {
                if (no < 1 || clients[no_of_selected_client - 1].deposits < no || closed_d[no] == true) {
                    mode = 0;
                    continue;
                }
                directory = "deposits";
            } else if (c == 'C') {
                if (no < 1 || clients[no_of_selected_client - 1].credits < no || closed_c[no] == true) {
                    mode = 0;
                    continue;
                }
                directory = "credits";
            } else {
                mode = 0;
                continue;
            }

            sprintf(filename, "%s/%s_%d.txt", directory, username, no);

            printf("\n1. Add \"Sum\" and \"Procent\" records\n");
            printf("2. Add end-date, start-date and \"Procent\" records\n");
            printf("3. Add end-date record (close)\n");

            int a;
            scanf("%d", &a);

            if (a < 1 && 3 < a) {
                mode = 0;
                continue;
            }

            if (a == 1) {
                printf ("\nType sum: ");
                int sum;
                scanf("%d", &sum);

                printf("\nType procent: ");
                int procent;
                scanf("%d", &procent);

                printf("\nType date of new record (DD.MM.YYYY): ");
                if (line != NULL) free(line);
                line = NULL;
                len = 0;
                getline(&line, &len, stdin);
                if (line != NULL) free(line);
                line = NULL;
                read = getline(&line, &len, stdin);
                
                char* date = line;
                if (!is_correct_date(date)) {
                    mode = 0;
                    continue;
                }

                char *daytok, *monthtok, *yeartok; 
                daytok = strtok(date, ".");
                monthtok = strtok(NULL, ".");
                yeartok = strtok(NULL, "\n");
                
                char day[3], month[3], year[5]; //such way of "increasing numbers length" is the only one not
                char zeros[4];                  //creating memory leaks or conditional jumps/invalid reads
                for (int i = 0; i < 4; ++i) {
                    zeros[i] = 0;
                }

                if (strlen(daytok) == 1) {
                    sprintf(day, "0%s", daytok);
                } else {
                    sprintf(day, "%s", daytok);
                }
                if (strlen(monthtok) == 1) {
                    sprintf(month, "0%s", monthtok);
                } else {
                    sprintf(month, "%s", monthtok);
                }

                for (int i = 0; i < 4 - strlen(yeartok); ++i) {
                    zeros[i] = '0';
                }
                sprintf(year, "%s%s", zeros, yeartok);
                
                FILE * fp;
                fp = fopen(filename, "a+");

                if (line != NULL) free(line);
                line = NULL;
                getline(&line, &len, fp); //name
                if (line != NULL) free(line);
                line = NULL;
                getline(&line, &len, fp); //number
                if (line != NULL) free(line);
                line = NULL;
                getline(&line, &len, fp); //sum
                char last_date_record[20];
                if (line != NULL) free(line);
                line = NULL;

                while ((read = getline(&line, &len, fp)) != -1) { //date
                    strcpy(last_date_record, line);
                    if (line != NULL) free(line);
                    line = NULL;
                    read = getline(&line, &len, fp);              //sum or procent
                    if (line != NULL) free(line);
                    line = NULL;
                }
                
                char *last_date = strtok(last_date_record, " ");

                char *last_daytok, *last_monthtok, *last_yeartok;
                last_daytok = strtok(NULL, ".");
                last_monthtok = strtok(NULL, ".");
                last_yeartok = strtok(NULL, "\n");

                char last_day[3], last_month[3], last_year[5];
                for (int i = 0; i < 4; ++i) {
                    zeros[i] = 0;
                }

                if (strlen(last_daytok) == 1) {
                    sprintf(last_day, "0%s", last_daytok);
                } else {
                    sprintf(last_day, "%s", last_daytok);
                }
                if (strlen(last_monthtok) == 1) {
                    sprintf(last_month, "0%s", last_monthtok);
                } else {
                    sprintf(last_month, "%s", last_monthtok);
                }

                for (int i = 0; i < 4 - strlen(last_yeartok); ++i) {
                    zeros[i] = '0';
                }
                sprintf(last_year, "%s%s", zeros, last_yeartok);
                if (!is_date_later(day, month, year, last_day, last_month, last_year)) {
                    mode = 0;
                    fclose(fp);
                    continue;
                }
                
                fprintf(fp, "Date: %s.%s.%s\n", day, month, year);
                fprintf(fp, "Sum: %d\n", sum);
                fprintf(fp, "Date: %s.%s.%s\n", day, month, year);
                fprintf(fp, "Procent: %d\n", procent);

                fclose(fp);

            } else if (a == 2) {
                printf("\nType procent: ");
                int procent;
                scanf("%d", &procent);

                printf("\nType date of new record (DD.MM.YYYY): ");
                if (line != NULL) free(line);
                line = NULL;
                len = 0;
                getline(&line, &len, stdin);
                if (line != NULL) free(line);
                line = NULL;
                read = getline(&line, &len, stdin);

                char* date = line;
                if (!is_correct_date(date)) {
                    mode = 0;
                    continue;
                }

                char *daytok, *monthtok, *yeartok;
                daytok = strtok(date, ".");
                monthtok = strtok(NULL, ".");
                yeartok = strtok(NULL, "\n");

                char day[3], month[3], year[5];
                char zeros[4];
                for (int i = 0; i < 4; ++i) {
                    zeros[i] = 0;
                }

                if (strlen(daytok) == 1) {
                    sprintf(day, "0%s", daytok);
                } else {
                    sprintf(day, "%s", daytok);
                }
                if (strlen(monthtok) == 1) {
                    sprintf(month, "0%s", monthtok);
                } else {
                    sprintf(month, "%s", monthtok);
                }

                for (int i = 0; i < 4 - strlen(yeartok); ++i) {
                    zeros[i] = '0';
                }
                sprintf(year, "%s%s", zeros, yeartok);
                
                FILE * fp;
                fp = fopen(filename, "a+");

                if (line != NULL) free(line);
                line = NULL;
                read = getline(&line, &len, fp); //name
                if (line != NULL) free(line);
                line = NULL;
                read = getline(&line, &len, fp); //number
                if (line != NULL) free(line);
                line = NULL;
                read = getline(&line, &len, fp); //sum

                char last_date_record[20];

                if (line != NULL) free(line);
                line = NULL;

                while ((read = getline(&line, &len, fp)) != -1) { //date
                    strcpy(last_date_record, line);
                    if (line != NULL) free(line);
                    line = NULL;
                    read = getline(&line, &len, fp);              //sum or procent
                    if (line != NULL) free(line);
                    line = NULL;
                }

                char *last_date = strtok(last_date_record, " ");

                char *last_daytok, *last_monthtok, *last_yeartok;
                last_daytok = strtok(NULL, ".");
                last_monthtok = strtok(NULL, ".");
                last_yeartok = strtok(NULL, "\n");

                char last_day[3], last_month[3], last_year[5];
                for (int i = 0; i < 4; ++i) {
                    zeros[i] = 0;
                }

                if (strlen(last_daytok) == 1) {
                    sprintf(last_day, "0%s", last_daytok);
                } else {
                    sprintf(last_day, "%s", last_daytok);
                }
                if (strlen(last_monthtok) == 1) {
                    sprintf(last_month, "0%s", last_monthtok);
                } else {
                    sprintf(last_month, "%s", last_monthtok);
                }

                for (int i = 0; i < 4 - strlen(last_yeartok); ++i) {
                    zeros[i] = '0';
                }
                sprintf(last_year, "%s%s", zeros, last_yeartok);

                if (!is_date_later(day, month, year, last_day, last_month, last_year)) {
                    mode = 0;
                    fclose(fp);
                    continue;
                }

                fprintf(fp, "Date: %s.%s.%s\n", day, month, year);
                fprintf(fp, "Procent: %d\n", procent);

                fclose(fp);
            } else if (a == 3) {
                printf("\nType date of last record (DD.MM.YYYY): ");
                if (line != NULL) free(line);
                line = NULL;
                len = 0;
                getline(&line, &len, stdin);
                if (line != NULL) free(line);
                line = NULL;
                read = getline(&line, &len, stdin);

                char* date = line;
                if (!is_correct_date(date)) {
                    mode = 0;
                    continue;
                }

                char *daytok, *monthtok, *yeartok;
                daytok = strtok(date, ".");
                monthtok = strtok(NULL, ".");
                yeartok = strtok(NULL, "\n");

                char day[3], month[3], year[5];
                char zeros[4];
                for (int i = 0; i < 4; ++i) {
                    zeros[i] = 0;
                }

                if (strlen(daytok) == 1) {
                    sprintf(day, "0%s", daytok);
                } else {
                    sprintf(day, "%s", daytok);
                }
                if (strlen(monthtok) == 1) {
                    sprintf(month, "0%s", monthtok);
                } else {
                    sprintf(month, "%s", monthtok);
                }

                for (int i = 0; i < 4 - strlen(yeartok); ++i) {
                    zeros[i] = '0';
                }
                sprintf(year, "%s%s", zeros, yeartok);
                
                FILE * fp;
                fp = fopen(filename, "a+");
    
                if (line != NULL) free(line);
                line = NULL;
                read = getline(&line, &len, fp); //name
                if (line != NULL) free(line);
                line = NULL;
                read = getline(&line, &len, fp); //number
                if (line != NULL) free(line);
                line = NULL;
                read = getline(&line, &len, fp); //sum

                char last_date_record[20];

                if (line != NULL) free(line);
                line = NULL;

                while ((read = getline(&line, &len, fp)) != -1) { //date
                    strcpy(last_date_record, line);
                    if (line != NULL) free(line);
                    line = NULL;
                    read = getline(&line, &len, fp);              //sum or procent
                    if (line != NULL) free(line);
                    line = NULL;
                }

                char *last_date = strtok(last_date_record, " ");

                char *last_daytok, *last_monthtok, *last_yeartok;
                last_daytok = strtok(NULL, ".");
                last_monthtok = strtok(NULL, ".");
                last_yeartok = strtok(NULL, "\n");

                char last_day[3], last_month[3], last_year[5];
                for (int i = 0; i < 4; ++i) {
                    zeros[i] = 0;
                }

                if (strlen(last_daytok) == 1) {
                    sprintf(last_day, "0%s", last_daytok);
                } else {
                    sprintf(last_day, "%s", last_daytok);
                }
                if (strlen(last_monthtok) == 1) {
                    sprintf(last_month, "0%s", last_monthtok);
                } else {
                    sprintf(last_month, "%s", last_monthtok);
                }

                for (int i = 0; i < 4 - strlen(last_yeartok); ++i) {
                    zeros[i] = '0';
                }
                sprintf(last_year, "%s%s", zeros, last_yeartok);

                if (!is_date_later(day, month, year, last_day, last_month, last_year)) {
                    mode = 0;
                    fclose(fp);
                    continue;
                }

                if (c == 'D') {
                    add_node(&closed_deposits[no_of_selected_client - 1], no);
                } else {
                    add_node(&closed_credits[no_of_selected_client - 1], no);
                }

                fprintf(fp, "Date: %s.%s.%s\n", day, month, year);
                fclose(fp);
            }

            printf("Operation finished successfully\n");

            a = 0;
            scanf("%d", &a);

            mode = 0;
        } else if (mode == 5) {
            exit = true;
        } else {
            if (!no_of_selected_client) {
                printf("no client selected\n\n");
            } else {
                printf("chosen client %d - %s (%s)\n\n", no_of_selected_client, clients[no_of_selected_client - 1].fullname, clients[no_of_selected_client - 1].username);
            }
            printf("1. Choose client\n");
            printf("2. Show deposits and credits (selected client required)\n");
            printf("3. Add deposit/credit\n");
            printf("4. Modify deposit/credit\n");
            printf("5. Exit\n\n");

            int a;
            scanf("%d", &a);

            if (1 <= a && a <= 5) {
                mode = a;
            }
        }
    }

    for (int i = 0; i < c_cnt; ++i) {
        if (clients[i].username != NULL) {
            free(clients[i].username);
        }
        if (clients[i].fullname != NULL) {
            free(clients[i].fullname);
        }

        node_t *iter1, *iter2;
    
        iter1 = closed_deposits[i];
        while (iter1 != NULL) {
            iter2 = iter1->next;
            free(iter1);
            iter1 = iter2;
        }

        iter1 = closed_credits[i];
        while (iter1 != NULL) {
            iter2 = iter1->next;
            free(iter1);
            iter1 = iter2;
        }
        
    }
    for (int i = 0; i < o_cnt; ++i) {
        if (officers[i].username != NULL) {
            free(officers[i].username);
        }
        if (officers[i].fullname != NULL) {
            free(officers[i].fullname);
        }
    }


    if (line != NULL) free(line);

    return 0;
}