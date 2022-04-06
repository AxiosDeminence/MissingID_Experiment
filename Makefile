.PHONY: missing_ids setup

SRC_DIR := ./src
DEP_DIR := ./dep
BUILD_DIR  := ./build
INCLUDE_DIR := ./include

ACCUM_FILE := rolls-1
# Replace it with where the libcsv directory is saved
LIBCSV_DIR := $${HOME}/libcsv-3.0.3

# Add libcsv and include directory
INCLUDES := -I${LIBCSV_DIR}/include -I$(INCLUDE_DIR)

# Making sure this does not get immediately computed. Use automatic dependency generation for header files as well
CPPFLAGS = -MT $@ -MMD -MP -MF $(DEP_DIR)/$*.d

# Enable all warnings as errors except unused params and missing field initializers (library functions and initializers)
CFLAGS := -O2 -Wall -Wextra -Wpedantic -g -ansi -Werror -Wno-error=unused-parameter -Wno-error=missing-field-initializers $(INCLUDES)

# Linking libraries for every file does not actually do any harm since if the library is not used, it is not added.
LDLIBS := -lcsv # Link libraries for final compilation
LDFLAGS := -L ${LIBCSV_DIR}/lib -Wl,-R,${LIBCSV_DIR}/lib # Statically link so executables can be moved and link files as needed

# Different
SRC_FILES = $(wildcard $(SRC_DIR)/*.c)
OBJ_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(BUILD_DIR)/%.o)
DEP_FILES = $(SRC_FILES:$(SRC_DIR)/%.c=$(DEP_DIR)/%.d)

all : main

$(OBJ_FILES) : $(BUILD_DIR)/%.o : $(SRC_DIR)/%.c $(DEP_DIR)/%.d | $(BUILD_DIR) $(DEP_DIR)
	$(CC) $(CPPFLAGS) $(CFLAGS) -c $< -o $@

$(DEP_FILES): ;

# Can't use implicit rules because of build and src directories.
# Must be in this order for proper linking.
main : $(BUILD_DIR)/main.o $(BUILD_DIR)/dynamic_long_array.o $(BUILD_DIR)/missing_id.o
	$(CC) $(CFLAGS) $(LDFLAGS) $^ $(LDLIBS) -o $@

# Creation of folders if they do not exist
$(BUILD_DIR) : ; @mkdir -p $@
$(DEP_DIR) : ; @mkdir -p $@

setup:
	@mkdir -p ./src ./build ./include
	@libcsv-3.0.3/configure && make -C 

clean:
	@rm -r ./build/*

-include $(DEP_FILES)

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
