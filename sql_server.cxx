#include <iostream>
#include <unistd.h>
#include <netdb.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <string>
#include <sstream>
#include <regex>
#include "Table.h"

//сервер для сети эвм

#define QLENGTH 100
//using namespace std;
THandle th;
int res;
std::string strr;
int WhereStr(std::string, std::string);
std::string Expr(THandle, std::string);
std::string LogicExpr (THandle, std::string);
static std::string Stack[100];
static int top = 0;
static void push (std::string x) {
  if (top == 100) throw "Stack is full.\n";
  Stack[top++] = x;
}
static std::string pop() {
  if (top == 0) throw "No elements.\n";
  return Stack[--top];
}

//ожидает подключения от клиента, после подключения клиент работает с ним

//мы передаём номер порта (argv[1])
int main(int argc, char const **argv) {
	int d, n, port, d1, sopt,fromlen;
	struct sockaddr_in sin, fromsin;
	if (argc != 2) {
		std::cerr << "Not right amount of parameters" << std::endl;
		return 1;
	}
//	gethostname (hostname, sizeof (hostname));
// прочитаем номер порта
	if ((sscanf(argv[1], "%d%n", &port, &n) != 1) || argv[1][n] || (port <= 0) || (port > 65535)) {
		std::cerr << "Bad port number: " << argv[1] << std::endl;
		return 1;
	}
// создаём сокет
	if ((d = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 1;
	}
// задаём режим сокета
	sopt = 1;
	if (setsockopt(d,SOL_SOCKET,SO_REUSEADDR,&sopt,sizeof(sopt))< 0){
		perror("setsockopt");
		return 1;
	}
// задаём адрес сокета
	sin.sin_family = AF_INET;
	sin.sin_port = htons(port);
	sin.sin_addr.s_addr = INADDR_ANY;
// привязываем сокет
	if (bind(d, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
		perror("bind");
		return 1;
	}
// включаем режим прослушивания запросов на сокет
	if (listen(d, 5) < 0) {
		perror("listen");
		return 1;
	}
// связываемся с клиентом через неименованный сокет с дескриптором d1;
// после установления связи адрес клиента содержится в структуре fromsin
	fromlen = sizeof fromsin;
	if((d1 = accept ( d, (struct sockaddr *)&fromsin, (socklen_t *)&fromlen)) < 0){
		perror("accept");
		return 1;
	}
// читаем сообщения клиента, пишем клиенту:
	FILE * fp;
	int c;
  char s [QLENGTH];
	fp = fdopen (d1, "r+");
	printf ("Waiting...\n");

  if (fgets (s, QLENGTH, fp) == NULL) {
    printf ("Empty string.\n");
    fputc ('0', fp);
    return 0;
  }
  //printf ("%s", s);
  switch (s[0]) {

    case '5': {
      //CREATE
      int count;
      std::string s1(s+1), name;
      std::istringstream query (s1);
      query >> name >> count;
      TableStruct ts;
      ts.numOfFields = count;
      ts.fieldsDef = new FieldDef [count];
      for (int i = 0; i < count; i++) {
        query >> ts.fieldsDef[i].name;
        std::string buf;
        query >> buf;
        if (buf == "LONG") {
          ts.fieldsDef[i].type = Long;
          ts.fieldsDef[i].len = sizeof(long);
        }
        if (buf == "TEXT") {
          ts.fieldsDef[i].type = Text;
          query >> ts.fieldsDef[i].len;
        }
      }
      if (createTable (name.c_str(), &ts) == OK) {
          fputc ('1', fp);
      }
      else {
          fputc ('0', fp);
          std::cerr << "Table is not created." << std::endl;
          break;
      }
      break;
    }

    case '6': {
      //DROP
      std::string s1(s+1), name;
      std::istringstream query (s1);
      query >> name;
      if (deleteTable(name.c_str()) != OK) {
        fputc ('0', fp);
        std::cerr << "Table is not deleted." << std::endl;
        break;
      }
      fputc('1', fp);
      break;
    }

    case '2': {
      //INSERT
      int count;  //количество полей
      std::string s1(s+1), name;
      std::istringstream query (s1);
      query >> name >> count;
      THandle th;       //туда записывается дескриптор для работы с таблицей
      if (openTable (name.c_str(), &th) != OK) {
        fputc ('0', fp);
        std::cerr << "Table is not opened." << std::endl;
        break;
      }
      //enum Errors createNew(THandle tableHandle);
      //enum Errors putTextNew(THandle tableHandle, char *fieldName, char *value);
      //enum Errors putLongNew(THandle tableHandle, char * fieldName, long value);
      //enum Errors insertzNew(THandle tableHandle);
      if (count != th->tableInfo.fieldNumber) {
        fputc ('0', fp);
        std::cerr << "Amount of fields is wrong according to the structure of table." << std::endl;
        break;
      }
      if (createNew (th) != OK) {
        fputc ('0', fp);
        std::cerr << "Record is not created." << std::endl;
        break;
      }
      for (int i = 0; i < count; i++) {
        std::string buf;
        query >> buf;   //тип
        if (buf == "LONG") {
          long value;
          query >> value; //величина
          if (putLongNew(th, th->pFieldStruct[i].fieldName, value) != OK) {
            fputc ('0', fp);
          }
        }
        if (buf == "TEXT") {
          std::string value;
          query >> value;
          if (putTextNew(th, th->pFieldStruct[i].fieldName, value.c_str()) != OK) {
            fputc ('0', fp);
          }
        }
      }
      if (insertzNew (th) != OK) {
          fputc ('0', fp);
          std::cerr << "New field is not inserted." << std::endl;
          break;
      }
      if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
      }
      fputc('1', fp);
      break;
    }

    case '1': {
      //SELECT
      int count;  //количество полей
      std::string s1(s+1), name;
      std::istringstream query (s1);
      query >> name;
      //THandle th;       //туда записывается дескриптор для работы с таблицей
      unlink("tmp1");
      if (openTable (name.c_str(), &th) != OK) {
        fputc ('0', fp);
        std::cerr << "Table is not opened." << std::endl;
        break;
      }
      std::string buf, buf1;
      query >> buf;
      if (buf == "*") {
      	query >> buf1;
        if (buf1 == "3") {
        	link (name.c_str(), "tmp1");
          fputc('1', fp);
          break;
        }
      	//создать файл tmp1 с таким же заголовком таблицы но в него войду
	      //не все строки, а только те, которые удовл условию
				if (buf1 == "1") {
          std::string fieldname, buf2, str;
          query >> fieldname;
          query >> buf2;
          //в buf2 либо LIKE либо NOTLIKE
          std::string strexample;
          query >> strexample;
          unsigned count;
          if (getFieldsNum(th, &count) != OK) {
            fputc ('0', fp);
            std::cerr << "Amount of field is not accessible." << std::endl;
            break;
          }
          TableStruct ts;
          ts.numOfFields = count;
          ts.fieldsDef = new FieldDef [count];
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            enum FieldType ft;
            strcpy (ts.fieldsDef[i].name, name);
            if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
              fputc ('0', fp);
              std::cerr << "Type is not accessible." << std::endl;
              break;
            }
            if (ft == Long) {
              ts.fieldsDef[i].type = Long;
              ts.fieldsDef[i].len = sizeof(long);
            }
            if (ft == Text) {
              ts.fieldsDef[i].type = Text;
              unsigned lnght;
              getFieldLen(th, ts.fieldsDef[i].name, &lnght);
              ts.fieldsDef[i].len = lnght;
            }
          }
          if (createTable ("tmp1", &ts) != OK) {
            fputc('0', fp);
            std::cerr << "Table is not created." << std::endl;
            break;
          }
          THandle th1;
          if (openTable ("tmp1", &th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not opened." << std::endl;
            break;
          }
          if (moveFirst (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "No record found." << std::endl;
            break;
          }
          do {
            if (createNew (th1) != OK) {
              fputc ('0', fp);
              std::cerr << "Record is not created." << std::endl;
              break;
            }
            for (int i = 0; i < count; i++) {
              char *name;
              if (getFieldName (th, i, &name) != OK) {
                fputc ('0', fp);
                std::cerr << "Field is not accessible." << std::endl;
                break;
              }
              for (int j = 0; j < count; j++) {
                if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                  enum FieldType ft;
                  if (getFieldType (th, name, &ft) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Type is not accessible." << std::endl;
                      break;
                    }
                  if (ft == Long) {
                    long value;
                    //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                    if (getLong (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    char *value;
                    //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                    if (getText (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                     }
                    if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  break;
                }
              }
              if (name == fieldname) {
                 enum FieldType ft;
                 if (getFieldType (th, name, &ft) != OK) {
                   fputc ('0', fp);
                   std::cerr << "Type is not accessible." << std::endl;
                   break;
                 }
                 if (ft == Long) {
                   long value;
                   if (getLong (th, name, &value) != OK) {
                     fputc ('0', fp);
                     std::cerr << "Record is not accessible." << std::endl;
                     break;
                   }
                   strr = std::to_string(value);
                 }
                 else if (ft == Text) {
                   char *value;
                   if (getText (th, name, &value) != OK) {
                     fputc ('0', fp);
                     std::cerr << "Record is not accessible." << std::endl;
                     break;
                    }
                    strr = value;
                   strr = "'" + strr + "'";
                 }
                 //std::cerr << "STR: "<< strr << std::endl;
                 //std::cerr << "STREXAMPLE: "<< strexample << std::endl;
                 res = (WhereStr(strr, strexample) && (buf2 == "LIKE")) || (!WhereStr(strr, strexample) && (buf2 == "NOTLIKE"));
                 //std::cerr << "Res: "<< res << std::endl;
               }
            }
            if (res){
              if (insertzNew (th1) != OK) {
                fputc ('0', fp);
                std::cerr << "New field is not inserted." << std::endl;
                break;
              }
            }
            moveNext (th);
          }
          while (afterLast(th) != TRUE);
          if (closeTable(th) != OK) {
             fputc ('0', fp);
             std::cerr << "Table is not closed." << std::endl;
             break;
           }
          if (closeTable(th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not closed." << std::endl;
            break;
          }
          fputc('1', fp);
					break;
				}
				if (buf1 == "2") {
          std::string ex, t;
          //ex = query.getline();
          std::getline(query, ex, '$');
          ex = ex + " $";
          query >> t;
          if ((t != "IN") && (t != "NOTIN")) {
            fputc ('0', fp);
          	std::cerr << "IN or NOT IN is expected." << std::endl;
            break;
          }
          std::string str;
          query >> str;       //количество констант
          std::string type;
          query >> type;      //либо STRING, либо NUMBER
          int amount = atoi(str.c_str());
          std::string cnst[amount];
          for (int i = 0; i < amount; i++) {
            query >> cnst[i];
          }
          unsigned count;
          if (getFieldsNum(th, &count) != OK) {
            fputc ('0', fp);
            std::cerr << "Amount of field is not accessible." << std::endl;
            break;
          }
          TableStruct ts;
          ts.numOfFields = count;
          ts.fieldsDef = new FieldDef [count];
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            enum FieldType ft;
            strcpy (ts.fieldsDef[i].name, name);
            if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
              fputc ('0', fp);
              std::cerr << "Type is not accessible." << std::endl;
              break;
            }
            if (ft == Long) {
              ts.fieldsDef[i].type = Long;
              ts.fieldsDef[i].len = sizeof(long);
            }
            if (ft == Text) {
              ts.fieldsDef[i].type = Text;
              unsigned lnght;
              getFieldLen(th, ts.fieldsDef[i].name, &lnght);
              ts.fieldsDef[i].len = lnght;
            }
          }
          if (createTable ("tmp1", &ts) != OK) {
            fputc('0', fp);
            std::cerr << "Table is not created." << std::endl;
            break;
          }
          THandle th1;
          if (openTable ("tmp1", &th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not opened." << std::endl;
            break;
          }
          if (moveFirst (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "No record found." << std::endl;
            break;
          }
          do {
            if (createNew (th1) != OK) {
              fputc ('0', fp);
              std::cerr << "Record is not created." << std::endl;
              break;
            }
            std::string result;
            if (type == "NUMBER") result = Expr(th, ex);
            else {
              char *value;
              //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
              if (getText (th, ex.c_str(), &value) != OK) {
                fputc ('0', fp);
                std::cerr << "Record is not accessible." << std::endl;
                break;
               }
              result = std::string(value);
            }
            //std::cerr << "EXPRESSION: "<< result << std::endl;
            int flag = 0;
            for (int i = 0; i < amount; i++) {
              if (result == cnst[i]) flag = 1;
            }
            //std::cerr << "FLAG: "<< flag << std::endl;
            if (!flag) {
              moveNext (th);
              continue;
            }
            for (int i = 0; i < count; i++) {
              char *name;
              if (getFieldName (th, i, &name) != OK) {
                fputc ('0', fp);
                std::cerr << "Field is not accessible." << std::endl;
                break;
              }
              for (int j = 0; j < count; j++) {
                //std::cerr <<"***" <<name << "***"<<ts.fieldsDef[j].name <<"***\n";
                if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                  enum FieldType ft;
                  if (getFieldType (th, name, &ft) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Type is not accessible." << std::endl;
                    break;
                  }
                  if (ft == Long) {
                    long value;
                    //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                    if (getLong (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    char *value;
                    //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                    if (getText (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  break;
                }
             }
          }
            if (insertzNew (th1) != OK) {
              fputc ('0', fp);
              std::cerr << "New field is not inserted." << std::endl;
              break;
            }
            moveNext (th);
          }
          while (afterLast(th) != TRUE);
          if (closeTable(th) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not closed." << std::endl;
            break;
          }
          if (closeTable(th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not closed." << std::endl;
            break;
          }
          fputc('1', fp);
          break;
        }
			  if (buf1 == "4") {
          std::string ex, t;
          std::getline(query, ex, '$');
          ex = ex + " $";
          unsigned count;
          if (getFieldsNum(th, &count) != OK) {
            fputc ('0', fp);
            std::cerr << "Amount of field is not accessible." << std::endl;
            break;
          }
          TableStruct ts;
          ts.numOfFields = count;
          ts.fieldsDef = new FieldDef [count];
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            enum FieldType ft;
            strcpy (ts.fieldsDef[i].name, name);
            if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
              fputc ('0', fp);
              std::cerr << "Type is not accessible." << std::endl;
              break;
            }
            if (ft == Long) {
              ts.fieldsDef[i].type = Long;
              ts.fieldsDef[i].len = sizeof(long);
            }
            if (ft == Text) {
              ts.fieldsDef[i].type = Text;
              unsigned lnght;
              getFieldLen(th, ts.fieldsDef[i].name, &lnght);
              ts.fieldsDef[i].len = lnght;
            }
          }
          if (createTable ("tmp1", &ts) != OK) {
            fputc('0', fp);
            std::cerr << "Table is not created." << std::endl;
            break;
          }
          THandle th1;
          if (openTable ("tmp1", &th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not opened." << std::endl;
            break;
          }
          if (moveFirst (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "No record found." << std::endl;
            break;
          }
          do {
            if (createNew (th1) != OK) {
              fputc ('0', fp);
              std::cerr << "Record is not created." << std::endl;
              break;
            }
            //вычислить лог выражение для текущей строки таблицы
            std::string result;
            result = LogicExpr(th, ex);
            int flag = 0;
            if (result == "1") flag = 1;
            if (!flag) {
              moveNext (th);
              continue;
            }
            for (int i = 0; i < count; i++) {
              char *name;
              if (getFieldName (th, i, &name) != OK) {
                fputc ('0', fp);
                std::cerr << "Field is not accessible." << std::endl;
                break;
              }
              for (int j = 0; j < count; j++) {
                //std::cerr <<"***" <<name << "***"<<ts.fieldsDef[j].name <<"***\n";
                if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                  enum FieldType ft;
                  if (getFieldType (th, name, &ft) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Type is not accessible." << std::endl;
                    break;
                  }
                  if (ft == Long) {
                    long value;
                    //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                    if (getLong (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                    }
                  }
                  else if (ft == Text) {
                    char *value;
                    //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                    if (getText (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                    }
                  }
                  break;
                }
             }
            }
            if (insertzNew (th1) != OK) {
              fputc ('0', fp);
              std::cerr << "New field is not inserted." << std::endl;
              break;
            }
            moveNext (th);
          }
          while (afterLast(th) != TRUE);

          if (closeTable(th) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not closed." << std::endl;
            break;
          }
          if (closeTable(th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not closed." << std::endl;
            break;
          }
          fputc('1', fp);
          break;
        }
      }
        //создать файл tmp1 с новым заголовком и переписать туда данные
      if (buf == "3") {
        	int amount;
          query >> amount;
          TableStruct ts;
          ts.numOfFields = amount;
          ts.fieldsDef = new FieldDef [amount];
					for (int i = 0; i < amount; i++) {
          	query >> ts.fieldsDef[i].name;
            enum FieldType ft;
            if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
            	fputc ('0', fp);
              std::cerr << "Type is not accessible." << std::endl;
              break;
					  }
            if (ft == Long) {
              ts.fieldsDef[i].type = Long;
              ts.fieldsDef[i].len = sizeof(long);
            }
            if (ft == Text) {
              ts.fieldsDef[i].type = Text;
              unsigned lnght;
              getFieldLen(th, ts.fieldsDef[i].name, &lnght);
              ts.fieldsDef[i].len = lnght;
            }
          }
					if (createTable ("tmp1", &ts) != OK) {
          	fputc('0', fp);
            std::cerr << "Table is not created." << std::endl;
            break;
          }
					THandle th1;
          if (openTable ("tmp1", &th1) != OK) {
          	fputc ('0', fp);
            std::cerr << "Table is not opened." << std::endl;
            break;
          }
          if (moveFirst (th1) != OK) {
          	fputc ('0', fp);
            std::cerr << "No record found." << std::endl;
            break;
          }
          unsigned count;
          getFieldsNum(th, &count);
          do {
            if (createNew (th1) != OK) {
              fputc ('0', fp);
              std::cerr << "Record is not created." << std::endl;
              break;
            }
            for (int i = 0; i < count; i++) {
              char *name;
              if (getFieldName (th, i, &name) != OK) {
            	  fputc ('0', fp);
                std::cerr << "Field is not accessible." << std::endl;
                break;
              }
              for (int j = 0; j < amount; j++) {
             	  //std::cerr <<"***" <<name << "***"<<ts.fieldsDef[j].name <<"***\n";
            	  if (strcmp (name, ts.fieldsDef[j].name) == 0) {
              	  enum FieldType ft;
                  if (getFieldType (th, name, &ft) != OK) {
                	  fputc ('0', fp);
                    std::cerr << "Type is not accessible." << std::endl;
                    break;
  							  }
                  if (ft == Long) {
                	  long value;
                    //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                    if (getLong (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                  	  break;
                    }
                    if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                    }
          			  }
                  else if (ft == Text) {
                    char *value;
                    //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                    if (getText (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                     }
                     if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                  	   fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                    }
                  }
                break;
              }
              }
            }
            if (insertzNew (th1) != OK) {
            	fputc ('0', fp);
              std::cerr << "New field is not inserted." << std::endl;
              break;
            }
            moveNext (th);
          }
          while (afterLast(th) != TRUE);
          if (closeTable(th) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not closed." << std::endl;
            break;
          }
          if (closeTable(th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Table is not closed." << std::endl;
            break;
          }
          fputc('1', fp);
          break;
        }
			if (buf == "1") {
        std::string fieldname, buf2, str;
        query >> fieldname;
        query >> buf2;
        //в buf2 либо LIKE либо NOTLIKE
        std::string strexample;
        query >> strexample;
        int amount;
        query >> amount;
        TableStruct ts;
        ts.numOfFields = amount;
        ts.fieldsDef = new FieldDef [amount];
        for (int i = 0; i < amount; i++) {
          query >> ts.fieldsDef[i].name;
          enum FieldType ft;
          if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
            fputc ('0', fp);
            std::cerr << "Type is not accessible." << std::endl;
            break;
          }
          if (ft == Long) {
            ts.fieldsDef[i].type = Long;
            ts.fieldsDef[i].len = sizeof(long);
          }
          if (ft == Text) {
            ts.fieldsDef[i].type = Text;
            unsigned lnght;
            getFieldLen(th, ts.fieldsDef[i].name, &lnght);
            ts.fieldsDef[i].len = lnght;
          }
        }
        if (createTable ("tmp1", &ts) != OK) {
          fputc('0', fp);
          std::cerr << "Table is not created." << std::endl;
          break;
        }
        THandle th1;
        if (openTable ("tmp1", &th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not opened." << std::endl;
          break;
        }
        if (moveFirst (th1) != OK) {
          fputc ('0', fp);
          std::cerr << "No record found." << std::endl;
          break;
        }
        unsigned count;
        getFieldsNum(th, &count);
        do {
          if (createNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Record is not created." << std::endl;
            break;
          }
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            for (int j = 0; j < amount; j++) {
              //std::cerr <<"***" <<name << "***"<<ts.fieldsDef[j].name <<"***\n";
              if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                enum FieldType ft;
                if (getFieldType (th, name, &ft) != OK) {
                  fputc ('0', fp);
                  std::cerr << "Type is not accessible." << std::endl;
                  break;
                }
                if (ft == Long) {
                  long value;
                  //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                  if (getLong (th, name, &value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                    break;
                  }
                  if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                  }
                }
                else if (ft == Text) {
                  char *value;
                  //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                  if (getText (th, name, &value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                    break;
                   }
                  if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                     fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                  }
                }
                break;
              }
            }
            if (name == fieldname) {
              enum FieldType ft;
              if (getFieldType (th, name, &ft) != OK) {
                fputc ('0', fp);
                std::cerr << "Type is not accessible." << std::endl;
                break;
              }
              if (ft == Long) {
                long value;
                if (getLong (th, name, &value) != OK) {
                  fputc ('0', fp);
                  std::cerr << "Record is not accessible." << std::endl;
                  break;
                }
                strr = std::to_string(value);
              }
              else if (ft == Text) {
                char *value;
                if (getText (th, name, &value) != OK) {
                  fputc ('0', fp);
                  std::cerr << "Record is not accessible." << std::endl;
                  break;
                 }
                 strr = value;
                strr = "'" + strr + "'";
              }
              //std::cerr << "STR: "<< strr << std::endl;
              //std::cerr << "STREXAMPLE: "<< strexample << std::endl;
              res = (WhereStr(strr, strexample) && (buf2 == "LIKE")) || (!WhereStr(strr, strexample) && (buf2 == "NOTLIKE"));
              //std::cerr << "Res: "<< res << std::endl;
            }
          }
          if (res){
            if (insertzNew (th1) != OK) {
              fputc ('0', fp);
              std::cerr << "New field is not inserted." << std::endl;
              break;
            }
          }
          moveNext (th);
        }
        while (afterLast(th) != TRUE);

        if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        if (closeTable(th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        fputc('1', fp);
        break;
      }
      if (buf == "2") {
        std::string ex, t;
        std::getline(query, ex, '$');
        ex = ex + " $";
        query >> t;
        if ((t != "IN") && (t != "NOTIN")) {
          fputc ('0', fp);
          std::cerr << "IN or NOT IN is expected." << std::endl;
          break;
        }
        std::string str;
        query >> str;       //количество констант
        std::string type;
        query >> type;      //либо STRING, либо NUMBER
        int amount = atoi(str.c_str());
        std::string cnst[amount];
        for (int i = 0; i < amount; i++) {
          query >> cnst[i];
        }
        int num;
        query >> num;  // число полей во временной таблице
        unsigned count;
        if (getFieldsNum(th, &count) != OK) {
          fputc ('0', fp);
          std::cerr << "Amount of field is not accessible." << std::endl;
          break;
        }
        TableStruct ts;
        ts.numOfFields = num;
        ts.fieldsDef = new FieldDef [num];
        for (int i = 0; i < num; i++) {
          query >> ts.fieldsDef[i].name;
          enum FieldType ft;
          if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
            fputc ('0', fp);
            std::cerr << "Type is not accessible." << std::endl;
            break;
          }
          if (ft == Long) {
            ts.fieldsDef[i].type = Long;
            ts.fieldsDef[i].len = sizeof(long);
          }
          if (ft == Text) {
            ts.fieldsDef[i].type = Text;
            unsigned lnght;
            getFieldLen(th, ts.fieldsDef[i].name, &lnght);
            ts.fieldsDef[i].len = lnght;
          }
        }
        if (createTable ("tmp1", &ts) != OK) {
          fputc('0', fp);
          std::cerr << "Table is not created." << std::endl;
          break;
        }
        THandle th1;
        if (openTable ("tmp1", &th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not opened." << std::endl;
          break;
        }
        if (moveFirst (th1) != OK) {
          fputc ('0', fp);
          std::cerr << "No record found." << std::endl;
          break;
        }
        do {
          if (createNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Record is not created." << std::endl;
            break;
          }
          //вычислить варажение для текущей строки таблицы
          //std::string result = Expr(th, ex);
          std::string result;
          if (type == "NUMBER") result = Expr(th, ex);
          else {
            char *value;
            //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
            if (getText (th, ex.c_str(), &value) != OK) {
              fputc ('0', fp);
              std::cerr << "Record is not accessible." << std::endl;
              break;
             }
            result = std::string(value);
          }
          //std::cerr << "EXPRESSION: "<< result << std::endl;
          int flag = 0;
          for (int i = 0; i < amount; i++) {
            if (result == cnst[i]) flag = 1;
          }
          //std::cerr << "FLAG: "<< flag << std::endl;
          if (!flag) {
            moveNext (th);
            continue;
          }
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            for (int j = 0; j < num; j++) {
              if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                enum FieldType ft;
                if (getFieldType (th, name, &ft) != OK) {
                  fputc ('0', fp);
                  std::cerr << "Type is not accessible." << std::endl;
                  break;
                }
                if (ft == Long) {
                  long value;
                  //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                  if (getLong (th, name, &value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                    break;
                  }
                  if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                  }
                }
                else if (ft == Text) {
                  char *value;
                  //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                  if (getText (th, name, &value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                    break;
                  }
                  if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                  }
                }
                break;
              }
            }
          }
          if (insertzNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "New field is not inserted." << std::endl;
            break;
          }
          moveNext (th);
        }
        while (afterLast(th) != TRUE);

        if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        if (closeTable(th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        fputc('1', fp);
        break;
      }
		  if (buf == "4") {
        std::string ex, t;
        std::getline(query, ex, '$');
        ex = ex + " $";
        int num;
        query >> num;  // число полей во временной таблице
        unsigned count;
        if (getFieldsNum(th, &count) != OK) {
          fputc ('0', fp);
          std::cerr << "Amount of field is not accessible." << std::endl;
          break;
        }
        TableStruct ts;
        ts.numOfFields = num;
        ts.fieldsDef = new FieldDef [num];
        for (int i = 0; i < num; i++) {
          query >> ts.fieldsDef[i].name;
          enum FieldType ft;
          if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
            fputc ('0', fp);
            std::cerr << "Type is not accessible." << std::endl;
            break;
          }
          if (ft == Long) {
            ts.fieldsDef[i].type = Long;
            ts.fieldsDef[i].len = sizeof(long);
          }
          if (ft == Text) {
            ts.fieldsDef[i].type = Text;
            unsigned lnght;
            getFieldLen(th, ts.fieldsDef[i].name, &lnght);
            ts.fieldsDef[i].len = lnght;
          }
        }
        if (createTable ("tmp1", &ts) != OK) {
          fputc('0', fp);
          std::cerr << "Table is not created." << std::endl;
          break;
        }
        THandle th1;
        if (openTable ("tmp1", &th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not opened." << std::endl;
          break;
        }
        if (moveFirst (th1) != OK) {
          fputc ('0', fp);
          std::cerr << "No record found." << std::endl;
          break;
        }
        do {
          if (createNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Record is not created." << std::endl;
            break;
          }
          //вычислить лог выражение для текущей строки таблицы
          std::string result;
          result = LogicExpr(th, ex);
          int flag = 0;
          if (result == "1") flag = 1;
          if (!flag) {
            moveNext (th);
            continue;
          }
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            for (int j = 0; j < num; j++) {
              //std::cerr <<"***" <<name << "***"<<ts.fieldsDef[j].name <<"***\n";
              if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                enum FieldType ft;
                if (getFieldType (th, name, &ft) != OK) {
                  fputc ('0', fp);
                  std::cerr << "Type is not accessible." << std::endl;
                  break;
                }
                if (ft == Long) {
                  long value;
                  //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                  if (getLong (th, name, &value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                    break;
                  }
                  if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                  }
                }
                else if (ft == Text) {
                  char *value;
                  //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                  if (getText (th, name, &value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                    break;
                  }
                  if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Record is not accessible." << std::endl;
                  }
                }
                break;
              }
            }
          }
          if (insertzNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "New field is not inserted." << std::endl;
            break;
          }
          moveNext (th);
        }
        while (afterLast(th) != TRUE);

        if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        if (closeTable(th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        fputc('1', fp);
        break;
      }
    }

    case '3': {
      //UPDATE
      unlink("tmp1");
      int wh;
      std::string s1(s+1), tname, fieldname, exp;
      std::istringstream query (s1);
      query >> tname >> wh;
      if (openTable(tname.c_str(), &th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not opened." << std::endl;
          break;
      }
      if (moveFirst (th) != OK) {
        fputc ('0', fp);
        std::cerr << "No record found." << std::endl;
        break;
      }
      unsigned count;
      if (getFieldsNum(th, &count) != OK) {
        fputc ('0', fp);
        std::cerr << "Amount of field is not accessible." << std::endl;
        break;
      }
      switch(wh) {
      case 1: {
        std::string fieldname, buf2, str, fieldlike, ex;
        query >> fieldlike;
        query >> buf2;
        //в buf2 либо LIKE либо NOTLIKE
        std::string strexample;
        query >> strexample;
        query >> fieldname;
        std::getline(query, ex, '$');
        //ex = ex + " $";
        query.get();
        unsigned count;
        if (getFieldsNum(th, &count) != OK) {
          fputc ('0', fp);
          std::cerr << "Amount of field is not accessible." << std::endl;
          break;
        }
        TableStruct ts;
        ts.numOfFields = count;
        ts.fieldsDef = new FieldDef [count];
        for (int i = 0; i < count; i++) {
          char *name;
          if (getFieldName (th, i, &name) != OK) {
            fputc ('0', fp);
            std::cerr << "Field is not accessible." << std::endl;
            break;
          }
          enum FieldType ft;
          strcpy (ts.fieldsDef[i].name, name);
          if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
            fputc ('0', fp);
            std::cerr << "Type is not accessible." << std::endl;
            break;
          }
          if (ft == Long) {
            ts.fieldsDef[i].type = Long;
            ts.fieldsDef[i].len = sizeof(long);
          }
          if (ft == Text) {
            ts.fieldsDef[i].type = Text;
            unsigned lnght;
            getFieldLen(th, ts.fieldsDef[i].name, &lnght);
            ts.fieldsDef[i].len = lnght;
          }
        }
        if (createTable ("tmp1", &ts) != OK) {
          fputc('0', fp);
          std::cerr << "Table is not created." << std::endl;
          break;
        }
        THandle th1;
        if (openTable ("tmp1", &th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not opened." << std::endl;
          break;
        }
        if (moveFirst (th1) != OK) {
          fputc ('0', fp);
          std::cerr << "No record found." << std::endl;
          break;
        }
        do {
          if (createNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Record is not created." << std::endl;
            break;
          }
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            for (int j = 0; j < count; j++) {
              if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                enum FieldType ft;
                if (getFieldType (th, name, &ft) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Type is not accessible." << std::endl;
                    break;
                }
                if (name == fieldlike) {
                   enum FieldType ft;
                   if (getFieldType (th, name, &ft) != OK) {
                     fputc ('0', fp);
                     std::cerr << "Type is not accessible." << std::endl;
                     break;
                   }
                   if (ft == Long) {
                     long value;
                     if (getLong (th, name, &value) != OK) {
                       fputc ('0', fp);
                       std::cerr << "Record is not accessible." << std::endl;
                       break;
                     }
                     strr = std::to_string(value);
                   }
                   else if (ft == Text) {
                     char *value;
                     if (getText (th, name, &value) != OK) {
                       fputc ('0', fp);
                       std::cerr << "Record is not accessible." << std::endl;
                       break;
                      }
                      strr = value;
                      strr = "'" + strr + "'";
                   }
                   //std::cerr << "STR: "<< strr << std::endl;
                   //std::cerr << "STREXAMPLE: "<< strexample << std::endl;
                   res = (WhereStr(strr, strexample) && (buf2 == "LIKE")) || (!WhereStr(strr, strexample) && (buf2 == "NOTLIKE"));
                   //std::cerr << "Res: "<< res << std::endl;
                 }
                 //movePrevios(THandle tableHandle)
                if ((name == fieldname) && (res)) {
                  if (ft == Long) {
                    //std::cerr << "надо заменять в Long\n";
                    //std::cerr<< "ex: " << ex << std::endl;
                    ex = ex + " $";
                    std::string result = Expr(th, ex);
                    //std::cerr << "Result:"<< result << std::endl;
                    if ( putLongNew (th1, th1->pFieldStruct[j].fieldName, atol(result.c_str())) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    //std::cerr << "надо заменять в Text\n";
                    char buf [MaxFieldNameLen];
                    //std::cerr << "TEXT: " <<ex << "STR: "<<(ex.c_str())+1<< "LENG: "<< strlen(ex.c_str())-2<< std::endl;
                    strncpy (buf,(ex.c_str())+1, strlen(ex.c_str())-2);
                    //std::cerr << "TEXT " <<buf<< std::endl;
                    if (putText (th1, th1->pFieldStruct[j].fieldName, buf) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                }
                else {
                  //std::cerr << "не надо заменять\n";
                  if (ft == Long) {
                    long value;
                    //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                    if (getLong (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    char *value;
                    //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                    if (getText (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                }
              }
            }
          }
          if (insertzNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "New field is not inserted." << std::endl;
            break;
          }
          moveNext (th);
        }
        while (afterLast(th) != TRUE);
        if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        if (closeTable(th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        fputc('1', fp);
        break;
      }
      case 2: {
        std::string ex, t, exin;
        std::getline(query, exin, '$');
        exin = exin + " $";
        //std::cerr << "exin: " << exin << std::endl;
        query >> t;
        if ((t != "IN") && (t != "NOTIN")) {
          fputc ('0', fp);
          std::cerr << "IN or NOT IN is expected." << std::endl;
          break;
        }
        std::string str;
        query >> str;       //количество констант
        std::string type;
        query >> type;      //либо STRING, либо NUMBER
        int amount = atoi(str.c_str());
        std::string cnst[amount];
        for (int i = 0; i < amount; i++) {
          query >> cnst[i];
        }
        query >> fieldname;
        std::getline(query, ex, '$');
        ex = ex + " $";
        unsigned count;
        if (getFieldsNum(th, &count) != OK) {
          fputc ('0', fp);
          std::cerr << "Amount of field is not accessible." << std::endl;
          break;
        }
        TableStruct ts;
        ts.numOfFields = count;
        ts.fieldsDef = new FieldDef [count];
        for (int i = 0; i < count; i++) {
          char *name;
          if (getFieldName (th, i, &name) != OK) {
            fputc ('0', fp);
            std::cerr << "Field is not accessible." << std::endl;
            break;
          }
          enum FieldType ft;
          strcpy (ts.fieldsDef[i].name, name);
          if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
            fputc ('0', fp);
            std::cerr << "Type is not accessible." << std::endl;
            break;
          }
          if (ft == Long) {
            ts.fieldsDef[i].type = Long;
            ts.fieldsDef[i].len = sizeof(long);
          }
          if (ft == Text) {
            ts.fieldsDef[i].type = Text;
            unsigned lnght;
            getFieldLen(th, ts.fieldsDef[i].name, &lnght);
            ts.fieldsDef[i].len = lnght;
          }
        }
        if (createTable ("tmp1", &ts) != OK) {
          fputc('0', fp);
          std::cerr << "Table is not created." << std::endl;
          break;
        }
        THandle th1;
        if (openTable ("tmp1", &th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not opened." << std::endl;
          break;
        }
        if (moveFirst (th1) != OK) {
          fputc ('0', fp);
          std::cerr << "No record found." << std::endl;
          break;
        }
        do {
          if (createNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Record is not created." << std::endl;
            break;
          }
          std::string result;
          if (type == "NUMBER") result = Expr(th, exin);
          else {
            char *value;
            //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
            if (getText (th, ex.c_str(), &value) != OK) {
              fputc ('0', fp);
              std::cerr << "Record is not accessible." << std::endl;
              break;
            }
            result = std::string(value);
          }
          //std::cerr << "EXPRESSION: "<< result << std::endl;
          int flag = 0;
          for (int i = 0; i < amount; i++) {
            if (result == cnst[i]) flag = 1;
          }
          //std::cerr << "FLAG: "<< flag << std::endl;
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            for (int j = 0; j < count; j++) {
              //std::cerr <<"***" <<name << "***"<<ts.fieldsDef[j].name <<"***\n";
              if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                enum FieldType ft;
                if (getFieldType (th, name, &ft) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Type is not accessible." << std::endl;
                    break;
                }
                if ((name == fieldname) && (flag)) {
                  //std::cerr << "надо заменять\n";
                  if (ft == Long) {
                    //std::cerr << "надо заменять в Long\n";
                    ex = ex + " $";
                    std::string res = Expr(th, ex);
                    //std::cerr << "Res:"<< res << std::endl;
                    if ( putLongNew (th1, th1->pFieldStruct[j].fieldName, atol(res.c_str())) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    //std::cerr << "надо заменять в Text\n";
                    char buf [MaxFieldNameLen];
                    //std::cerr << "TEXT: " <<ex << "STR: "<<(ex.c_str())+1<< "LENG: "<< strlen(ex.c_str())-2<< std::endl;
                    strncpy (buf,(ex.c_str())+1, strlen(ex.c_str())-2);
                    //std::cerr << "TEXT " <<buf<< std::endl;
                    if (putText (th1, th1->pFieldStruct[j].fieldName, buf) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                }
                else {
                  //std::cerr << "не надо заменять\n";
                  if (ft == Long) {
                    long value;
                    //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                    if (getLong (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    char *value;
                    //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                    if (getText (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                }
              }
            }
          }
          if (insertzNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "New field is not inserted." << std::endl;
            break;
          }
          moveNext (th);
        }
        while (afterLast(th) != TRUE);
        if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        if (closeTable(th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        fputc('1', fp);
        break;
      }
      case 3: {
        query >> fieldname;
        std::string ex, t;
        query.get();
        std::getline(query, ex, '$');
        std::cerr<< "ex: " << ex << std::endl;
        unsigned count;
        if (getFieldsNum(th, &count) != OK) {
          fputc ('0', fp);
          std::cerr << "Amount of field is not accessible." << std::endl;
          break;
        }
        TableStruct ts;
        ts.numOfFields = count;
        ts.fieldsDef = new FieldDef [count];
        for (int i = 0; i < count; i++) {
          char *name;
          if (getFieldName (th, i, &name) != OK) {
            fputc ('0', fp);
            std::cerr << "Field is not accessible." << std::endl;
            break;
          }
          enum FieldType ft;
          strcpy (ts.fieldsDef[i].name, name);
          if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
            fputc ('0', fp);
            std::cerr << "Type is not accessible." << std::endl;
            break;
          }
          if (ft == Long) {
            ts.fieldsDef[i].type = Long;
            ts.fieldsDef[i].len = sizeof(long);
          }
          if (ft == Text) {
            ts.fieldsDef[i].type = Text;
            unsigned lnght;
            getFieldLen(th, ts.fieldsDef[i].name, &lnght);
            ts.fieldsDef[i].len = lnght;
          }
        }
        if (createTable ("tmp1", &ts) != OK) {
          fputc('0', fp);
          std::cerr << "Table is not created." << std::endl;
          break;
        }
        THandle th1;
        if (openTable ("tmp1", &th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not opened." << std::endl;
          break;
        }
        if (moveFirst (th1) != OK) {
          fputc ('0', fp);
          std::cerr << "No record found." << std::endl;
          break;
        }
        do {
          if (createNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Record is not created." << std::endl;
            break;
          }
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            for (int j = 0; j < count; j++) {
              if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                enum FieldType ft;
                if (getFieldType (th, name, &ft) != OK) {
                    fputc ('0', fp);
                    std::cerr << "Type is not accessible." << std::endl;
                    break;
                }
                if (name == fieldname) {
                  //std::cerr << "надо заменять\n";
                  if (ft == Long) {
                    //std::cerr << "надо заменять в Long\n";
                    ex = ex + " $";
                    std::string res = Expr(th, ex);
                    //std::cerr << "Res:"<< res << std::endl;
                    if ( putLongNew (th1, th1->pFieldStruct[j].fieldName, atol(res.c_str())) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    //std::cerr << "надо заменять в Text\n";
                    char buf [MaxFieldNameLen];
                    //std::cerr << "TEXT: " <<ex << "STR: "<<(ex.c_str())+1<< "LENG: "<< strlen(ex.c_str())-2<< std::endl;
                    strncpy (buf,(ex.c_str())+1, strlen(ex.c_str())-2);
                    //std::cerr << "TEXT " <<buf<< std::endl;
                    if (putText (th1, th1->pFieldStruct[j].fieldName, buf) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                }
                else {
                  //std::cerr << "не надо заменять\n";
                  if (ft == Long) {
                    long value;
                    //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                    if (getLong (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    char *value;
                    //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                    if (getText (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                }
              }
            }
          }
          if (insertzNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "New field is not inserted." << std::endl;
            break;
          }
          moveNext (th);
        }
        while (afterLast(th) != TRUE);
        if (closeTable(th) != OK) {
           fputc ('0', fp);
           std::cerr << "Table is not closed." << std::endl;
           break;
        }
        if (closeTable(th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        /*
        if (unlink(tname.c_str()) == -1) {
          perror("unlink");
        }
        if (link("tmp1", tname.c_str()) == -1) {
          perror("link");
        }
        unlink("tmp1");
        */
        break;
      }
      case 4: {
        std::string logex, t, ex;
        query.get();
        query.get();
        std::getline(query, logex, '$');
        //std::cerr<< "logex: " << logex << std::endl;
        query.get();
        query >> fieldname;
        //std::cerr<< "fieldname: " << fieldname << std::endl;
        query.get();
        query.get();
        std::getline(query, ex, '$');
        query.get();
        //std::cerr<< "ex: " << ex << std::endl;
        unsigned count;
        if (getFieldsNum(th, &count) != OK) {
          fputc ('0', fp);
          std::cerr << "Amount of field is not accessible." << std::endl;
          break;
        }
        TableStruct ts;
        ts.numOfFields = count;
        ts.fieldsDef = new FieldDef [count];
        for (int i = 0; i < count; i++) {
          char *name;
          if (getFieldName (th, i, &name) != OK) {
            fputc ('0', fp);
            std::cerr << "Field is not accessible." << std::endl;
            break;
          }
          enum FieldType ft;
          strcpy (ts.fieldsDef[i].name, name);
          if (getFieldType (th, ts.fieldsDef[i].name, &ft) != OK) {
            fputc ('0', fp);
            std::cerr << "Type is not accessible." << std::endl;
            break;
          }
          if (ft == Long) {
            ts.fieldsDef[i].type = Long;
            ts.fieldsDef[i].len = sizeof(long);
          }
          if (ft == Text) {
            ts.fieldsDef[i].type = Text;
            unsigned lnght;
            getFieldLen(th, ts.fieldsDef[i].name, &lnght);
            ts.fieldsDef[i].len = lnght;
          }
        }
        if (createTable ("tmp1", &ts) != OK) {
          fputc('0', fp);
          std::cerr << "Table is not created." << std::endl;
          break;
        }
        THandle th1;
        if (openTable ("tmp1", &th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not opened." << std::endl;
          break;
        }
        if (moveFirst (th1) != OK) {
          fputc ('0', fp);
          std::cerr << "No record found." << std::endl;
          break;
        }
        do {
          if (createNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "Record is not created." << std::endl;
            break;
          }
          //вычислить лог выражение для текущей строки таблицы
          std::string result;
          logex = logex + " $";
          result = LogicExpr(th, logex);
          //std::cerr<< "result: " << result << std::endl;
          int flag = 0;
          if (result == "1") flag = 1;
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            for (int j = 0; j < count; j++) {
              if (strcmp (name, ts.fieldsDef[j].name) == 0) {
                enum FieldType ft;
                if (getFieldType (th, name, &ft) != OK) {
                  fputc ('0', fp);
                  std::cerr << "Type is not accessible." << std::endl;
                  break;
                }
                if ((name == fieldname) && (flag)) {
                  //std::cerr << "надо заменять\n";
                  if (ft == Long) {
                    //std::cerr << "надо заменять в Long\n";
                    ex = ex + " $";
                    //std::cerr << "EX:"<< ex << std::endl;
                    std::string res = Expr(th, ex);
                    std::cerr << "Res:"<< res << std::endl;
                    if ( putLongNew (th1, th1->pFieldStruct[j].fieldName, atol(res.c_str())) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    //std::cerr << "надо заменять в Text\n";
                    char buf [MaxFieldNameLen];
                    //std::cerr << "TEXT: " <<ex << "STR: "<<(ex.c_str())+1<< "LENG: "<< strlen(ex.c_str())-2<< std::endl;
                    strncpy (buf,(ex.c_str())+1, strlen(ex.c_str())-2);
                    //std::cerr << "TEXT " <<buf<< std::endl;
                    if (putText (th1, th1->pFieldStruct[j].fieldName, buf) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                }
                else {
                  //std::cerr << "не надо заменять\n";
                  if (ft == Long) {
                    long value;
                    //enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
                    if (getLong (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putLongNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                  else if (ft == Text) {
                    char *value;
                    //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
                    if (getText (th, name, &value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                    if (putTextNew(th1, th1->pFieldStruct[j].fieldName, value) != OK) {
                      fputc ('0', fp);
                      std::cerr << "Record is not accessible." << std::endl;
                      break;
                    }
                  }
                }
              }
            }
          }
          if (insertzNew (th1) != OK) {
            fputc ('0', fp);
            std::cerr << "New field is not inserted." << std::endl;
            break;
          }
          moveNext (th);
        }
        while (afterLast(th) != TRUE);
        if (closeTable(th) != OK) {
           fputc ('0', fp);
           std::cerr << "Table is not closed." << std::endl;
           break;
        }
        if (closeTable(th1) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
      }
    }
    }

    case '4': {
      //DELETE
      int wh;
      enum Errors err;
      std::string s1(s+1), name;
      std::istringstream query (s1);
      query >> name >> wh;
      if (openTable(name.c_str(), &th) != OK) {
        fputc ('0', fp);
        std::cerr << "Table is not opened." << std::endl;
        break;
      }
      //THandle th;       //туда записывается дескриптор для работы с таблицей
      switch (wh) {
      case 3: {
        do {
        	if ((err = deleteRec(th)) != OK) {
            //enum Errors deleteRec(THandle tableHandle);
            fputc ('0', fp);
            std::cerr << "Record is not deleted.  " << getErrorString(err) << std::endl;
        	}
        	moveNext(th);
				}
        while (afterLast(th) != TRUE);
        if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        fputc('1', fp);
        break;
      }
      case 1: {
        std::string fieldname, buf2, strexample;
        query >> fieldname;
        query >> buf2;
        query >> strexample; // строка-образец
        unsigned count;
        getFieldsNum(th, &count);
        do {
          if (getFieldsNum(th, &count) != OK) {
            fputc ('0', fp);
            std::cerr << "Amount of field is not accessible." << std::endl;
            break;
          }
          for (int i = 0; i < count; i++) {
            char *name;
            if (getFieldName (th, i, &name) != OK) {
              fputc ('0', fp);
              std::cerr << "Field is not accessible." << std::endl;
              break;
            }
            if (name == fieldname) {
              enum FieldType ft;
              if (getFieldType (th, name, &ft) != OK) {
                fputc ('0', fp);
                std::cerr << "Type is not accessible." << std::endl;
                break;
              }
              if (ft == Long) {
                long value;
                if (getLong (th, name, &value) != OK) {
                  fputc ('0', fp);
                  std::cerr << "Record is not accessible." << std::endl;
                  break;
                }
                strr = std::to_string(value);
              }
              else if (ft == Text) {
                char *value;
                if (getText (th, name, &value) != OK) {
                  fputc ('0', fp);
                  std::cerr << "Record is not accessible." << std::endl;
                  break;
                }
                strr = value;
                strr = "'" + strr + "'";
              }
              //std::cerr << "STR: "<< strr << std::endl;
              //std::cerr << "STREXAMPLE: "<< strexample << std::endl;
              res = (WhereStr(strr, strexample) && (buf2 == "LIKE")) || (!WhereStr(strr, strexample) && (buf2 == "NOTLIKE"));
              //std::cerr << "Res: "<< res << std::endl;
            }
          }
          if (res){
            if ((err = deleteRec(th)) != OK) {
              //enum Errors deleteRec(THandle tableHandle);
              fputc ('0', fp);
              std::cerr << "Record is not deleted.  " << getErrorString(err) << std::endl;
            }
          }
          moveNext (th);
				}
        while (afterLast(th) != TRUE);
        if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        fputc('1', fp);
        break;
      }
      case 2: {
        std::string ex, t;
        std::getline(query, ex, '$');
        ex = ex + " $";
        query >> t;
        if ((t != "IN") && (t != "NOTIN")) {
          fputc ('0', fp);
          std::cerr << "IN or NOT IN is expected." << std::endl;
          break;
        }
        std::string str;
        query >> str;       //количество констант
        std::string type;
        query >> type;      //либо STRING, либо NUMBER
        int amount = atoi(str.c_str());
        std::string cnst[amount];
        for (int i = 0; i < amount; i++) {
          query >> cnst[i];
        }
        do {
          std::string result;
          if (type == "NUMBER") result = Expr(th, ex);
          else {
            char *value;
            //enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
            if (getText (th, ex.c_str(), &value) != OK) {
              fputc ('0', fp);
              std::cerr << "Record is not accessible." << std::endl;
              break;
            }
            result = std::string(value);
          }
          int flag = 0;
          for (int i = 0; i < amount; i++) {
            if (result == cnst[i]) flag = 1;
          }
          if (!flag) {
            moveNext (th);
            continue;
          }
          if ((err = deleteRec(th)) != OK) {
            //enum Errors deleteRec(THandle tableHandle);
            fputc ('0', fp);
            std::cerr << "Record is not deleted.  " << getErrorString(err) << std::endl;
        	}
          moveNext (th);
        }
        while (afterLast(th) != TRUE);
        if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        fputc('1', fp);
        break;
      }
      case 4: {
        std::string ex, t;
        std::getline(query, ex, '$');
        ex = ex + " $";
        do {
          std::string result;
          result = LogicExpr(th, ex);
          int flag = 0;
          if (result == "1") flag = 1;
          if (!flag) {
            moveNext (th);
            continue;
          }
          if ((err = deleteRec(th)) != OK) {
            //enum Errors deleteRec(THandle tableHandle);
            fputc ('0', fp);
            std::cerr << "Record is not deleted.  " << getErrorString(err) << std::endl;
        	}
          moveNext (th);
        }
        while (afterLast(th) != TRUE);
        if (closeTable(th) != OK) {
          fputc ('0', fp);
          std::cerr << "Table is not closed." << std::endl;
          break;
        }
        fputc('1', fp);
        break;
      }
      }
      break;
    }
  }

	fclose (fp);
	close(d);
	return 0;
}

std::string Expr (THandle th, std::string poliz){
  std::stringstream strm;
  std::string lex;
  strm << poliz;
  strm >> lex;
  std::string operand1, operand2;
  while (lex != "$") {
    if ((lex == "+") || (lex == "-") || (lex == "*") || (lex == "/") || (lex == "%"))  {
      operand2 = pop();
      operand1 = pop();
      long op1 = atoi(operand1.c_str()); //0 если не число
      //если операнд1 число то перевод в int
      if (!op1) {
        //иначе взять из таблицы поле по этому названию и проверить что его тип LONG
        long value;
        if (getLong (th, operand1.c_str(), &value) != OK) {
          std::cerr << "Fieldtype is expected to be LONG." << std::endl;
          break;
        }
        op1 = value;
      }
      int op2 = atoi(operand2.c_str()); //0 если не число
      //если операнд2 число то перевод в int
      //иначе взять из таблицы поле по этому названию и проверить что его тип LONG
      if (!op2) {
        long value;
        if (getLong (th, operand2.c_str(), &value) != OK) {
          std::cerr << "Fieldtype is expected to be LONG." << std::endl;
          break;
        }
        op2 = value;
      }
      //выполнить операцию, результат положить в стек в виде строки
      if (lex == "+") push(std::to_string(op1 + op2));
      if (lex == "-") push(std::to_string(op1 - op2));
      if (lex == "*") push(std::to_string(op1 * op2));
      if (lex == "/") push(std::to_string(op1 / op2));
      if (lex == "%") push(std::to_string(op1 % op2));
    }
    else {
      long op = atoi(lex.c_str()); //0 если не число
      if (!op) {
        //иначе взять из таблицы поле по этому названию и проверить что его тип LONG
        long value;
        if (getLong (th, lex.c_str(), &value) != OK) {
          std::cerr << "Fieldtype is expected to be LONG." << std::endl;
          break;
        }
        op = value;
      }
      push (std::to_string(op));
      //std::cerr << "EXPR: " << op << std::endl;
    }
    strm >> lex;
  }
  return pop();
}

std::string LogicExpr (THandle th, std::string poliz){
  std::stringstream strm;
  std::string lex;
  strm << poliz;
  strm >> lex;
  std::string operand1, operand2, operand;
  while (lex != "$") {
    if ((lex == "=") || (lex == ">") || (lex == "<") || (lex == ">=") || (lex == "<=") || (lex == "!="))  {
      operand2 = pop();
      operand1 = pop();
      long num1 = atoi(operand1.c_str()); //0 если не число
      long num2 = atoi(operand2.c_str()); //0 если не число
      if (num1 || num2) {
        //анализ числового выражения
        if (!num1) {
          //иначе взять из таблицы поле по этому названию и проверить что его тип LONG
          long value;
          if (getLong (th, operand1.c_str(), &value) != OK) {
            std::cerr << "Fieldtype is expected to be LONG." << std::endl;
            break;
          }
          num1 = value;
        }
        if (!num2) {
          long value;
          if (getLong (th, operand2.c_str(), &value) != OK) {
            std::cerr << "Fieldtype is expected to be LONG." << std::endl;
            break;
          }
          num2 = value;
        }
        if (lex == "=") push(std::to_string(num1 == num2));
        if (lex == ">") push(std::to_string(num1 > num2));
        if (lex == "<") push(std::to_string(num1 < num2));
        if (lex == ">=") push(std::to_string(num1 >= num2));
        if (lex == "<=") push(std::to_string(num1 <= num2));
        if (lex == "!=") push(std::to_string(num1 != num2));
      }
      else {
        std::string op1, op2;
        if (operand1.find("\'") != std::string::npos) {
          //так и оставляем в виде строки
          op1 = operand1;
        }
        else {
          char *value;
          if (getText (th, operand1.c_str(), &value) != OK) {
            std::cerr << "Fieldtype is expected to be TEXT." << std::endl;
            break;
          }
          op1 = value;
          op1 = "\'" + op1 + "\'";
        }
        if (operand2.find("\'") != std::string::npos) {
          //так и оставляем в виде строки
          op2 = operand2;
        }
        else {
          char *value;
          if (getText (th, operand2.c_str(), &value) != OK) {
            std::cerr << "Fieldtype is expected to be TEXT." << std::endl;
            break;
          }
          op2 = value;
          op2 = "\'" + op2 + "\'";
        }
        if (lex == "=") {
          if (op1 == op2) push("1");
          else push("0");
        }
        if (lex == ">") {
          if (op1 > op2) push("1");
          else push("0");
        }
        if (lex == "<") {
          if (op1 < op2) push("1");
          else push("0");
        }
        if (lex == ">=") {
          if (op1 >= op2) push("1");
          else push("0");
        }
        if (lex == "<=") {
          if (op1 <= op2) push("1");
          else push("0");
        }
        if (lex == "!=") {
          if (op1 != op2) push("1");
          else push("0");
        }
      }
    }
    else if (lex == "NOT") {
      operand = pop();
      if (operand == "1") push("0");
      if (operand == "0") push("1");
    }
    else if (lex == "AND") {
      operand2 = pop();
      operand1 = pop();
      if ((operand1 == "1") && (operand2 == "1")) push("1");
      else push("0");
    }
    else if (lex == "OR") {
      operand2 = pop();
      operand1 = pop();
      if ((operand1 == "0") && (operand2 == "0")) push("0");
      else push("1");
    }
    else push(lex);
    strm >> lex;
  }
  return pop();
}

int WhereStr(std::string str, std::string exmpl) {
  //https://regex101.com/

  //. ^ $ * + ? { } | ( ) - надо экранировать
  //надо заменять "_" на "."
  exmpl = std::regex_replace(exmpl, std::regex("_"), ".");
  //надо заменять "%" на ".*"
  exmpl = std::regex_replace(exmpl, std::regex("%"), ".*");
/*
  exmpl = std::regex_replace(exmpl, std::regex("-"), "\-");
  exmpl = std::regex_replace(exmpl, std::regex("."), "\.");
  exmpl = std::regex_replace(exmpl, std::regex("{"), "\{");
  exmpl = std::regex_replace(exmpl, std::regex("}"), "\}");
  exmpl = std::regex_replace(exmpl, std::regex("|"), "\|");
  exmpl = std::regex_replace(exmpl, std::regex("("), "\(");
  exmpl = std::regex_replace(exmpl, std::regex(")"), "\)");
  exmpl = std::regex_replace(exmpl, std::regex("$"), "\$");
  exmpl = std::regex_replace(exmpl, std::regex("*"), "\*");
  exmpl = std::regex_replace(exmpl, std::regex("?"), "\?");
  exmpl = std::regex_replace(exmpl, std::regex("+"), "\+");
*/
  std::smatch sm;
  const std::regex regex(exmpl);
  //std::cerr << "regex: "<< exmpl << std::endl;
  std::string match;
  if (std::regex_match(str, sm, regex)) {
    return 1;
  }
  return 0;
}
