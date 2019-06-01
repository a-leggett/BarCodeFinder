using BarCodeFinder.Native;
using System;

namespace BarCodeFinder
{
    public static class BitmapHelper
    {
        public static void ConvertFromBGRAToRGBA(IntPtr src, IntPtr dst, int width, int height)
        {
            Imports.ConvertFromBGRAToRGBA(src, dst, width, height);
        }

        public static void ConvertFromRGBAToBGRA(IntPtr src, IntPtr dst, int width, int height)
        {
            Imports.ConvertFromRGBAToBGRA(src, dst, width, height);
        }
    }
}
