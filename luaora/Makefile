CC		= gcc
OCI_FLAGS	= -I$(ORACLE_HOME)/rdbms/demo -I$(ORACLE_HOME)/rdbms/public -L$(ORACLE_HOME)/lib -Xlinker -rpath $(ORACLE_HOME)/lib
OCI_LIBS	= -lclntsh
CFLAGS		= -DLUA_MODULE `pkg-config lua5.1 --cflags` -shared -fPIC -fno-rtti -fno-exceptions $(OCI_FLAGS)
LIBS		= $(OCI_LIBS)

all:
	$(CC) $(CFLAGS) -o luaora.so luaora.cpp $(LIBS)
