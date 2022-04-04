.PHONY: missing_ids setup

SRC_DIR := ./src
BUILD_DIR  := ./build
INCLUDE_DIR := ./include

ACCUM_FILE := rolls-1
# Replace it with where the libcsv directory is saved
LIBCSV_DIR := $${HOME}/libcsv-3.0.3

INCLUDES := -I${LIBCSV_DIR}/include -I$(INCLUDE_DIR)# Add libcsv include directory
CPPFLAGS := -MMD -MP
CFLAGS := -Wall -g ${INCLUDES} -std=c89 -Werror-implicit-function-declaration # Compile with these flags. Use gnu99 because of argp
# Linking libraries for every file does not actually do any harm since if the library is not used, it is not added.
LDLIBS := -lcsv # Link libraries for final compilation
LDFLAGS := -L ${LIBCSV_DIR}/lib -Wl,-R,${LIBCSV_DIR}/lib -static # Statically link so executables can be moved and link files as needed

SRC_FILES := $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES := $(SRC_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEPS := $(OBJS:.o=.d)
# OBJ_FILES := $(patsubst ${SRC_DIR}/%.c,${BUILD_DIR}/%.o,${SOURCE})

TARGETS := main

all : $(TARGETS)

# Can't use implicit rules because of build and src directories
# The use of : % : is done for implicit pattern substitution of expanding targets to another target
$(TARGETS): $(OBJ_FILES)
	$(CC) $(LDFLAGS) $^ $(LDLIBS) -o $@

$(BUILD_DIR)/%.o : $(SRC_DIR)/%.c
	mkdir -p $(BUILD_DIR)
	$(CC) $(CPP_FLAGS) $(CFLAGS) -c $< -o $@

setup:
	mkdir -p ./src ./build ./include

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

-include $(DEPS)
