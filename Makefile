CC = gcc
CFLAGS = -Wall -Wextra -pthread
TARGET = os_project
SRCS = src/main.c src/producer.c src/consumer.c src/common/utils.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)
	./$(TARGET) configs/config.txt

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJS)

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

exp1: $(TARGET)
	./$(TARGET) configs/exp1_LowLoad.txt

exp2: $(TARGET)
	./$(TARGET) configs/exp2_HighLoad.txt

exp3: $(TARGET)
	./$(TARGET) configs/exp3_Deadlock.txt

exp4: $(TARGET)
	./$(TARGET) configs/exp4_Bottleneck.txt

exp5: $(TARGET)
	./$(TARGET) configs/exp5_CircularDependency.txt

clean:
	rm -f $(OBJS) $(TARGET)