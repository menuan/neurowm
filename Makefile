#-----------------------------------------------------------------------------------------------------------------------
# CONFIG
#-----------------------------------------------------------------------------------------------------------------------

# Version and Name
PKG_VERSION = 0.20
PKG_NAME = neurowm
PKG_MYNAME = my${PKG_NAME}

# Compiler flags
CC = gcc
#DFLAGS = -DPKG_VERSION=\"${PKG_VERSION}\" -DPKG_NAME=\"${PKG_NAME}\" -DPKG_MYNAME=\"${PKG_MYNAME}\"
DFLAGS = -DNDEBUG -DPKG_VERSION=\"${PKG_VERSION}\" -DPKG_NAME=\"${PKG_NAME}\" -DPKG_MYNAME=\"${PKG_MYNAME}\"
CFLAGS = -Wall -Wextra -Wformat -Werror -Wfatal-errors -Wpedantic -pedantic-errors -fpic -O3 ${DFLAGS}
LDADD = -lX11 -pthread
LDADDTEST = -lX11 -pthread -lcunit


# Mod names
MOD_NAMES = neurowm config dzen event rule workspace layout client core system geometry type theme action

# Source names
SOURCE_BIN_NAME = main.c
SOURCE_NEUROWM_TEST_NAME = ${PKG_NAME}_test.c
SOURCE_CUNIT_TEST_NAME = cunit_test.c

# Object names
OBJECT_BIN_NAME = main.o
OBJECT_NEUROWM_TEST_NAME = ${PKG_NAME}_test.o
OBJECT_CUNIT_TEST_NAME = cunit_test.o

# Target names
TARGET_BIN_NAME = ${PKG_NAME}
TARGET_STATIC_LIB_NAME = lib${TARGET_BIN_NAME}.a
TARGET_SHARED_LIB_NAME = lib${TARGET_BIN_NAME}.so.${PKG_VERSION}
TARGET_SHARED_LNK_NAME = lib${TARGET_BIN_NAME}.so
TARGET_NEUROWM_TEST_NAME = ${PKG_MYNAME}_test
TARGET_CUNIT_TEST_NAME = cunit_test

# Source directories
SOURCE_DIR = src
SOURCE_NEURO_DIR = ${SOURCE_DIR}/neuro
SOURCE_TEST_DIR = ${SOURCE_DIR}/test

# Target directories
TARGET_DIR = build
TARGET_OBJ_DIR = ${TARGET_DIR}/obj
TARGET_LIB_DIR = ${TARGET_DIR}/lib
TARGET_BIN_DIR = ${TARGET_DIR}/bin

