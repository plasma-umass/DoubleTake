/*133 EXPORT_SYMBOL char **HX_split(const char *str, const char *delim,

 134     int *cp, int max)

 135 {

 141         int count = 0;

 142         char **ret;

 143 

 144         if (cp == NULL)

 145                 cp = &count;

 146         *cp = 1;

 147 

 148         {

 149                 const char *wp = str;

 150                 while ((wp = strpbrk(wp, delim)) != NULL) {

 151                         ++*cp;

 152                         ++wp;

 153                 }

 154         }

 155 

 156         if (max == 0) //if (max == 0 || *cp < max)

 157                 max = *cp;

 158         else if (*cp > max)

 159                 *cp = max;

 160 

 161         ret = malloc(sizeof(char *) * (*cp + 1));

 162         ret[*cp] = NULL;

 163 

 164         {

 165                 char *seg, *wp = HX_strdup(str), *bg = wp;

 166                 size_t i = 0;

 167 

 168                 while (--max > 0) {

 169                         seg      = HX_strsep(&wp, delim);

 170                         ret[i++] = HX_strdup(seg);

 171                 }

 172 

 173                 ret[i++] = HX_strdup(wp);

 174                 free(bg);

 175         }

 176 

 177         return ret;

 178 }


Affects all versions prior to, and including, 3.5.
*/
#include <stdlib.h>
#include <string.h>
#include <stdio.h>

int main(int argc, char* argv[]){
  int count = 4;
	char **ret;
	int *cp = &count;
	int max = argc; //overflow when argc > (4 + 1)
	if(max == 0) { //To corrct:  if (max == 0 || *cp < max)
		max = *cp;
  }
	else if(*cp > max) {
		*cp = max;
  }

	ret = (char **) malloc(sizeof(char*) * (*cp + 1));
  //fprintf(stderr, "max is %d *cp is %d ret is from %p to %p\n", max, *cp, ret, (void *)((unsigned long)ret + (sizeof(char*) * (*cp + 1))));
	ret[*cp] = NULL;
	
	{
		size_t i = 0;
		while(--max > 0){
			ret[i] = (char*)malloc(strlen(argv[i]) + 1);
			strcpy(ret[i], argv[i]);
    //  fprintf(stderr, "copy %d to %p &ret[i] is %p\n", i, ret[i], &ret[i]);
			i++;
		}
    fprintf(stderr, "The value of 0x2aab6c531058 is %lx\n", *((unsigned long *)0x2aab6c531058));
		ret[i] = (char*)malloc(strlen(argv[i]) + 1);
		strcpy(ret[i], argv[i]);
	}
  //free(ret);
	return 0;
}

