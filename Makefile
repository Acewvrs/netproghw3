default: control.out client.out

# On your local machine (and on Submitty), do not put files in a part1/ directory
# The part1/ is required because of how Submitty handles the multi-language gradeables
# If you add more source files, you will need to make sure each of them has part1/
# in front of it.
client.out: part1/client.c
	gcc -g part1/client.c libunp.a -o client.out -lm
control.out: part1/server.c
	gcc -g part1/server.c libunp.a -o control.out -lm
