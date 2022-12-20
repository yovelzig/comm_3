all: sender reciever

reciever: reciever.c
	gcc -o reciever reciever.c

sender: sender.c
	gcc -o sender sender.c

clean:
	rm -f *.o reciever sender