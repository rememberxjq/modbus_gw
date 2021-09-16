ifdef test
all : test.o linklist.o parse_db_config.o radis_data_opt.o
	gcc -o gw test.o parse_db_config.o radis_data_opt.o linklist.o -lsqlite3 -lhiredis -lmodbus -lcjson -lpthread

test.o : test.c linklist.h radis_data_opt.h parse_db_config.h
	gcc -c test.c -lsqlite3 -lhiredis -lmodbus -lcjson -lpthread

parse_db_config.o : parse_db_config.c test.h
	gcc -c parse_db_config.c -lsqlite3 -lhiredis -lmodbus -lcjson -lpthread

radis_data_opt.o : radis_data_opt.c test.h
	gcc -c radis_data_opt.c -lsqlite3 -lhiredis -lmodbus -lcjson -lpthread
endif

ifdef real
all : main.o linklist.o parse_db_config.o radis_data_opt.o
	gcc -o gw main.o parse_db_config.o radis_data_opt.o linklist.o -lsqlite3 -lhiredis -lmodbus -lcjson -lpthread

main.o : main.c linklist.h radis_data_opt.h parse_db_config.h
	gcc -c main.c -lsqlite3 -lhiredis -lmodbus -lcjson -lpthread

parse_db_config.o : parse_db_config.c main.h
	gcc -c parse_db_config.c -lsqlite3 -lhiredis -lmodbus -lcjson -lpthread

radis_data_opt.o : radis_data_opt.c main.h
	gcc -c radis_data_opt.c -lsqlite3 -lhiredis -lmodbus -lcjson -lpthread
endif

clean :
	rm gw *.o tb_forward >/dev/null