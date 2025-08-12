/**
  Program for converting an outputImage to C array for Licar. The input
  outputImage must be 64x64, in PPM format (tested with the one exported by
  GIMP). Pass the outputImage on standard input, the array will be output on
  standard output and preview PPM outputImage will be saved. The array is saved
  in 565 indexed mode: first 512 bytes is the 565 palette, then 64x64 1 byte
  indices follow.
*/

#include <stdio.h>

#define IMG_SIZE 64

int inChar = 0;
int state = 0;
unsigned int inputPixels[IMG_SIZE * IMG_SIZE]; // 565 format
unsigned int palette[256];
unsigned char outputImage[IMG_SIZE * IMG_SIZE];

unsigned int rgbTo565(unsigned char red, unsigned char green,
  unsigned char blue)
{
  return (((unsigned int) (red / 8)) << 11) |
    (((unsigned int) (green / 4)) << 5) | (blue / 8);
}

void rgbFrom565(unsigned int colorIndex, unsigned char *red,
  unsigned char *green, unsigned char *blue)
{
  unsigned char value = colorIndex >> 11;
  *red = value != 31 ? value * 8 : 255;

  value = (colorIndex >> 5) & 0x3f;
  *green = value != 63 ? value * 4 : 255;

  value = colorIndex & 0x1f;
  *blue = value != 31 ? value * 8 : 255;
}

unsigned char findClosesPaletteColor(unsigned int color)
{
  unsigned char r, g, b;
  unsigned int bestDist = 0xffff;
  unsigned int bestIndex = 0;
    
  rgbFrom565(color,&r,&g,&b);

  for (int j = 0; j < 256; ++j)
  {
    unsigned char r2, g2, b2;
    unsigned int d;

    rgbFrom565(palette[j],&r2,&g2,&b2);

    d = (r > r2 ? r - r2 : (r2 - r)) +
        (g > g2 ? g - g2 : (g2 - g)) +
        (b > b2 ? b - b2 : (b2 - b));

    if (d == 0)
      return j;
    else if (d < bestDist)
    {
      bestIndex = j;
      bestDist = d;
    }
  }

  return bestIndex;
}

void savePreview(void)
{
  FILE *f = fopen("preview.ppm","w");

  fwrite("P6\n64\n64\n255\n",13,1,f);

  for (int i = 0; i < IMG_SIZE * IMG_SIZE; ++i)
  {
    unsigned char color[3];

    rgbFrom565(palette[outputImage[i]],color,color + 1,color + 2);
    fwrite(color,3,1,f);
  }

  fclose(f);
}

int main(void)
{
  while (state < 4) // just skip beyond the last field ("255<newline>")
  {
    inChar = getchar();

    switch (state)
    {
      case 0: state = inChar == '2' ? 1 : 0; break;
      case 1: state = inChar == '5' ? 2 : 0; break;
      case 2: state = inChar == '5' ? 3 : 0; break;
      case 3: state = inChar == 10  ? 4 : 0; break;
      default: break;
    }
  }

  for (int i = 0; i < 256; ++i) // start with 332 palette
    palette[i] = ((i >> 5) << 13) | (((i >> 2) & 0x07) << 8) | ((i & 0x3) << 3);

  for (int i = 0; i < IMG_SIZE * IMG_SIZE; ++i)
  {
    unsigned int r, g, b, color;

    r = getchar();
    g = getchar();
    b = getchar();

    color = rgbTo565(r,g,b);

    inputPixels[i] = color;

    // we're constructing the palette by bumping common colors higher up
    for (int j = 0; j < 256; ++j)
      if ((palette[j] & 0xf7de) == (color & 0xf7de)) // approximate comparison
      {
        if (j != 0)
        {
          // bump the color to higher place
          palette[j] = palette[j - 1];
          palette[j - 1] = color;
        }

        break;
      }
      else if (j == 255)
        palette[255 - (i % 16)] = color;
  }

  for (int i = 0; i < IMG_SIZE * IMG_SIZE; ++i)
    outputImage[i] = findClosesPaletteColor(inputPixels[i]);

  savePreview();

  for (int j = 0; j < 32; ++j)
  {
    printf("  ");

    for (int i = 0; i < 8; ++i)
    {
      unsigned int color = palette[j * 8 + i];
      printf("0x%02x,0x%02x,",color & 0xff,color >> 8);
    } 

    putchar('\n');
  }

  printf("  ");

  for (int i = 0; i < IMG_SIZE * IMG_SIZE; ++i)
  {
    printf("0x%02x",outputImage[i]);
  
    if (i != IMG_SIZE * IMG_SIZE - 1)
    {
      putchar(',');

      if ((i + 1) % 16 == 0)
        printf("\n  ");
    }
  }

  return 0;
}
