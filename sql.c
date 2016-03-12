//#include </mysql/mysql.h>
#include </usr/include/mysql/mysql.h>
//#include </usr/local/mysql/include/mysql.h>
//#include </usr/local/Cellar/mysql/5.7.11/include/mysql/plugin_auth_common.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
main() {
   MYSQL *conn;
   MYSQL_RES *res;
   MYSQL_ROW row;
   FILE *file;

   char *server = "sql3.freesqldatabase.com";
   char *user = "sql3110061";
   char *password = "ecsD2HSjvg"; /* set me first */
   char *database = "sql3110061";
   //int num_fields = mysql_num_fields(res);
   conn = mysql_init(NULL);
   /* Connect to database */
   if (!mysql_real_connect(conn, server,
         user, password, database, 0, NULL, 0)) {
      fprintf(stderr, "%s\n", mysql_error(conn));
      exit(1);
   }

   char select[200] = "SELECT * FROM UIDList LIMIT 1";
   char delete[200] = "DELETE FROM UIDList WHERE uid = '"; 
   char reggy[200] = "INSERT INTO AutoMail(`uid`,`registered`,`email`,`password`) VALUES("; 
   char data[200] = "INSERT INTO AutoMailData(`uid`,`weight`) VALUES(";
   char storage[37];  
   memset(storage, 0, sizeof(storage));   

   mysql_query(conn, select);
   res = mysql_store_result(conn);
   row = mysql_fetch_row(res);
   char buff[37];
   strcpy(buff, row[0]);
   strcat(buff, "\0");
   printf("%s \n", buff);
   //while((row = mysql_fetch_row(res)) != NULL)
   //printf("row%s \n", row[0]);

   file = fopen("uid.txt","r+");
   fscanf(file, "%s", storage);
   printf("%d \n", strlen(buff));
   printf("%d \n", strlen(storage));
   if (strlen(storage) < 1 ) {
     fputs(buff, file);
     //printf("%s \n", buff);
     fflush(file);
     strcat(delete, buff);
     strcat(delete, "'");
     //printf("%s", delete);
     mysql_query(conn, delete);
     strcat(reggy, "'");
     strcat(reggy, buff);
     strcat(reggy, "',0,'','')");
     //printf("%s", reggy);
     mysql_query(conn, reggy);
   } 

   char doubleToString[5];
   double value = 10.0;
   snprintf(doubleToString, sizeof(doubleToString), "%f", value);
   strcat(data, "'");
   strcat(data, buff); 
   strcat(data, "',");
   strcat(data, doubleToString);
   strcat(data, ")");
   printf("%s", data);
   mysql_query(conn, data);

   res = mysql_use_result(conn);
   /* output table name */
   printf("MySQL Tables in mysql database:\n");
   while ((row = mysql_fetch_row(res)) != NULL) {
      printf("%s \n", row[0]);
   }
   
   fclose(file);
   /* close connection */
   mysql_free_result(res);
   mysql_close(conn);
}
