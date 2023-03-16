/**********************************************************************
 * Convert PHA ".dat" file to ".dat"+POSTFIX text file.               *
 *                                                                    *
 *     Size (the number of channels) and live time of each ".dat" is  *
 *     reported to console before converting it.                      *
 *                                                                    *
 *     Byte order compatibility is taken into count.                  *
 *                                                                    *
 *     By default, the output text is Windows (CRLF) adapted. This    *
 *     setting could be changed via modifying the definition of macro *
 *     TERMINATOR below in this source file.                          *
 *                                                                    *
 * Author  lyazj (https://github.com/lyazj)                           *
 * Mail    seeson@pku.edu.cn                                          *
 * Create  Nov 10 2022                                                *
 * Modify  Nov 12 2022                                                *
 **********************************************************************/

#include <stdint.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>

#define POSTFIX ".new.txt"
#define TERMINATOR "\r\n"

/* For mingwrt, this forces cmdline wildcard expanding. */
int _CRT_glob = -1;

#ifndef le32toh
/* This function is portable and will probably compile to a bswap on
 * supported BE platforms if suitable optimizations enabled.
 * Use of the macro from <endian.h> is avoided for compatibility. */
static uint32_t le32toh(uint32_t le32)
{
  uint8_t *u = (uint8_t *)&le32;
  return (uint32_t)u[0]
    | (uint32_t)(u[1] <<  8)
    | (uint32_t)(u[2] << 16)
    | (uint32_t)(u[3] << 24);
}
#endif  /* le32toh */

static void convert(const char *path)
{
  char buf[FILENAME_MAX + 1];
  uint32_t size, live, val, i;
  FILE *fin, *fout;

  if(strlen(path) + strlen(POSTFIX) > FILENAME_MAX)
  {
    fprintf(stderr, "%s: %s\n", path, "path too long");
    return;
  }
  strcpy(buf, path);
  strcat(buf, ".new.txt");

  if((fin = fopen(path, "rb")) == NULL)
  {
    fprintf(stderr, "fopen: %s: %s\n", path, strerror(errno));
    return;
  }

  /* "wb": we will explicitly force terminators afterwards */
  if((fout = fopen(buf, "wb")) == NULL)
  {
    fprintf(stderr, "fopen: %s: %s\n", buf, strerror(errno));
    goto convert_cleanup_fin;
  }

  /* read the 4-byte size (the number of channels) */
  if(fread(&size, 1, sizeof size, fin) != sizeof size)
  {
    fprintf(stderr, "fread: %s: %s\n", path, "error reading size");
    goto convert_cleanup_fout;
  }
  size = le32toh(size);

  /* discard the unknown 2-byte data */
  if(fread(&val, 1, 2, fin) != 2)
  {
    fprintf(stderr, "fread: %s: %s\n", path, "error reading gap bytes");
    goto convert_cleanup_fout;
  }

  /* read the 4-byte live time (higher 2 bytes used only) */
  if(fread(&live, 1, sizeof live, fin) != sizeof live)
  {
    fprintf(stderr, "fread: %s: %s\n", path, "error reading live time");
    goto convert_cleanup_fout;
  }
  live = le32toh(live);

  printf("converting: %-16s\t%4u channels\t%4u live seconds\n",
      path, (unsigned)size, (unsigned)live);

  /* transcript SIZE channel counter values */
  for(i = 0; i < size; ++i)
  {
    if(fread(&val, 1, sizeof val, fin) != sizeof val)
    {
      fprintf(stderr, "fread: %s: %s\n", path, "error reading counter value");
      goto convert_cleanup_fout;
    }
    val = le32toh(val);
    fprintf(fout, "%u" TERMINATOR, (unsigned)val);
  }
  /* trailing 8 bytes (if any) with unknown meaning discarded */

convert_cleanup_fout:
  fclose(fout);

convert_cleanup_fin:
  fclose(fin);
}

int main(int argc, const char *argv[])
{
  int i;

  if(argc == 1)
  {
    fprintf(stderr, "usage: %s <datfiles>\n", argv[0]);
    return 1;
  }
  for(i = 1; i < argc; ++i)
    convert(argv[i]);
  return 0;
}
