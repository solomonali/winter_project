all: producer consumer

producer: producer.c
	gcc -lmraa -lm -o producer producer.c LSM9DS0.c

consumer: consumer.c
	gcc -lmraa -lm -lfann -o consumer consumer.c

clean:
	rm -f data_* producer consumer

