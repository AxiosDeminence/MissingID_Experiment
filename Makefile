.PHONY: missing_ids

SRC_DIR := ./src/
BUILD_DIR  := ./build/

ACCUM_FILE := rolls-1
# Replace it with where the libcsv directory is saved
LIBCSV_DIR := $${HOME}/libcsv-3.0.3/

INCLUDES := -I${LIBCSV_DIR}include # Add libcsv include directory
CFLAGS := -Wall -g ${INCLUDES} -MMD -std=c89 -Werror-implicit-function-declaration # Compile with these flags. Use gnu99 because of argp
# Linking libraries for every file does not actually do any harm since if the library is not used, it is not added.
LDLIBS := -lcsv # Link libraries for final compilation
LDFLAGS := -L ${LIBCSV_DIR}lib -Wl,-R,${LIBCSV_DIR}lib -static # Statically link so executables can be moved and link files as needed

SRC_FILES := $(wildcard ${SRC_DIR}*.c)
OBJ_FILES := $(patsubst ${SRC_DIR}%.c,${BUILD_DIR}%.o,${SOURCES})

TARGETS := main testbench missing_id

# Make the missing_id target also rely on the dynamic_long_array
all : ${TARGETS}

main : ${BUILD_DIR}missing_id.o ${BUILD_DIR}dynamic_long_array.o
missing_id : ${BUILD_DIR}missing_id.o ${BUILD_DIR}dynamic_long_array.o

# Can't use implicit rules because of build and src directories
# The use of : % : is done for implicit pattern substitution of expanding targets to another target
${TARGETS}: % : ${BUILD_DIR}%.o
	${CC} ${LDFLAGS} $^ ${LDLIBS} -o ${BIN_DIR}$@

${BUILD_DIR}%.o : ${SRC_DIR}%.c 
	${CC} ${CFLAGS} -c $^ -o $@

csvvalid : csvvalid.c
	${CC} ${CFLAGS} -c $<
	${CC} ${LDFLAGS} csvvalid.o ${LDLIBS} -o $@ # gcc ${BUILD_OPTS} -o csvtest -L${libcsv_DIR}/lib csvtest.o -lcsv -Wl,-R${libcsv_DIR}/lib

csvtest : csvtest.c
	${CC} ${CFLAGS} -c $<
	${CC} ${LDFLAGS} csvtest.o ${LDLIBS} -o $@ # gcc ${BUILD_OPTS} -o csvtest -L${libcsv_DIR}/lib csvtest.o -lcsv -Wl,-R${libcsv_DIR}/lib

csvfix : csvfix.c
	gcc ${BUILD_OPTS} -c -I${libcsv_DIR}/include csvfix.c
	gcc ${BUILD_OPTS} -o csvfix -L${libcsv_DIR}/lib csvfix.o -lcsv -Wl,-R${libcsv_DIR}/lib
	
missing_ids: $(accum_file).tsv working.tsv
# Means the following
# 1) Traverse the rolls-1.tsv and working.tsv files by lines
# 2) If it is the first line ever traversed or not the first line in the file, add to output
# 3) Pipe the output and sort it by unique values and IDs (first column)
# 4) Pipe the sorted output to another awk
# 5) Given a comma separated values, if it is the first line ever traversed, set variable p to the first (field - 1), continue to next line
# 6) If the current line's value is not equal to p+1, then print the gap between the two lines
# 7) Set p to the current value value. Reiterate for each line 
# awk '(NR==1) || (FNR > 1)' $(accum_file).tsv working.tsv | sort -nu | awk -F, 'NR==1{p=$$1-1; next} $$1!=p+1{print p+1"-"$$1-1} {p=$$1}'

# Same steps as above except
# 5) If the current line is not the header and the first column is not equal to p+1 then, print gap
# 5a) Takes advantage of awk functionality where an uninitialized variable+1 will always pass
# 5b) Set p to the current line. Reiterate for each line
	awk '(NR==1) || (FNR > 1)' $(accum_file).tsv working.tsv | sort -nu | awk -F "\t" '(FNR>1 && $$1!=p+1){print p+1"-"$$1-1} {p=$$1}' | head -2 | tail -1

merge_work: $(accum_file).tsv working.tsv
	cat working.tsv >> $(accum_file).tsv
	sort -o $(accum_file).tsv -nu $(accum_file).tsv 
	> working.tsv

merge_complete:
	awk '(NR==1) || (FNR > 1)' rolls-*.tsv | sort -nu > rolls_total.tsv