# Do not change
OBJS = $(addprefix ${TARGET_OBJ_DIR}/, $(addsuffix .o, ${MOD_NAMES}))
HDRS = $(addprefix ${SOURCE_NEURO_DIR}/, $(addsuffix .h, ${MOD_NAMES}))
CLEANEXTS = ${SOURCE_NEURO_DIR}/*~ ${SOURCE_TEST_DIR}/*~ ${SOURCE_DIR}/*~ ${SOURCE_TEST_DIR}/*~ ${OBJS} \
            ${TARGET_LIB_DIR}/${TARGET_STATIC_LIB_NAME} ${TARGET_LIB_DIR}/${TARGET_SHARED_LIB_NAME} \
            ${TARGET_LIB_DIR}/${TARGET_SHARED_LNK_NAME} ${TARGET_BIN_DIR}/${TARGET_BIN_NAME} \
            ${TARGET_OBJ_DIR}/${OBJECT_BIN_NAME} ${TARGET_OBJ_DIR}/${OBJECT_NEUROWM_TEST_NAME} \
            ${TARGET_OBJ_DIR}/${OBJECT_CUNIT_TEST_NAME} ${TARGET_BIN_DIR}/${TARGET_NEUROWM_TEST_NAME} \
            ${TARGET_BIN_DIR}/${TARGET_CUNIT_TEST_NAME}


#-----------------------------------------------------------------------------------------------------------------------
# BUILDING
#-----------------------------------------------------------------------------------------------------------------------

all: static_lib shared_lnk main test

test: neurowm_test cunit_test

obj: ${OBJS}

# build/obj/*.o: src/neuro/*.c
${TARGET_OBJ_DIR}/%.o: ${SOURCE_NEURO_DIR}/%.c
	@${CC} ${CFLAGS} -c -o $@ $<
	@echo "Compiling $<"

# build/obj/main.o: src/main.c
${TARGET_OBJ_DIR}/${OBJECT_BIN_NAME}: ${SOURCE_DIR}/${SOURCE_BIN_NAME}
	@${CC} ${CFLAGS} -c -o $@ $<
	@echo "Compiling $<"

# build/obj/neurowm_test.o: src/test/neurowm_test.c
${TARGET_OBJ_DIR}/${OBJECT_NEUROWM_TEST_NAME}: ${SOURCE_TEST_DIR}/${SOURCE_NEUROWM_TEST_NAME}
	@${CC} ${CFLAGS} -c -o $@ $<
	@echo "Compiling $<"

# build/obj/cunit_test.o: src/test/cunit_test.c
${TARGET_OBJ_DIR}/${OBJECT_CUNIT_TEST_NAME}: ${SOURCE_TEST_DIR}/${SOURCE_CUNIT_TEST_NAME}
	@${CC} ${CFLAGS} -c -o $@ $<
	@echo "Compiling $<"

main: ${OBJS} ${TARGET_OBJ_DIR}/${OBJECT_BIN_NAME}
	@${CC} ${CFLAGS} -o ${TARGET_BIN_DIR}/${TARGET_BIN_NAME} ${TARGET_OBJ_DIR}/${OBJECT_BIN_NAME} ${OBJS} ${LDADD}
	@echo "Linking   ${TARGET_BIN_DIR}/${TARGET_BIN_NAME}"

neurowm_test: ${OBJS} ${TARGET_OBJ_DIR}/${OBJECT_NEUROWM_TEST_NAME}
	@${CC} ${CFLAGS} -o ${TARGET_BIN_DIR}/${TARGET_NEUROWM_TEST_NAME} ${TARGET_OBJ_DIR}/${OBJECT_NEUROWM_TEST_NAME} ${OBJS} ${LDADDTEST}
	@echo "Linking   ${TARGET_BIN_DIR}/${TARGET_NEUROWM_TEST_NAME}"

cunit_test: ${OBJS} ${TARGET_OBJ_DIR}/${OBJECT_CUNIT_TEST_NAME}
	@${CC} ${CFLAGS} -o ${TARGET_BIN_DIR}/${TARGET_CUNIT_TEST_NAME} ${TARGET_OBJ_DIR}/${OBJECT_CUNIT_TEST_NAME} ${OBJS} ${LDADDTEST}
	@echo "Linking   ${TARGET_BIN_DIR}/${TARGET_CUNIT_TEST_NAME}"

static_lib: obj
	@ar -cq ${TARGET_LIB_DIR}/${TARGET_STATIC_LIB_NAME} ${OBJS}
	@echo "Creating  ${TARGET_LIB_DIR}/${TARGET_STATIC_LIB_NAME}"

shared_lib: obj
	@${CC} -shared -o ${TARGET_LIB_DIR}/${TARGET_SHARED_LIB_NAME} ${OBJS}
	@echo "Creating  ${TARGET_LIB_DIR}/${TARGET_SHARED_LIB_NAME}"

shared_lnk: shared_lib
	@ln -s -r ${TARGET_LIB_DIR}/${TARGET_SHARED_LIB_NAME} ${TARGET_LIB_DIR}/${TARGET_SHARED_LNK_NAME}
	@echo "Creating  ${TARGET_LIB_DIR}/${TARGET_SHARED_LNK_NAME}"

clean:
	@rm -f ${CLEANEXTS}


#-----------------------------------------------------------------------------------------------------------------------
# INSTALL / UNINSTALL
#-----------------------------------------------------------------------------------------------------------------------

# Install no bin
install_no_bin:
	@echo -n ":: Installing headers...   "
	@mkdir -p /usr/include/neuro
	@cp ${HDRS} /usr/include/neuro
	@echo "OK"
	@echo -n ":: Installing libraries...   "
	@mkdir -p /usr/lib/neuro
	@cp ${TARGET_LIB_DIR}/${TARGET_STATIC_LIB_NAME} /usr/lib/neuro
	@cp ${TARGET_LIB_DIR}/${TARGET_SHARED_LIB_NAME} /usr/lib/neuro
	@ln -s -r -f /usr/lib/neuro/${TARGET_SHARED_LIB_NAME} /usr/lib/neuro/${TARGET_SHARED_LNK_NAME}
	@echo "OK"
	@echo -n ":: Installing man page...   "
	@mkdir -p /usr/local/man/man1
	@cp man/neurowm.1 /usr/local/man/man1
	@chmod 644 /usr/local/man/man1/neurowm.1
	@echo "OK"
	@echo -n ":: Installin themes...   "
	@mkdir -p /usr/share/themes/nnoell
	@cp -r themes/nnoell/neurowm /usr/share/themes/nnoell
	@echo "OK"

# Install
install: install_no_bin
	@echo -n ":: Installing binary...   "
	@cp ${TARGET_BIN_DIR}/${TARGET_BIN_NAME} /usr/bin
	@echo "OK"

# Uninstall no bin
uninstall_no_bin:
	@echo -n ":: Uninstalling headers...   "
	@rm -rf /usr/include/neuro
	@echo "OK"
	@echo -n ":: Uninstalling libraries...   "
	@rm -rf /usr/lib/neuro
	@echo "OK"
	@echo -n ":: Uninstalling man page...   "
	@rm -f /usr/local/man/man1/neurowm.1
	@echo "OK"
	@echo -n ":: Uninstalling themes...   "
	@rm -rf /usr/share/themes/nnoell/neurowm
	@echo "OK"

# Uninstall
uninstall: uninstall_no_bin
	@echo -n ":: Uninstalling binary...   "
	@rm -f /usr/bin/neurowm
	@echo "OK"
