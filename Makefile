CFLAGS=-Wall -O -g -ansi 
LDFLAGS=
LDLIBS=-lsqlite3

OBJECTS=storage.o test.o

all: test serql_test

test: $(OBJECTS)
	$(CC) -o test $(CFLAGS) $(LDFLAGS) $(LDLIBS) $(OBJECTS)

serql.yy.o serql.tab.o: serql.y serql.l
	bison -bserql -d serql.y
	flex -oserql.yy.c -i serql.l
	$(CC) -g -c serql.tab.c
	$(CC) -g -c serql.yy.c
	rm serql.tab.h serql.tab.c serql.yy.c

serql_test: serql.yy.o serql.tab.o pool.o serql_test.o
	$(CC) -o serql_test $(LDFLAGS) $(LDBLIBS) \
		serql.yy.o serql.tab.o pool.o serql_test.o

clean:
	- rm test serql_test 
	- rm *.o
