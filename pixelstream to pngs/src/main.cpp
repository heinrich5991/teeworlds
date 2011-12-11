#include <zlib/zlib.h>
#include <pnglite/pnglite.h>
#include <base/system.h>

int main(void)
{
    dbg_logger_stdout();
    char aUserdir[1024] = {0};
    char aPixelFile[1024] = {0};
    int PngCounter = 0;

    fs_storage_path("Teeworlds", aUserdir, sizeof(aUserdir));
    str_format(aPixelFile, sizeof(aPixelFile), "%s/tmp/pixelstream/video.stream", aUserdir);
    dbg_msg("", aPixelFile);
    IOHANDLE PixelStream = io_open(aPixelFile, IOFLAG_READ);
    while(PixelStream)
    {
        long long t = 0;
        int Size = 0;
        int W = 0;
        int H = 0;
        char *pData = 0;
        io_read(PixelStream, &t, sizeof(t));
        io_read(PixelStream, &Size, sizeof(Size));
        io_read(PixelStream, &W, sizeof(W));
        io_read(PixelStream, &H, sizeof(H));
        pData = new char[Size];
        io_read(PixelStream, pData, Size);

        PngCounter++;
        char aBuf[1024];
        if (PngCounter < 10)
            str_format(aBuf, sizeof(aBuf), "%s/tmp/pixelstream/png0000%i.png", aUserdir, PngCounter);
        if (PngCounter < 100)
            str_format(aBuf, sizeof(aBuf), "%s/tmp/pixelstream/png000%i.png", aUserdir, PngCounter);
        if (PngCounter < 1000)
            str_format(aBuf, sizeof(aBuf), "%s/tmp/pixelstream/png00%i.png", aUserdir, PngCounter);
        if (PngCounter < 10000)
            str_format(aBuf, sizeof(aBuf), "%s/tmp/pixelstream/png0%i.png", aUserdir, PngCounter);
        if (PngCounter < 100000)
            str_format(aBuf, sizeof(aBuf), "%s/tmp/pixelstream/png%i.png", aUserdir, PngCounter);

        char *pTempRow = new char[W * 3];
        for(int y = 0; y < H/2; y++)
        {
            mem_copy(pTempRow, pData+y*W*3, W*3);
            mem_copy(pData+y*W*3, pData+(H-y-1)*W*3, W*3);
            mem_copy(pData+(H-y-1)*W*3, pTempRow,W*3);
        }

        png_t Png;
        png_init(0,0);
        png_open_file_write(&Png, aBuf); // ignore_convention
        png_set_data(&Png, W, H, 8, PNG_TRUECOLOR, (unsigned char *)pData); // ignore_convention
        png_close_file(&Png); // ignore_convention
    }
    return 0;
}
