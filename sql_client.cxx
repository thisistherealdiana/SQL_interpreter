#include <iostream>
#include <stdio.h>
#include <string>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string>
#include <sstream>
#include <iostream>
#include "Table.h"

// клиент для сети эвм

//открывает соединение на заданный адрес (argv[1]) и порт (argv[2]); шлёт посылку

int sql_query(FILE *);

int SEL(FILE *);
int INS(FILE *);
int UPD(FILE *);
int DEL(FILE *);
int CR(FILE *);
int DR(FILE *);
std::string Where();

std::string lex = "";
std::stringstream is;

int main(int argc, char const **argv) {

	/* struct hostent * gethostbyname (char * hostname);
	 * struct hostent {
	 * 	char * h_name; 		   hostname ЭВМ
	 *  сhar ** h_aliases; 	   список синонимов
	 *  int h_addrtype; 	   тип адресов ЭВМ
	 *  int h_length; 		   длина адреса
	 *  char ** h_addr_list;   списокадресов(для разных сетей)
	 *
	 *  #define h_addr h_addr_list[0] };
	*/
	int port; 		// может быть от 0 до 65535
	int n, sfd;
	char hostname[64];
	struct hostent *hst;
	struct sockaddr_in sin;

	if (argc != 3) {
		std::cerr << "Not right amount of parameters" << std::endl;
		return 1;
	}
	//gethostname (hostname, sizeof (hostname));
	// получим адрес компьютера (сетевой номер своей машины)
	if ((hst = gethostbyname(argv[1])) == NULL) {
		std::cerr << "Bad host name: " << argv[1] << std::endl;
		return 1;
	}
	// прочитаем номер порта
	if ((sscanf(argv[2], "%d%n", &port, &n) != 1) || argv[2][n] || (port <= 0) || (port > 65535)) {
		std::cerr << "Bad port number: " << argv[2] << std::endl;
		return 1;
	}
	// создаём сокет
	if ((sfd = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
		perror("socket");
		return 1;
	}
	// для нашего клиента привязка сокета не важна, поэтому bind пропускаем
	// устанавливаем адрес подключения, по которому будем связываться с сервером
	sin.sin_family = AF_INET;
	//sin.sin_port = htons(1234);
	sin.sin_port = htons(port);
	//копируем сетевой номер
	memcpy(&sin.sin_addr, hst->h_addr_list[0], hst->h_length);
	// подсоединяемся к серверу
	if (connect(sfd, (struct sockaddr*) &sin, sizeof(sin)) < 0) {
		perror("connect");
		return 1;
	}

	//читаем сообщения сервера, пишем серверу
	FILE * fp;
	fp = fdopen (sfd, "r+");
	sql_query(fp);
	//std::cerr << "Server has done everything." << std::endl;
	fclose(fp);
	// закрываем сокет
	close(sfd);
	return 0;
}


//если запрос выполнился вернём 1, иначе 0
int sql_query(FILE * f) {
	std::string s;
	std::cin >> s;	//чтение до пробела
	if (s == "CREATE") {
		return (CR(f) == 1);
	}
	if (s == "DROP") {
		return (DR(f) == 1);
	}
	if (s == "SELECT") {
		return (SEL(f) == 1);
	}
	if (s == "INSERT") {
		return (INS(f) == 1);
	}
	if (s == "UPDATE") {
		return (UPD(f) == 1);
	}
	if (s == "DELETE") {
		return (DEL(f) == 1);
	}
	return 0;
}

/* GRAMMAR:
    <SQL-предложение> ::= 	<SELECT-предложение> |
     						<INSERT-предложение> |
     						<UPDATE-предложение> |
     						<DELETE-предложение> |
     						<CREATE-предложение> |
     						<DROP-предложение>
  SELECT -- "1"
  INSERT -- "2"
  UPDATE -- "3"
  DELETE -- "4"
  CREATE -- "5"
  DROP -- "6"
*/

static char c;
//возвращает символ
char gc() {
	do c = std::cin.get();
	while (c == ' ');
	return c;
}

std::string ReadName() {
	std::string n = "";
	char x;
	do x = std::cin.get();
	while (x == ' ');
	if (!isalpha(x) && (x != '_')) {
		std::cin.unget();
		return "";
	}
	while (isalpha(x) || isdigit(x) || (x == '_')) {
		n = n + x;
		x = std::cin.get();
	}
	std::cin.unget();
	return n;
}


/*<CREATE-предложение> ::= CREATE TABLE <имя таблицы> ( <список описаний полей> )
	<список описаний полей> ::= <описание поля> { , <описание поля> }
	<описание поля> ::= <имя поля> <тип поля>
	<тип поля> ::= TEXT ( <целое без знака> ) | LONG
*/

//<описание поля> ::= <имя поля> <тип поля>
//<тип поля> ::= TEXT ( <целое без знака> ) | LONG
std::string FDescr () {	//возвращает описание одного поля
	std::string fieldname, s, str;
	std::string number = "";
	if ((fieldname = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return "";
	}
	str = fieldname;
	if ((s = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return "";
	}
	if (s == "TEXT") {
		str = str + " TEXT";
		c = gc();
		if (c != '(') {
			printf ("( is expected.\n");
			return "";
		}
		c = gc();
		while (isdigit(c)) {
			number = number + c;
			c = gc();
		}
		if (c != ')') {
			printf (") is expected.\n");
			return "";
		}
		c = gc();
		str = str + " " + number;
		return str;
	}
	else if (s == "LONG") {
		str = str + " LONG";
		c = gc();
		return str;
	}
	printf ("TEXT or LONG is expected.\n");
	return "";
}


//возвращаем 0 при ошибке
//<CREATE-предложение> ::= CREATE TABLE <имя таблицы> ( <список описаний полей> )
int CR(FILE * f) {
	std::string str = "";
	std::string s;
	std::cin >> s;
	if (s != "TABLE") {
		printf ("TABLE is expected.\n");
		return 0;
	}

	std::string tablename;
	if ((tablename = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return 0;
	}
	c = gc();
	if (c != '(') {
		printf ("( is expected.\n");
		return 0;
	}
	if ((s = FDescr()) != "") str = str + " " + s;
	else {
		printf ("Field description is incorrect.\n");
		return 0;
	}
	int count = 1;
	while (c == ',') {
		if ((s = FDescr()) != "") str = str + " " + s;
		else {
			printf ("Field description is incorrect.\n");
			return 0;
		}
		count++;
	}
	if (c != ')') {
		printf (") is expected.\n");
		return 0;
	}
	c = gc();
	str = "5" + tablename + " " + std::to_string(count) + " " + str;
	//c_str : перевод из string в const char *
	//fprintf(stdout, "%s\n", str.c_str());
	fprintf(f, "%s\n", str.c_str()); // если запрос правильный отправляем на сервер
	if (fgetc(f) == '1') {
		printf ("ALL DONE.\n");
		return 1;
	}
	else return 0;
}

//<DROP-предложение> ::= DROP TABLE <имя таблицы>

int DR(FILE * f) {
	std::string str = "";
	std::string s;
	std::cin >> s;
	if (s != "TABLE") {
		printf ("TABLE is expected.\n");
		return 0;
	}
	std::string tablename;
	if ((tablename = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return 0;
	}
	str = "6" + tablename;
	//fprintf(stdout, "%s\n", str.c_str());
	fprintf(f, "%s\n", str.c_str());

	if (fgetc(f) == '1') {
		printf ("ALL DONE.\n");
		return 1;
	}
	else return 0;
}

/*
<SELECT-предложение> ::= SELECT <список полей> FROM <имя таблицы> <WHERE-клауза>
<список полей> ::= <имя поля> { , <имя поля> } | *
<имя таблицы> ::= <имя>
<имя поля> ::= <имя>
<имя>::= <идентификатор языка Си>
*/
int SEL(FILE * f) {
	int flag = 0;
	int count;
	std::string str, buf;
	std::string s, tablename;
	c = gc();
	if (c == '*') {
		flag = 1;
	} else {
		std::cin.putback(c);
		if ((s = ReadName()) == "") {
			printf ("Identifier is expected.\n");
			return 0;
		}
		buf = s;
		count = 1;
		c = gc();
		while (c == ',') {
			if ((s = ReadName()) == "") {
				printf ("Identifier is expected.\n");
				return 0;
			}
			buf = buf + " " + s;
			count++;
			c = gc();
		}
		std::cin.putback(c);
	}
	std::cin >> s;
	if (s != "FROM") {
		printf ("FROM is expected.\n");
		return 0;
	}
	if ((tablename = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return 0;
	}

	std::cin >> s;
	if (s != "WHERE") {
		printf ("WHERE is expected.\n");
		return 0;
	}
	if (flag == 1) str = "1" + tablename  + " * " +  Where();
	else str = "1" + tablename + " " + Where() + " " + std::to_string(count) + " " + buf;
	//fprintf(stdout, "%s\n", str.c_str());
	fprintf(f, "%s\n", str.c_str());

	if (fgetc(f) == '1') {
		printf ("ALL DONE.\n");
		THandle th;
		if (openTable ("tmp1", &th) != OK) {
			std::cerr << "Table is not opened." << std::endl;
			return 0;
    }
    if (moveFirst (th) != OK) {
			std::cerr << "No record found." << std::endl;
			return 0;
		}
		unsigned count;
		getFieldsNum(th, &count);
		if (count == 0) {
			std::cerr << "Table is empty." << std::endl;
			return 0;
		}
		for (int i = 0; i < count; i++) {
			char *name;
			if (getFieldName (th, i, &name) != OK) {
				std::cerr << "Field is not accessible." << std::endl;
				return 0;
			}
			std::cerr << name;
			std::cerr << "\t\t";
		}
		std::cerr << std::endl;
		do {
			for (int i = 0; i < count; i++) {
				char *name;
				if (getFieldName (th, i, &name) != OK) {
					std::cerr << "Field is not accessible." << std::endl;
					return 0;
				}
				enum FieldType ft;
				if (getFieldType (th, name, &ft) != OK) {
					std::cerr << "Type is not accessible." << std::endl;
					return 0;
				}
				unsigned leng;
				getFieldLen (th, name, &leng);
				if (ft == Long) {
					long value;
					//enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
					if (getLong (th, name, &value) != OK) {
						std::cerr << "Record is not accessible." << std::endl;
						return 0;
					}
					std::cerr << value;
					std::cerr << "\t\t";
				}
				else if (ft == Text) {
					char *value;
					//enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
					if (getText (th, name, &value) != OK) {
						std::cerr << "Record is not accessible." << std::endl;
						return 0;
					}
					std::string valstr(value); //to convert to string
					std::cerr << value;
					std::cerr << "\t\t";
				}

			}
			std::cerr << std::endl;
			moveNext (th);
		}
		//Bool afterLast(THandle tableHandle);
		while (afterLast(th) != TRUE);
		if (closeTable(th) != OK) {
			std::cerr << "Table is not closed." << std::endl;
      return 0;
    }
    unlink("tmp1");		//удаление временного файла
		return 1;
	}
	else return 0;
}

/*
<INSERT-предложение> ::= INSERT INTO <имя таблицы> (<значение поля> { , <значение поля> })
<значение поля> ::= <строка> | <длинное целое>
<строка> ::= '<символ> {<символ>}'
<символ> ::= <любой, представляемый в компьютере символ, кроме апострофа '>
*/

std::string FValue () {	//возвращает описание одного поля
	std::string str = "";
	std::string s = "";
	std::string number = "";
	c = gc();
	if (isdigit(c)) {
		while (isdigit(c)) {
			number = number + c;
			c = gc();
		}
		return	("LONG " + number);
	}
	if (c != '\'') {
		printf ("Opening ' is expected.\n");
		return "";
	}
	c = gc();
	if (((isalpha(c)) || isdigit(c)) && (c != '\'')) {
		while(((isalpha(c)) || isdigit(c) || (c == '-')) && (c != '\'')) {
			s = s + c;
			c = gc();
		}
		if (c != '\'') {
			printf ("Closing ' is expected.\n");
			return "";
		}
		c = gc();
		return ("TEXT " + s);
	}
	printf ("A string or a number is expected.\n");
	return "";
}

//<INSERT-предложение> ::= INSERT INTO <имя таблицы> (<значение поля> { , <значение поля> })
int INS(FILE * f) {
	std::string str = "";
	std::string s;
	if ((s = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return 0;
	}
	if (s != "INTO") {
		printf ("INTO is expected.\n");
		return 0;
	}
	std::string tablename;
	if ((tablename = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return 0;
	}
	c = gc();
	if (c != '(') {
		printf ("( is expected.\n");
		return 0;
	}
	if ((s = FValue()) != "") str = str + " " + s;
	else {
		printf ("Field value is incorrect.\n");
		return 0;
	}
	int count = 1;
	while (c == ',') {
		if ((s = FValue()) != "") str = str + " " + s;
		else {
			printf ("Field value is incorrect.\n");
			return 0;
		}
		count++;
	}
	if (c != ')') {
		printf (") is expected.\n");
		return 0;
	}
	c = gc();
	str = "2" + tablename + " " + std::to_string(count) + " " + str;
	//fprintf(stdout, "%s\n", str.c_str());
	fprintf(f, "%s\n", str.c_str());

	if (fgetc(f) == '1') {
		printf ("ALL DONE.\n");
		return 1;
	}
	else return 0;
}


enum lex_type_t { CMP, OR, AND, PLUS, MINUS, MULT, DIV, MOD, NUMBER, IDENT, STRING, OPEN, CLOSE, END } cur_lex_type;

void getlex(std::istream & is = std::cin) {
	enum state_t { H, N, ID, OK} state = H;
  lex = "";
  while (state != OK) {
		c = is.get();
    switch (state) {
    case H:
    	if (isspace(c)) {
      	// stay in H
      } else if (c == '=') {
				lex = "=";
        cur_lex_type = CMP;
        state = OK;
      } else if ((c == '<') || (c == '>')) {
				lex = c;
				c = is.get();
				if (c == '=') {
					lex = lex + c;
				}
				else is.putback(c);
        cur_lex_type = CMP;
        state = OK;
      } else if (c == '!') {
				c = is.get();
				if (c == '=') {
					lex = "!=";
					cur_lex_type = CMP;
				}
				else throw "Wrong comparing operator.";
        state = OK;
      } else if (c == '+') {
				lex = "+";
        cur_lex_type = PLUS;
        state = OK;
      } else if (c == '-') {
				lex = "-";
        cur_lex_type = MINUS;
        state = OK;
      } else if (c == '*') {
				lex = "*";
        cur_lex_type = MULT;
        state = OK;
      } else if (c == '/') {
				lex = "/";
        cur_lex_type = DIV;
        state = OK;
      } else if (c == '%') {
				lex = "%";
        cur_lex_type = MOD;
        state = OK;
      } else if (c == '(') {
				lex = "(";
        cur_lex_type = OPEN;
        state = OK;
      } else if (c == '\'') {
				lex = c;
				c = is.get();
				while (isalpha(c)||isdigit(c)||(c == '^')||(c == '_')||(c == '%')||(c == '-')||(c == '[')||(c == ']')) {
        	lex = lex + char(c);
					c = is.get();
        }
        lex = lex + c;
        if (c != '\'') throw ("Ending ' is expected");
        cur_lex_type = STRING;
        state = OK;
      } else if (c == ')') {
				lex = ")";
        cur_lex_type = CLOSE;
        state = OK;
      } else if (isdigit(c)) {
        lex = char(c);
        state = N;
      } else if (isalpha(c)) {
        lex = char(c);
        state = ID;
      } else  if (c == EOF) {
        cur_lex_type = END;
        state = OK;
      }
      break;

    case N:
    	if ( isdigit(c) ){
        while (isdigit(c)) {
        	lex = lex + char(c);
					c = is.get();
        }
        if (isalpha(c) ) throw "Number is expected.";
      }
      cur_lex_type = NUMBER;
      state = OK;
			is.putback(c);
      break;

    case ID:
      if (isalpha(c)) {
      	while (isalpha(c) || isdigit(c) || (c == '_')) {
          lex = lex + char(c);
					c = is.get();
        }
			}
			cur_lex_type = IDENT;
			state = OK;
			is.putback(c);
      break;

    case OK:
      break;
    }
  }
}

/* <выражение> ::= <Long-выражение> | <Text-выражение>
 * <Long-выражение> ::= <Long-слагаемое> { <+|-> <Long-слагаемое> }
 * <Long-слагаемое> ::= <Long-множитель> { <*|/|%> <Long-множитель> }
 * <Long-множитель> ::= <Long-величина> | ( <Long-выражение> )
 * <+|->::=+|-
 * <*|/|%>::=*|/|%
 * <Long-величина> ::= <имя поля типа LONG> | <длинное целое>
 * <Text-выражение> ::= <имя поля типа TEXT> | <строка>
 *
 * E -> M {+|- M} | string
 * M -> F {*|/|% F}
 * F -> (E) | id | num
 *
 */


char ch;
std::string exp;

void E(std::istream & is);
void M(std::istream & is);
void F(std::istream & is);

void E(std::istream & is = std::cin) {
	M(is);
	while ((cur_lex_type == PLUS) || (cur_lex_type == MINUS)){
		lex_type_t m = cur_lex_type;
		getlex(is);
		M(is);
		if (m == PLUS) exp = exp + " +";
		if (m == MINUS) exp = exp + " -";
	}
}

void M(std::istream & is = std::cin) {
	F(is);
	while ((cur_lex_type == MULT) || (cur_lex_type == DIV) || (cur_lex_type == MOD)){
		lex_type_t m = cur_lex_type;
		getlex(is);
		F(is);
		if (m == MULT) exp = exp + " *";
		if (m == DIV) exp = exp + " /";
		if (m == MOD) exp = exp + " %";
	}
}

void F(std::istream & is = std::cin) {
	if (cur_lex_type == OPEN) {
    getlex(is);
    E(is);
    if (cur_lex_type != CLOSE) throw ") is expexted.";
    getlex(is);
  }
  else if (cur_lex_type == NUMBER) {
    exp = exp + " " + lex;
    getlex(is);
  }
  else if (cur_lex_type == IDENT) {
		exp = exp + " " + lex;
    getlex(is);
	} else throw "Wrong expression.";
}

std::string Expr(std::istream & is = std::cin) {
	try {
		exp = "";
		getlex(is);
		if (cur_lex_type == STRING) return lex;
		E(is);
		return exp;
	}
	catch (const char * s) {
    std::cerr << std::endl << s << std::endl;
  }
  return "";
}

/* <логическое выражение> ::= <логическое слагаемое> { OR <логическое слагаемое> }
<логическое слагаемое> ::= <логический множитель> { AND <логический множитель> }
<логический множитель> ::= NOT <логический множитель> | ( <логическое выражение> ) |
						(<отношение>)
<отношение> ::= <Text-отношение> | <Long-отношение>
<Text-отношение> ::= <Text-выражение> <операция сравнения> <Text-выражение>
<Long-отношение> ::= <Long-выражение> <операция сравнения> <Long-выражение>
<операция сравнения>::= = | > | < | >= | <= | !=
<Text-выражение> ::= <имя поля типа TEXT> | <строка>


LE -> LM {or LM}
LM -> LF {and LF}
LF -> "NOT" LF | ( <если следующий '(' или NOT > LE) | (A)
A -> [ident|string|number] [= | > | < | >= | <= | != ] [ident|string|number]
*/
void LE();
void LM();
void LF();
void A();
void B();

void LE() {
	LM();
	while (lex == "OR") {
		getlex(is);
		LM();
		exp = exp + " OR";
	}
}

void LM() {
	LF();
	while (lex == "AND") {
		getlex(is);
		LF();
		exp = exp + " AND";
	}
}

void LF() {
	if (lex == "NOT") {
		getlex(is);
		LF();
		exp = exp + " NOT";
	}
	else if (cur_lex_type == OPEN) {
		getlex(is);
		if ((lex == "NOT") || (cur_lex_type == OPEN)) {
			LE();
			if (cur_lex_type != CLOSE) throw ") is expexted.";
			getlex(is);
		}
		else {
			A();
			if (cur_lex_type != CLOSE) throw ") is expexted.";
			getlex(is);
		}
	}
	else throw "Wrong expression.";
}

void A() {
	lex_type_t m = cur_lex_type;
	std::string k;
	if (cur_lex_type == NUMBER) {
		m = NUMBER;
		exp = exp + " " + lex;
		getlex(is);
	}
  else if (cur_lex_type == IDENT) {
		m = IDENT;
		exp = exp + " " + lex;
    getlex(is);
	}
	else if (cur_lex_type == STRING) {
		m = STRING;
		exp = exp + " " + lex;
    getlex(is);
	}
	else throw "Wrong expression.";
	if (cur_lex_type == CMP) {
		k = lex;
    getlex(is);
	}
	else throw "Comparing is expected.";
	if ((cur_lex_type == NUMBER) && (m != STRING)) {
		exp = exp + " " + lex;
		getlex(is);
  }
  else if (cur_lex_type == IDENT) {
		exp = exp + " " + lex;
    getlex(is);
	}
	else if ((cur_lex_type == STRING) && (m != NUMBER))  {
		exp = exp + " " + lex;
    getlex(is);
	}
	else throw "Wrong expression.";
	exp = exp + " " + k;
}

std::string LogicExpr (std::istream & is = std::cin) {
	try {
		exp = "";
		getlex(is);
		LE();
		return exp;
	}
	catch (const char * s) {
    std::cerr << std::endl << s << std::endl;
  }
  return "";
}

/*<WHERE-клауза> ::= WHERE <имя поля типа TEXT> [ NOT ] LIKE <строка-образец> |
 					WHERE<выражение> [ NOT ] IN ( <список констант> ) |
					WHERE <логическое выражение> |
					WHERE ALL
<список констант> ::= <строка> { , <строка> } | <длинное целое> { , <длинное целое> }
WHERE ALL - "3"
*/
std::string s1;

std::string Where () {
	std::string s1;
	std::string s, str = "";
	//прочитать в s до конца строки
	std::getline(std::cin, s1);
	is << s1;
	if (s1.find("ALL")!= std::string::npos) {
		return "3";
	}
	else {
		//если в строке есть IN - номер 2
		if (s1.find("IN") != std::string::npos) {
			//std::cerr << "*****" << str << "*****" << std::endl;
			str = "2 " + Expr(is);
			if ((lex != "NOT") && (lex != "IN")) {
				printf ("NOT IN or IN is expected.\n");
				return 0;
			}
			if (lex == "NOT") {
				getlex(is);
				if (lex != "IN") {
					printf ("NOT IN is expected.\n");
					return 0;
				}
				str = str + "$ NOTIN";
			}
			else str = str + "$ IN";	//если LIKE
			c = is.get();
			if (c != '(') {
				printf ("( is expected.\n");
				return 0;
			}
			getlex(is);
			int count;
			std::string buf;
			if (cur_lex_type == STRING) {
				buf = "STRING " + lex;
				count = 1;
				c = is.get();
				while (c == ',') {
					getlex(is);
					if (cur_lex_type == STRING) {
						buf = buf + " " + lex;
						count++;
					}
					c = is.get();
				}
			}
			if (cur_lex_type == NUMBER) {
				buf = "NUMBER " + lex;
				count = 1;
				c = is.get();
				while (c == ',') {
					getlex(is);
					if (cur_lex_type == NUMBER) {
						buf = buf + " " + lex;
						count++;
					}
					c = is.get();
				}
			}
			if (c != ')') {
				printf (") is expected.\n");
				return 0;
			}
			return (str + " " + std::to_string(count) + " " + buf);
		}
		else {
			//если в строке есть LIKE - номер 1
			if (s1.find("LIKE") != std::string::npos) {
				getlex(is);
				if (cur_lex_type != IDENT) {
					printf ("Identifier is expected.\n");
					return 0;
				}
				str = "1 " + lex;
				getlex(is);
				if ((lex != "NOT") && (lex != "LIKE")) {
					printf ("NOT LIKE or LIKE is expected.\n");
					return 0;
				}
				if (lex == "NOT") {
					//если NOT LIKE
					getlex(is);
					if (lex != "LIKE") {
						printf ("NOT LIKE is expected.\n");
						return 0;
					}
					str = str + " NOTLIKE";
				}
				else str = str + " LIKE";	//если LIKE
				getlex(is);
				if (cur_lex_type != STRING) {
					printf ("String is expected after LIKE.\n");
					return 0;
				}
				str = str + " " + lex;
				return str;
			}
			else {
				//логическое выражение - номер 4
				str = "4 " + LogicExpr(is) + "$";
				return str;
			}
		}
	}
	return "";
}

//<UPDATE-предложение> ::= UPDATE <имя таблицы> SET <имя поля> = <выражение> <WHERE-клауза>
int UPD(FILE * f) {
	//std::cerr << "**********" << is.str() << "*********" << std::endl;
	std::string str = "3";
	std::string tablename;
	if ((tablename = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return 0;
	}
	str = str + tablename;
	std::string s;
	std::cin >> s;
	if (s != "SET") {
		printf ("SET is expected.\n");
		return 0;
	}
	std::string fieldname;
	if ((fieldname = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return 0;
	}
	std::string fd = fieldname;
	c = gc();
	if (c != '=') {
		printf ("= is expected.\n");
		return 0;
	}
	std::string s1 = "";
	getlex();
	while (lex != "WHERE") {
		s1 = s1 + " " + lex;
		getlex();
	}
	if (lex != "WHERE") {
		printf ("WHERE is expected.\n");
		return 0;
	}
	std::stringstream strexpr(s1);
	//std::cerr << "**********" << is.str() << "*********" << std::endl;
	std::string buf = Expr(strexpr) + "$";
	//std::cerr << "s1 " << s1 << std::endl;
	str = str + " " + Where() + " " + fd + " " + buf;
	//fprintf(stdout, "%s\n", str.c_str());
	fprintf(f, "%s\n", str.c_str());
	if (fgetc(f) == '1') {
		printf ("ALL DONE.\n");
		if (unlink(tablename.c_str()) == -1) {
			perror("unlink");
		}
		if (link("tmp1", tablename.c_str()) == -1) {
			perror("link");
		}
		/*
		THandle th;
		if (openTable (tablename.c_str(), &th) != OK) {
		//if (openTable ("tmp1", &th) != OK) {
			std::cerr << "Table is not opened." << std::endl;
			return 0;
    }
    if (moveFirst (th) != OK) {
			std::cerr << "No record found." << std::endl;
			return 0;
		}
		unsigned count;
		getFieldsNum(th, &count);
		if (count == 0) {
			std::cerr << "Table is empty." << std::endl;
			return 0;
		}
		for (int i = 0; i < count; i++) {
			char *name;
			if (getFieldName (th, i, &name) != OK) {
				std::cerr << "Field is not accessible." << std::endl;
				return 0;
			}
			std::cerr << name;
			std::cerr << "\t\t";
		}
		std::cerr << std::endl;
		do {
			for (int i = 0; i < count; i++) {
				char *name;
				if (getFieldName (th, i, &name) != OK) {
					std::cerr << "Field is not accessible." << std::endl;
					return 0;
				}
				enum FieldType ft;
				if (getFieldType (th, name, &ft) != OK) {
					std::cerr << "Type is not accessible." << std::endl;
					return 0;
				}
				unsigned leng;
				getFieldLen (th, name, &leng);
				if (ft == Long) {
					long value;
					//enum Errors getLong(THandle tableHandle, const char *fieldName, long *pvalue);
					if (getLong (th, name, &value) != OK) {
						std::cerr << "Record is not accessible." << std::endl;
						return 0;
					}
					std::cerr << value;
					std::cerr << "\t\t";
				}
				else if (ft == Text) {
					char *value;
					//enum Errors getText(THandle tableHandle, const char *fieldName,char **pvalue);
					if (getText (th, name, &value) != OK) {
						std::cerr << "Record is not accessible." << std::endl;
						return 0;
					}

					std::string valstr(value); //to convert to string

					std::cerr << value;
					std::cerr << "\t\t";
				}

			}
			std::cerr << std::endl;
			moveNext (th);
		}
		//Bool afterLast(THandle tableHandle);
		while (afterLast(th) != TRUE);
		if (closeTable(th) != OK) {
			std::cerr << "Table is not closed." << std::endl;
      return 0;
    }
		*/
    unlink("tmp1");		//удаление временного файла
		return 1;
	}
	else return 0;
}

//<DELETE-предложение> ::= DELETE FROM <имя таблицы> <WHERE-клауза>
int DEL(FILE * f) {
	std::string str = "4";
	std::string s;
	if ((s = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return 0;
	}
	if (s != "FROM") {
		printf ("FROM is expected.\n");
		return 0;
	}
	std::string tablename;
	if ((tablename = ReadName()) == "") {
		printf ("Identifier is expected.\n");
		return 0;
	}
	str = str + tablename;
	std::cin >> s;
	if (s != "WHERE") {
		printf ("WHERE is expected.\n");
		return 0;
	}
	str = str + " " + Where();
	//fprintf(stdout, "%s\n", str.c_str());
	fprintf(f, "%s\n", str.c_str());
	if (fgetc(f) == '1') {
		printf ("ALL DONE.\n");
		return 1;
	}
	else return 0;
}
