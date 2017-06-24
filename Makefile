CXX=g++
CPPFALGS= -std=c++11
target=data_reader csv_reader 
SRC_DIR=src
INC_DIR=include
EXE_DIR=bin
OBJ_DIR=obj
OBJ=obj/utils.o

.PHONY: clean all

all: DIR ${target}

vpath %.h ${INC_DIR}/
vpath %.cpp ${SRC_DIR}/


INCLUDE= -I ${INC_DIR}/

DIR:
	@echo "\033[36;1m Checking object and executable directories \033[0m"
	@mkdir -p ${EXE_DIR}
	@mkdir -p ${OBJ_DIR}

data_reader:${OBJ} ${SRC_DIR}/read_data.cpp 
	@echo "\033[33;1m Building executables $@\033[0m"
	@${CXX} ${CPPFALGS} ${INCLUDE} -o ${EXE_DIR}/$@ $^

csv_reader: ${OBJ} ${SRC_DIR}/read_csv.cpp
	@echo "\033[33;1m Building executables $@\033[0m"
	@${CXX} ${CPPFLAGS} ${INCLUDE} -o ${EXE_DIR}/$@ $^

obj/%.o: src/%.cpp include/%.h
	${CXX} ${CPPFLAGS} ${INCLUDE} -o $@ -c $<

clean:
	rm -f ${EXE_DIR}/* ${OBJ_DIR}/*
