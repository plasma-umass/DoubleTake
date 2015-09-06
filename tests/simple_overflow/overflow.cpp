void overflow (void * buf, int actualSize, int overflowSize)
{
  char * ptr = (char *) buf;
  for (int i = 0; i < overflowSize; i++) {
    ptr[actualSize+i] = 'x';
  }
}
