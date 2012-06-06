/* simple test application which invokes the vfw system
this lacks sufficient originality to be eligible for copyright;
therefore, public domain */


#include <stdio.h>
#include <stdlib.h>


#include <windows.h>

#include <vfw.h>

AVISTREAMINFO m_header;
PAVIFILE m_file;
BITMAPINFOHEADER m_bitmap;
PAVISTREAM m_stream;
PAVISTREAM m_stream_c;

AVICOMPRESSOPTIONS m_options;
AVICOMPRESSOPTIONS *m_options_p;


void addframes (void)
{
  unsigned char *framedata = malloc (256 * 256 * 3);
  
  int j = 0;
  
  for (int f = 0; f < 600; f++)
  {
    for (int y = 0; y < 256; y++)
    {
      for (int x = 0; x < 256; x++)
      {
        framedata[y * 768 + x * 3 + 0] = j++;
        framedata[y * 768 + x * 3 + 1] = j++;
        j += 2;
        framedata[y * 768 + x * 3 + 2] = j++;
        j += 3;
      }
    }
    // what does writing AVIIF_KEYFRAME mean exactly?
    if (0 != AVIStreamWrite (m_stream_c, f, 1, (void *) framedata, m_bitmap.biSizeImage, AVIIF_KEYFRAME, NULL, NULL))
    {
      printf ("AVIStreamWrite!!\n");
      exit (0);
    }
    j += 11;
  }
   
  free (framedata);
}


int main (int argc, char *argv[])
{
  int ret;
  
  if (!argv[1])
  {
    printf ("give filename\n");
    return 0;
  }

  FILE *f = fopen (argv[1], "wb");
  if (!f)
  {
    printf ("couldn't do file\n");
    return 0;
  }
  fclose (f);
  AVIFileInit ();
  
  ret = AVIFileOpen (&m_file, argv[1], OF_WRITE | OF_CREATE, NULL);
  if (ret != 0)
  {
    printf ("AVIFileOpen!! %i\n", ret);
    return 0;
  }
  
  // set up the still image information
  
  // is there some sort of pitch limitation on DIBs?
  
  m_bitmap.biSize = sizeof (BITMAPINFOHEADER);
  m_bitmap.biWidth = 256;
  m_bitmap.biHeight = 256; // negative would be for top-down arangement
  m_bitmap.biPlanes = 1;
  m_bitmap.biBitCount = 24; // BGR24
  m_bitmap.biCompression = BI_RGB; // uncompressed
  m_bitmap.biSizeImage = 256 * 256 * 3; // size, can be zero for BI_RGB
  //m_bitmap.biXPelsPerMeter = 1000; // pixels per meter? not likely used
  //m_bitmap.biYPelsPerMeter = 1000; // pixels per meter? not likely used
  //m_bitmap.biClrUsed = 0;
  //m_bitmap.biClrImportant = 0;
  
  // set up video stream information
  m_header.fccType = streamtypeVIDEO;
  m_header.dwScale = 1; // fpsden
  m_header.dwRate = 60; // fpsnum
  m_header.dwSuggestedBufferSize = m_bitmap.biSizeImage;

  if (0 != AVIFileCreateStream (m_file, &m_stream, &m_header))
  {
    printf ("AVIFileCreateStream!!\n");
    return 0;
  }
  
  // use VCM to select codec
  m_options_p = &m_options;
  
  if (1 != AVISaveOptions (0, 0, 1, &m_stream, &m_options_p))
  {
    printf ("AVISaveOptions!!\n");
    return 0;
  }
  
  // create compressed stream
  if (0 != AVIMakeCompressedStream (&m_stream_c, m_stream, &m_options, NULL))
  {
    printf ("AVIMakeCompressedStream!!\n");
    return 0;
  }
  
  
  if (0 != AVIStreamSetFormat (m_stream_c, 0, &m_bitmap, m_bitmap.biSize + m_bitmap.biClrUsed * sizeof (RGBQUAD)))
  {
    printf ("AVIStreamSetFormat!!\n");
    return 0;
  }
  
  // add frames
  addframes ();
  
  AVIStreamClose (m_stream_c);
  AVIStreamClose (m_stream);
  AVIFileClose (m_file);
  
  AVIFileExit ();
  
  return 0;
}

