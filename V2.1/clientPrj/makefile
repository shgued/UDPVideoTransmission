# the compiler: gcc for C program, define as g++ for C++
CC = g++

# compiler flags:
#  -g    adds debugging information to the executable file
#  -Wall turns on most, but not all, compiler warnings
CFLAGS  = -g -std=c++11
RFLAGS  = -Wall 
PROJECT_PATH = .
# define any directories containing header files other than /usr/include
INCLUDES = -I/usr/local/include
# define library paths in addition to /usr/lib
LFLAGS = -L/usr/local/lib 

# define any libraries to link into executable:
#   if I want to link in libraries (libx.so or libx.a) I use the -llibname 
#   option, something like (this will link in libmylib.so and libm.so:
#LIBS = -lopencv_core -lopencv_imgproc -lopencv_highgui -lopencv_ml -lopencv_videoio -lopencv_features2d -lopencv_calib3d -lopencv_objdetect -lopencv_flann -lopencv_imgcodecs

LIBS = -lopencv_core  -lopencv_highgui  -lopencv_videoio -lopencv_imgcodecs -lpthread

# define the C++ source files
SRCS = client.cpp

# define the C object files 
#
# This uses Suffix Replacement within a macro:
#   $(name:string1=string2)
#         For each word in 'name' replace 'string1' with 'string2'
# Below we are replacing the suffix .c of all words in the macro SRCS
# with the .o suffix
#
OBJS = $(SRCS:.cpp=.o)

DIRS = ./
# define the executable file 
MAIN = client
RELEASE = R
#
# The following part of the makefile is generic; it can be used to 
# build any executable just by changing the definitions above and by
# deleting dependencies appended to the file from 'make depend'
#

.PHONY: depend clean

all:$(MAIN)
	@echo  Simple compiler named kit has been compiled 

$(MAIN): $(OBJS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS) 

$(RELEASE): $(OBJS)
	$(CC) $(RFLAGS) $(INCLUDES) -o $(MAIN) $(OBJS) $(LFLAGS) $(LIBS) 

# this is a suffix replacement rule for building .o's from .c's
# it uses automatic variables $<: the name of the prerequisite of
# the rule(a .c file) and $@: the name of the target of the rule (a .o file) 
# (see the gnu make manual section about automatic variables)
.cpp.o:
	$(CC) $(CFLAGS) $(INCLUDES) -c $<  -o $@

clean:
	-$(RM) *.o $(MAIN)
	@#-for d in $(DIRS); do (cd $$d; $(MAKE) clean); done
	
	
depend: $(SRCS)
	makedepend $(INCLUDES) $^

# DO NOT DELETE THIS LINE -- make depend needs it
