#reishaul1@gmail.com
# Makefile for the systems2 project
# List of subdirectories
SUBDIRS := part_1 part_2 part_3 part_4 part_5 part_6 

all: $(SUBDIRS)
# Build all subdirectories
$(SUBDIRS):
	$(MAKE) -C $@

# Clean all subdirectories with for loop
clean:
	for dir in $(SUBDIRS); do \
		$(MAKE) -C $$dir clean; \
	done
# Ensure that the clean target is phony
.PHONY: all clean part_1 part_2 part_3 part_4 part_5 part_6