/* write out the documentation for the compose key */

/* copy the string from Fl_Input.C */
static const char* const compose_pairs =
"  ! @ # $ y=| & : c a <<~ - r _ * +-2 3 ' u p . , 1 o >>141234? "
"A`A'A^A~A:A*AEC,E`E'E^E:I`I'I^I:D-N~O`O'O^O~O:x O/U`U'U^U:Y'DDss"
"a`a'a^a~a:a*aec,e`e'e^e:i`i'i^i:d-n~o`o'o^o~o:-:o/u`u'u^u:y'ddy:";

#include <stdio.h>

int main() {
  int x,y;
  for (x = 0; x<16; x++) {
    for (y = 0; y<6; y++) {
      const char *p = compose_pairs + (16*y+x)*2;
      if (p[1] == ' ')
	printf("<td><code>%c&nbsp</code>&nbsp&nbsp&nbsp%c\n",
	       p[0],(p-compose_pairs)/2+0xA0);
      else
	printf("<td><code>%c%c</code>&nbsp&nbsp&nbsp%c\n",
	       p[0],p[1],(p-compose_pairs)/2+0xA0);
    }
    printf("<tr>");
  }
  return 0;
}
