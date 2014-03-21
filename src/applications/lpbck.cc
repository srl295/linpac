/* Loopback command */
#include <stdio.h>
#include <string.h>

char inpline[256];
int n = 0;

int main()
{
  fprintf(stdout, "This is LOOPBACK (end with /ex)\n");
  setbuf(stdout, NULL);
  do
  {
    if (fgets(inpline, 256, stdin) == NULL)
    {
      fprintf(stderr, "Could not read input\n");
      break;
    }
    fprintf(stdout, "(%i) Read: %s", n++, inpline);
  } while (strcmp(inpline, "/ex\n") != 0);
  fprintf(stdout, "Finished\n");
  return 0;
}

