test:
	gcc -lncursesw -o test test.c win.c text.c -g
clean:
	rm test
