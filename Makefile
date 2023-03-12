CC		= gcc
CFLAGS		=
LDFLAGS		=
LIBS		=
SRCS		= main.c myfunc.c
OBJS		= $(SRCS:.c=.o)
TARGET		= main


all:		clean $(TARGET)

$(TARGET):	$(OBJS)
		$(CC) $^ $(LDFLAGS) $(LIBS) -o $@

clean:;		rm -f *.o *~ $(TARGET)

