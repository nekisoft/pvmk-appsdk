/**
  Converts car 3D model in obj format to the game's internal C arrays. The model
  is to be passed on stdin.

  A specific subset of obj format is assumed. UVs must be present but not
  normals or other such things.

  The following materials must be set for model triangles: "a" for body, "b"
  for front wheels, "c" and "d" for back wheels.
*/

#include <stdio.h>

#define UNIT 512
#define ARRAY_MAX 1024

#define UV_OFFSET -0.01

int vertices[ARRAY_MAX * 3];
int vertexTypes[ARRAY_MAX];
int vertexCount;

int uvs[ARRAY_MAX * 2];
int uvCount;

int triangles[ARRAY_MAX * 3];
int triangleUvs[ARRAY_MAX * 3];
int triangleCount;

char line[128];

void error(int n)
{
  printf("ERROR %d\n",n);
  exit(1);
}

void writeArray(int *array, unsigned int len, const char *name,
  unsigned int align)
{
  printf("%s[] =\n{",name);

  for (int i = 0; i < len; ++i)
  {
    if (i != 0)
      printf(", ");

    if (i % align == 0)
      printf("\n  ");

    printf("%d",array[i]);
  }

  puts("\n};\n");
}

int main(void)
{
  unsigned char vertexType = 0;

  while (1)
  {
    if (gets(line) == NULL)
      break;

    if (line[0] == 'f') // face
    {
       unsigned int n[6];

       if (sscanf(line + 1,"%u/%u %u/%u %u/%u",
         n,n + 1,n + 2,n + 3,n + 4,n + 5) != 6)
         error(0);

       for (int i = 0; i < 6; ++i)
         n[i]--; // obj indices are 1-based

       triangles[triangleCount * 3] = n[0];
       triangles[triangleCount * 3 + 1] = n[4];
       triangles[triangleCount * 3 + 2] = n[2];

       triangleUvs[triangleCount * 3] = n[1];
       triangleUvs[triangleCount * 3 + 1] = n[5];
       triangleUvs[triangleCount * 3 + 2] = n[3];

       vertexTypes[n[0]] = vertexType;
       vertexTypes[n[2]] = vertexType;
       vertexTypes[n[4]] = vertexType;

       triangleCount++;

       if (triangleCount >= ARRAY_MAX)
         error(1);
    }
    else if (line[0] == 'v' && line[1] != 't') // vertex
    {
       float a, b, c;

       if (sscanf(line + 1,"%f %f %f",&a,&b,&c) != 3)
         error(2);

       vertices[3 * vertexCount] = a * UNIT;
       vertices[3 * vertexCount + 1] = b * UNIT;
       vertices[3 * vertexCount + 2] = c * UNIT;

       vertexCount++;

       if (vertexCount >= ARRAY_MAX)
         error(3);
    }
    else if (line[0] == 'v' && line[1] == 't') // texture vertex
    {
       float a, b;

       if (sscanf(line + 2,"%f %f",&a,&b) != 2)
         error(4);

       a += UV_OFFSET;
       b += UV_OFFSET;

       uvs[2 * uvCount] = a * UNIT;
       uvs[2 * uvCount + 1] = (1 - b) * UNIT;

       uvCount++;

       if (uvCount >= ARRAY_MAX)
         error(5);
    }
    else if (line[0] == 'u' && line[1] == 's') // material
    {
      vertexType = line[7] - 'a';
    }
  }

  printf("#define LCR_CAR_VERTEX_COUNT %d\n"
    "#define LCR_CAR_TRIANGLE_COUNT %d\n\n",vertexCount,triangleCount);

  writeArray(vertices,vertexCount * 3,
    "static const int32_t LCR_carVertices",10);

  writeArray(triangles,triangleCount * 3,
    "static const uint16_t LCR_carTriangles",13);

  writeArray(uvs,uvCount * 2,
    "static const uint16_t LCR_carUvs",10);

  writeArray(triangleUvs,triangleCount * 3,
    "static const uint16_t LCR_carTriangleUvs",13);

  writeArray(vertexTypes,vertexCount,
    "static const uint8_t LCR_carVertexTypes",18);

  return 0;
}
